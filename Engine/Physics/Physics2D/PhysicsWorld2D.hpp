#pragma once

#include "../../Core/Base.hpp"
#include "RigidBody2D.hpp"
#include "Collider2D.hpp"
#include "ContactSolver2D.hpp"
#include "Joint2D.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace GameEngine {
namespace Physics2D {

    struct RaycastHit2D {
        RigidBody2D* Body = nullptr;
        glm::vec2 Point{0.0f};
        glm::vec2 Normal{0.0f};
        float Distance = 0.0f;
    };

    using CollisionCallback = std::function<void(RigidBody2D*, RigidBody2D*)>;

    class PhysicsWorld2D {
    public:
        PhysicsWorld2D(const glm::vec2& gravity = glm::vec2(0.0f, -9.81f));
        ~PhysicsWorld2D() = default;

        // Simulation
        void step(float dt, int velocityIterations = 8, int positionIterations = 3);

        // Body management
        RigidBody2D* createBody(RigidBody2D::BodyType type = RigidBody2D::BodyType::Dynamic, float mass = 1.0f);
        void destroyBody(RigidBody2D* body);
        const std::vector<Scope<RigidBody2D>>& getBodies() const { return m_Bodies; }

        // Collider management — attach collider to a body
        void attachCollider(RigidBody2D* body, Ref<Collider2D> collider);

        // Joint management
        template<typename T, typename... Args>
        T* createJoint(RigidBody2D* bodyA, RigidBody2D* bodyB, Args&&... args) {
            auto joint = CreateScope<T>(std::forward<Args>(args)...);
            joint->BodyA = bodyA;
            joint->BodyB = bodyB;
            T* ptr = joint.get();
            m_Joints.push_back(std::move(joint));
            return ptr;
        }
        void destroyJoint(Joint2D* joint);

        // Queries
        bool raycast(const glm::vec2& origin, const glm::vec2& direction, float maxDist,
                     RaycastHit2D& hit, uint16_t layerMask = 0xFFFF) const;
        RigidBody2D* pointQuery(const glm::vec2& point, uint16_t layerMask = 0xFFFF) const;
        std::vector<RigidBody2D*> aabbQuery(const AABB2D& aabb, uint16_t layerMask = 0xFFFF) const;

        // Gravity
        void setGravity(const glm::vec2& gravity) { m_Gravity = gravity; }
        glm::vec2 getGravity() const { return m_Gravity; }

        // Collision callbacks
        void setOnCollisionEnter(CollisionCallback cb) { m_OnCollisionEnter = std::move(cb); }
        void setOnCollisionExit(CollisionCallback cb) { m_OnCollisionExit = std::move(cb); }
        void setOnCollisionStay(CollisionCallback cb) { m_OnCollisionStay = std::move(cb); }

        // Stats
        int getBodyCount() const { return static_cast<int>(m_Bodies.size()); }
        int getContactCount() const { return static_cast<int>(m_Manifolds.size()); }
        int getJointCount() const { return static_cast<int>(m_Joints.size()); }

    private:
        // Broadphase using spatial hash
        void broadphase();
        void narrowphase();
        void solveVelocityConstraints(int iterations);
        void solvePositionConstraints(int iterations);
        void integrateForces(float dt);
        void integrateVelocities(float dt);
        void cleanupBrokenJoints();
        void fireCollisionCallbacks();

        // Spatial hash helpers
        struct SpatialHashKey {
            int x, y;
            bool operator==(const SpatialHashKey& other) const { return x == other.x && y == other.y; }
        };
        struct SpatialHashKeyHash {
            size_t operator()(const SpatialHashKey& k) const {
                return std::hash<int>()(k.x) ^ (std::hash<int>()(k.y) << 16);
            }
        };

        using PairKey = std::pair<uint32_t, uint32_t>;
        struct PairKeyHash {
            size_t operator()(const PairKey& p) const {
                return std::hash<uint64_t>()(static_cast<uint64_t>(p.first) << 32 | p.second);
            }
        };

        glm::vec2 m_Gravity;
        float m_CellSize = 2.0f;
        uint32_t m_NextBodyId = 1;

        std::vector<Scope<RigidBody2D>> m_Bodies;
        std::unordered_map<RigidBody2D*, Ref<Collider2D>> m_Colliders;
        std::vector<Scope<Joint2D>> m_Joints;
        std::vector<ContactManifold2D> m_Manifolds;

        // Broadphase pairs
        std::unordered_set<PairKey, PairKeyHash> m_BroadphasePairs;

        // Collision tracking for callbacks
        std::unordered_set<PairKey, PairKeyHash> m_ActiveCollisions;
        std::unordered_set<PairKey, PairKeyHash> m_PreviousCollisions;

        // Callbacks
        CollisionCallback m_OnCollisionEnter;
        CollisionCallback m_OnCollisionExit;
        CollisionCallback m_OnCollisionStay;
    };

}}
