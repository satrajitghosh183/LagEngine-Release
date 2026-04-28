// engine/physics/VerletSystem.hpp
//
// 2D Verlet particle system. All types use GLM.
// Rendering is handled externally by the Vulkan entry points.
#pragma once

#include <vector>
#include <functional>
#include <glm/glm.hpp>
#include "Particle.hpp"
#include "Constraint2D.hpp"

namespace engine::physics {

    /**
     * @brief A complete 2D particle system using Verlet integration
     * Supports particles, constraints, and collision handling
     */
    class VerletSystem {
    public:
        std::vector<Particle> particles;
        std::vector<Constraint2D> constraints;

        // Physics parameters
        int solverIterations = 5;
        float timeScale = 1.0f;
        glm::vec2 gravity = {0.0f, 980.0f};
        float damping = 0.99f; // Velocity damping factor

        VerletSystem() = default;

        /**
         * @brief Add a particle to the system
         * @param position Initial position
         * @return Index of the created particle
         */
        int addParticle(const glm::vec2& position);

        /**
         * @brief Add a particle with initial velocity
         * @param position Initial position
         * @param initialVelocity Initial velocity
         * @return Index of the created particle
         */
        int addParticle(const glm::vec2& position, const glm::vec2& initialVelocity);

        /**
         * @brief Add a constraint between two particles
         * @param i1 Index of first particle
         * @param i2 Index of second particle
         * @param restLength Rest length (if 0, uses current distance)
         * @return Index of the created constraint
         */
        int addConstraint(int i1, int i2, float restLength = 0.0f);

        /**
         * @brief Update physics for the system
         * @param dt Delta time in seconds
         * @param windowBounds Optional window bounds for constraining particles (width, height)
         */
        void update(float dt, const glm::uvec2& windowBounds = {0, 0});

        /**
         * @brief Clear all particles and constraints
         */
        void clear();

        /**
         * @brief Apply an impulse to a specific particle
         * @param index Particle index
         * @param impulse Impulse force vector
         */
        void applyImpulse(int index, const glm::vec2& impulse);
    };

} // namespace engine::physics
