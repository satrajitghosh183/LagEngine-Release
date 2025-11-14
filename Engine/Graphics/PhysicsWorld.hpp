#pragma once

#include "../Core/Base.hpp"
#include "RigidBody.hpp"
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
     * - Collision detection (basic)
     * - Constraint solving (future)
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
         * @brief Set gravity
         */
        void SetGravity(const glm::vec3& gravity) { m_Gravity = gravity; }
        glm::vec3 GetGravity() const { return m_Gravity; }
        
        /**
         * @brief Get all rigid bodies
         */
        const std::vector<Ref<RigidBody>>& GetRigidBodies() const { return m_RigidBodies; }
        
        /**
         * @brief Physics settings
         */
        void SetIterations(int iterations) { m_Iterations = iterations; }
        int GetIterations() const { return m_Iterations; }
        
    private:
        void ApplyGravity();
        void IntegrateVelocities(float deltaTime);
        void DetectCollisions();
        void ResolveCollisions();
        
    private:
        std::vector<Ref<RigidBody>> m_RigidBodies;
        glm::vec3 m_Gravity;
        int m_Iterations;
    };

}}