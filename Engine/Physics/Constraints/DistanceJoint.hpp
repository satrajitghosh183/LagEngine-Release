#pragma once

#include "Constraint.hpp"

namespace GameEngine {
namespace Physics {

    /**
     * @brief Distance constraint (rope/spring)
     * 
     * Maintains a fixed distance between two anchor points
     */
    class DistanceJoint : public Constraint {
    public:
        DistanceJoint(RigidBody* bodyA, RigidBody* bodyB,
                     const glm::vec3& anchorA, const glm::vec3& anchorB,
                     float distance = -1.0f);
        
        void Prepare(float deltaTime) override;
        void Solve() override;
        void SolvePosition() override;
        
        RigidBody* GetBodyA() const override { return m_BodyA; }
        RigidBody* GetBodyB() const override { return m_BodyB; }
        
        /**
         * @brief Get/Set distance
         */
        float GetDistance() const { return m_Distance; }
        void SetDistance(float distance) { m_Distance = distance; }
        
        /**
         * @brief Spring properties (if springy)
         */
        bool IsSpring = false;
        float SpringStiffness = 100.0f;
        float SpringDamping = 10.0f;
        
    private:
        RigidBody* m_BodyA;
        RigidBody* m_BodyB;
        glm::vec3 m_AnchorA;  // Local space anchor on body A
        glm::vec3 m_AnchorB;  // Local space anchor on body B
        float m_Distance;
        
        // Cached data for solving
        float m_EffectiveMass;
        float m_Bias;
        float m_AccumulatedImpulse;
    };

}}