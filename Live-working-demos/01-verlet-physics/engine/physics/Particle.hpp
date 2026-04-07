// #pragma once
// #include <SFML/Graphics.hpp>

// namespace engine::physics {

//     class Particle {
//     public:
//         sf::Vector2f pos;
//         sf::Vector2f oldPos;
//         bool locked = false;

//         Particle() = default;
//         Particle(const sf::Vector2f& position);
//         Particle(const sf::Vector2f& position, const sf::Vector2f& velocity);

//         void update(float dt, const sf::Vector2f& acceleration);
//         void constrainToWindow(const sf::RenderWindow& window);
//         void draw(sf::RenderWindow& window) const;
//     };

// }


// Particle.hpp
#pragma once
#include <SFML/Graphics.hpp>

namespace engine::physics {

    class Particle {
    public:
        sf::Vector2f pos;      // Current position
        sf::Vector2f oldPos;   // Previous position (for velocity calculation)
        bool locked = false;   // If true, particle won't move

        /**
         * Default constructor
         */
        Particle() = default;

        /**
         * Create a particle at the specified position with zero velocity
         * @param position Initial position
         */
        Particle(const sf::Vector2f& position);

        /**
         * Create a particle with position and velocity
         * @param position Initial position
         * @param velocity Initial velocity
         */
        Particle(const sf::Vector2f& position, const sf::Vector2f& velocity);

        /**
         * Updates particle position using Verlet integration
         * @param dt Delta time in seconds
         * @param acceleration Acceleration vector (e.g., gravity)
         */
        void update(float dt, const sf::Vector2f& acceleration);

        /**
         * Keeps the particle within window bounds
         * @param window Reference window for boundary checking
         */
        void constrainToWindow(const sf::RenderWindow& window);

        /**
         * Draws the particle for debugging purposes
         * @param window Target rendering window
         */
        void draw(sf::RenderWindow& window) const;

        /**
         * Gets the current velocity from position difference
         * @return Current velocity vector
         */
        sf::Vector2f getVelocity() const;

        /**
         * Sets velocity directly
         * @param velocity New velocity vector
         */
        void setVelocity(const sf::Vector2f& velocity);
    };

}