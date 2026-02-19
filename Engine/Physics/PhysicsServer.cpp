#include "PhysicsServer.hpp"
#include "../Core/Logger.hpp"

namespace GameEngine {

    PhysicsServer::PhysicsServer() {
    }

    void PhysicsServer::Initialize() {
        m_PhysicsWorld = CreateRef<Physics::PhysicsWorld>();
        GE_CORE_INFO("PhysicsServer initialized with collision detection and constraint solver");
    }

    void PhysicsServer::Shutdown() {
        m_PhysicsWorld.reset();
        GE_CORE_INFO("PhysicsServer shutdown");
    }

    void PhysicsServer::Step(float deltaTime) {
        if (m_PhysicsWorld) {
            m_PhysicsWorld->Update(deltaTime);
            
            // Update contact cache for debug visualization
            UpdateContactCache();
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
    
    void PhysicsServer::UpdateContactCache() {
        m_Contacts.clear();
        m_CollisionShapes.clear();
        
        if (!m_PhysicsWorld) return;
        
        // Convert PhysicsWorld contact manifolds to debug contact points
        const auto& manifolds = m_PhysicsWorld->GetContactManifolds();
        for (const auto& manifold : manifolds) {
            for (const auto& contact : manifold.GetContacts()) {
                ContactPoint cp;
                cp.position = (contact.PositionA + contact.PositionB) * 0.5f;
                cp.normal = contact.Normal;
                cp.penetration = contact.Penetration;
                m_Contacts.push_back(cp);
            }
        }
        
        // Get collision shapes from rigid bodies
        const auto& bodies = m_PhysicsWorld->GetRigidBodies();
        for (const auto& body : bodies) {
            if (body->HasCollisionShape()) {
                auto shape = body->GetCollisionShape();
                CollisionShape cs;
                
                switch (shape->GetType()) {
                    case Physics::CollisionShape::ShapeType::Sphere:
                        cs.type = CollisionShape::Sphere;
                        break;
                    case Physics::CollisionShape::ShapeType::Box:
                        cs.type = CollisionShape::Box;
                        break;
                    case Physics::CollisionShape::ShapeType::Capsule:
                        cs.type = CollisionShape::Capsule;
                        break;
                    default:
                        cs.type = CollisionShape::Mesh;
                        break;
                }
                
                cs.position = body->GetPosition();
                cs.rotation = body->GetRotation();
                cs.scale = glm::vec3(1.0f); // TODO: Get actual scale from shape
                m_CollisionShapes.push_back(cs);
            }
        }
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

