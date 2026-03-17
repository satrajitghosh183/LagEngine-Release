#include "RigidBody.hpp"
#include <glm/gtx/quaternion.hpp>
#include <algorithm>

namespace GameEngine {
namespace Physics {

    RigidBody::RigidBody(BodyType type, float mass)
        : m_BodyType(type)
        , m_Position(0.0f)
        , m_Rotation(1.0f, 0.0f, 0.0f, 0.0f)
        , m_LinearVelocity(0.0f)
        , m_ForceAccumulator(0.0f)
        , m_AngularVelocity(0.0f)
        , m_TorqueAccumulator(0.0f)
        , m_LinearDamping(0.95f)
        , m_AngularDamping(0.95f)
        , m_UseGravity(true)
        , m_IsAwake(true)
        , m_Motion(1.0f) {
        
        SetMass(mass);
        
        // Default inertia tensor (sphere)
        float inertia = (2.0f / 5.0f) * mass * 1.0f * 1.0f;
        SetInertiaTensor(glm::mat3(
            inertia, 0, 0,
            0, inertia, 0,
            0, 0, inertia
        ));
    }

    void RigidBody::SetMass(float mass) {
        if (m_BodyType == BodyType::Static) {
            m_Mass = 0.0f;
            m_InverseMass = 0.0f;
        } else if (m_BodyType == BodyType::Dynamic) {
            m_Mass = mass;
            m_InverseMass = (mass > 0.0f) ? 1.0f / mass : 0.0f;
        } else {
            m_Mass = 0.0f;
            m_InverseMass = 0.0f;
        }
    }

    void RigidBody::SetInertiaTensor(const glm::mat3& inertia) {
        m_InertiaTensor = inertia;
        
        // Calculate inverse
        float det = glm::determinant(inertia);
        if (det > 0.0001f) {
            m_InverseInertiaTensor = glm::inverse(inertia);
        } else {
            m_InverseInertiaTensor = glm::mat3(0.0f);
        }
    }

    void RigidBody::SetBodyType(BodyType type) {
        m_BodyType = type;
        
        if (type == BodyType::Static) {
            m_Mass = 0.0f;
            m_InverseMass = 0.0f;
            m_LinearVelocity = glm::vec3(0.0f);
            m_AngularVelocity = glm::vec3(0.0f);
        } else if (type == BodyType::Kinematic) {
            m_InverseMass = 0.0f;
        }
    }

    void RigidBody::ApplyForce(const glm::vec3& force) {
        if (m_BodyType != BodyType::Dynamic) return;
        
        m_ForceAccumulator += force;
        m_IsAwake = true;
    }

    void RigidBody::ApplyForceAtPoint(const glm::vec3& force, const glm::vec3& point) {
        if (m_BodyType != BodyType::Dynamic) return;
        
        m_ForceAccumulator += force;
        
        // Calculate torque
        glm::vec3 r = point - m_Position;
        m_TorqueAccumulator += glm::cross(r, force);
        
        m_IsAwake = true;
    }

    void RigidBody::ApplyTorque(const glm::vec3& torque) {
        if (m_BodyType != BodyType::Dynamic) return;
        
        m_TorqueAccumulator += torque;
        m_IsAwake = true;
    }

    void RigidBody::ApplyImpulse(const glm::vec3& impulse) {
        if (m_BodyType != BodyType::Dynamic) return;
        
        m_LinearVelocity += impulse * m_InverseMass;
        m_IsAwake = true;
    }

    void RigidBody::ApplyAngularImpulse(const glm::vec3& impulse) {
        if (m_BodyType != BodyType::Dynamic) return;
        
        m_AngularVelocity += m_InverseInertiaTensor * impulse;
        m_IsAwake = true;
    }

    void RigidBody::ClearForces() {
        m_ForceAccumulator = glm::vec3(0.0f);
        m_TorqueAccumulator = glm::vec3(0.0f);
    }

    void RigidBody::Integrate(float deltaTime) {
        // Full integration (for backward compatibility)
        IntegrateForces(deltaTime);
        IntegrateVelocities(deltaTime);
    }
    
    void RigidBody::IntegrateForces(float deltaTime) {
        if (!m_IsAwake || m_BodyType == BodyType::Static) return;
        
        if (m_BodyType == BodyType::Dynamic) {
            // Linear motion: accumulate forces -> velocity
            glm::vec3 acceleration = m_ForceAccumulator * m_InverseMass;
            m_LinearVelocity += acceleration * deltaTime;
            
            // Angular motion: accumulate torques -> angular velocity
            glm::vec3 angularAcceleration = m_InverseInertiaTensor * m_TorqueAccumulator;
            m_AngularVelocity += angularAcceleration * deltaTime;
        }
        
        // Clear forces after integration
        ClearForces();
    }
    
    void RigidBody::IntegrateVelocities(float deltaTime) {
        if (!m_IsAwake || m_BodyType == BodyType::Static) return;

        // Update position from velocity (before damping, so full velocity is used)
        m_Position += m_LinearVelocity * deltaTime;

        // Update rotation from angular velocity (exact formula using angleAxis)
        float angularSpeed = glm::length(m_AngularVelocity);
        if (angularSpeed > 0.0001f) {
            glm::vec3 axis = m_AngularVelocity / angularSpeed;
            float angle = angularSpeed * deltaTime;
            m_Rotation = glm::normalize(glm::angleAxis(angle, axis) * m_Rotation);
        }

        if (m_BodyType == BodyType::Dynamic || m_BodyType == BodyType::Kinematic) {
            // Apply damping after position update
            m_LinearVelocity *= std::pow(m_LinearDamping, deltaTime);
            m_AngularVelocity *= std::pow(m_AngularDamping, deltaTime);
        }
        
        // Update motion for sleep calculation
        float currentMotion = glm::dot(m_LinearVelocity, m_LinearVelocity) + 
                             glm::dot(m_AngularVelocity, m_AngularVelocity);
        
        float bias = 0.5f;
        m_Motion = bias * m_Motion + (1.0f - bias) * currentMotion;
        
        // Check if can sleep
        if (CanSleep()) {
            m_IsAwake = false;
            m_LinearVelocity = glm::vec3(0.0f);
            m_AngularVelocity = glm::vec3(0.0f);
        }
    }

    bool RigidBody::CanSleep() const {
        if (m_BodyType != BodyType::Dynamic) return false;

        const float sleepEpsilon = 0.01f;
        return m_Motion < sleepEpsilon;
    }

    void RigidBody::UpdateInertiaTensor() {
        if (m_CollisionShape) {
            SetInertiaTensor(m_CollisionShape->CalculateInertiaTensor(m_Mass));
        }
    }

}}
