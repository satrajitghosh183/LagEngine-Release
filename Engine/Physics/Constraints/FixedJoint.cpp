#include "FixedJoint.hpp"
#include <glm/gtx/quaternion.hpp>

namespace GameEngine {
namespace Physics {

    FixedJoint::FixedJoint(RigidBody* bodyA, RigidBody* bodyB,
                          const glm::vec3& anchorA, const glm::vec3& anchorB)
        : m_BodyA(bodyA)
        , m_BodyB(bodyB)
        , m_AnchorA(anchorA)
        , m_AnchorB(anchorB)
        , m_AccumulatedImpulse(0.0f) {
        
        // Store initial relative rotation
        m_RelativeRotation = glm::inverse(m_BodyA->GetRotation()) * m_BodyB->GetRotation();
    }

    void FixedJoint::Prepare(float deltaTime) {
        if (!Enabled) return;
        m_DeltaTime = deltaTime > 0.0f ? deltaTime : 0.016f;

        // Warm start
        glm::vec3 worldAnchorA = m_BodyA->GetPosition() + m_BodyA->GetRotation() * m_AnchorA;
        glm::vec3 worldAnchorB = m_BodyB->GetPosition() + m_BodyB->GetRotation() * m_AnchorB;
        
        glm::vec3 rA = worldAnchorA - m_BodyA->GetPosition();
        glm::vec3 rB = worldAnchorB - m_BodyB->GetPosition();
        
        m_BodyA->ApplyImpulse(-m_AccumulatedImpulse);
        m_BodyB->ApplyImpulse(m_AccumulatedImpulse);
        m_BodyA->ApplyAngularImpulse(-glm::cross(rA, m_AccumulatedImpulse));
        m_BodyB->ApplyAngularImpulse(glm::cross(rB, m_AccumulatedImpulse));
    }

    void FixedJoint::Solve() {
        if (!Enabled) return;
        
        // Point-to-point constraint
        glm::vec3 worldAnchorA = m_BodyA->GetPosition() + m_BodyA->GetRotation() * m_AnchorA;
        glm::vec3 worldAnchorB = m_BodyB->GetPosition() + m_BodyB->GetRotation() * m_AnchorB;
        
        glm::vec3 rA = worldAnchorA - m_BodyA->GetPosition();
        glm::vec3 rB = worldAnchorB - m_BodyB->GetPosition();
        
        // Calculate relative velocity
        glm::vec3 vA = m_BodyA->GetLinearVelocity() + glm::cross(m_BodyA->GetAngularVelocity(), rA);
        glm::vec3 vB = m_BodyB->GetLinearVelocity() + glm::cross(m_BodyB->GetAngularVelocity(), rB);
        glm::vec3 relativeVelocity = vB - vA;
        
        // Position error
        glm::vec3 positionError = worldAnchorB - worldAnchorA;
        const float baumgarte = 0.2f;
        glm::vec3 bias = baumgarte * positionError / m_DeltaTime;  // Assuming ~60fps
        
        // Effective mass
        float invMassA = m_BodyA->GetInverseMass();
        float invMassB = m_BodyB->GetInverseMass();
        
        glm::mat3 invInertiaA = m_BodyA->GetInverseInertiaTensor();
        glm::mat3 invInertiaB = m_BodyB->GetInverseInertiaTensor();
        
        auto skew = [](const glm::vec3& v) -> glm::mat3 {
            return glm::mat3(
                0.0f, v.z, -v.y,
                -v.z, 0.0f, v.x,
                v.y, -v.x, 0.0f
            );
        };
        
        glm::mat3 skewA = skew(rA);
        glm::mat3 skewB = skew(rB);
        
        glm::mat3 K = glm::mat3(invMassA + invMassB) 
                    - skewA * invInertiaA * glm::transpose(skewA)
                    - skewB * invInertiaB * glm::transpose(skewB);
        
        glm::mat3 effectiveMass = glm::inverse(K);
        
        // Calculate and apply impulse
        glm::vec3 Cdot = relativeVelocity + bias;
        glm::vec3 lambda = effectiveMass * (-Cdot);
        
        m_AccumulatedImpulse += lambda;
        
        m_BodyA->ApplyImpulse(-lambda);
        m_BodyB->ApplyImpulse(lambda);
        m_BodyA->ApplyAngularImpulse(-glm::cross(rA, lambda));
        m_BodyB->ApplyAngularImpulse(glm::cross(rB, lambda));
        
        // Angular constraint - maintain relative rotation
        glm::quat currentRelative = glm::inverse(m_BodyA->GetRotation()) * m_BodyB->GetRotation();
        glm::quat rotationError = currentRelative * glm::inverse(m_RelativeRotation);
        
        // Convert quaternion error to axis-angle
        if (rotationError.w < 0.0f) {
            rotationError = -rotationError;  // Ensure shortest path
        }
        
        glm::vec3 angularError(rotationError.x, rotationError.y, rotationError.z);
        angularError *= 2.0f;  // Approximate conversion for small angles
        
        // Relative angular velocity
        glm::vec3 relativeAngular = m_BodyB->GetAngularVelocity() - m_BodyA->GetAngularVelocity();
        
        // Angular bias
        glm::vec3 angularBias = baumgarte * angularError / m_DeltaTime;
        
        // Angular effective mass
        glm::mat3 angularK = invInertiaA + invInertiaB;
        glm::mat3 angularEffectiveMass = glm::inverse(angularK);
        
        // Calculate and apply angular impulse
        glm::vec3 angularCdot = relativeAngular + angularBias;
        glm::vec3 angularLambda = angularEffectiveMass * (-angularCdot);
        
        m_BodyA->ApplyAngularImpulse(-angularLambda);
        m_BodyB->ApplyAngularImpulse(angularLambda);
    }

    void FixedJoint::SolvePosition() {
        if (!Enabled) return;

        // Linear anchor position correction
        glm::vec3 worldAnchorA = m_BodyA->GetPosition() + m_BodyA->GetRotation() * m_AnchorA;
        glm::vec3 worldAnchorB = m_BodyB->GetPosition() + m_BodyB->GetRotation() * m_AnchorB;
        glm::vec3 posError = worldAnchorB - worldAnchorA;

        float invMassA = m_BodyA->GetInverseMass();
        float invMassB = m_BodyB->GetInverseMass();
        float totalInvMass = invMassA + invMassB;
        if (totalInvMass > 0.0001f) {
            const float correctionScale = 0.5f;
            glm::vec3 correction = posError * correctionScale;
            m_BodyA->SetPosition(m_BodyA->GetPosition() + correction * (invMassA / totalInvMass));
            m_BodyB->SetPosition(m_BodyB->GetPosition() - correction * (invMassB / totalInvMass));
        }
    }

}}
