#pragma once

#include "../Core/Base.hpp"
#include "Shapes/CollisionShape.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace GameEngine {
namespace Physics {

    /**
     * @brief Rigid body for 3D physics simulation
     * 
     * Features:
     * - Mass and inertia tensor
     * - Linear and angular velocity
     * - Force and torque accumulation
     * - Collision shape
     * - Verlet integration
     * - Sleep/wake system for optimization
     */
    class RigidBody {
    public:
        enum class BodyType {
            Static,      // Infinite mass, never moves
            Kinematic,   // Infinite mass, moves by velocity
            Dynamic      // Finite mass, affected by forces
        };
        
        RigidBody(BodyType type = BodyType::Dynamic, float mass = 1.0f);
        ~RigidBody() = default;
        
        /**
         * @brief Integrate physics (called by physics world)
         */
        void Integrate(float deltaTime);
        
        /**
         * @brief Integrate forces to velocities (step 1)
         */
        void IntegrateForces(float deltaTime);
        
        /**
         * @brief Integrate velocities to positions (step 2, after constraint solving)
         */
        void IntegrateVelocities(float deltaTime);
        
        /**
         * @brief Apply forces
         */
        void ApplyForce(const glm::vec3& force);
        void ApplyForceAtPoint(const glm::vec3& force, const glm::vec3& point);
        void ApplyTorque(const glm::vec3& torque);
        void ApplyImpulse(const glm::vec3& impulse);
        void ApplyAngularImpulse(const glm::vec3& impulse);
        
        /**
         * @brief Clear accumulated forces
         */
        void ClearForces();
        
        /**
         * @brief Position and rotation
         */
        void SetPosition(const glm::vec3& position) { m_Position = position; }
        glm::vec3 GetPosition() const { return m_Position; }
        
        void SetRotation(const glm::quat& rotation) { m_Rotation = rotation; }
        glm::quat GetRotation() const { return m_Rotation; }
        
        /**
         * @brief Velocity
         */
        void SetLinearVelocity(const glm::vec3& velocity) { m_LinearVelocity = velocity; }
        glm::vec3 GetLinearVelocity() const { return m_LinearVelocity; }
        
        void SetAngularVelocity(const glm::vec3& velocity) { m_AngularVelocity = velocity; }
        glm::vec3 GetAngularVelocity() const { return m_AngularVelocity; }
        
        /**
         * @brief Mass properties
         */
        void SetMass(float mass);
        float GetMass() const { return m_Mass; }
        float GetInverseMass() const { return m_InverseMass; }
        
        void SetInertiaTensor(const glm::mat3& inertia);
        glm::mat3 GetInertiaTensor() const { return m_InertiaTensor; }
        glm::mat3 GetInverseInertiaTensor() const { return m_InverseInertiaTensor; }
        
        /**
         * @brief Damping (friction)
         */
        void SetLinearDamping(float damping) { m_LinearDamping = damping; }
        float GetLinearDamping() const { return m_LinearDamping; }
        
        void SetAngularDamping(float damping) { m_AngularDamping = damping; }
        float GetAngularDamping() const { return m_AngularDamping; }
        
        /**
         * @brief Gravity
         */
        void SetUseGravity(bool useGravity) { m_UseGravity = useGravity; }
        bool GetUseGravity() const { return m_UseGravity; }
        
        /**
         * @brief Body type
         */
        void SetBodyType(BodyType type);
        BodyType GetBodyType() const { return m_BodyType; }
        
        /**
         * @brief Sleep state (optimization)
         */
        void SetAwake(bool awake) { m_IsAwake = awake; }
        bool IsAwake() const { return m_IsAwake; }
        
        /**
         * @brief Check if body can sleep
         */
        bool CanSleep() const;
        
        /**
         * @brief Collision shape
         */
        void SetCollisionShape(const Ref<CollisionShape>& shape) { m_CollisionShape = shape; }
        Ref<CollisionShape> GetCollisionShape() const { return m_CollisionShape; }
        bool HasCollisionShape() const { return m_CollisionShape != nullptr; }
        
    private:
        void UpdateInertiaTensor();
        
    private:
        BodyType m_BodyType;
        
        // Collision
        Ref<CollisionShape> m_CollisionShape;
        
        // Transform
        glm::vec3 m_Position;
        glm::quat m_Rotation;
        
        // Linear motion
        glm::vec3 m_LinearVelocity;
        glm::vec3 m_ForceAccumulator;
        
        // Angular motion
        glm::vec3 m_AngularVelocity;
        glm::vec3 m_TorqueAccumulator;
        
        // Mass properties
        float m_Mass;
        float m_InverseMass;
        glm::mat3 m_InertiaTensor;
        glm::mat3 m_InverseInertiaTensor;
        
        // Damping
        float m_LinearDamping;
        float m_AngularDamping;
        
        // State
        bool m_UseGravity;
        bool m_IsAwake;
        float m_Motion; // For sleep calculation
    };

}}
