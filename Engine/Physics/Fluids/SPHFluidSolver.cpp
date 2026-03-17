#include "SPHFluidSolver.hpp"
#include "SPHKernels.hpp"
#include <cmath>
#include <random>

namespace GameEngine::Physics {

    // -----------------------------------------------------------------------
    // Init — default random scatter (kept for API compat)
    // -----------------------------------------------------------------------
    void SPHFluidSolver::Init(int numParticles, const glm::vec3& domainMin, const glm::vec3& domainMax) {
        DomainMin = domainMin;
        DomainMax = domainMax;
        m_Grid.SetCellSize(SmoothingLength);

        // Prefer a regular block over random scatter (matches old sph_water.cpp)
        glm::vec3 range = domainMax - domainMin;
        float spacing = SmoothingLength * 0.62f;
        int nx = std::max(1, static_cast<int>(range.x * 0.8f / spacing));
        int ny = std::max(1, static_cast<int>(range.y * 0.7f / spacing));
        int nz = std::max(1, static_cast<int>(range.z * 0.8f / spacing));
        glm::vec3 origin = domainMin + glm::vec3(range.x * 0.1f, spacing, range.z * 0.1f);
        InitBlock(nx, ny, nz, spacing, origin);
        (void)numParticles;
    }

    // -----------------------------------------------------------------------
    // InitBlock — port of old_code/src/sph_water.cpp SPH::init()
    // Spawns a rectangular lattice of particles (proper water-column init)
    // -----------------------------------------------------------------------
    void SPHFluidSolver::InitBlock(int nx, int ny, int nz, float spacing, const glm::vec3& origin) {
        m_Particles.clear();
        m_Particles.reserve(static_cast<size_t>(nx) * ny * nz);
        m_Grid.SetCellSize(SmoothingLength);

        for (int iz = 0; iz < nz; ++iz)
        for (int iy = 0; iy < ny; ++iy)
        for (int ix = 0; ix < nx; ++ix) {
            FluidParticle p;
            p.position = origin + glm::vec3(ix * spacing, iy * spacing, iz * spacing);
            p.velocity = glm::vec3(0.0f);
            m_Particles.push_back(p);
        }
    }

    // -----------------------------------------------------------------------
    // AddParticle — dynamic emission (for emitter-based effects)
    // -----------------------------------------------------------------------
    void SPHFluidSolver::AddParticle(const glm::vec3& position, const glm::vec3& velocity) {
        FluidParticle p;
        p.position = position;
        p.velocity = velocity;
        m_Particles.push_back(p);
    }

    void SPHFluidSolver::Clear() { m_Particles.clear(); }

    std::vector<glm::vec3> SPHFluidSolver::GetPositions() const {
        std::vector<glm::vec3> pos;
        pos.reserve(m_Particles.size());
        for (auto& p : m_Particles) pos.push_back(p.position);
        return pos;
    }

    void SPHFluidSolver::Update(float dt) {
        m_Grid.Clear();
        for (int i = 0; i < static_cast<int>(m_Particles.size()); i++) {
            m_Grid.Insert(i, m_Particles[i].position);
        }
        ComputeDensityPressure();
        ComputeForces();
        Integrate(dt);
        HandleBoundaries();
    }

    void SPHFluidSolver::ComputeDensityPressure() {
        float h = SmoothingLength;
        for (int i = 0; i < static_cast<int>(m_Particles.size()); i++) {
            auto& pi = m_Particles[i];
            pi.density = 0.0f;
            auto neighbors = m_Grid.Query(pi.position, h);
            for (int j : neighbors) {
                glm::vec3 diff = pi.position - m_Particles[j].position;
                float r2 = glm::dot(diff, diff);
                pi.density += SPHKernels::Poly6(r2, h);
            }
            pi.pressure = PressureCoefficient * (pi.density - RestDensity);
        }
    }

    void SPHFluidSolver::ComputeForces() {
        float h = SmoothingLength;
        for (int i = 0; i < static_cast<int>(m_Particles.size()); i++) {
            auto& pi = m_Particles[i];
            glm::vec3 fPressure(0.0f), fViscosity(0.0f);
            auto neighbors = m_Grid.Query(pi.position, h);

            for (int j : neighbors) {
                if (i == j) continue;
                auto& pj = m_Particles[j];
                glm::vec3 diff = pi.position - pj.position;
                float r = glm::length(diff);
                if (r < 1e-6f || r >= h) continue;

                float avgDens = std::max(pi.density * pj.density, 1e-6f);
                fPressure += -glm::normalize(diff) * ((pi.pressure + pj.pressure) / (2.0f * avgDens))
                           * glm::length(SPHKernels::SpikyGrad(diff, r, h));
                fViscosity += ViscosityCoefficient * (pj.velocity - pi.velocity) / std::max(pj.density, 1e-6f)
                            * SPHKernels::ViscosityLap(r, h);
            }

            pi.force = fPressure + fViscosity + Gravity * pi.density;
        }
    }

    void SPHFluidSolver::Integrate(float dt) {
        for (auto& p : m_Particles) {
            glm::vec3 acc = p.force / std::max(p.density, 1e-6f);
            p.velocity += acc * dt;
            p.position += p.velocity * dt;
        }
    }

    void SPHFluidSolver::HandleBoundaries() {
        for (auto& p : m_Particles) {
            for (int axis = 0; axis < 3; axis++) {
                if (p.position[axis] < DomainMin[axis]) {
                    p.position[axis] = DomainMin[axis];
                    p.velocity[axis] *= -BounceCoefficient;
                }
                if (p.position[axis] > DomainMax[axis]) {
                    p.position[axis] = DomainMax[axis];
                    p.velocity[axis] *= -BounceCoefficient;
                }
            }
        }
    }

}
