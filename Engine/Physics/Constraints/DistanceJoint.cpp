#include "DistanceJoint.hpp"
#include <glm/gtx/quaternion.hpp>

namespace GameEngine {
namespace Physics {

    DistanceJoint::DistanceJoint(RigidBody* bodyA, RigidBody* bodyB,
                                 const glm::vec3& anchorA, const glm::vec3& anchorB,
                                 float distance)
        : m_BodyA(bodyA)
        , m_BodyB(bodyB)
        , m_AnchorA(anchorA)
        , m_AnchorB(anchorB)
        , m_EffectiveMass(0.0f)
        , m_Bias(0.0f)
        , m_AccumulatedImpulse(0.0f) {
        
        if (distance < 0.0f) {
            // Calculate initial distance
            glm::vec3 worldAnchorA = bodyA->GetPosition() + bodyA->GetRotation() * anchorA;
            glm::vec3 worldAnchorB = bodyB->GetPosition() + bodyB->GetRotation() * anchorB;
            m_Distance = glm::length(worldAnchorB - worldAnchorA);
        } else {
            m_Distance = distance;
        }
    }

    void DistanceJoint::Prepare(float deltaTime) {
        if (!Enabled) return;
        
        // Calculate world anchor points
        glm::vec3 worldAnchorA = m_BodyA->GetPosition() + m_BodyA->GetRotation() * m_AnchorA;
        glm::vec3 worldAnchorB = m_BodyB->GetPosition() + m_BodyB->GetRotation() * m_AnchorB;
        
        // Calculate constraint axis
        glm::vec3 delta = worldAnchorB - worldAnchorA;
        float currentDistance = glm::length(delta);
        
        if (currentDistance < 0.0001f) return;
        
        glm::vec3 normal = delta / currentDistance;
        
        // Calculate relative position from center of mass
        glm::vec3 rA = worldAnchorA - m_BodyA->GetPosition();
        glm::vec3 rB = worldAnchorB - m_BodyB->GetPosition();
        
        // Calculate effective mass
        float invMassA = m_BodyA->GetInverseMass();
        float invMassB = m_BodyB->GetInverseMass();
        
        glm::mat3 invInertiaA = m_BodyA->GetInverseInertiaTensor();
        glm::mat3 invInertiaB = m_BodyB->GetInverseInertiaTensor();
        
        glm::vec3 angularComponentA = glm::cross(rA, normal);
        glm::vec3 angularComponentB = glm::cross(rB, normal);
        
        float angularMassA = glm::dot(angularComponentA, invInertiaA * angularComponentA);
        float angularMassB = glm::dot(angularComponentB, invInertiaB * angularComponentB);
        
        m_EffectiveMass = invMassA + invMassB + angularMassA + angularMassB;
        
        if (m_EffectiveMass > 0.0f) {
            m_EffectiveMass = 1.0f / m_EffectiveMass;
        }
        
        // Calculate bias (Baumgarte stabilization)
        float error = currentDistance - m_Distance;
        const float baumgarte = 0.2f;
        m_Bias = (baumgarte / deltaTime) * error;
        
        // Warm start
        glm::vec3 impulse = normal * m_AccumulatedImpulse;
        m_BodyA->ApplyImpulse(-impulse);
        m_BodyB->ApplyImpulse(impulse);
        m_BodyA->ApplyAngularImpulse(-glm::cross(rA, impulse));
        m_BodyB->ApplyAngularImpulse(glm::cross(rB, impulse));
    }

    void DistanceJoint::Solve() {
        if (!Enabled) return;
        
        // Calculate world anchor points
        glm::vec3 worldAnchorA = m_BodyA->GetPosition() + m_BodyA->GetRotation() * m_AnchorA;
        glm::vec3 worldAnchorB = m_BodyB->GetPosition() + m_BodyB->GetRotation() * m_AnchorB;
        
        glm::vec3 delta = worldAnchorB - worldAnchorA;
        float currentDistance = glm::length(delta);
        
        if (currentDistance < 0.0001f) return;
        
        glm::vec3 normal = delta / currentDistance;
        
        glm::vec3 rA = worldAnchorA - m_BodyA->GetPosition();
        glm::vec3 rB = worldAnchorB - m_BodyB->GetPosition();
        
        // Calculate relative velocity at contact point
        glm::vec3 vA = m_BodyA->GetLinearVelocity() + glm::cross(m_BodyA->GetAngularVelocity(), rA);
        glm::vec3 vB = m_BodyB->GetLinearVelocity() + glm::cross(m_BodyB->GetAngularVelocity(), rB);
        glm::vec3 relativeVelocity = vB - vA;
        
        // Calculate constraint violation
        float constraintViolation = glm::dot(relativeVelocity, normal) + m_Bias;
        
        // Calculate impulse
        float lambda = -m_EffectiveMass * constraintViolation;
        
        // Accumulate impulse
        m_AccumulatedImpulse += lambda;
        
        // Apply impulse
        glm::vec3 impulse = normal * lambda;
        m_BodyA->ApplyImpulse(-impulse);
        m_BodyB->ApplyImpulse(impulse);
        m_BodyA->ApplyAngularImpulse(-glm::cross(rA, impulse));
        m_BodyB->ApplyAngularImpulse(glm::cross(rB, impulse));
    }

    void DistanceJoint::SolvePosition() {
        if (!Enabled || IsSpring) return;

        glm::vec3 worldAnchorA = m_BodyA->GetPosition() + m_BodyA->GetRotation() * m_AnchorA;
        glm::vec3 worldAnchorB = m_BodyB->GetPosition() + m_BodyB->GetRotation() * m_AnchorB;
        glm::vec3 delta = worldAnchorB - worldAnchorA;
        float currentDistance = glm::length(delta);
        if (currentDistance < 0.0001f) return;

        float error = currentDistance - m_Distance;
        if (std::abs(error) < 0.0001f) return;

        glm::vec3 normal = delta / currentDistance;
        float invMassA = m_BodyA->GetInverseMass();
        float invMassB = m_BodyB->GetInverseMass();
        float totalInvMass = invMassA + invMassB;
        if (totalInvMass < 0.0001f) return;

        const float correctionScale = 0.5f;
        glm::vec3 correction = normal * error * correctionScale;
        m_BodyA->SetPosition(m_BodyA->GetPosition() + correction * (invMassA / totalInvMass));
        m_BodyB->SetPosition(m_BodyB->GetPosition() - correction * (invMassB / totalInvMass));
    }

}}