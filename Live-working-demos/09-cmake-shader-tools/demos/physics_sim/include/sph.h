#pragma once
#include "vec2.h"
#include "spatial_hash.h"
#include <vector>
#include <string>
#include <cmath>

// ─── SPH Parameters ────────────────────────────────────────────
struct SPHParams {
    float smoothingRadius  = 16.0f;
    float restDensity      = 1000.0f;
    float gasConstant      = 2000.0f;
    float viscosity        = 250.0f;
    float gravity          = 12000.0f;
    float particleMass     = 65.0f;
    float boundDamping     = -0.5f;
    float dt               = 0.0008f;
    int   substeps         = 8;
    int   colorScheme      = 0;   // 0=water, 1=honey, 2=gas, 3=lava
};

// ─── Line-segment obstacle ─────────────────────────────────────
struct Segment {
    Vec2 a, b;
};

// ─── Scene definition ──────────────────────────────────────────
struct FluidBlock {
    float x0, y0, x1, y1, spacing;
};

struct Scene {
    std::string name;
    std::vector<Segment>    obstacles;
    std::vector<FluidBlock> blocks;
};

// ─── Particle ──────────────────────────────────────────────────
struct SPHParticle {
    Vec2  pos;
    Vec2  vel;
    Vec2  force;
    float density  = 0.0f;
    float pressure = 0.0f;
};

// ─── SPH Solver ────────────────────────────────────────────────
class SPHSolver {
public:
    SPHSolver(float worldW, float worldH, const SPHParams& params = {});

    void addParticle(Vec2 pos, Vec2 vel = {0, 0});
    void addBlock(float x0, float y0, float x1, float y1, float spacing);
    void update();
    void reset();
    void recalcKernels();

    // Obstacles
    void setObstacles(const std::vector<Segment>& segs) { m_obstacles = segs; }
    const std::vector<Segment>& obstacles() const { return m_obstacles; }

    void loadScene(const Scene& scene);

    const std::vector<SPHParticle>& particles() const { return m_particles; }
    size_t count() const { return m_particles.size(); }
    const SPHParams& params() const { return m_params; }
    SPHParams& params() { return m_params; }

    float maxSpeed() const { return m_maxSpeed; }

private:
    void computeDensityPressure();
    void computeForces();
    void integrate();
    void enforceBoundary();
    void enforceObstacles();
    void buildGrid();

    float kernelPoly6(float r2) const;
    float kernelViscosityLaplacian(float r) const;

    std::vector<SPHParticle> m_particles;
    std::vector<Vec2>        m_positions;   // mirror for spatial hash
    std::vector<Segment>     m_obstacles;
    SPHParams m_params;
    float m_worldW, m_worldH;
    float m_h2, m_poly6, m_spikyGrad, m_viscLap;
    float m_lastH  = 0.0f;   // cached smoothing radius for recalcKernels
    float m_maxSpeed = 1.0f;  // tracked during integrate

    SpatialHash m_grid;
};
