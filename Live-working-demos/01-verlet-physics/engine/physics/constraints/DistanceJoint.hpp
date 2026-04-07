#pragma once
#include "Joint.hpp"

namespace engine::physics {

/**
 * @brief Distance joint keeps two bodies at a fixed distance
 * Useful for rope, chains, and distance constraints
 */
class DistanceJoint : public Joint {
public:
    DistanceJoint(RigidBody* bodyA, RigidBody* bodyB,
                  const glm::vec3& anchorA, const glm::vec3& anchorB,
                  float restLength = -1.0f);  // -1 = use current distance

    // Constraint interface
    void solve(float dt) override;
    void prepare(float dt) override;
    void warmStart() override;
    void storeImpulses() override;
    
    // Joint interface
    glm::vec3 getReactionForce() const override;
    glm::vec3 getReactionTorque() const override;
    
    // Distance joint specific
    float getRestLength() const { return restLength; }
    void setRestLength(float length) { restLength = length; }
    
    float getStiffness() const { return stiffness; }
    void setStiffness(float k) { stiffness = glm::clamp(k, 0.0f, 1.0f); }
    
    float getDamping() const { return damping; }
    void setDamping(float d) { damping = glm::clamp(d, 0.0f, 1.0f); }
    
    // Constraint limits
    void setMinLength(float minLen) { minLength = minLen; hasMinLength = true; }
    void setMaxLength(float maxLen) { maxLength = maxLen; hasMaxLength = true; }
    void clearLengthLimits() { hasMinLength = hasMaxLength = false; }
    
    float getCurrentLength() const;
    float getCurrentError() const;
    
    // Debug
    std::string getDebugInfo() const override;
    float getAppliedImpulse() const override { return impulse; }

private:
    float restLength;
    float stiffness = 1.0f;
    float damping = 0.1f;
    
    // Length limits
    bool hasMinLength = false;
    bool hasMaxLength = false;
    float minLength = 0.0f;
    float maxLength = FLT_MAX;
    
    // Solver variables
    float impulse = 0.0f;
    float bias = 0.0f;
    float effectiveMass = 0.0f;
    glm::vec3 jacobian;
    glm::vec3 normal;  // Direction from A to B
    
    // Spring-damper system
    bool useSpringDamper = false;
    void solveSpringDamper(float dt);
    void solveRigidConstraint(float dt);
};

} // namespace engine::physics