#pragma once

#include "Component.hpp"
#include "../../Physics/RigidBody.hpp"
#include <glm/glm.hpp>

namespace GameEngine {

    /**
     * @brief Rigid body component
     * 
     * Links entity to physics system
     */
    class RigidBodyComponent : public Component {
    public:
        enum class BodyType {
            Static = 0,
            Kinematic = 1,
            Dynamic = 2
        };
        
        RigidBodyComponent();
        
        COMPONENT_TYPE(RigidBodyComponent)
        
        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& data) override;
        
        void OnCreate() override;
        void OnFixedUpdate(float fixedDeltaTime) override;
        void OnDestroy() override;
        
        /**
         * @brief Sync physics body with transform
         */
        void SyncFromTransform();
        void SyncToTransform();
        
        /**
         * @brief Get physics body
         */
        Ref<Physics::RigidBody> GetRigidBody() const { return m_RigidBody; }
        
        /**
         * @brief Apply forces
         */
        void ApplyForce(const glm::vec3& force);
        void ApplyForceAtPoint(const glm::vec3& force, const glm::vec3& point);
        void ApplyTorque(const glm::vec3& torque);
        void ApplyImpulse(const glm::vec3& impulse);
        
        /**
         * @brief Velocity
         */
        void SetLinearVelocity(const glm::vec3& velocity);
        glm::vec3 GetLinearVelocity() const;
        
        void SetAngularVelocity(const glm::vec3& velocity);
        glm::vec3 GetAngularVelocity() const;
        
    public:
        BodyType Type = BodyType::Dynamic;
        float Mass = 1.0f;
        float LinearDamping = 0.1f;
        float AngularDamping = 0.1f;
        bool UseGravity = true;
        bool IsKinematic = false;
        float Restitution = 0.3f;  // Bounciness (0=no bounce, 1=fully elastic)
        glm::vec3 LinearVelocity = glm::vec3(0.0f);
        glm::vec3 AngularVelocity = glm::vec3(0.0f);
        
    private:
        Ref<Physics::RigidBody> m_RigidBody;
    };
}