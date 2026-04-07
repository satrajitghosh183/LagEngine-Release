#include "Physics.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

// ─────────────────────────────────────────────────────────────────────────────
// Mutation
// ─────────────────────────────────────────────────────────────────────────────
int PhysicsWorld::addBlock(const Block& b) {
    blocks.push_back(b);
    return (int)blocks.size() - 1;
}

int PhysicsWorld::addSpring(const Spring& s) {
    springs.push_back(s);
    return (int)springs.size() - 1;
}

void PhysicsWorld::removeBlock(int idx) {
    if (idx < 0 || idx >= (int)blocks.size()) return;
    blocks.erase(blocks.begin() + idx);
    // Fix up spring references
    for (auto& sp : springs) {
        if (sp.blockA == idx) sp.blockA = -1;
        if (sp.blockB == idx) sp.blockB = -1;
        if (sp.blockA > idx) --sp.blockA;
        if (sp.blockB > idx) --sp.blockB;
    }
}

void PhysicsWorld::removeSpring(int idx) {
    if (idx < 0 || idx >= (int)springs.size()) return;
    springs.erase(springs.begin() + idx);
}

void PhysicsWorld::resetVelocities() {
    for (auto& b : blocks) { b.velocity = {0,0,0}; b.force = {0,0,0}; }
}

// ─────────────────────────────────────────────────────────────────────────────
// Spring endpoints
// ─────────────────────────────────────────────────────────────────────────────
glm::vec3 PhysicsWorld::springEndA(int si) const {
    const Spring& s = springs[si];
    return (s.blockA >= 0) ? blocks[s.blockA].position : s.anchorA;
}

glm::vec3 PhysicsWorld::springEndB(int si) const {
    const Spring& s = springs[si];
    return (s.blockB >= 0) ? blocks[s.blockB].position : s.anchorB;
}

// ─────────────────────────────────────────────────────────────────────────────
// Force accumulation
// ─────────────────────────────────────────────────────────────────────────────
void PhysicsWorld::accumulateForces() {
    // Reset
    for (auto& b : blocks) b.force = {0,0,0};

    // Gravity
    for (auto& b : blocks)
        if (!b.isAnchored)
            b.force += gravity * b.mass;

    // Spring forces (Hooke + damping)
    for (const auto& s : springs) {
        glm::vec3 pA = springEndA((int)(&s - &springs[0]));
        glm::vec3 pB = springEndB((int)(&s - &springs[0]));

        glm::vec3 delta  = pB - pA;
        float     len    = glm::length(delta);
        if (len < 1e-5f) continue;

        glm::vec3 dir   = delta / len;
        float     ext   = len - s.restLength;

        // Relative velocity along spring axis for damping
        glm::vec3 vA = (s.blockA >= 0) ? blocks[s.blockA].velocity : glm::vec3(0);
        glm::vec3 vB = (s.blockB >= 0) ? blocks[s.blockB].velocity : glm::vec3(0);
        float     relV = glm::dot(vB - vA, dir);

        float forceMag = s.k * ext + s.damping * relV;
        glm::vec3 F    = forceMag * dir;

        if (s.blockA >= 0 && !blocks[s.blockA].isAnchored)
            blocks[s.blockA].force += F;
        if (s.blockB >= 0 && !blocks[s.blockB].isAnchored)
            blocks[s.blockB].force -= F;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Semi-implicit (symplectic) Euler integration
// ─────────────────────────────────────────────────────────────────────────────
void PhysicsWorld::integrate(float dt) {
    for (auto& b : blocks) {
        if (b.isAnchored) continue;
        glm::vec3 accel = b.force / b.mass;
        b.velocity += accel * dt;
        b.position += b.velocity * dt;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Ground plane collision
// ─────────────────────────────────────────────────────────────────────────────
void PhysicsWorld::resolveGround() {
    for (auto& b : blocks) {
        if (b.isAnchored) continue;
        float bottom = b.position.y - b.halfExtents.y;
        if (bottom < groundY) {
            b.position.y = groundY + b.halfExtents.y;
            if (b.velocity.y < 0.0f)
                b.velocity.y = -b.velocity.y * b.restitution;
            // Friction on xz plane
            float speed = glm::length(glm::vec2(b.velocity.x, b.velocity.z));
            if (speed > 1e-4f) {
                float normal_force = glm::length(gravity) * b.mass;
                float fri_decel    = b.friction * normal_force / b.mass;
                float new_speed    = std::max(0.0f, speed - fri_decel * 0.016f);
                float scale        = new_speed / speed;
                b.velocity.x *= scale;
                b.velocity.z *= scale;
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Simple AABB block-block collision (impulse-based)
// ─────────────────────────────────────────────────────────────────────────────
void PhysicsWorld::resolveBlockCollisions() {
    for (int i = 0; i < (int)blocks.size(); ++i) {
        for (int j = i + 1; j < (int)blocks.size(); ++j) {
            Block& a = blocks[i];
            Block& b = blocks[j];
            if (a.isAnchored && b.isAnchored) continue;

            glm::vec3 delta = b.position - a.position;
            glm::vec3 combined = a.halfExtents + b.halfExtents;

            // Overlap test on each axis
            float ox = combined.x - std::abs(delta.x);
            float oy = combined.y - std::abs(delta.y);
            float oz = combined.z - std::abs(delta.z);

            if (ox <= 0 || oy <= 0 || oz <= 0) continue; // no collision

            // Push apart on minimum overlap axis
            glm::vec3 normal(0);
            float penetration;
            if (ox < oy && ox < oz) {
                penetration = ox;
                normal = glm::vec3((delta.x > 0) ? 1.0f : -1.0f, 0, 0);
            } else if (oy < oz) {
                penetration = oy;
                normal = glm::vec3(0, (delta.y > 0) ? 1.0f : -1.0f, 0);
            } else {
                penetration = oz;
                normal = glm::vec3(0, 0, (delta.z > 0) ? 1.0f : -1.0f);
            }

            float totalMass = a.mass + b.mass;
            float e = (a.restitution + b.restitution) * 0.5f;

            // Positional correction (50% each unless anchored)
            float pushA = b.isAnchored ? penetration : penetration * b.mass / totalMass;
            float pushB = a.isAnchored ? penetration : penetration * a.mass / totalMass;
            if (!a.isAnchored) a.position -= normal * pushA;
            if (!b.isAnchored) b.position += normal * pushB;

            // Impulse
            glm::vec3 relV = b.velocity - a.velocity;
            float vAlongN  = glm::dot(relV, normal);
            if (vAlongN >= 0) continue; // separating

            float jN = -(1.0f + e) * vAlongN;
            jN /= (a.isAnchored ? 0.0f : 1.0f / a.mass) +
                  (b.isAnchored ? 0.0f : 1.0f / b.mass) + 1e-9f;

            glm::vec3 impulse = jN * normal;
            if (!a.isAnchored) a.velocity -= impulse / a.mass;
            if (!b.isAnchored) b.velocity += impulse / b.mass;
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Public step()
// ─────────────────────────────────────────────────────────────────────────────
void PhysicsWorld::step(float dt) {
    if (!running || dt <= 0) return;

    float subDt = dt * timeScale / (float)subSteps;
    for (int i = 0; i < subSteps; ++i) {
        accumulateForces();
        integrate(subDt);
        resolveGround();
        resolveBlockCollisions();
    }
}
