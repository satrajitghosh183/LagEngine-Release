#include "HingeJoint.hpp"
#include <sstream>
#include <cmath>

namespace engine::physics {

HingeJoint::HingeJoint(RigidBody* bodyA, RigidBody* bodyB,
                       const glm::vec3& anchorA, const glm::vec3& anchorB,
                       const glm::vec3& axis)
    : Joint(bodyA, bodyB, anchorA, anchorB), localAxisA(glm::normalize(axis)) {
    
    // Calculate local axis for body B
    if (bodyB) {
        glm::quat invRotB = glm::inverse(bodyB->getOrientation());
        glm::quat rotA = bodyA ? bodyA->getOrientation() : glm::quat(1, 0, 0, 0);
        localAxisB = invRotB * rotA * localAxisA;
    } else {
        localAxisB = localAxisA;
    }
    
    // Store reference angle
    referenceAngle = getCurrentAngle();
    
    // Initialize constraint data
    for (int i = 0; i < 3; ++i) {
        linearConstraints[i] = {};
    }
    for (int i = 0; i < 2; ++i) {
        angularConstraints[i] = {};
    }
    limitConstraint = {};
    motorConstraint = {};
}

void HingeJoint::prepare(float dt) {
    if (!enabled || broken) return;
    
    updateAnchors();
    updateAxes();
    
    prepareLinearConstraints(dt);
    prepareAngularConstraints(dt);
    
    if (hasLimits) {
        prepareLimitConstraint(dt);
    }
    
    if (motorEnabled) {
        prepareMotorConstraint(dt);
    }
}

void HingeJoint::updateAxes() {
    // Transform axes to world space
    worldAxisA = bodyA ? bodyA->getOrientation() * localAxisA : localAxisA;
    worldAxisB = bodyB ? bodyB->getOrientation() * localAxisB : localAxisB;
    
    // Calculate perpendicular axes for angular constraints
    perpA1 = getPerpendicularVector(worldAxisA);
    perpA2 = glm::cross(worldAxisA, perpA1);
    
    perpB1 = getPerpendicularVector(worldAxisB);
    perpB2 = glm::cross(worldAxisB, perpB1);
}

void HingeJoint::prepareLinearConstraints(float dt) {
    // Position error
    glm::vec3 positionError = worldAnchorB - worldAnchorA;
    
    // Create three linear constraints (X, Y, Z)
    glm::vec3 axes[3] = {
        glm::vec3(1, 0, 0),
        glm::vec3(0, 1, 0),
        glm::vec3(0, 0, 1)
    };
    
    for (int i = 0; i < 3; ++i) {
        HingeConstraint& constraint = linearConstraints[i];
        
        // Jacobian
        constraint.jacobianLinearA = -axes[i];
        constraint.jacobianLinearB = axes[i];
        constraint.jacobianAngularA = -glm::cross(rA, axes[i]);
        constraint.jacobianAngularB = glm::cross(rB, axes[i]);
        
        // Effective mass
        constraint.effectiveMass = calculateEffectiveMass(
            constraint.jacobianLinearA, constraint.jacobianAngularA,
            constraint.jacobianLinearB, constraint.jacobianAngularB);
        
        // Bias for position correction
        float baumgarte = 0.2f;
        constraint.bias = (baumgarte / dt) * glm::dot(positionError, axes[i]);
        
        // No limits for linear constraints
        constraint.lowerLimit = -FLT_MAX;
        constraint.upperLimit = FLT_MAX;
    }
}

void HingeJoint::prepareAngularConstraints(float dt) {
    // Two angular constraints to prevent rotation around perpendicular axes
    glm::vec3 angularAxes[2] = { perpA1, perpA2 };
    
    for (int i = 0; i < 2; ++i) {
        HingeConstraint& constraint = angularConstraints[i];
        
        // Jacobian (pure angular)
        constraint.jacobianLinearA = glm::vec3(0);
        constraint.jacobianLinearB = glm::vec3(0);
        constraint.jacobianAngularA = -angularAxes[i];
        constraint.jacobianAngularB = angularAxes[i];
        
        // Effective mass
        constraint.effectiveMass = calculateEffectiveMass(
            constraint.jacobianLinearA, constraint.jacobianAngularA,
            constraint.jacobianLinearB, constraint.jacobianAngularB);
        
        // Angular error
        float angularError = glm::dot(worldAxisB - worldAxisA, angularAxes[i]);
        
        // Bias for angular correction
        float baumgarte = 0.2f;
        constraint.bias = (baumgarte / dt) * angularError;
        
        // No limits for angular constraints
        constraint.lowerLimit = -FLT_MAX;
        constraint.upperLimit = FLT_MAX;
    }
}

void HingeJoint::prepareLimitConstraint(float dt) {
    float currentAngle = getCurrentAngle();
    float angleError = 0.0f;
    
    if (currentAngle < lowerLimit) {
        angleError = currentAngle - lowerLimit;
        limitConstraint.lowerLimit = 0.0f;
        limitConstraint.upperLimit = FLT_MAX;
    } else if (currentAngle > upperLimit) {
        angleError = currentAngle - upperLimit;
        limitConstraint.lowerLimit = -FLT_MAX;
        limitConstraint.upperLimit = 0.0f;
    } else {
        // Within limits, no constraint needed
        limitConstraint.effectiveMass = 0.0f;
        return;
    }
    
    // Jacobian along hinge axis
    limitConstraint.jacobianLinearA = glm::vec3(0);
    limitConstraint.jacobianLinearB = glm::vec3(0);
    limitConstraint.jacobianAngularA = -worldAxisA;
    limitConstraint.jacobianAngularB = worldAxisA;
    
    // Effective mass
    limitConstraint.effectiveMass = calculateEffectiveMass(
        limitConstraint.jacobianLinearA, limitConstraint.jacobianAngularA,
        limitConstraint.jacobianLinearB, limitConstraint.jacobianAngularB);
    
    // Bias for angle correction
    float baumgarte = 0.2f;
    limitConstraint.bias = (baumgarte / dt) * angleError;
}

void HingeJoint::prepareMotorConstraint(float dt) {
    // Motor constraint along hinge axis
    motorConstraint.jacobianLinearA = glm::vec3(0);
    motorConstraint.jacobianLinearB = glm::vec3(0);
    motorConstraint.jacobianAngularA = -worldAxisA;
    motorConstraint.jacobianAngularB = worldAxisA;
    
    // Effective mass
    motorConstraint.effectiveMass = calculateEffectiveMass(
        motorConstraint.jacobianLinearA, motorConstraint.jacobianAngularA,
        motorConstraint.jacobianLinearB, motorConstraint.jacobianAngularB);
    
    // Bias for motor speed
    float currentAngularVel = getAngularVelocity();
    motorConstraint.bias = motorSpeed - currentAngularVel;
    
    // Motor torque limits
    float maxImpulse = maxMotorTorque * dt;
    motorConstraint.lowerLimit = -maxImpulse;
    motorConstraint.upperLimit = maxImpulse;
}

void HingeJoint::warmStart() {
    if (!enabled || broken) return;
    
    // Warm start linear constraints
    for (int i = 0; i < 3; ++i) {
        HingeConstraint& constraint = linearConstraints[i];
        applyImpulse(constraint.jacobianLinearA * constraint.accumulatedImpulse,
                    constraint.jacobianAngularA * constraint.accumulatedImpulse,
                    constraint.jacobianLinearB * constraint.accumulatedImpulse,
                    constraint.jacobianAngularB * constraint.accumulatedImpulse);
    }
    
    // Warm start angular constraints
    for (int i = 0; i < 2; ++i) {
        HingeConstraint& constraint = angularConstraints[i];
        applyImpulse(constraint.jacobianLinearA * constraint.accumulatedImpulse,
                    constraint.jacobianAngularA * constraint.accumulatedImpulse,
                    constraint.jacobianLinearB * constraint.accumulatedImpulse,
                    constraint.jacobianAngularB * constraint.accumulatedImpulse);
    }
    
    // Warm start limit constraint
    if (hasLimits && limitConstraint.effectiveMass > 0.0f) {
        applyImpulse(limitConstraint.jacobianLinearA * limitConstraint.accumulatedImpulse,
                    limitConstraint.jacobianAngularA * limitConstraint.accumulatedImpulse,
                    limitConstraint.jacobianLinearB * limitConstraint.accumulatedImpulse,
                    limitConstraint.jacobianAngularB * limitConstraint.accumulatedImpulse);
    }
    
    // Warm start motor constraint
    if (motorEnabled && motorConstraint.effectiveMass > 0.0f) {
        applyImpulse(motorConstraint.jacobianLinearA * motorConstraint.accumulatedImpulse,
                    motorConstraint.jacobianAngularA * motorConstraint.accumulatedImpulse,
                    motorConstraint.jacobianLinearB * motorConstraint.accumulatedImpulse,
                    motorConstraint.jacobianAngularB * motorConstraint.accumulatedImpulse);
    }
}

void HingeJoint::solve(float dt) {
    if (!enabled || broken) return;
    
    solveLinearConstraints();
    solveAngularConstraints();
    
    if (hasLimits) {
        solveLimitConstraint();
    }
    
    if (motorEnabled) {
        solveMotorConstraint();
    }
}

void HingeJoint::solveLinearConstraints() {
    for (int i = 0; i < 3; ++i) {
        HingeConstraint& constraint = linearConstraints[i];
        
        if (constraint.effectiveMass <= 0.0f) continue;
        
        // Calculate relative velocity
        glm::vec3 velA = bodyA ? bodyA->getLinearVelocity() + 
                               glm::cross(bodyA->getAngularVelocity(), rA) : glm::vec3(0);
        glm::vec3 velB = bodyB ? bodyB->getLinearVelocity() + 
                               glm::cross(bodyB->getAngularVelocity(), rB) : glm::vec3(0);
        
        float deltaVelocity = glm::dot(velB - velA, constraint.jacobianLinearA);
        
        // Solve constraint
        float lambda = solveConstraint(constraint, deltaVelocity);
        
        // Apply impulse
        applyImpulse(constraint.jacobianLinearA * lambda,
                    constraint.jacobianAngularA * lambda,
                    constraint.jacobianLinearB * lambda,
                    constraint.jacobianAngularB * lambda);
    }
}

void HingeJoint::solveAngularConstraints() {
    for (int i = 0; i < 2; ++i) {
        HingeConstraint& constraint = angularConstraints[i];
        
        if (constraint.effectiveMass <= 0.0f) continue;
        
        // Calculate relative angular velocity
        glm::vec3 angVelA = bodyA ? bodyA->getAngularVelocity() : glm::vec3(0);
        glm::vec3 angVelB = bodyB ? bodyB->getAngularVelocity() : glm::vec3(0);
        
        float deltaVelocity = glm::dot(angVelB - angVelA, constraint.jacobianAngularA);
        
        // Solve constraint
        float lambda = solveConstraint(constraint, deltaVelocity);
        
        // Apply impulse
        applyImpulse(constraint.jacobianLinearA * lambda,
                    constraint.jacobianAngularA * lambda,
                    constraint.jacobianLinearB * lambda,
                    constraint.jacobianAngularB * lambda);
    }
}

void HingeJoint::solveLimitConstraint() {
    if (limitConstraint.effectiveMass <= 0.0f) return;
    
    // Calculate relative angular velocity along hinge axis
    float angularVel = getAngularVelocity();
    
    // Solve constraint
    float lambda = solveConstraint(limitConstraint, angularVel);
    
    // Apply impulse
    applyImpulse(limitConstraint.jacobianLinearA * lambda,
                limitConstraint.jacobianAngularA * lambda,
                limitConstraint.jacobianLinearB * lambda,
                limitConstraint.jacobianAngularB * lambda);
}

void HingeJoint::solveMotorConstraint() {
    if (motorConstraint.effectiveMass <= 0.0f) return;
    
    // Calculate relative angular velocity along hinge axis
    float angularVel = getAngularVelocity();
    
    // Solve constraint
    float lambda = solveConstraint(motorConstraint, angularVel);
    
    // Apply impulse
    applyImpulse(motorConstraint.jacobianLinearA * lambda,
                motorConstraint.jacobianAngularA * lambda,
                motorConstraint.jacobianLinearB * lambda,
                motorConstraint.jacobianAngularB * lambda);
}

void HingeJoint::storeImpulses() {
    // Apply decay to accumulated impulses
    for (int i = 0; i < 3; ++i) {
        linearConstraints[i].accumulatedImpulse *= 0.95f;
    }
    for (int i = 0; i < 2; ++i) {
        angularConstraints[i].accumulatedImpulse *= 0.95f;
    }
    
    if (hasLimits) {
        limitConstraint.accumulatedImpulse *= 0.95f;
    }
    
    if (motorEnabled) {
        motorConstraint.accumulatedImpulse *= 0.95f;
    }
}

float HingeJoint::getCurrentAngle() const {
    if (!bodyA || !bodyB) return 0.0f;
    
    // Calculate angle between the two axes
    glm::vec3 axisA = bodyA->getOrientation() * localAxisA;
    glm::vec3 axisB = bodyB->getOrientation() * localAxisB;
    
    float dot = glm::clamp(glm::dot(axisA, axisB), -1.0f, 1.0f);
    return std::acos(dot) - referenceAngle;
}

float HingeJoint::getAngularVelocity() const {
    glm::vec3 angVelA = bodyA ? bodyA->getAngularVelocity() : glm::vec3(0);
    glm::vec3 angVelB = bodyB ? bodyB->getAngularVelocity() : glm::vec3(0);
    
    return glm::dot(angVelB - angVelA, worldAxisA);
}

void HingeJoint::setAngleLimits(float lower, float upper) {
    lowerLimit = lower;
    upperLimit = upper;
    hasLimits = true;
}

glm::vec3 HingeJoint::getReactionForce() const {
    glm::vec3 force(0);
    
    for (int i = 0; i < 3; ++i) {
        force += linearConstraints[i].accumulatedImpulse * linearConstraints[i].jacobianLinearA;
    }
    
    return -force;
}

glm::vec3 HingeJoint::getReactionTorque() const {
    glm::vec3 torque(0);
    
    for (int i = 0; i < 2; ++i) {
        torque += angularConstraints[i].accumulatedImpulse * angularConstraints[i].jacobianAngularA;
    }
    
    if (hasLimits) {
        torque += limitConstraint.accumulatedImpulse * limitConstraint.jacobianAngularA;
    }
    
    if (motorEnabled) {
        torque += motorConstraint.accumulatedImpulse * motorConstraint.jacobianAngularA;
    }
    
    return -torque;
}

float HingeJoint::getAppliedImpulse() const {
    float totalImpulse = 0.0f;
    
    for (int i = 0; i < 3; ++i) {
        totalImpulse += std::abs(linearConstraints[i].accumulatedImpulse);
    }
    for (int i = 0; i < 2; ++i) {
        totalImpulse += std::abs(angularConstraints[i].accumulatedImpulse);
    }
    
    return totalImpulse;
}

std::string HingeJoint::getDebugInfo() const {
    std::ostringstream oss;
    oss << "HingeJoint:\n";
    oss << "  Enabled: " << (enabled ? "true" : "false") << "\n";
    oss << "  Broken: " << (broken ? "true" : "false") << "\n";
    oss << "  Current Angle: " << getCurrentAngle() << " rad\n";
    oss << "  Angular Velocity: " << getAngularVelocity() << " rad/s\n";
    
    if (hasLimits) {
        oss << "  Angle Limits: [" << lowerLimit << ", " << upperLimit << "]\n";
    }
    
    if (motorEnabled) {
        oss << "  Motor Enabled: true\n";
        oss << "  Motor Speed: " << motorSpeed << " rad/s\n";
        oss << "  Max Motor Torque: " << maxMotorTorque << "\n";
    }
    
    return oss.str();
}

glm::vec3 HingeJoint::getPerpendicularVector(const glm::vec3& v) const {
    if (std::abs(v.x) >= 0.57735f) {
        return glm::normalize(glm::vec3(v.y, -v.x, 0.0f));
    } else {
        return glm::normalize(glm::vec3(0.0f, v.z, -v.y));
    }
}

} // namespace engine::physics