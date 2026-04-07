#pragma once

#include "../../Core/Base.hpp"
#include "Constraint.hpp"
#include "../Collision/ContactManifold.hpp"
#include <vector>

namespace GameEngine {
namespace Physics {

    /**
     * @brief Sequential impulse constraint solver
     * 
     * Solves all constraints (contacts and joints) iteratively
     * Uses Projected Gauss-Seidel method
     */
    class ConstraintSolver {
    public:
        ConstraintSolver();
        
        /**
         * @brief Solve all constraints
         */
        void Solve(std::vector<Ref<Constraint>>& constraints,
                  std::vector<ContactManifold>& contacts,
                  float deltaTime);
        
        /**
         * @brief Set solver iterations
         */
        void SetIterations(int velocityIterations, int positionIterations);
        
        /**
         * @brief Get solver iterations
         */
        int GetVelocityIterations() const { return m_VelocityIterations; }
        int GetPositionIterations() const { return m_PositionIterations; }
        
    private:
        void SolveVelocityConstraints(std::vector<Ref<Constraint>>& constraints,
                                      std::vector<ContactManifold>& contacts);
        
        void SolvePositionConstraints(std::vector<ContactManifold>& contacts);
        
        void SolveContactVelocity(ContactManifold& manifold);
        void SolveContactPosition(ContactManifold& manifold);
        
    private:
        int m_VelocityIterations;
        int m_PositionIterations;
    };

}}