#include "HingeJoint.hpp"
#include <glm/gtx/quaternion.hpp>

namespace GameEngine {
namespace Physics {

    HingeJoint::HingeJoint(RigidBody* bodyA, RigidBody* bodyB,
                          const glm::vec3& anchorA, const glm::vec3& anchorB,
                          const glm::vec3& axisA, const glm::vec3& axisB)
        : m_BodyA(bodyA)
        , m_BodyB(bodyB)
        , m_AnchorA(anchorA)
        , m_AnchorB(anchorB)
        , m_AxisA(glm::normalize(axisA))
        , m_AxisB(glm::normalize(axisB))
        , m_EffectiveMass(1.0f)
        , m_Bias(0.0f)
        , m_AccumulatedImpulse(0.0f) {
    }

    void HingeJoint::Prepare(float deltaTime) {
        if (!Enabled) return;
        
        // Calculate world anchor points
        glm::vec3 worldAnchorA = m_BodyA->GetPosition() + m_BodyA->GetRotation() * m_AnchorA;
        glm::vec3 worldAnchorB = m_BodyB->GetPosition() + m_BodyB->GetRotation() * m_AnchorB;
        
        // Calculate relative position from center of mass
        glm::vec3 rA = worldAnchorA - m_BodyA->GetPosition();
        glm::vec3 rB = worldAnchorB - m_BodyB->GetPosition();
        
        // Calculate effective mass for point-to-point constraint
        float invMassA = m_BodyA->GetInverseMass();
        float invMassB = m_BodyB->GetInverseMass();
        
        glm::mat3 invInertiaA = m_BodyA->GetInverseInertiaTensor();
        glm::mat3 invInertiaB = m_BodyB->GetInverseInertiaTensor();
        
        // Skew symmetric matrix for cross product
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
        
        m_EffectiveMass = glm::inverse(K);
        
        // Calculate bias (Baumgarte stabilization)
        glm::vec3 positionError = worldAnchorB - worldAnchorA;
        const float baumgarte = 0.2f;
        m_Bias = (baumgarte / deltaTime) * positionError;
        
        // Warm start
        m_BodyA->ApplyImpulse(-m_AccumulatedImpulse);
        m_BodyB->ApplyImpulse(m_AccumulatedImpulse);
        m_BodyA->ApplyAngularImpulse(-glm::cross(rA, m_AccumulatedImpulse));
        m_BodyB->ApplyAngularImpulse(glm::cross(rB, m_AccumulatedImpulse));
    }

    void HingeJoint::Solve() {
        if (!Enabled) return;
        
        // Calculate world anchor points
        glm::vec3 worldAnchorA = m_BodyA->GetPosition() + m_BodyA->GetRotation() * m_AnchorA;
        glm::vec3 worldAnchorB = m_BodyB->GetPosition() + m_BodyB->GetRotation() * m_AnchorB;
        
        glm::vec3 rA = worldAnchorA - m_BodyA->GetPosition();
        glm::vec3 rB = worldAnchorB - m_BodyB->GetPosition();
        
        // Calculate relative velocity at anchor points
        glm::vec3 vA = m_BodyA->GetLinearVelocity() + glm::cross(m_BodyA->GetAngularVelocity(), rA);
        glm::vec3 vB = m_BodyB->GetLinearVelocity() + glm::cross(m_BodyB->GetAngularVelocity(), rB);
        glm::vec3 relativeVelocity = vB - vA;
        
        // Calculate constraint violation
        glm::vec3 Cdot = relativeVelocity + m_Bias;
        
        // Calculate impulse
        glm::vec3 lambda = m_EffectiveMass * (-Cdot);
        
        // Accumulate impulse
        m_AccumulatedImpulse += lambda;
        
        // Apply impulse
        m_BodyA->ApplyImpulse(-lambda);
        m_BodyB->ApplyImpulse(lambda);
        m_BodyA->ApplyAngularImpulse(-glm::cross(rA, lambda));
        m_BodyB->ApplyAngularImpulse(glm::cross(rB, lambda));
        
        // Angular constraint - keep axes aligned
        glm::vec3 worldAxisA = m_BodyA->GetRotation() * m_AxisA;
        glm::vec3 worldAxisB = m_BodyB->GetRotation() * m_AxisB;
        
        // Calculate two perpendicular axes to constrain
        glm::vec3 perpA = glm::normalize(glm::cross(worldAxisA, 
            (std::abs(glm::dot(worldAxisA, glm::vec3(1, 0, 0))) < 0.9f) ?
            glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0)));
        glm::vec3 perpB = glm::cross(worldAxisA, perpA);
        
        // Calculate angular errors
        float angularErrorA = glm::dot(worldAxisB, perpA);
        float angularErrorB = glm::dot(worldAxisB, perpB);
        
        // Apply angular correction
        glm::mat3 invInertiaA = m_BodyA->GetInverseInertiaTensor();
        glm::mat3 invInertiaB = m_BodyB->GetInverseInertiaTensor();
        
        float kA = glm::dot(perpA, invInertiaA * perpA) + glm::dot(perpA, invInertiaB * perpA);
        float kB = glm::dot(perpB, invInertiaA * perpB) + glm::dot(perpB, invInertiaB * perpB);
        // Note: both bodies contribute inertia resistance to angular corrections
        
        if (kA > 0.0f) {
            float lambdaAngA = -angularErrorA / kA * 0.2f;
            m_BodyA->ApplyAngularImpulse(-perpA * lambdaAngA);
            m_BodyB->ApplyAngularImpulse(perpA * lambdaAngA);
        }
        
        if (kB > 0.0f) {
            float lambdaAngB = -angularErrorB / kB * 0.2f;
            m_BodyA->ApplyAngularImpulse(-perpB * lambdaAngB);
            m_BodyB->ApplyAngularImpulse(perpB * lambdaAngB);
        }
        
        // Motor
        if (UseMotor) {
            glm::vec3 relativeAngular = m_BodyB->GetAngularVelocity() - m_BodyA->GetAngularVelocity();
            float currentSpeed = glm::dot(relativeAngular, worldAxisA);
            float motorError = MotorSpeed - currentSpeed;
            
            float motorK = glm::dot(worldAxisA, invInertiaA * worldAxisA) + 
                          glm::dot(worldAxisA, invInertiaB * worldAxisA);
            
            if (motorK > 0.0f) {
                float motorLambda = motorError / motorK;
                motorLambda = glm::clamp(motorLambda, -MaxMotorTorque, MaxMotorTorque);
                
                m_BodyA->ApplyAngularImpulse(-worldAxisA * motorLambda);
                m_BodyB->ApplyAngularImpulse(worldAxisA * motorLambda);
            }
        }
    }

    float HingeJoint::GetCurrentAngle() const {
        glm::vec3 worldAxisA = m_BodyA->GetRotation() * m_AxisA;
        
        // Find a reference direction perpendicular to the hinge axis
        glm::vec3 refA = glm::cross(worldAxisA, 
            (std::abs(glm::dot(worldAxisA, glm::vec3(1, 0, 0))) < 0.9f) ?
            glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0));
        refA = m_BodyA->GetRotation() * refA;
        
        glm::vec3 refB = m_BodyB->GetRotation() * refA;
        
        float dot = glm::clamp(glm::dot(refA, refB), -1.0f, 1.0f);
        float cross = glm::dot(worldAxisA, glm::cross(refA, refB));
        
        return std::atan2(cross, dot);
    }

}}
