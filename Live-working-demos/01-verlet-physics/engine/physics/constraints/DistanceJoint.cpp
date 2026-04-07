#include "DistanceJoint.hpp"
#include <sstream>

namespace engine::physics {

DistanceJoint::DistanceJoint(RigidBody* bodyA, RigidBody* bodyB,
                            const glm::vec3& anchorA, const glm::vec3& anchorB,
                            float restLen)
    : Joint(bodyA, bodyB, anchorA, anchorB) {
    
    if (restLen <= 0.0f) {
        // Calculate rest length from current distance
        restLength = getCurrentLength();
    } else {
        restLength = restLen;
    }
}

void DistanceJoint::prepare(float dt) {
    if (!enabled || broken) return;
    
    updateAnchors();
    
    // Calculate direction and distance
    glm::vec3 delta = worldAnchorB - worldAnchorA;
    float currentLength = glm::length(delta);
    
    if (currentLength < 0.0001f) {
        // Avoid division by zero
        normal = glm::vec3(1, 0, 0);
        effectiveMass = 0.0f;
        bias = 0.0f;
        return;
    }
    
    normal = delta / currentLength;
    
    // Handle length constraints
    float error = 0.0f;
    if (hasMinLength && currentLength < minLength) {
        error = currentLength - minLength;
    } else if (hasMaxLength && currentLength > maxLength) {
        error = currentLength - maxLength;
    } else if (!hasMinLength && !hasMaxLength) {
        error = currentLength - restLength;
    }
    
    // Calculate Jacobian (direction constraint)
    jacobian = normal;
    
    // Calculate effective mass
    float invMassA = bodyA ? bodyA->getInverseMass() : 0.0f;
    float invMassB = bodyB ? bodyB->getInverseMass() : 0.0f;
    
    effectiveMass = invMassA + invMassB;
    
    if (bodyA) {
        glm::vec3 rAcrossN = glm::cross(rA, normal);
        effectiveMass += glm::dot(rAcrossN, bodyA->getInverseInertiaTensor() * rAcrossN);
    }
    if (bodyB) {
        glm::vec3 rBcrossN = glm::cross(rB, normal);
        effectiveMass += glm::dot(rBcrossN, bodyB->getInverseInertiaTensor() * rBcrossN);
    }
    
    if (effectiveMass > 0.0f) {
        effectiveMass = 1.0f / effectiveMass;
    }
    
    // Calculate bias for position correction
    const float baumgarte = 0.2f;
    bias = (baumgarte / dt) * error;
    
    // Apply damping
    if (damping > 0.0f) {
        float relativeVelocity = glm::dot(getRelativeVelocity(), normal);
        bias += damping * relativeVelocity;
    }
}

void DistanceJoint::warmStart() {
    if (!enabled || broken || effectiveMass <= 0.0f) return;
    
    // Apply cached impulse
    glm::vec3 impulseVector = impulse * normal;
    
    if (bodyA) {
        bodyA->setLinearVelocity(bodyA->getLinearVelocity() - impulseVector * bodyA->getInverseMass());
        bodyA->setAngularVelocity(bodyA->getAngularVelocity() - 
                                 bodyA->getInverseInertiaTensor() * glm::cross(rA, impulseVector));
    }
    if (bodyB) {
        bodyB->setLinearVelocity(bodyB->getLinearVelocity() + impulseVector * bodyB->getInverseMass());
        bodyB->setAngularVelocity(bodyB->getAngularVelocity() + 
                                 bodyB->getInverseInertiaTensor() * glm::cross(rB, impulseVector));
    }
}

void DistanceJoint::solve(float dt) {
    if (!enabled || broken || effectiveMass <= 0.0f) return;
    
    if (stiffness < 1.0f && useSpringDamper) {
        solveSpringDamper(dt);
    } else {
        solveRigidConstraint(dt);
    }
}

void DistanceJoint::solveRigidConstraint(float dt) {
    // Calculate relative velocity along constraint
    glm::vec3 relVel = getRelativeVelocity();
    float relativeVelocity = glm::dot(relVel, normal);
    
    // Calculate impulse
    float lambda = -(relativeVelocity + bias) * effectiveMass;
    
    // Handle inequality constraints (min/max length)
    if (hasMinLength || hasMaxLength) {
        float oldImpulse = impulse;
        
        if (hasMinLength && !hasMaxLength) {
            // One-sided constraint (minimum length)
            impulse = glm::max(0.0f, impulse + lambda);
        } else if (hasMaxLength && !hasMinLength) {
            // One-sided constraint (maximum length)
            impulse = glm::min(0.0f, impulse + lambda);
        } else {
            // No constraints, or both min and max
            impulse += lambda;
        }
        
        lambda = impulse - oldImpulse;
    } else {
        impulse += lambda;
    }
    
    // Apply impulse
    glm::vec3 impulseVector = lambda * normal;
    
    if (bodyA) {
        bodyA->setLinearVelocity(bodyA->getLinearVelocity() - impulseVector * bodyA->getInverseMass());
        bodyA->setAngularVelocity(bodyA->getAngularVelocity() - 
                                 bodyA->getInverseInertiaTensor() * glm::cross(rA, impulseVector));
    }
    if (bodyB) {
        bodyB->setLinearVelocity(bodyB->getLinearVelocity() + impulseVector * bodyB->getInverseMass());
        bodyB->setAngularVelocity(bodyB->getAngularVelocity() + 
                                 bodyB->getInverseInertiaTensor() * glm::cross(rB, impulseVector));
    }
    
    // Check for breakage
    checkBreakage(std::abs(lambda));
}

void DistanceJoint::solveSpringDamper(float dt) {
    float currentLength = glm::length(worldAnchorB - worldAnchorA);
    float error = currentLength - restLength;
    
    // Spring force: F = -k * x
    float springForce = -stiffness * error;
    
    // Damping force: F = -c * v
    glm::vec3 relVel = getRelativeVelocity();
    float dampingForce = -damping * glm::dot(relVel, normal);
    
    float totalForce = springForce + dampingForce;
    
    // Convert force to impulse
    float lambda = totalForce * dt * effectiveMass;
    impulse += lambda;
    
    // Apply impulse
    glm::vec3 impulseVector = lambda * normal;
    
    if (bodyA) {
        bodyA->setLinearVelocity(bodyA->getLinearVelocity() - impulseVector * bodyA->getInverseMass());
        bodyA->setAngularVelocity(bodyA->getAngularVelocity() - 
                                 bodyA->getInverseInertiaTensor() * glm::cross(rA, impulseVector));
    }
    if (bodyB) {
        bodyB->setLinearVelocity(bodyB->getLinearVelocity() + impulseVector * bodyB->getInverseMass());
        bodyB->setAngularVelocity(bodyB->getAngularVelocity() + 
                                 bodyB->getInverseInertiaTensor() * glm::cross(rB, impulseVector));
    }
}

void DistanceJoint::storeImpulses() {
    // Impulse is already stored in member variable
    // Apply decay for next frame
    impulse *= 0.95f;  // Small decay to prevent accumulation
}

glm::vec3 DistanceJoint::getReactionForce() const {
    return impulse * normal;
}

glm::vec3 DistanceJoint::getReactionTorque() const {
    glm::vec3 force = getReactionForce();
    glm::vec3 torqueA(0.0f), torqueB(0.0f);
    
    if (bodyA) {
        torqueA = glm::cross(rA, -force);
    }
    if (bodyB) {
        torqueB = glm::cross(rB, force);
    }
    
    return torqueA + torqueB;
}

float DistanceJoint::getCurrentLength() const {
    return glm::length(getWorldAnchorB() - getWorldAnchorA());
}

float DistanceJoint::getCurrentError() const {
    float currentLength = getCurrentLength();
    
    if (hasMinLength && currentLength < minLength) {
        return currentLength - minLength;
    } else if (hasMaxLength && currentLength > maxLength) {
        return currentLength - maxLength;
    } else {
        return currentLength - restLength;
    }
}

std::string DistanceJoint::getDebugInfo() const {
    std::ostringstream oss;
    oss << "DistanceJoint:\n";
    oss << "  Enabled: " << (enabled ? "true" : "false") << "\n";
    oss << "  Broken: " << (broken ? "true" : "false") << "\n";
    oss << "  Rest Length: " << restLength << "\n";
    oss << "  Current Length: " << getCurrentLength() << "\n";
    oss << "  Error: " << getCurrentError() << "\n";
    oss << "  Applied Impulse: " << impulse << "\n";
    oss << "  Stiffness: " << stiffness << "\n";
    oss << "  Damping: " << damping << "\n";
    
    if (hasMinLength) {
        oss << "  Min Length: " << minLength << "\n";
    }
    if (hasMaxLength) {
        oss << "  Max Length: " << maxLength << "\n";
    }
    
    return oss.str();
}

} // namespace engine::physics