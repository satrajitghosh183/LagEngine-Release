#pragma once

#include "../Base.hpp"
#include "../../Physics/RigidBody.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <string>

namespace GameEngine {

    // Forward declarations for debug structures
    struct ContactPoint {
        glm::vec3 position;
        glm::vec3 normal;
        float penetration;
    };

    struct CollisionShape {
        enum Type { Sphere, Box, Capsule, Mesh };
        Type type;
        glm::vec3 position;
        glm::vec3 scale;
        glm::quat rotation;
    };

    struct Constraint {
        enum Type { Distance, Hinge, BallSocket };
        Type type;
        glm::vec3 anchorA;
        glm::vec3 anchorB;
    };

    /**
     * @brief Physics server interface
     * 
     * Provides physics simulation without exposing implementation details
     */
    class IPhysicsServer {
    public:
        virtual ~IPhysicsServer() = default;

        /**
         * @brief Initialize physics server
         */
        virtual void Initialize() = 0;

        /**
         * @brief Shutdown physics server
         */
        virtual void Shutdown() = 0;

        /**
         * @brief Step physics simulation
         */
        virtual void Step(float deltaTime) = 0;

        /**
         * @brief Create rigid body
         */
        virtual Ref<Physics::RigidBody> CreateRigidBody(const glm::vec3& position, float mass = 1.0f) = 0;

        /**
         * @brief Remove rigid body
         */
        virtual void RemoveRigidBody(Ref<Physics::RigidBody> body) = 0;

        /**
         * @brief Set gravity
         */
        virtual void SetGravity(const glm::vec3& gravity) = 0;

        /**
         * @brief Get gravity
         */
        virtual glm::vec3 GetGravity() const = 0;

        // Debug APIs
        /**
         * @brief Get all contact points
         */
        virtual std::vector<ContactPoint> GetContacts() const = 0;

        /**
         * @brief Get all collision shapes for visualization
         */
        virtual std::vector<CollisionShape> GetCollisionShapes() const = 0;

        /**
         * @brief Get broadphase debug information
         */
        virtual void GetBroadphaseDebug(std::vector<glm::vec3>& bounds) const = 0;

        /**
         * @brief Get all constraints
         */
        virtual std::vector<Constraint> GetConstraints() const = 0;
    };

}

