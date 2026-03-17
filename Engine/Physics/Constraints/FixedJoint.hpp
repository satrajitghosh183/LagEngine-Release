#pragma once

#include "Constraint.hpp"

namespace GameEngine {
namespace Physics {

    /**
     * @brief Fixed joint (weld joint)
     * 
     * Rigidly connects two bodies together
     */
    class FixedJoint : public Constraint {
    public:
        FixedJoint(RigidBody* bodyA, RigidBody* bodyB,
                  const glm::vec3& anchorA, const glm::vec3& anchorB);
        
        void Prepare(float deltaTime) override;
        void Solve() override;
        void SolvePosition() override;
        
        RigidBody* GetBodyA() const override { return m_BodyA; }
        RigidBody* GetBodyB() const override { return m_BodyB; }
        
    private:
        RigidBody* m_BodyA;
        RigidBody* m_BodyB;
        glm::vec3 m_AnchorA;
        glm::vec3 m_AnchorB;
        glm::quat m_RelativeRotation;
        glm::vec3 m_AccumulatedImpulse;
        float m_DeltaTime = 0.016f;
    };

}}