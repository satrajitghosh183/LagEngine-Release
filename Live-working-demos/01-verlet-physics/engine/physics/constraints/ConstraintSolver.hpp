#pragma once
#include "Constraint.hpp"
#include "../collision/ContactManifold.hpp"
#include <vector>
#include <memory>

namespace engine::physics {

/**
 * @brief Advanced constraint solver using sequential impulse method
 */
class ConstraintSolver {
public:
    ConstraintSolver();
    
    void solve(std::vector<ContactManifold>& contacts,
               std::vector<std::shared_ptr<Constraint>>& constraints,
               float dt,
               int velocityIterations = 8,
               int positionIterations = 3);
    
    // Configuration
    void setVelocityIterations(int iterations) { velocityIterations = iterations; }
    void setPositionIterations(int iterations) { positionIterations = iterations; }
    void setContactTolerance(float tolerance) { contactTolerance = tolerance; }
    void setJointTolerance(float tolerance) { jointTolerance = tolerance; }
    
    // Statistics
    struct SolverStats {
        int contactConstraints = 0;
        int jointConstraints = 0;
        float contactError = 0.0f;
        float jointError = 0.0f;
        float solveTime = 0.0f;
    };
    
    const SolverStats& getStats() const { return stats; }

private:
    int velocityIterations = 8;
    int positionIterations = 3;
    float contactTolerance = 0.01f;
    float jointTolerance = 0.001f;
    
    SolverStats stats;
    
    // Contact constraint data
    struct SolverContact {
        ContactManifold* manifold;
        
        // Constraint axes
        glm::vec3 normal;
        glm::vec3 tangent1, tangent2;
        
        struct ContactData {
            glm::vec3 rA, rB;  // Relative positions
            float normalMass;
            float tangentMass1, tangentMass2;
            float bias;
            float normalImpulse;
            float tangentImpulse1, tangentImpulse2;
            float velocityBias;
        };
        
        std::vector<ContactData> points;
        float friction;
        float restitution;
    };
    
    std::vector<SolverContact> solverContacts;
    
    // Contact solving
    void setupContacts(std::vector<ContactManifold>& contacts, float dt);
    void warmStartContacts();
    void solveVelocityConstraints(std::vector<std::shared_ptr<Constraint>>& constraints);
    void solvePositionConstraints(std::vector<ContactManifold>& contacts);
    void storeContactImpulses(std::vector<ContactManifold>& contacts);
    
    // Individual constraint solving
    void solveContactVelocityConstraint(SolverContact& contact);
    void solveContactPositionConstraint(ContactManifold& manifold);
    
    // Helper functions
    float calculateMixedRestitution(float restitutionA, float restitutionB);
    float calculateMixedFriction(float frictionA, float frictionB);
    void calculateContactTangents(const glm::vec3& normal, glm::vec3& tangent1, glm::vec3& tangent2);
    
    // Statistics
    void updateStatistics(const std::vector<ContactManifold>& contacts,
                         const std::vector<std::shared_ptr<Constraint>>& constraints);
};

} // namespace engine::physics