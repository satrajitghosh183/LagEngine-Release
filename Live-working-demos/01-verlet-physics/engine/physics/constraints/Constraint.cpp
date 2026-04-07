#include "Constraint.hpp"
#include "../../core/Logger.hpp"
#include <algorithm>

namespace engine::physics {

Constraint::Constraint(RigidBody* a, RigidBody* b) 
    : bodyA(a), bodyB(b) {
    if (!bodyA) {
        throw std::invalid_argument("Constraint bodyA cannot be null");
    }
}

float Constraint::calculateEffectiveMass(const glm::vec3& jacobianLinearA,
                                        const glm::vec3& jacobianAngularA,
                                        const glm::vec3& jacobianLinearB,
                                        const glm::vec3& jacobianAngularB) const {
    float effectiveMass = 0.0f;
    
    if (bodyA && bodyA->getBodyType() == RigidBody::BodyType::Dynamic) {
        effectiveMass += bodyA->getInverseMass() * glm::dot(jacobianLinearA, jacobianLinearA);
        effectiveMass += glm::dot(jacobianAngularA, bodyA->getInverseInertiaTensor() * jacobianAngularA);
    }
    
    if (bodyB && bodyB->getBodyType() == RigidBody::BodyType::Dynamic) {
        effectiveMass += bodyB->getInverseMass() * glm::dot(jacobianLinearB, jacobianLinearB);
        effectiveMass += glm::dot(jacobianAngularB, bodyB->getInverseInertiaTensor() * jacobianAngularB);
    }
    
    return effectiveMass > 0.0f ? 1.0f / effectiveMass : 0.0f;
}

void Constraint::applyImpulse(const glm::vec3& linearImpulseA,
                             const glm::vec3& angularImpulseA,
                             const glm::vec3& linearImpulseB,
                             const glm::vec3& angularImpulseB) {
    if (bodyA && bodyA->getBodyType() == RigidBody::BodyType::Dynamic) {
        bodyA->setLinearVelocity(bodyA->getLinearVelocity() + linearImpulseA * bodyA->getInverseMass());
        bodyA->setAngularVelocity(bodyA->getAngularVelocity() + bodyA->getInverseInertiaTensor() * angularImpulseA);
    }
    
    if (bodyB && bodyB->getBodyType() == RigidBody::BodyType::Dynamic) {
        bodyB->setLinearVelocity(bodyB->getLinearVelocity() + linearImpulseB * bodyB->getInverseMass());
        bodyB->setAngularVelocity(bodyB->getAngularVelocity() + bodyB->getInverseInertiaTensor() * angularImpulseB);
    }
}

float Constraint::solveConstraint(ConstraintData& constraint, float deltaVelocity) {
    float lambda = -deltaVelocity - constraint.bias;
    lambda /= constraint.effectiveMass;
    
    float oldImpulse = constraint.accumulatedImpulse;
    constraint.accumulatedImpulse = glm::clamp(oldImpulse + lambda, 
                                              constraint.lowerLimit, 
                                              constraint.upperLimit);
    lambda = constraint.accumulatedImpulse - oldImpulse;
    
    // Apply impulse
    applyImpulse(constraint.jacobianLinearA * lambda,
                constraint.jacobianAngularA * lambda,
                constraint.jacobianLinearB * lambda,
                constraint.jacobianAngularB * lambda);
    
    return lambda;
}

void Constraint::checkBreakage(float appliedForce) {
    if (!broken && std::abs(appliedForce) > breakForce) {
        broken = true;
        enabled = false;
        engine::core::log::Logger::log("Constraint broken due to excessive force: " + 
                                      std::to_string(appliedForce), 
                                      engine::core::log::LogLevel::Info);
    }
}

} // namespace engine::physics