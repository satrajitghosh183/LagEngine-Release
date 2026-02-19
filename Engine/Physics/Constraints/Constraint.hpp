#pragma once

#include "../../Core/Base.hpp"
#include "../RigidBody.hpp"
#include <glm/glm.hpp>

namespace GameEngine {
namespace Physics {

    /**
     * @brief Base constraint interface
     * 
     * Constraints restrict the motion of rigid bodies
     * Implemented using sequential impulse method
     */
    class Constraint {
    public:
        virtual ~Constraint() = default;
        
        /**
         * @brief Prepare constraint for solving
         */
        virtual void Prepare(float deltaTime) = 0;
        
        /**
         * @brief Solve constraint (apply corrective impulses)
         */
        virtual void Solve() = 0;
        
        /**
         * @brief Get constraint bodies
         */
        virtual RigidBody* GetBodyA() const = 0;
        virtual RigidBody* GetBodyB() const = 0;
        
        /**
         * @brief Enable/disable constraint
         */
        bool Enabled = true;
        
        /**
         * @brief Breaking force threshold (0 = unbreakable)
         */
        float BreakForce = 0.0f;
    };

}}