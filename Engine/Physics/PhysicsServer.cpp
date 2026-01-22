#include "PhysicsServer.hpp"
#include "../Core/Logger.hpp"

namespace GameEngine {

    PhysicsServer::PhysicsServer() {
    }

    void PhysicsServer::Initialize() {
        m_PhysicsWorld = CreateRef<Physics::PhysicsWorld>();
        GE_CORE_INFO("PhysicsServer initialized");
    }

    void PhysicsServer::Shutdown() {
        m_PhysicsWorld.reset();
        GE_CORE_INFO("PhysicsServer shutdown");
    }

    void PhysicsServer::Step(float deltaTime) {
        if (m_PhysicsWorld) {
            m_PhysicsWorld->Update(deltaTime);
        }
    }

    Ref<Physics::RigidBody> PhysicsServer::CreateRigidBody(const glm::vec3& position, float mass) {
        auto body = CreateRef<Physics::RigidBody>(Physics::RigidBody::BodyType::Dynamic, mass);
        body->SetPosition(position);
        if (m_PhysicsWorld) {
            m_PhysicsWorld->AddRigidBody(body);
        }
        return body;
    }

    void PhysicsServer::RemoveRigidBody(Ref<Physics::RigidBody> body) {
        if (m_PhysicsWorld) {
            m_PhysicsWorld->RemoveRigidBody(body);
        }
    }

    void PhysicsServer::SetGravity(const glm::vec3& gravity) {
        if (m_PhysicsWorld) {
            m_PhysicsWorld->SetGravity(gravity);
        }
    }

    glm::vec3 PhysicsServer::GetGravity() const {
        if (m_PhysicsWorld) {
            return m_PhysicsWorld->GetGravity();
        }
        return glm::vec3(0.0f, -9.81f, 0.0f);
    }

    std::vector<ContactPoint> PhysicsServer::GetContacts() const {
        return m_Contacts;
    }

    std::vector<CollisionShape> PhysicsServer::GetCollisionShapes() const {
        return m_CollisionShapes;
    }

    void PhysicsServer::GetBroadphaseDebug(std::vector<glm::vec3>& bounds) const {
        // TODO: Implement when broadphase is added
        bounds.clear();
    }

    std::vector<Constraint> PhysicsServer::GetConstraints() const {
        return m_Constraints;
    }

}

