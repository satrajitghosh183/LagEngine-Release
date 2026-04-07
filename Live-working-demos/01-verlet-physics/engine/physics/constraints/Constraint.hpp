#pragma once
#include <glm/glm.hpp>
#include "../RigidBody.hpp"

namespace engine::physics {

/**
 * @brief Base class for all physics constraints
 */
class Constraint {
public:
    Constraint(RigidBody* bodyA, RigidBody* bodyB = nullptr);
    virtual ~Constraint() = default;

    // Called once per frame to solve the constraint
    virtual void solve(float dt) = 0;
    
    // Called to prepare constraint for solving (calculate Jacobians, etc.)
    virtual void prepare(float dt) = 0;
    
    // Called to apply cached impulses from previous frame
    virtual void warmStart() = 0;
    
    // Called after solving to store impulses for next frame
    virtual void storeImpulses() = 0;
    
    // Getters
    RigidBody* getBodyA() const { return bodyA; }
    RigidBody* getBodyB() const { return bodyB; }
    
    bool isEnabled() const { return enabled; }
    void setEnabled(bool enable) { enabled = enable; }
    
    float getBreakForce() const { return breakForce; }
    void setBreakForce(float force) { breakForce = force; }
    
    bool isBroken() const { return broken; }
    
    // Debug information
    virtual std::string getDebugInfo() const = 0;
    virtual float getAppliedImpulse() const = 0;

protected:
    RigidBody* bodyA;
    RigidBody* bodyB;
    
    bool enabled = true;
    bool broken = false;
    float breakForce = FLT_MAX;  // Force required to break constraint
    
    // Constraint solving helpers
    struct ConstraintData {
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
    
    // Helper functions for constraint solving
    float calculateEffectiveMass(const glm::vec3& jacobianLinearA,
                                const glm::vec3& jacobianAngularA,
                                const glm::vec3& jacobianLinearB,
                                const glm::vec3& jacobianAngularB) const;
    
    void applyImpulse(const glm::vec3& linearImpulseA,
                     const glm::vec3& angularImpulseA,
                     const glm::vec3& linearImpulseB,
                     const glm::vec3& angularImpulseB);
    
    float solveConstraint(ConstraintData& constraint, float deltaVelocity);
    
    // Check if constraint should break
    void checkBreakage(float appliedForce);
};

} // namespace engine::physics