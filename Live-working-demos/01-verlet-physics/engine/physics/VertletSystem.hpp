// // engine/physics/VerletSystem.hpp
// #pragma once
// #include <vector>
// #include <SFML/Graphics.hpp>
// #include "Particle.hpp"
// #include <glm/gtc/matrix_transform.hpp>

// namespace engine::physics {

//     // A simple 2D particle system using Verlet physics
//     class VerletSystem {
//     public:
//         std::vector<Particle> particles;

//         VerletSystem() = default;

//         void addParticle(const sf::Vector2f& position);
//         void addParticle(const sf::Vector2f& position, const sf::Vector2f& initialVelocity);
//         void update(float dt, const sf::Vector2f& acceleration, const sf::RenderWindow& window);
//         void draw(sf::RenderWindow& window);
//     };

// } // namespace engine::physics



// engine/physics/VerletSystem.hpp
#pragma once
#include <vector>
#include <SFML/Graphics.hpp>
#include "Particle.hpp"
#include "Constraint2D.hpp"
#include <functional>

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
        sf::Vector2f gravity = {0.0f, 980.0f};
        float damping = 0.99f; // Velocity damping factor
        
        VerletSystem() = default;

        /**
         * @brief Add a particle to the system
         * @param position Initial position
         * @return Index of the created particle
         */
        int addParticle(const sf::Vector2f& position);
        
        /**
         * @brief Add a particle with initial velocity
         * @param position Initial position
         * @param initialVelocity Initial velocity
         * @return Index of the created particle
         */
        int addParticle(const sf::Vector2f& position, const sf::Vector2f& initialVelocity);
        
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
         * @param windowBounds Optional window bounds for constraining particles
         */
        void update(float dt, const sf::Vector2u& windowBounds = {0, 0});
        
        /**
         * @brief Draw all particles and constraints
         * @param window Target window
         */
        void draw(sf::RenderWindow& window);
        
        /**
         * @brief Clear all particles and constraints
         */
        void clear();
        
        /**
         * @brief Apply an impulse to a specific particle
         * @param index Particle index
         * @param impulse Impulse force vector
         */
        void applyImpulse(int index, const sf::Vector2f& impulse);
    };

} // namespace engine::physics