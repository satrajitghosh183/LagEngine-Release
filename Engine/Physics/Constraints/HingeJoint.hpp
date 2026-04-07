#pragma once

#include "Constraint.hpp"

namespace GameEngine {
namespace Physics {

    /**
     * @brief Hinge joint (revolute joint)
     * 
     * Allows rotation around a single axis
     * Can have angular limits and motor
     */
    class HingeJoint : public Constraint {
    public:
        HingeJoint(RigidBody* bodyA, RigidBody* bodyB,
                  const glm::vec3& anchorA, const glm::vec3& anchorB,
                  const glm::vec3& axisA, const glm::vec3& axisB);
        
        void Prepare(float deltaTime) override;
        void Solve() override;
        void SolvePosition() override;
        
        RigidBody* GetBodyA() const override { return m_BodyA; }
        RigidBody* GetBodyB() const override { return m_BodyB; }
        
        /**
         * @brief Angular limits
         */
        bool UseLimits = false;
        float LowerLimit = -glm::pi<float>();
        float UpperLimit = glm::pi<float>();
        
        /**
         * @brief Motor
         */
        bool UseMotor = false;
        float MotorSpeed = 0.0f;
        float MaxMotorTorque = 100.0f;
        
        /**
         * @brief Get current angle
         */
        float GetCurrentAngle() const;
        
    private:
        RigidBody* m_BodyA;
        RigidBody* m_BodyB;
        glm::vec3 m_AnchorA;
        glm::vec3 m_AnchorB;
        glm::vec3 m_AxisA;
        glm::vec3 m_AxisB;
        
        // Cached solve data
        glm::mat3 m_EffectiveMass;
        glm::vec3 m_Bias;
        glm::vec3 m_AccumulatedImpulse;
    };

}}