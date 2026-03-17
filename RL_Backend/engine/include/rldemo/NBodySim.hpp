#pragma once

#include "Types.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

namespace rldemo {

/**
 * Single body in the N-body simulation.
 * Used for gravity, brute-force AABB collision, and Euler integration.
 */
struct NBody {
    glm::vec3 pos{0.f};
    glm::vec3 vel{0.f};
    glm::vec3 acc{0.f};
    float mass = 1.f;
    float radius = 0.5f;
};

/**
 * N-body gravitational simulation with brute-force collision and constraints.
 * Shared by Stage 1 (single-threaded), Stage 2 (parallel ranges), and Stage 3 (CUDA).
 *
 * - O(n^2) pairwise gravity
 * - O(n^2) brute-force AABB collision detection
 * - Floor plane + bounding box constraints
 * - Euler integration
 */
class NBodySim {
public:
    static constexpr float kGravityConst = 1e-4f;
    static constexpr float kSofteningSq = 1e-6f;
    static constexpr float kFloorY = -40.f;
    static constexpr float kBoundsHalf = 50.f;
    static constexpr float kRestitution = 0.5f;

    NBodySim() = default;
    explicit NBodySim(uint32_t maxBodies);

    /** Resize and optionally randomize initial state */
    void Resize(uint32_t count);
    void RandomizePositionsAndVelocities(float boxHalf, float speedScale = 1.f);

    /** Full step (single-threaded): gravity -> collision detect -> resolve -> integrate */
    void Step(float dt);

    /** Range-based APIs for parallel execution (Stage 2). Bodies array must be shared. */
    void ComputeGravityRange(int start, int end, std::vector<NBody>& bodies) const;
    void DetectAndResolveCollisionsRange(int start, int end, std::vector<NBody>& bodies) const;
    void IntegrateRange(int start, int end, std::vector<NBody>& bodies, float dt) const;

    /** Fill instance data for rendering: model matrix + color (by index) for range [start, end) */
    static void BuildInstanceDataRange(int start, int end, const std::vector<NBody>& bodies,
                                       std::vector<float>& outModelColors, int floatsPerInstance = 20);

    /** Single-body gravity contribution from all others (for range-based gravity) */
    glm::vec3 ComputeGravityForBody(int i, const std::vector<NBody>& bodies) const;

    /** Accessors */
    std::vector<NBody>& GetBodies() { return m_Bodies; }
    const std::vector<NBody>& GetBodies() const { return m_Bodies; }
    uint32_t GetBodyCount() const { return static_cast<uint32_t>(m_Bodies.size()); }
    uint32_t GetMaxBodies() const { return m_MaxBodies; }

private:
    void computeGravity();
    void resolveCollisions(float dt);
    void integrate(float dt);

    uint32_t m_MaxBodies = 0;
    std::vector<NBody> m_Bodies;
};

} // namespace rldemo
