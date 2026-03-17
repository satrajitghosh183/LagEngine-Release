#include "sph.h"
#include <algorithm>

static constexpr float PI = 3.14159265358979323846f;

SPHSolver::SPHSolver(float worldW, float worldH, const SPHParams& params)
    : m_params(params), m_worldW(worldW), m_worldH(worldH)
{
    m_lastH = 0.0f;  // force first recalc
    recalcKernels();
}

void SPHSolver::recalcKernels() {
    float h = m_params.smoothingRadius;
    if (h == m_lastH) return;   // skip if unchanged
    m_lastH    = h;
    m_h2       = h * h;
    m_poly6    = 315.0f / (64.0f * PI * std::pow(h, 9.0f));
    m_spikyGrad = -45.0f / (PI * std::pow(h, 6.0f));
    m_viscLap  = 45.0f / (PI * std::pow(h, 6.0f));
}

void SPHSolver::addParticle(Vec2 pos, Vec2 vel) {
    SPHParticle p;
    p.pos = pos;
    p.vel = vel;
    m_particles.push_back(p);
}

void SPHSolver::addBlock(float x0, float y0, float x1, float y1, float spacing) {
    for (float y = y0; y < y1; y += spacing)
        for (float x = x0; x < x1; x += spacing)
            addParticle({x, y});
}

void SPHSolver::reset() {
    m_particles.clear();
    m_positions.clear();
}

void SPHSolver::loadScene(const Scene& scene) {
    m_particles.clear();
    m_positions.clear();
    m_obstacles = scene.obstacles;
    for (const auto& b : scene.blocks)
        addBlock(b.x0, b.y0, b.x1, b.y1, b.spacing);
}

// ─── Build spatial hash from current positions ──────────────────

void SPHSolver::buildGrid() {
    size_t n = m_particles.size();
    m_positions.resize(n);
    for (size_t i = 0; i < n; ++i)
        m_positions[i] = m_particles[i].pos;
    m_grid.build(m_positions, m_params.smoothingRadius);
}

// ─── Main update ────────────────────────────────────────────────

void SPHSolver::update() {
    for (int s = 0; s < m_params.substeps; ++s) {
        buildGrid();
        computeDensityPressure();
        computeForces();
        integrate();
        enforceBoundary();
        enforceObstacles();
    }
}

// ─── Density & pressure (spatial hash accelerated) ──────────────

void SPHSolver::computeDensityPressure() {
    const float h2    = m_h2;
    const float mass  = m_params.particleMass;
    const float gasK  = m_params.gasConstant;
    const float rho0  = m_params.restDensity;
    const float poly6 = m_poly6;

    for (size_t i = 0; i < m_particles.size(); ++i) {
        auto& pi = m_particles[i];
        float density = 0.0f;
        Vec2 posi = pi.pos;

        m_grid.queryNeighbors(posi, [&](uint32_t j) {
            Vec2 diff = m_particles[j].pos - posi;
            float r2 = diff.x * diff.x + diff.y * diff.y;
            if (r2 < h2) {
                float d = h2 - r2;
                density += mass * poly6 * d * d * d;
            }
        });

        pi.density  = density;
        pi.pressure = gasK * (density - rho0);
    }
}

// ─── Forces (spatial hash accelerated, no redundant sqrt) ───────

void SPHSolver::computeForces() {
    const float h2   = m_h2;
    const float h    = m_params.smoothingRadius;
    const float mass = m_params.particleMass;
    const float visc = m_params.viscosity;
    const float spikyGrad = m_spikyGrad;
    const float viscLap   = m_viscLap;
    const float grav      = m_params.gravity;

    for (size_t i = 0; i < m_particles.size(); ++i) {
        auto& pi = m_particles[i];
        Vec2 fPressure{0, 0}, fViscosity{0, 0};
        Vec2 posi = pi.pos;
        Vec2 veli = pi.vel;
        float pi_press = pi.pressure;

        m_grid.queryNeighbors(posi, [&](uint32_t j) {
            if (j == static_cast<uint32_t>(i)) return;
            const auto& pj = m_particles[j];
            float dx = pj.pos.x - posi.x;
            float dy = pj.pos.y - posi.y;
            float r2 = dx * dx + dy * dy;
            if (r2 >= h2 || r2 < 1e-12f) return;

            float r = std::sqrt(r2);
            float rho_j = std::max(pj.density, 1e-6f);

            // Spiky gradient: inline, no redundant normalize
            float h_r = h - r;
            float c = spikyGrad * h_r * h_r / r;  // includes 1/r for normalization
            float pressScale = mass * (pi_press + pj.pressure) / (2.0f * rho_j);
            fPressure.x -= dx * c * pressScale;
            fPressure.y -= dy * c * pressScale;

            // Viscosity laplacian
            float viscScale = visc * mass * viscLap * (h - r) / rho_j;
            fViscosity.x += (pj.vel.x - veli.x) * viscScale;
            fViscosity.y += (pj.vel.y - veli.y) * viscScale;
        });

        pi.force.x = fPressure.x + fViscosity.x;
        pi.force.y = fPressure.y + fViscosity.y + grav * pi.density;
    }
}

// ─── Integration (also tracks maxSpeed) ─────────────────────────

void SPHSolver::integrate() {
    float dt = m_params.dt;
    float maxSpd2 = 1.0f;

    for (auto& p : m_particles) {
        float rho = std::max(p.density, 1e-6f);
        float invRho = dt / rho;
        p.vel.x += p.force.x * invRho;
        p.vel.y += p.force.y * invRho;
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;

        float s2 = p.vel.x * p.vel.x + p.vel.y * p.vel.y;
        if (s2 > maxSpd2) maxSpd2 = s2;
    }
    m_maxSpeed = std::sqrt(maxSpd2);
}

// ─── World boundary ────────────────────────────────────────────

void SPHSolver::enforceBoundary() {
    float d = m_params.boundDamping;
    float margin = m_params.smoothingRadius * 0.5f;
    float maxX = m_worldW - margin;
    float maxY = m_worldH - margin;

    for (auto& p : m_particles) {
        if (p.pos.x < margin)  { p.pos.x = margin;  p.vel.x *= d; }
        if (p.pos.x > maxX)    { p.pos.x = maxX;    p.vel.x *= d; }
        if (p.pos.y < margin)  { p.pos.y = margin;  p.vel.y *= d; }
        if (p.pos.y > maxY)    { p.pos.y = maxY;    p.vel.y *= d; }
    }
}

// ─── Obstacle collision ────────────────────────────────────────

void SPHSolver::enforceObstacles() {
    if (m_obstacles.empty()) return;
    float margin = m_params.smoothingRadius * 0.45f;

    for (auto& p : m_particles) {
        for (const auto& seg : m_obstacles) {
            Vec2 ab = seg.b - seg.a;
            float len2 = ab.lengthSq();
            if (len2 < 1e-6f) continue;

            float t = (p.pos - seg.a).dot(ab) / len2;
            t = std::clamp(t, 0.0f, 1.0f);
            Vec2 closest = seg.a + ab * t;

            Vec2  diff = p.pos - closest;
            float dist = diff.length();

            if (dist < margin && dist > 1e-6f) {
                Vec2 normal = diff * (1.0f / dist);
                p.pos = closest + normal * margin;
                float vn = p.vel.dot(normal);
                if (vn < 0) {
                    p.vel -= normal * (1.6f * vn);
                    Vec2 tangent{-normal.y, normal.x};
                    float vt = p.vel.dot(tangent);
                    p.vel -= tangent * (vt * 0.05f);
                }
            }
        }
    }
}

// ─── Kernels ───────────────────────────────────────────────────

float SPHSolver::kernelPoly6(float r2) const {
    float d = m_h2 - r2;
    return m_poly6 * d * d * d;
}

float SPHSolver::kernelViscosityLaplacian(float r) const {
    return m_viscLap * (m_params.smoothingRadius - r);
}
