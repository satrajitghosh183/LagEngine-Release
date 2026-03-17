#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>

// ─────────────────────────────────────────────────────────────────────────────
// Block – a rigid box with PBR material
// ─────────────────────────────────────────────────────────────────────────────
struct Block {
    std::string name        = "Block";
    glm::vec3   position    = {0, 1, 0};
    glm::vec3   velocity    = {0, 0, 0};
    glm::vec3   force       = {0, 0, 0};
    float       mass        = 1.0f;
    glm::vec3   halfExtents = {0.5f, 0.5f, 0.5f};
    bool        isAnchored  = false;
    float       restitution = 0.4f;   // bounciness [0,1]
    float       friction    = 0.5f;   // sliding friction coeff

    // PBR
    glm::vec3 albedo    = {0.72f, 0.72f, 0.76f};  // steel-grey
    float     metallic  = 0.9f;
    float     roughness = 0.2f;
    float     ao        = 1.0f;
};

// ─────────────────────────────────────────────────────────────────────────────
// Spring – connects two endpoints (block index or fixed world anchor)
//   index == -1  →  use the anchorXxx world-position
// ─────────────────────────────────────────────────────────────────────────────
struct Spring {
    std::string name    = "Spring";
    int         blockA  = -1;
    int         blockB  = -1;
    glm::vec3   anchorA = {0, 4, 0};
    glm::vec3   anchorB = {0, 0, 0};

    float k          = 80.0f;   // spring constant  [N/m]
    float damping    = 3.0f;    // damping coeff    [N·s/m]
    float restLength = 2.0f;    // natural length   [m]

    // PBR
    glm::vec3 albedo    = {0.65f, 0.65f, 0.68f};
    float     metallic  = 0.95f;
    float     roughness = 0.15f;
};

// ─────────────────────────────────────────────────────────────────────────────
// PhysicsWorld
// ─────────────────────────────────────────────────────────────────────────────
class PhysicsWorld {
public:
    std::vector<Block>  blocks;
    std::vector<Spring> springs;

    glm::vec3 gravity     = {0.0f, -9.81f, 0.0f};
    float     groundY     = 0.0f;
    bool      running     = true;
    float     timeScale   = 1.0f;
    // Sub-steps per rendered frame for stability
    int       subSteps    = 8;

    // ── Mutation helpers ──────────────────────────────────────────────────────
    int  addBlock(const Block& b);
    int  addSpring(const Spring& s);
    void removeBlock(int idx);
    void removeSpring(int idx);

    // Advance the simulation by dt seconds (already scaled externally)
    void step(float dt);

    // Reset all velocities and forces
    void resetVelocities();

    // World-space endpoints of a spring
    glm::vec3 springEndA(int si) const;
    glm::vec3 springEndB(int si) const;

private:
    void accumulateForces();
    void integrate(float dt);
    void resolveGround();
    void resolveBlockCollisions();
};
