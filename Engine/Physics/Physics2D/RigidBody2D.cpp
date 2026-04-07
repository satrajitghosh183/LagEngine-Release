#include "RigidBody2D.hpp"
#include <cmath>
#include <algorithm>

namespace GameEngine {
namespace Physics2D {

    RigidBody2D::RigidBody2D(BodyType type, float mass)
        : m_BodyType(type)
        , m_Position(0.0f, 0.0f)
        , m_Rotation(0.0f)
        , m_LinearVelocity(0.0f, 0.0f)
        , m_ForceAccumulator(0.0f, 0.0f)
        , m_AngularVelocity(0.0f)
        , m_TorqueAccumulator(0.0f)
        , m_LinearDamping(0.1f)
        , m_AngularDamping(0.1f)
        , m_GravityScale(1.0f)
        , m_FixedRotation(false)
        , m_UseCCD(false)
        , m_IsAwake(true)
        , m_SleepTimer(0.0f)
        , m_UserData(nullptr) {

        setMass(mass);
        // Default inertia for a unit disk: I = 0.5 * m * r^2
        setInertia(0.5f * mass * 1.0f);
    }

    // ----- Integration -----

    void RigidBody2D::integrateForces(float deltaTime, const glm::vec2& gravity) {
        if (!m_IsAwake || m_BodyType == BodyType::Static) return;

        if (m_BodyType == BodyType::Dynamic) {
            // Apply gravity
            glm::vec2 gravityForce = gravity * m_Mass * m_GravityScale;

            // Linear: a = F/m -> v += a * dt
            glm::vec2 acceleration = (m_ForceAccumulator + gravityForce) * m_InverseMass;
            m_LinearVelocity += acceleration * deltaTime;

            // Angular: alpha = tau / I -> omega += alpha * dt
            if (!m_FixedRotation) {
                float angularAcceleration = m_TorqueAccumulator * m_InverseInertia;
                m_AngularVelocity += angularAcceleration * deltaTime;
            }
        }

        clearForces();
    }

    void RigidBody2D::integrateVelocities(float deltaTime) {
        if (!m_IsAwake || m_BodyType == BodyType::Static) return;

        // Update position
        m_Position += m_LinearVelocity * deltaTime;

        // Update rotation
        if (!m_FixedRotation) {
            m_Rotation += m_AngularVelocity * deltaTime;
        }

        // Apply damping
        if (m_BodyType == BodyType::Dynamic || m_BodyType == BodyType::Kinematic) {
            m_LinearVelocity *= std::pow(1.0f - m_LinearDamping, deltaTime);
            m_AngularVelocity *= std::pow(1.0f - m_AngularDamping, deltaTime);
        }

        // Sleep logic
        float motion = glm::dot(m_LinearVelocity, m_LinearVelocity) +
                        m_AngularVelocity * m_AngularVelocity;

        const float sleepThreshold = 0.001f;
        const float sleepDelay = 0.5f; // seconds of low motion before sleeping

        if (motion < sleepThreshold) {
            m_SleepTimer += deltaTime;
            if (m_SleepTimer >= sleepDelay && canSleep()) {
                setAwake(false);
            }
        } else {
            m_SleepTimer = 0.0f;
        }
    }

    // ----- Force / impulse application -----

    void RigidBody2D::applyForce(const glm::vec2& force) {
        if (m_BodyType != BodyType::Dynamic) return;
        m_ForceAccumulator += force;
        setAwake(true);
    }

    void RigidBody2D::applyForceAtPoint(const glm::vec2& force, const glm::vec2& worldPoint) {
        if (m_BodyType != BodyType::Dynamic) return;
        m_ForceAccumulator += force;
        glm::vec2 r = worldPoint - m_Position;
        // 2D cross product: r x F = r.x * F.y - r.y * F.x
        m_TorqueAccumulator += r.x * force.y - r.y * force.x;
        setAwake(true);
    }

    void RigidBody2D::applyTorque(float torque) {
        if (m_BodyType != BodyType::Dynamic) return;
        m_TorqueAccumulator += torque;
        setAwake(true);
    }

    void RigidBody2D::applyLinearImpulse(const glm::vec2& impulse) {
        if (m_BodyType != BodyType::Dynamic) return;
        m_LinearVelocity += impulse * m_InverseMass;
        setAwake(true);
    }

    void RigidBody2D::applyLinearImpulseAtPoint(const glm::vec2& impulse, const glm::vec2& worldPoint) {
        if (m_BodyType != BodyType::Dynamic) return;
        m_LinearVelocity += impulse * m_InverseMass;
        glm::vec2 r = worldPoint - m_Position;
        if (!m_FixedRotation) {
            m_AngularVelocity += (r.x * impulse.y - r.y * impulse.x) * m_InverseInertia;
        }
        setAwake(true);
    }

    void RigidBody2D::applyAngularImpulse(float impulse) {
        if (m_BodyType != BodyType::Dynamic) return;
        if (!m_FixedRotation) {
            m_AngularVelocity += impulse * m_InverseInertia;
        }
        setAwake(true);
    }

    void RigidBody2D::clearForces() {
        m_ForceAccumulator = glm::vec2(0.0f, 0.0f);
        m_TorqueAccumulator = 0.0f;
    }

    // ----- Mass properties -----

    void RigidBody2D::setMass(float mass) {
        if (m_BodyType == BodyType::Static || m_BodyType == BodyType::Kinematic) {
            m_Mass = 0.0f;
            m_InverseMass = 0.0f;
        } else {
            m_Mass = mass;
            m_InverseMass = (mass > 0.0f) ? 1.0f / mass : 0.0f;
        }
    }

    void RigidBody2D::setInertia(float inertia) {
        if (m_BodyType == BodyType::Static || m_BodyType == BodyType::Kinematic || m_FixedRotation) {
            m_Inertia = 0.0f;
            m_InverseInertia = 0.0f;
        } else {
            m_Inertia = inertia;
            m_InverseInertia = (inertia > 0.0f) ? 1.0f / inertia : 0.0f;
        }
    }

    // ----- Body type -----

    void RigidBody2D::setBodyType(BodyType type) {
        if (m_BodyType == type) return;

        m_BodyType = type;

        if (type == BodyType::Static) {
            m_Mass = 0.0f;
            m_InverseMass = 0.0f;
            m_Inertia = 0.0f;
            m_InverseInertia = 0.0f;
            m_LinearVelocity = glm::vec2(0.0f);
            m_AngularVelocity = 0.0f;
        } else if (type == BodyType::Kinematic) {
            m_Mass = 0.0f;
            m_InverseMass = 0.0f;
            m_Inertia = 0.0f;
            m_InverseInertia = 0.0f;
        }
    }

    // ----- Sleep state -----

    void RigidBody2D::setAwake(bool awake) {
        m_IsAwake = awake;
        if (awake) {
            m_SleepTimer = 0.0f;
        } else {
            m_LinearVelocity = glm::vec2(0.0f);
            m_AngularVelocity = 0.0f;
        }
    }

    bool RigidBody2D::canSleep() const {
        if (m_BodyType != BodyType::Dynamic) return false;
        return true;
    }

}}
