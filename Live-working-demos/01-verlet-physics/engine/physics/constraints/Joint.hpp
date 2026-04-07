#pragma once
#include "Constraint.hpp"

namespace engine::physics {

/**
 * @brief Base class for all joint constraints
 */
class Joint : public Constraint {
public:
    Joint(RigidBody* bodyA, RigidBody* bodyB,
          const glm::vec3& anchorA, const glm::vec3& anchorB);

    // Joint-specific interface
    virtual glm::vec3 getReactionForce() const = 0;
    virtual glm::vec3 getReactionTorque() const = 0;
    
    // Anchor points
    const glm::vec3& getLocalAnchorA() const { return localAnchorA; }
    const glm::vec3& getLocalAnchorB() const { return localAnchorB; }
    
    void setLocalAnchorA(const glm::vec3& anchor) { localAnchorA = anchor; }
    void setLocalAnchorB(const glm::vec3& anchor) { localAnchorB = anchor; }
    
    // World space anchor points
    glm::vec3 getWorldAnchorA() const;
    glm::vec3 getWorldAnchorB() const;
    
    // Joint limits
    void setMotorEnabled(bool enabled) { motorEnabled = enabled; }
    bool isMotorEnabled() const { return motorEnabled; }
    
    void setMotorSpeed(float speed) { motorSpeed = speed; }
    float getMotorSpeed() const { return motorSpeed; }
    
    void setMaxMotorForce(float force) { maxMotorForce = force; }
    float getMaxMotorForce() const { return maxMotorForce; }

protected:
    glm::vec3 localAnchorA;  // Anchor point in body A's local space
    glm::vec3 localAnchorB;  // Anchor point in body B's local space
    
    // Motor properties
    bool motorEnabled = false;
    float motorSpeed = 0.0f;
    float maxMotorForce = 0.0f;
    
    // Cached values for solver
    glm::vec3 worldAnchorA;
    glm::vec3 worldAnchorB;
    glm::vec3 rA, rB;  // Relative anchor positions
    
    void updateAnchors();
    
    // Helper functions
    glm::vec3 getRelativeVelocity() const;
    float getRelativeAngularVelocity(const glm::vec3& axis) const;
};

} // namespace engine::physics