#pragma once

#include "RigidBody2D.hpp"
#include <glm/glm.hpp>
#include <cmath>

namespace GameEngine {
namespace Physics2D {

    enum class JointType2D {
        Distance,
        Revolute,
        Prismatic,
        Weld,
        Mouse
    };

    class Joint2D {
    public:
        virtual ~Joint2D() = default;
        virtual JointType2D getType() const = 0;
        virtual void solve(float dt) = 0;

        RigidBody2D* BodyA = nullptr;
        RigidBody2D* BodyB = nullptr;
        float BreakForce = 0.0f; // 0 = unbreakable
        bool IsBroken = false;
    };

    // =====================================================================
    // Distance Joint — maintains distance between two anchor points
    // =====================================================================

    class DistanceJoint2D : public Joint2D {
    public:
        glm::vec2 LocalAnchorA{0.0f};
        glm::vec2 LocalAnchorB{0.0f};
        float RestLength = 1.0f;
        float Stiffness = 100.0f;  // spring constant
        float Damping = 5.0f;

        JointType2D getType() const override { return JointType2D::Distance; }

        void solve(float dt) override {
            if (IsBroken || !BodyA || !BodyB) return;

            glm::vec2 worldA = BodyA->getPosition() + Collider2D::rotatePoint(LocalAnchorA, BodyA->getRotation());
            glm::vec2 worldB = BodyB->getPosition() + Collider2D::rotatePoint(LocalAnchorB, BodyB->getRotation());

            glm::vec2 diff = worldB - worldA;
            float dist = glm::length(diff);
            if (dist < 1e-6f) return;

            glm::vec2 dir = diff / dist;
            float displacement = dist - RestLength;

            // Spring-damper force
            glm::vec2 relVel = BodyB->getLinearVelocity() - BodyA->getLinearVelocity();
            float dampingForce = glm::dot(relVel, dir) * Damping;

            float force = Stiffness * displacement + dampingForce;

            // Check break force
            if (BreakForce > 0.0f && std::abs(force) > BreakForce) {
                IsBroken = true;
                return;
            }

            glm::vec2 impulse = dir * force * dt;

            BodyA->applyLinearImpulse(impulse);
            BodyB->applyLinearImpulse(-impulse);
        }
    };

    // =====================================================================
    // Revolute Joint — pin/hinge joint with optional motor and angle limits
    // =====================================================================

    class RevoluteJoint2D : public Joint2D {
    public:
        glm::vec2 LocalAnchorA{0.0f};
        glm::vec2 LocalAnchorB{0.0f};
        bool EnableLimits = false;
        float LowerAngle = 0.0f;
        float UpperAngle = 0.0f;
        bool EnableMotor = false;
        float MotorSpeed = 0.0f;
        float MaxMotorTorque = 10.0f;

        JointType2D getType() const override { return JointType2D::Revolute; }

        void solve(float dt) override {
            if (IsBroken || !BodyA || !BodyB) return;

            // Positional constraint: anchor points must coincide
            glm::vec2 worldA = BodyA->getPosition() + Collider2D::rotatePoint(LocalAnchorA, BodyA->getRotation());
            glm::vec2 worldB = BodyB->getPosition() + Collider2D::rotatePoint(LocalAnchorB, BodyB->getRotation());

            glm::vec2 diff = worldB - worldA;
            float invMassSum = BodyA->getInverseMass() + BodyB->getInverseMass();
            if (invMassSum == 0.0f) return;

            // Position correction
            glm::vec2 correction = diff * 0.5f;
            if (BodyA->getBodyType() == RigidBody2D::BodyType::Dynamic) {
                BodyA->setPosition(BodyA->getPosition() + correction * (BodyA->getInverseMass() / invMassSum));
            }
            if (BodyB->getBodyType() == RigidBody2D::BodyType::Dynamic) {
                BodyB->setPosition(BodyB->getPosition() - correction * (BodyB->getInverseMass() / invMassSum));
            }

            // Velocity constraint
            glm::vec2 rA = worldA - BodyA->getPosition();
            glm::vec2 rB = worldB - BodyB->getPosition();
            glm::vec2 relVel = (BodyB->getLinearVelocity() + glm::vec2(-BodyB->getAngularVelocity() * rB.y,
                                                                         BodyB->getAngularVelocity() * rB.x))
                             - (BodyA->getLinearVelocity() + glm::vec2(-BodyA->getAngularVelocity() * rA.y,
                                                                         BodyA->getAngularVelocity() * rA.x));

            glm::vec2 impulse = -relVel / invMassSum * dt * 10.0f;

            BodyA->applyLinearImpulse(-impulse);
            BodyB->applyLinearImpulse(impulse);

            // Angular motor
            if (EnableMotor) {
                float currentAngVel = BodyB->getAngularVelocity() - BodyA->getAngularVelocity();
                float angError = MotorSpeed - currentAngVel;
                float motorImpulse = std::clamp(angError * dt, -MaxMotorTorque * dt, MaxMotorTorque * dt);
                BodyA->applyAngularImpulse(-motorImpulse);
                BodyB->applyAngularImpulse(motorImpulse);
            }

            // Angle limits
            if (EnableLimits) {
                float relAngle = BodyB->getRotation() - BodyA->getRotation();
                if (relAngle < LowerAngle) {
                    float correction = (LowerAngle - relAngle) * 0.3f;
                    BodyB->setRotation(BodyB->getRotation() + correction);
                } else if (relAngle > UpperAngle) {
                    float correction = (UpperAngle - relAngle) * 0.3f;
                    BodyB->setRotation(BodyB->getRotation() + correction);
                }
            }
        }
    };

    // =====================================================================
    // Prismatic Joint — slider along an axis with optional limits
    // =====================================================================

    class PrismaticJoint2D : public Joint2D {
    public:
        glm::vec2 LocalAnchorA{0.0f};
        glm::vec2 LocalAnchorB{0.0f};
        glm::vec2 Axis{1.0f, 0.0f}; // Local axis on BodyA
        bool EnableLimits = false;
        float LowerTranslation = 0.0f;
        float UpperTranslation = 1.0f;

        JointType2D getType() const override { return JointType2D::Prismatic; }

        void solve(float dt) override {
            if (IsBroken || !BodyA || !BodyB) return;

            glm::vec2 worldAxis = Collider2D::rotatePoint(Axis, BodyA->getRotation());
            glm::vec2 perpAxis(-worldAxis.y, worldAxis.x);

            glm::vec2 worldA = BodyA->getPosition() + Collider2D::rotatePoint(LocalAnchorA, BodyA->getRotation());
            glm::vec2 worldB = BodyB->getPosition() + Collider2D::rotatePoint(LocalAnchorB, BodyB->getRotation());
            glm::vec2 diff = worldB - worldA;

            // Constrain perpendicular movement
            float perpError = glm::dot(diff, perpAxis);
            float invMassSum = BodyA->getInverseMass() + BodyB->getInverseMass();
            if (invMassSum == 0.0f) return;

            float correction = -perpError * 0.3f;
            if (BodyA->getBodyType() == RigidBody2D::BodyType::Dynamic) {
                BodyA->setPosition(BodyA->getPosition() - perpAxis * correction * (BodyA->getInverseMass() / invMassSum));
            }
            if (BodyB->getBodyType() == RigidBody2D::BodyType::Dynamic) {
                BodyB->setPosition(BodyB->getPosition() + perpAxis * correction * (BodyB->getInverseMass() / invMassSum));
            }

            // Translation limits
            if (EnableLimits) {
                float translation = glm::dot(diff, worldAxis);
                if (translation < LowerTranslation) {
                    float c = (LowerTranslation - translation) * 0.3f;
                    BodyB->setPosition(BodyB->getPosition() + worldAxis * c);
                } else if (translation > UpperTranslation) {
                    float c = (UpperTranslation - translation) * 0.3f;
                    BodyB->setPosition(BodyB->getPosition() + worldAxis * c);
                }
            }

            // Lock rotation
            float relAngVel = BodyB->getAngularVelocity() - BodyA->getAngularVelocity();
            BodyA->applyAngularImpulse(relAngVel * dt * 0.5f);
            BodyB->applyAngularImpulse(-relAngVel * dt * 0.5f);
        }
    };

    // =====================================================================
    // Weld Joint — rigid connection between two bodies
    // =====================================================================

    class WeldJoint2D : public Joint2D {
    public:
        glm::vec2 LocalAnchorA{0.0f};
        glm::vec2 LocalAnchorB{0.0f};
        float ReferenceAngle = 0.0f;

        JointType2D getType() const override { return JointType2D::Weld; }

        void solve(float dt) override {
            if (IsBroken || !BodyA || !BodyB) return;

            // Position constraint
            glm::vec2 worldA = BodyA->getPosition() + Collider2D::rotatePoint(LocalAnchorA, BodyA->getRotation());
            glm::vec2 worldB = BodyB->getPosition() + Collider2D::rotatePoint(LocalAnchorB, BodyB->getRotation());

            glm::vec2 diff = worldB - worldA;
            float invMassSum = BodyA->getInverseMass() + BodyB->getInverseMass();
            if (invMassSum > 0.0f) {
                glm::vec2 correction = diff * 0.5f;
                if (BodyA->getBodyType() == RigidBody2D::BodyType::Dynamic) {
                    BodyA->setPosition(BodyA->getPosition() + correction * (BodyA->getInverseMass() / invMassSum));
                }
                if (BodyB->getBodyType() == RigidBody2D::BodyType::Dynamic) {
                    BodyB->setPosition(BodyB->getPosition() - correction * (BodyB->getInverseMass() / invMassSum));
                }
            }

            // Angle constraint
            float angleDiff = BodyB->getRotation() - BodyA->getRotation() - ReferenceAngle;
            float angleCorrection = angleDiff * 0.3f;
            float invInertiaSum = BodyA->getInverseInertia() + BodyB->getInverseInertia();
            if (invInertiaSum > 0.0f) {
                if (BodyA->getBodyType() == RigidBody2D::BodyType::Dynamic) {
                    BodyA->setRotation(BodyA->getRotation() + angleCorrection * (BodyA->getInverseInertia() / invInertiaSum));
                }
                if (BodyB->getBodyType() == RigidBody2D::BodyType::Dynamic) {
                    BodyB->setRotation(BodyB->getRotation() - angleCorrection * (BodyB->getInverseInertia() / invInertiaSum));
                }
            }
        }
    };

    // =====================================================================
    // Mouse Joint — drag a body to a target point (useful for interaction)
    // =====================================================================

    class MouseJoint2D : public Joint2D {
    public:
        glm::vec2 Target{0.0f};
        glm::vec2 LocalAnchor{0.0f};
        float MaxForce = 100.0f;
        float Stiffness = 50.0f;
        float Damping = 5.0f;

        JointType2D getType() const override { return JointType2D::Mouse; }

        void solve(float dt) override {
            if (IsBroken || !BodyB) return; // BodyA is the "world" anchor

            glm::vec2 worldAnchor = BodyB->getPosition() + Collider2D::rotatePoint(LocalAnchor, BodyB->getRotation());
            glm::vec2 diff = Target - worldAnchor;

            glm::vec2 force = diff * Stiffness - BodyB->getLinearVelocity() * Damping;

            float forceMag = glm::length(force);
            if (forceMag > MaxForce) {
                force = force / forceMag * MaxForce;
            }

            BodyB->applyForce(force);
        }
    };

}}
