#include "RigidBodyComponent.hpp"
#include "TransformComponent.hpp"
#include "ColliderComponent.hpp"
#include "../Scene.hpp"
#include "../../Core/Logger.hpp"

namespace GameEngine {

    RigidBodyComponent::RigidBodyComponent() {
    }

    nlohmann::json RigidBodyComponent::Serialize() const {
        nlohmann::json j;
        j["type"] = GetTypeName();
        j["enabled"] = Enabled;
        j["bodyType"] = static_cast<int>(Type);
        j["mass"] = Mass;
        j["linearDamping"] = LinearDamping;
        j["angularDamping"] = AngularDamping;
        j["useGravity"] = UseGravity;
        j["isKinematic"] = IsKinematic;
        j["linearVelocity"] = { LinearVelocity.x, LinearVelocity.y, LinearVelocity.z };
        j["angularVelocity"] = { AngularVelocity.x, AngularVelocity.y, AngularVelocity.z };
        return j;
    }

    void RigidBodyComponent::Deserialize(const nlohmann::json& data) {
        if (data.contains("enabled"))
            Enabled = data["enabled"];
        
        if (data.contains("bodyType"))
            Type = static_cast<BodyType>(data["bodyType"].get<int>());
        
        if (data.contains("mass"))
            Mass = data["mass"];
        
        if (data.contains("linearDamping"))
            LinearDamping = data["linearDamping"];
        
        if (data.contains("angularDamping"))
            AngularDamping = data["angularDamping"];
        
        if (data.contains("useGravity"))
            UseGravity = data["useGravity"];
        
        if (data.contains("isKinematic"))
            IsKinematic = data["isKinematic"];
        
        if (data.contains("linearVelocity")) {
            auto vel = data["linearVelocity"];
            LinearVelocity = glm::vec3(vel[0], vel[1], vel[2]);
        }
        
        if (data.contains("angularVelocity")) {
            auto vel = data["angularVelocity"];
            AngularVelocity = glm::vec3(vel[0], vel[1], vel[2]);
        }
    }

    void RigidBodyComponent::OnCreate() {
        // Create physics rigid body
        Physics::RigidBody::BodyType physicsType;
        switch (Type) {
            case BodyType::Static:    physicsType = Physics::RigidBody::BodyType::Static; break;
            case BodyType::Kinematic: physicsType = Physics::RigidBody::BodyType::Kinematic; break;
            case BodyType::Dynamic:   
            default:
                physicsType = Physics::RigidBody::BodyType::Dynamic; break;
        }
        
        m_RigidBody = CreateRef<Physics::RigidBody>(physicsType, Mass);
        
        // Set properties
        m_RigidBody->SetLinearDamping(LinearDamping);
        m_RigidBody->SetAngularDamping(AngularDamping);
        m_RigidBody->SetUseGravity(UseGravity);
        m_RigidBody->SetLinearVelocity(LinearVelocity);
        m_RigidBody->SetAngularVelocity(AngularVelocity);
        
        // Sync initial position from transform
        SyncFromTransform();
        
        // Check if there's a collider component and get its shape
        if (GetOwnerEntity() && GetOwnerEntity().HasComponent<ColliderComponent>()) {
            auto& collider = GetOwnerEntity().GetComponent<ColliderComponent>();
            if (collider.GetShape()) {
                m_RigidBody->SetCollisionShape(collider.GetShape());
                GE_CORE_DEBUG("RigidBodyComponent: Attached collision shape from ColliderComponent");
            }
        }
        
        // Register with physics world
        if (GetOwnerEntity()) {
            Scene* scene = GetOwnerEntity().GetScene();
            if (scene) {
                auto physicsWorld = scene->GetPhysicsWorld();
                if (physicsWorld) {
                    physicsWorld->AddRigidBody(m_RigidBody);
                    GE_CORE_DEBUG("RigidBodyComponent: Added to physics world");
                }
            }
        }
    }

    void RigidBodyComponent::OnFixedUpdate(float fixedDeltaTime) {
        // Sync transform from physics body after physics step
        // Only for dynamic bodies - static and kinematic are controlled externally
        if (!m_RigidBody || !Enabled) return;
        
        if (Type == BodyType::Dynamic) {
            SyncToTransform();
        } else if (Type == BodyType::Kinematic) {
            // Kinematic bodies: sync physics from transform
            SyncFromTransform();
        }
    }

    void RigidBodyComponent::OnDestroy() {
        // Unregister from physics world
        if (m_RigidBody && GetOwnerEntity()) {
            Scene* scene = GetOwnerEntity().GetScene();
            if (scene) {
                auto physicsWorld = scene->GetPhysicsWorld();
                if (physicsWorld) {
                    physicsWorld->RemoveRigidBody(m_RigidBody);
                    GE_CORE_DEBUG("RigidBodyComponent: Removed from physics world");
                }
            }
        }
        m_RigidBody.reset();
    }

    void RigidBodyComponent::SyncFromTransform() {
        if (!GetOwnerEntity() || !m_RigidBody) return;
        
        if (GetOwnerEntity().HasComponent<TransformComponent>()) {
            auto& transform = GetOwnerEntity().GetComponent<TransformComponent>();
            m_RigidBody->SetPosition(transform.GetWorldPosition());
            m_RigidBody->SetRotation(transform.GetWorldRotation());
        }
    }

    void RigidBodyComponent::SyncToTransform() {
        if (!GetOwnerEntity() || !m_RigidBody) return;
        
        if (GetOwnerEntity().HasComponent<TransformComponent>()) {
            auto& transform = GetOwnerEntity().GetComponent<TransformComponent>();
            transform.SetWorldPosition(m_RigidBody->GetPosition());
            transform.Rotation = m_RigidBody->GetRotation();
            transform.SetDirty();  // Mark transform cache as dirty so rendering sees the change
        }
    }

    void RigidBodyComponent::ApplyForce(const glm::vec3& force) {
        if (m_RigidBody) m_RigidBody->ApplyForce(force);
    }

    void RigidBodyComponent::ApplyForceAtPoint(const glm::vec3& force, const glm::vec3& point) {
        if (m_RigidBody) m_RigidBody->ApplyForceAtPoint(force, point);
    }

    void RigidBodyComponent::ApplyTorque(const glm::vec3& torque) {
        if (m_RigidBody) m_RigidBody->ApplyTorque(torque);
    }

    void RigidBodyComponent::ApplyImpulse(const glm::vec3& impulse) {
        if (m_RigidBody) m_RigidBody->ApplyImpulse(impulse);
    }

    void RigidBodyComponent::SetLinearVelocity(const glm::vec3& velocity) {
        LinearVelocity = velocity;
        if (m_RigidBody) m_RigidBody->SetLinearVelocity(velocity);
    }

    glm::vec3 RigidBodyComponent::GetLinearVelocity() const {
        return m_RigidBody ? m_RigidBody->GetLinearVelocity() : LinearVelocity;
    }

    void RigidBodyComponent::SetAngularVelocity(const glm::vec3& velocity) {
        AngularVelocity = velocity;
        if (m_RigidBody) m_RigidBody->SetAngularVelocity(velocity);
    }

    glm::vec3 RigidBodyComponent::GetAngularVelocity() const {
        return m_RigidBody ? m_RigidBody->GetAngularVelocity() : AngularVelocity;
    }
}