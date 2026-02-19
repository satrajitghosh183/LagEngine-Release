#pragma once

#include "Constraint.hpp"

namespace GameEngine {
namespace Physics {

    /**
     * @brief Slider joint (prismatic joint)
     * 
     * Allows linear motion along a single axis
     */
    class SliderJoint : public Constraint {
    public:
        SliderJoint(RigidBody* bodyA, RigidBody* bodyB,
                   const glm::vec3& axis);
        
        void Prepare(float deltaTime) override;
        void Solve() override;
        
        RigidBody* GetBodyA() const override { return m_BodyA; }
        RigidBody* GetBodyB() const override { return m_BodyB; }
        
        /**
         * @brief Linear limits
         */
        bool UseLimits = false;
        float LowerLimit = -1.0f;
        float UpperLimit = 1.0f;
        
        /**
         * @brief Motor
         */
        bool UseMotor = false;
        float MotorSpeed = 0.0f;
        float MaxMotorForce = 100.0f;
        
    private:
        RigidBody* m_BodyA;
        RigidBody* m_BodyB;
        glm::vec3 m_Axis;
        float m_AccumulatedImpulse;
        float m_DeltaTime = 0.016f;
    };

}}