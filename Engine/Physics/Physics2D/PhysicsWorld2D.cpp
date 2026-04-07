#include "PhysicsWorld2D.hpp"
#include <algorithm>
#include <cmath>

namespace GameEngine {
namespace Physics2D {

    PhysicsWorld2D::PhysicsWorld2D(const glm::vec2& gravity)
        : m_Gravity(gravity) {}

    // =====================================================================
    // Main simulation step
    // =====================================================================

    void PhysicsWorld2D::step(float dt, int velocityIterations, int positionIterations) {
        // 1. Integrate forces
        integrateForces(dt);

        // 2. Broadphase collision detection
        broadphase();

        // 3. Narrowphase — generate contact manifolds
        narrowphase();

        // 4. Solve velocity constraints (impulse solver)
        solveVelocityConstraints(velocityIterations);

        // 5. Integrate velocities to positions
        integrateVelocities(dt);

        // 6. Solve position constraints (Baumgarte correction)
        solvePositionConstraints(positionIterations);

        // 7. Solve joints
        for (auto& joint : m_Joints) {
            if (!joint->IsBroken) {
                joint->solve(dt);
            }
        }

        // 8. Clean up broken joints
        cleanupBrokenJoints();

        // 9. Fire collision callbacks
        fireCollisionCallbacks();
    }

    // =====================================================================
    // Body management
    // =====================================================================

    RigidBody2D* PhysicsWorld2D::createBody(RigidBody2D::BodyType type, float mass) {
        auto body = CreateScope<RigidBody2D>(type, mass);
        body->setId(m_NextBodyId++);
        RigidBody2D* ptr = body.get();
        m_Bodies.push_back(std::move(body));
        return ptr;
    }

    void PhysicsWorld2D::destroyBody(RigidBody2D* body) {
        // Remove associated collider
        m_Colliders.erase(body);

        // Remove joints referencing this body
        m_Joints.erase(
            std::remove_if(m_Joints.begin(), m_Joints.end(),
                [body](const Scope<Joint2D>& j) {
                    return j->BodyA == body || j->BodyB == body;
                }),
            m_Joints.end());

        // Remove the body
        m_Bodies.erase(
            std::remove_if(m_Bodies.begin(), m_Bodies.end(),
                [body](const Scope<RigidBody2D>& b) { return b.get() == body; }),
            m_Bodies.end());
    }

    void PhysicsWorld2D::attachCollider(RigidBody2D* body, Ref<Collider2D> collider) {
        collider->setBody(body);
        m_Colliders[body] = collider;

        // Auto-compute inertia from shape and mass
        if (body->getBodyType() == RigidBody2D::BodyType::Dynamic && body->getMass() > 0.0f) {
            float inertia = collider->computeInertia(body->getMass());
            body->setInertia(inertia);
        }
    }

    void PhysicsWorld2D::destroyJoint(Joint2D* joint) {
        m_Joints.erase(
            std::remove_if(m_Joints.begin(), m_Joints.end(),
                [joint](const Scope<Joint2D>& j) { return j.get() == joint; }),
            m_Joints.end());
    }

    // =====================================================================
    // Integration
    // =====================================================================

    void PhysicsWorld2D::integrateForces(float dt) {
        for (auto& body : m_Bodies) {
            body->integrateForces(dt, m_Gravity);
        }
    }

    void PhysicsWorld2D::integrateVelocities(float dt) {
        for (auto& body : m_Bodies) {
            body->integrateVelocities(dt);
        }
    }

    // =====================================================================
    // Broadphase — spatial hash grid
    // =====================================================================

    void PhysicsWorld2D::broadphase() {
        m_BroadphasePairs.clear();

        // Build spatial hash
        std::unordered_map<SpatialHashKey, std::vector<RigidBody2D*>, SpatialHashKeyHash> grid;

        for (auto& body : m_Bodies) {
            auto it = m_Colliders.find(body.get());
            if (it == m_Colliders.end()) continue;

            AABB2D aabb = it->second->computeAABB(body->getPosition(), body->getRotation());

            int minCellX = static_cast<int>(std::floor(aabb.Min.x / m_CellSize));
            int minCellY = static_cast<int>(std::floor(aabb.Min.y / m_CellSize));
            int maxCellX = static_cast<int>(std::floor(aabb.Max.x / m_CellSize));
            int maxCellY = static_cast<int>(std::floor(aabb.Max.y / m_CellSize));

            for (int cx = minCellX; cx <= maxCellX; cx++) {
                for (int cy = minCellY; cy <= maxCellY; cy++) {
                    grid[{cx, cy}].push_back(body.get());
                }
            }
        }

        // Find potentially colliding pairs
        for (auto& [key, bodies] : grid) {
            for (size_t i = 0; i < bodies.size(); i++) {
                for (size_t j = i + 1; j < bodies.size(); j++) {
                    uint32_t idA = bodies[i]->getId();
                    uint32_t idB = bodies[j]->getId();
                    if (idA > idB) std::swap(idA, idB);

                    // Skip static-static pairs
                    if (bodies[i]->getBodyType() == RigidBody2D::BodyType::Static &&
                        bodies[j]->getBodyType() == RigidBody2D::BodyType::Static) {
                        continue;
                    }

                    // Check collision layers
                    auto colliderA = m_Colliders[bodies[i]];
                    auto colliderB = m_Colliders[bodies[j]];
                    if (!colliderA->shouldCollideWith(*colliderB)) continue;

                    // AABB overlap test
                    AABB2D aabbA = colliderA->computeAABB(bodies[i]->getPosition(), bodies[i]->getRotation());
                    AABB2D aabbB = colliderB->computeAABB(bodies[j]->getPosition(), bodies[j]->getRotation());
                    if (aabbA.overlaps(aabbB)) {
                        m_BroadphasePairs.insert({idA, idB});
                    }
                }
            }
        }
    }

    // =====================================================================
    // Narrowphase — generate contact manifolds
    // =====================================================================

    void PhysicsWorld2D::narrowphase() {
        m_Manifolds.clear();

        // Build ID to body map
        std::unordered_map<uint32_t, RigidBody2D*> idMap;
        for (auto& body : m_Bodies) {
            idMap[body->getId()] = body.get();
        }

        for (const auto& pair : m_BroadphasePairs) {
            RigidBody2D* bodyA = idMap[pair.first];
            RigidBody2D* bodyB = idMap[pair.second];

            if (!bodyA || !bodyB) continue;
            if (!bodyA->isAwake() && !bodyB->isAwake()) continue;

            auto colliderA = m_Colliders[bodyA];
            auto colliderB = m_Colliders[bodyB];
            if (!colliderA || !colliderB) continue;

            ContactManifold2D manifold;
            manifold.BodyA = bodyA;
            manifold.BodyB = bodyB;
            manifold.ColliderA = colliderA.get();
            manifold.ColliderB = colliderB.get();

            if (ContactSolver2D::testCollision(
                    colliderA.get(), bodyA->getPosition(), bodyA->getRotation(),
                    colliderB.get(), bodyB->getPosition(), bodyB->getRotation(),
                    manifold)) {
                // Ensure body pointers are set correctly after potential swaps
                manifold.BodyA = bodyA;
                manifold.BodyB = bodyB;

                // Wake up sleeping bodies
                bodyA->setAwake(true);
                bodyB->setAwake(true);

                m_Manifolds.push_back(manifold);
            }
        }
    }

    // =====================================================================
    // Constraint solving
    // =====================================================================

    void PhysicsWorld2D::solveVelocityConstraints(int iterations) {
        for (int iter = 0; iter < iterations; iter++) {
            for (auto& manifold : m_Manifolds) {
                if (manifold.ColliderA->IsTrigger || manifold.ColliderB->IsTrigger) continue;
                ContactSolver2D::resolveContact(manifold, 0.0f);
            }
        }
    }

    void PhysicsWorld2D::solvePositionConstraints(int iterations) {
        for (int iter = 0; iter < iterations; iter++) {
            for (auto& manifold : m_Manifolds) {
                if (manifold.ColliderA->IsTrigger || manifold.ColliderB->IsTrigger) continue;
                ContactSolver2D::correctPositions(manifold);
            }
        }
    }

    // =====================================================================
    // Joint cleanup
    // =====================================================================

    void PhysicsWorld2D::cleanupBrokenJoints() {
        m_Joints.erase(
            std::remove_if(m_Joints.begin(), m_Joints.end(),
                [](const Scope<Joint2D>& j) { return j->IsBroken; }),
            m_Joints.end());
    }

    // =====================================================================
    // Collision callbacks
    // =====================================================================

    void PhysicsWorld2D::fireCollisionCallbacks() {
        m_PreviousCollisions.swap(m_ActiveCollisions);
        m_ActiveCollisions.clear();

        for (const auto& manifold : m_Manifolds) {
            uint32_t idA = manifold.BodyA->getId();
            uint32_t idB = manifold.BodyB->getId();
            if (idA > idB) std::swap(idA, idB);
            PairKey key = {idA, idB};

            m_ActiveCollisions.insert(key);

            if (m_PreviousCollisions.find(key) == m_PreviousCollisions.end()) {
                if (m_OnCollisionEnter) m_OnCollisionEnter(manifold.BodyA, manifold.BodyB);
            } else {
                if (m_OnCollisionStay) m_OnCollisionStay(manifold.BodyA, manifold.BodyB);
            }
        }

        // Fire exit callbacks
        for (const auto& key : m_PreviousCollisions) {
            if (m_ActiveCollisions.find(key) == m_ActiveCollisions.end()) {
                if (m_OnCollisionExit) {
                    // Find bodies by ID
                    RigidBody2D* bodyA = nullptr;
                    RigidBody2D* bodyB = nullptr;
                    for (auto& b : m_Bodies) {
                        if (b->getId() == key.first) bodyA = b.get();
                        if (b->getId() == key.second) bodyB = b.get();
                    }
                    if (bodyA && bodyB) m_OnCollisionExit(bodyA, bodyB);
                }
            }
        }
    }

    // =====================================================================
    // Queries
    // =====================================================================

    bool PhysicsWorld2D::raycast(const glm::vec2& origin, const glm::vec2& direction, float maxDist,
                                 RaycastHit2D& hit, uint16_t layerMask) const {
        float closestDist = maxDist;
        bool found = false;

        glm::vec2 dir = glm::normalize(direction);

        for (auto& body : m_Bodies) {
            auto it = m_Colliders.find(body.get());
            if (it == m_Colliders.end()) continue;

            if ((it->second->CollisionLayer & layerMask) == 0) continue;

            // Simple circle/AABB raycast test
            AABB2D aabb = it->second->computeAABB(body->getPosition(), body->getRotation());

            // AABB-ray intersection (slab method)
            float tmin = 0.0f;
            float tmax = closestDist;

            for (int axis = 0; axis < 2; axis++) {
                float aabbMin = (axis == 0) ? aabb.Min.x : aabb.Min.y;
                float aabbMax = (axis == 0) ? aabb.Max.x : aabb.Max.y;
                float o = (axis == 0) ? origin.x : origin.y;
                float d = (axis == 0) ? dir.x : dir.y;

                if (std::abs(d) < 1e-8f) {
                    if (o < aabbMin || o > aabbMax) { tmin = maxDist + 1.0f; break; }
                } else {
                    float t1 = (aabbMin - o) / d;
                    float t2 = (aabbMax - o) / d;
                    if (t1 > t2) std::swap(t1, t2);
                    tmin = std::max(tmin, t1);
                    tmax = std::min(tmax, t2);
                    if (tmin > tmax) break;
                }
            }

            if (tmin <= tmax && tmin < closestDist && tmin >= 0.0f) {
                closestDist = tmin;
                hit.Body = body.get();
                hit.Point = origin + dir * tmin;
                hit.Distance = tmin;

                // Approximate normal from AABB
                glm::vec2 center = aabb.center();
                glm::vec2 diff = hit.Point - center;
                glm::vec2 ext = aabb.extents();
                if (std::abs(diff.x / ext.x) > std::abs(diff.y / ext.y)) {
                    hit.Normal = glm::vec2(diff.x > 0.0f ? 1.0f : -1.0f, 0.0f);
                } else {
                    hit.Normal = glm::vec2(0.0f, diff.y > 0.0f ? 1.0f : -1.0f);
                }
                found = true;
            }
        }

        return found;
    }

    RigidBody2D* PhysicsWorld2D::pointQuery(const glm::vec2& point, uint16_t layerMask) const {
        for (auto& body : m_Bodies) {
            auto it = m_Colliders.find(body.get());
            if (it == m_Colliders.end()) continue;
            if ((it->second->CollisionLayer & layerMask) == 0) continue;

            AABB2D aabb = it->second->computeAABB(body->getPosition(), body->getRotation());
            if (point.x >= aabb.Min.x && point.x <= aabb.Max.x &&
                point.y >= aabb.Min.y && point.y <= aabb.Max.y) {
                return body.get();
            }
        }
        return nullptr;
    }

    std::vector<RigidBody2D*> PhysicsWorld2D::aabbQuery(const AABB2D& aabb, uint16_t layerMask) const {
        std::vector<RigidBody2D*> results;
        for (auto& body : m_Bodies) {
            auto it = m_Colliders.find(body.get());
            if (it == m_Colliders.end()) continue;
            if ((it->second->CollisionLayer & layerMask) == 0) continue;

            AABB2D bodyAABB = it->second->computeAABB(body->getPosition(), body->getRotation());
            if (aabb.overlaps(bodyAABB)) {
                results.push_back(body.get());
            }
        }
        return results;
    }

}}
