#pragma once

#include "../../Core/Base.hpp"
#include <glm/glm.hpp>
#include <cstdint>

namespace GameEngine {
namespace Physics2D {

    /**
     * @brief Material properties for 2D physics bodies and colliders
     */
    struct PhysicsMaterial2D {
        float Friction = 0.4f;
        float Restitution = 0.2f;
        float Density = 1.0f;
    };

    /**
     * @brief Rigid body for 2D physics simulation
     *
     * Features:
     * - Dynamic, static, and kinematic body types
     * - Mass and rotational inertia (auto-calculated from shape)
     * - Linear and angular velocity
     * - Force and torque accumulation
     * - Linear/angular damping
     * - Gravity scale
     * - Fixed rotation flag
     * - CCD flag
     * - Sleep state management
     */
    class RigidBody2D {
    public:
        enum class BodyType {
            Static,      // Infinite mass, never moves
            Kinematic,   // Infinite mass, moves by velocity
            Dynamic      // Finite mass, affected by forces
        };

        RigidBody2D(BodyType type = BodyType::Dynamic, float mass = 1.0f);
        ~RigidBody2D() = default;

        // ----- Integration (called by physics world) -----

        /**
         * @brief Integrate forces into velocities (step 1)
         */
        void integrateForces(float deltaTime, const glm::vec2& gravity);

        /**
         * @brief Integrate velocities into positions (step 2, after constraint solving)
         */
        void integrateVelocities(float deltaTime);

        // ----- Force / impulse / torque application -----

        void applyForce(const glm::vec2& force);
        void applyForceAtPoint(const glm::vec2& force, const glm::vec2& worldPoint);
        void applyTorque(float torque);
        void applyLinearImpulse(const glm::vec2& impulse);
        void applyLinearImpulseAtPoint(const glm::vec2& impulse, const glm::vec2& worldPoint);
        void applyAngularImpulse(float impulse);

        void clearForces();

        // ----- Position / rotation -----

        void setPosition(const glm::vec2& position) { m_Position = position; }
        glm::vec2 getPosition() const { return m_Position; }

        void setRotation(float radians) { m_Rotation = radians; }
        float getRotation() const { return m_Rotation; }

        // ----- Velocity -----

        void setLinearVelocity(const glm::vec2& velocity) { m_LinearVelocity = velocity; }
        glm::vec2 getLinearVelocity() const { return m_LinearVelocity; }

        void setAngularVelocity(float omega) { m_AngularVelocity = omega; }
        float getAngularVelocity() const { return m_AngularVelocity; }

        // ----- Mass properties -----

        void setMass(float mass);
        float getMass() const { return m_Mass; }
        float getInverseMass() const { return m_InverseMass; }

        void setInertia(float inertia);
        float getInertia() const { return m_Inertia; }
        float getInverseInertia() const { return m_InverseInertia; }

        // ----- Damping -----

        void setLinearDamping(float damping) { m_LinearDamping = damping; }
        float getLinearDamping() const { return m_LinearDamping; }

        void setAngularDamping(float damping) { m_AngularDamping = damping; }
        float getAngularDamping() const { return m_AngularDamping; }

        // ----- Gravity scale -----

        void setGravityScale(float scale) { m_GravityScale = scale; }
        float getGravityScale() const { return m_GravityScale; }

        // ----- Body type -----

        void setBodyType(BodyType type);
        BodyType getBodyType() const { return m_BodyType; }

        // ----- Flags -----

        void setFixedRotation(bool fixed) { m_FixedRotation = fixed; }
        bool isFixedRotation() const { return m_FixedRotation; }

        void setUseCCD(bool ccd) { m_UseCCD = ccd; }
        bool useCCD() const { return m_UseCCD; }

        // ----- Sleep state -----

        void setAwake(bool awake);
        bool isAwake() const { return m_IsAwake; }
        bool canSleep() const;

        // ----- Material -----

        void setMaterial(const PhysicsMaterial2D& mat) { m_Material = mat; }
        const PhysicsMaterial2D& getMaterial() const { return m_Material; }

        // ----- User data (e.g. entity index) -----

        void setUserData(void* data) { m_UserData = data; }
        void* getUserData() const { return m_UserData; }

        // ----- Unique id (set by PhysicsWorld2D) -----

        void setId(uint32_t id) { m_Id = id; }
        uint32_t getId() const { return m_Id; }

    private:
        BodyType m_BodyType;
        uint32_t m_Id = 0;

        // Transform
        glm::vec2 m_Position;
        float m_Rotation; // radians

        // Linear motion
        glm::vec2 m_LinearVelocity;
        glm::vec2 m_ForceAccumulator;

        // Angular motion
        float m_AngularVelocity;
        float m_TorqueAccumulator;

        // Mass properties
        float m_Mass;
        float m_InverseMass;
        float m_Inertia;        // Rotational inertia (scalar in 2D)
        float m_InverseInertia;

        // Damping
        float m_LinearDamping;
        float m_AngularDamping;

        // Gravity
        float m_GravityScale;

        // Flags
        bool m_FixedRotation;
        bool m_UseCCD;

        // Sleep
        bool m_IsAwake;
        float m_SleepTimer;

        // Material
        PhysicsMaterial2D m_Material;

        // User data
        void* m_UserData;
    };

}}
