#include "Joint.hpp"

namespace engine::physics {

Joint::Joint(RigidBody* bodyA, RigidBody* bodyB,
             const glm::vec3& anchorA, const glm::vec3& anchorB)
    : Constraint(bodyA, bodyB), localAnchorA(anchorA), localAnchorB(anchorB) {
    updateAnchors();
}

glm::vec3 Joint::getWorldAnchorA() const {
    if (bodyA) {
        return bodyA->localToWorld(localAnchorA);
    }
    return localAnchorA;
}

glm::vec3 Joint::getWorldAnchorB() const {
    if (bodyB) {
        return bodyB->localToWorld(localAnchorB);
    }
    return localAnchorB;
}

void Joint::updateAnchors() {
    worldAnchorA = getWorldAnchorA();
    worldAnchorB = getWorldAnchorB();
    
    if (bodyA) {
        rA = worldAnchorA - bodyA->getPosition();
    }
    if (bodyB) {
        rB = worldAnchorB - bodyB->getPosition();
    }
}

glm::vec3 Joint::getRelativeVelocity() const {
    glm::vec3 velA(0.0f), velB(0.0f);
    
    if (bodyA) {
        velA = bodyA->getLinearVelocity() + glm::cross(bodyA->getAngularVelocity(), rA);
    }
    if (bodyB) {
        velB = bodyB->getLinearVelocity() + glm::cross(bodyB->getAngularVelocity(), rB);
    }
    
    return velB - velA;
}

float Joint::getRelativeAngularVelocity(const glm::vec3& axis) const {
    float angVelA = 0.0f, angVelB = 0.0f;
    
    if (bodyA) {
        angVelA = glm::dot(bodyA->getAngularVelocity(), axis);
    }
    if (bodyB) {
        angVelB = glm::dot(bodyB->getAngularVelocity(), axis);
    }
    
    return angVelB - angVelA;
}

} // namespace engine::physics