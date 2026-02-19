#pragma once

#include "../Core/Base.hpp"
#include "RigidBody.hpp"
#include "Collision/ContactManifold.hpp"
#include "Collision/CollisionDetector.hpp"
#include "Collision/SpatialHash.hpp"
#include "Constraints/ConstraintSolver.hpp"
#include "Constraints/Constraint.hpp"
#include <glm/glm.hpp>
#include <vector>

namespace GameEngine {
namespace Physics {

    /**
     * @brief Physics world - manages all physics simulation
     * 
     * Features:
     * - Rigid body management
     * - Gravity
     * - Collision detection with narrow phase
     * - Constraint solving (sequential impulse)
     * - Joint/constraint support
     * - Fixed timestep integration
     */
    class PhysicsWorld {
    public:
        PhysicsWorld();
        ~PhysicsWorld();
        
        /**
         * @brief Update physics simulation
         */
        void Update(float deltaTime);
        
        /**
         * @brief Add rigid body to world
         */
        void AddRigidBody(const Ref<RigidBody>& body);
        
        /**
         * @brief Remove rigid body from world
         */
        void RemoveRigidBody(const Ref<RigidBody>& body);
        
        /**
         * @brief Add constraint (joint) to world
         */
        void AddConstraint(const Ref<Constraint>& constraint);
        
        /**
         * @brief Remove constraint from world
         */
        void RemoveConstraint(const Ref<Constraint>& constraint);
        
        /**
         * @brief Set gravity
         */
        void SetGravity(const glm::vec3& gravity) { m_Gravity = gravity; }
        glm::vec3 GetGravity() const { return m_Gravity; }
        
        /**
         * @brief Get all rigid bodies
         */
        const std::vector<Ref<RigidBody>>& GetRigidBodies() const { return m_RigidBodies; }
        
        /**
         * @brief Get all constraints
         */
        const std::vector<Ref<Constraint>>& GetConstraints() const { return m_Constraints; }
        
        /**
         * @brief Get contact manifolds (for debug visualization)
         */
        const std::vector<ContactManifold>& GetContactManifolds() const { return m_ContactManifolds; }

        /**
         * @brief Raycast against all rigid bodies in the world
         * @param origin Ray origin
         * @param direction Ray direction (should be normalized)
         * @param maxDistance Maximum distance
         * @param outHit Closest hit result
         * @return true if something was hit
         */
        bool Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance,
                     CollisionShape::RaycastHit& outHit) const;
        
        /**
         * @brief Physics settings
         */
        void SetIterations(int iterations) { m_Iterations = iterations; }
        int GetIterations() const { return m_Iterations; }
        
        void SetVelocityIterations(int iterations);
        void SetPositionIterations(int iterations);
        
    private:
        void ApplyGravity();
        void DetectCollisions();
        void ResolveCollisions(float deltaTime);
        
    private:
        std::vector<Ref<RigidBody>> m_RigidBodies;
        std::vector<Ref<Constraint>> m_Constraints;
        std::vector<ContactManifold> m_ContactManifolds;
        
        Scope<ConstraintSolver> m_ConstraintSolver;
        Scope<SpatialHash> m_BroadPhase;
        
        glm::vec3 m_Gravity;
        int m_Iterations;
        bool m_UseBroadPhase = true;
    };

}}
