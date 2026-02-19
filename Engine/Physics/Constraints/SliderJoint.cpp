#include "SliderJoint.hpp"
#include <glm/gtx/quaternion.hpp>

namespace GameEngine {
namespace Physics {

    SliderJoint::SliderJoint(RigidBody* bodyA, RigidBody* bodyB, const glm::vec3& axis)
        : m_BodyA(bodyA)
        , m_BodyB(bodyB)
        , m_Axis(glm::normalize(axis))
        , m_AccumulatedImpulse(0.0f) {
    }

    void SliderJoint::Prepare(float deltaTime) {
        if (!Enabled) return;

        m_DeltaTime = deltaTime > 0.0f ? deltaTime : 0.016f;

        // Warm start - apply accumulated impulse along constraint directions
        glm::vec3 worldAxis = m_BodyA->GetRotation() * m_Axis;
        
        // Find two perpendicular axes
        glm::vec3 perpA = glm::normalize(glm::cross(worldAxis,
            (std::abs(glm::dot(worldAxis, glm::vec3(1, 0, 0))) < 0.9f) ?
            glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0)));
        glm::vec3 perpB = glm::cross(worldAxis, perpA);
        
        // Note: warm starting for prismatic is more complex, simplified here
        // Would need to track impulses for each constrained DOF
    }

    void SliderJoint::Solve() {
        if (!Enabled) return;
        
        glm::vec3 worldAxis = m_BodyA->GetRotation() * m_Axis;
        
        // Constrain to move only along axis
        // We need to constrain 2 perpendicular directions and all 3 rotations
        
        glm::vec3 perpA = glm::normalize(glm::cross(worldAxis,
            (std::abs(glm::dot(worldAxis, glm::vec3(1, 0, 0))) < 0.9f) ?
            glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0)));
        glm::vec3 perpB = glm::cross(worldAxis, perpA);
        
        float invMassA = m_BodyA->GetInverseMass();
        float invMassB = m_BodyB->GetInverseMass();
        
        glm::mat3 invInertiaA = m_BodyA->GetInverseInertiaTensor();
        glm::mat3 invInertiaB = m_BodyB->GetInverseInertiaTensor();
        
        // Linear constraint perpendicular to axis
        glm::vec3 relativePos = m_BodyB->GetPosition() - m_BodyA->GetPosition();
        glm::vec3 relativeVel = m_BodyB->GetLinearVelocity() - m_BodyA->GetLinearVelocity();
        
        // Constrain perpA direction
        float errorA = glm::dot(relativePos, perpA);
        float velocityA = glm::dot(relativeVel, perpA);
        
        float effectiveMassA = invMassA + invMassB;
        if (effectiveMassA > 0.0f) {
            float biasA = 0.2f * errorA / m_DeltaTime;
            float lambdaA = -(velocityA + biasA) / effectiveMassA;
            
            m_BodyA->ApplyImpulse(-perpA * lambdaA);
            m_BodyB->ApplyImpulse(perpA * lambdaA);
        }
        
        // Constrain perpB direction
        float errorB = glm::dot(relativePos, perpB);
        float velocityB = glm::dot(relativeVel, perpB);
        
        float effectiveMassB = invMassA + invMassB;
        if (effectiveMassB > 0.0f) {
            float biasB = 0.2f * errorB / m_DeltaTime;
            float lambdaB = -(velocityB + biasB) / effectiveMassB;
            
            m_BodyA->ApplyImpulse(-perpB * lambdaB);
            m_BodyB->ApplyImpulse(perpB * lambdaB);
        }
        
        // Angular constraint - keep axes aligned
        glm::vec3 relativeAngular = m_BodyB->GetAngularVelocity() - m_BodyA->GetAngularVelocity();
        
        glm::mat3 angularK = invInertiaA + invInertiaB;
        float det = glm::determinant(angularK);
        
        if (std::abs(det) > 0.0001f) {
            glm::mat3 angularEffectiveMass = glm::inverse(angularK);
            glm::vec3 angularLambda = angularEffectiveMass * (-relativeAngular);
            
            m_BodyA->ApplyAngularImpulse(-angularLambda);
            m_BodyB->ApplyAngularImpulse(angularLambda);
        }
        
        // Linear limits
        if (UseLimits) {
            float position = glm::dot(relativePos, worldAxis);
            float velocity = glm::dot(relativeVel, worldAxis);
            
            float effectiveMass = invMassA + invMassB;
            
            // Lower limit
            if (position < LowerLimit && effectiveMass > 0.0f) {
                float bias = 0.2f * (position - LowerLimit) / m_DeltaTime;
                float lambda = -(velocity + bias) / effectiveMass;
                lambda = std::max(lambda, 0.0f);  // Only push, don't pull
                
                m_BodyA->ApplyImpulse(-worldAxis * lambda);
                m_BodyB->ApplyImpulse(worldAxis * lambda);
            }
            
            // Upper limit
            if (position > UpperLimit && effectiveMass > 0.0f) {
                float bias = 0.2f * (position - UpperLimit) / m_DeltaTime;
                float lambda = -(velocity + bias) / effectiveMass;
                lambda = std::min(lambda, 0.0f);  // Only push, don't pull
                
                m_BodyA->ApplyImpulse(-worldAxis * lambda);
                m_BodyB->ApplyImpulse(worldAxis * lambda);
            }
        }
        
        // Motor
        if (UseMotor) {
            float velocity = glm::dot(relativeVel, worldAxis);
            float motorError = MotorSpeed - velocity;
            
            float effectiveMass = invMassA + invMassB;
            if (effectiveMass > 0.0f) {
                float motorLambda = motorError / effectiveMass;
                motorLambda = glm::clamp(motorLambda, -MaxMotorForce * m_DeltaTime, MaxMotorForce * m_DeltaTime);
                
                m_BodyA->ApplyImpulse(-worldAxis * motorLambda);
                m_BodyB->ApplyImpulse(worldAxis * motorLambda);
            }
        }
    }

}}
