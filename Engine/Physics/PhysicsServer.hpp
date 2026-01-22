#pragma once

#include "../Core/Runtime/IPhysicsServer.hpp"
#include "../Graphics/PhysicsWorld.hpp"
#include "../Graphics/RigidBody.hpp"
#include <glm/glm.hpp>
#include <vector>

namespace GameEngine {

    /**
     * @brief Physics server implementation
     * 
     * Wraps PhysicsWorld behind IPhysicsServer interface
     */
    class PhysicsServer : public IPhysicsServer {
    public:
        PhysicsServer();
        ~PhysicsServer() override = default;

        void Initialize() override;
        void Shutdown() override;

        void Step(float deltaTime) override;

        Ref<Physics::RigidBody> CreateRigidBody(const glm::vec3& position, float mass = 1.0f) override;
        void RemoveRigidBody(Ref<Physics::RigidBody> body) override;

        void SetGravity(const glm::vec3& gravity) override;
        glm::vec3 GetGravity() const override;

        // Debug APIs
        std::vector<ContactPoint> GetContacts() const override;
        std::vector<CollisionShape> GetCollisionShapes() const override;
        void GetBroadphaseDebug(std::vector<glm::vec3>& bounds) const override;
        std::vector<Constraint> GetConstraints() const override;

    private:
        Ref<Physics::PhysicsWorld> m_PhysicsWorld;
        std::vector<ContactPoint> m_Contacts;
        std::vector<CollisionShape> m_CollisionShapes;
        std::vector<Constraint> m_Constraints;
    };

}

