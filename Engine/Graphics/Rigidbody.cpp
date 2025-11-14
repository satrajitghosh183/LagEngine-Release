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
        , m_Motion(0.0f) {
        
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
        if (!m_IsAwake || m_BodyType == BodyType::Static) return;
        
        if (m_BodyType == BodyType::Dynamic) {
            // Linear motion
            glm::vec3 acceleration = m_ForceAccumulator * m_InverseMass;
            m_LinearVelocity += acceleration * deltaTime;
            m_LinearVelocity *= std::pow(m_LinearDamping, deltaTime);
            
            // Angular motion
            glm::vec3 angularAcceleration = m_InverseInertiaTensor * m_TorqueAccumulator;
            m_AngularVelocity += angularAcceleration * deltaTime;
            m_AngularVelocity *= std::pow(m_AngularDamping, deltaTime);
        }
        
        // Update position
        m_Position += m_LinearVelocity * deltaTime;
        
        // Update rotation
        glm::quat angularQuat(0.0f, m_AngularVelocity.x, m_AngularVelocity.y, m_AngularVelocity.z);
        m_Rotation += 0.5f * angularQuat * m_Rotation * deltaTime;
        m_Rotation = glm::normalize(m_Rotation);
        
        // Clear forces
        ClearForces();
        
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

}}