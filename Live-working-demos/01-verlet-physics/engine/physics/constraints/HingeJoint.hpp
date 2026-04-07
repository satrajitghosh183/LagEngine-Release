#pragma once
#include "Joint.hpp"

namespace engine::physics {

/**
 * @brief Hinge joint allows rotation around a single axis
 * Perfect for doors, wheels, and rotating mechanisms
 */
class HingeJoint : public Joint {
public:
    HingeJoint(RigidBody* bodyA, RigidBody* bodyB,
               const glm::vec3& anchorA, const glm::vec3& anchorB,
               const glm::vec3& axis);

    // Constraint interface
    void solve(float dt) override;
    void prepare(float dt) override;
    void warmStart() override;
    void storeImpulses() override;
    
    // Joint interface
    glm::vec3 getReactionForce() const override;
    glm::vec3 getReactionTorque() const override;
    
    // Hinge-specific
    const glm::vec3& getAxis() const { return localAxisA; }
    void setAxis(const glm::vec3& axis) { localAxisA = axis; }
    
    float getCurrentAngle() const;
    float getAngularVelocity() const;
    
    // Angle limits
    void setAngleLimits(float lower, float upper);
    void clearAngleLimits() { hasLimits = false; }
    bool hasAngleLimits() const { return hasLimits; }
    float getLowerLimit() const { return lowerLimit; }
    float getUpperLimit() const { return upperLimit; }
    
    // Motor
    void setMotorEnabled(bool enabled) override { motorEnabled = enabled; }
    void setMotorSpeed(float speed) override { motorSpeed = speed; }
    void setMaxMotorTorque(float torque) { maxMotorTorque = torque; }
    float getMaxMotorTorque() const { return maxMotorTorque; }
    
    // Debug
    std::string getDebugInfo() const override;
    float getAppliedImpulse() const override;

private:
    glm::vec3 localAxisA;     // Hinge axis in body A's local space
    glm::vec3 localAxisB;     // Hinge axis in body B's local space
    
    // Angle limits
    bool hasLimits = false;
    float lowerLimit = 0.0f;
    float upperLimit = 0.0f;
    float referenceAngle = 0.0f;  // Initial angle when joint was created
    
    // Motor
    float maxMotorTorque = 0.0f;
    
    // Solver data (6 constraints: 3 linear + 2 angular)
    struct HingeConstraint {
        glm::vec3 jacobianLinearA;
        glm::vec3 jacobianAngularA;
        glm::vec3 jacobianLinearB;
        glm::vec3 jacobianAngularB;
        float bias;
        float effectiveMass;
        float accumulatedImpulse;
        float lowerLimit;
        float upperLimit;
    };
    
    HingeConstraint linearConstraints[3];  // Position constraints
    HingeConstraint angularConstraints[2]; // Two perpendicular angular constraints
    HingeConstraint limitConstraint;       // Angle limit constraint
    HingeConstraint motorConstraint;       // Motor constraint
    
    // Cached values
    glm::vec3 worldAxisA, worldAxisB;
    glm::vec3 perpA1, perpA2;  // Two perpendicular axes to hinge axis
    glm::vec3 perpB1, perpB2;
    
    void updateAxes();
    void prepareLinearConstraints(float dt);
    void prepareAngularConstraints(float dt);
    void prepareLimitConstraint(float dt);
    void prepareMotorConstraint(float dt);
    
    void solveLinearConstraints();
    void solveAngularConstraints();
    void solveLimitConstraint();
    void solveMotorConstraint();
    
    float getAngleBetweenAxes() const;
    glm::vec3 getPerpendicularVector(const glm::vec3& v) const;
};

} // namespace engine::physics