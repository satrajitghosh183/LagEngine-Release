
// #pragma once

// #include <SFML/Graphics.hpp>
// #include "engine/physics/Particle.hpp"
// #include "engine/scene/Object2D.hpp"

// namespace engine::objects {

//     class Ball2D : public engine::scene::Object2D {
//     public:
//         engine::physics::Particle particle;
//         float radius;
//         float restitution;
//         sf::Vector2f gravity = {0.f, 980.f}; // ✅ gravity added internally

//         Ball2D(const sf::Vector2f& position,
//                const sf::Vector2f& velocity,
//                float r,
//                float rest = 0.8f);

//         void update(float dt) override;
//         void render(sf::RenderWindow& window) override;
//     };

// }


// Ball2D.hpp
#pragma once

#include <SFML/Graphics.hpp>
#include "engine/physics/Particle.hpp"
#include "engine/scene/Object2D.hpp"

namespace engine::objects {

    class Ball2D : public engine::scene::Object2D {
    public:
        engine::physics::Particle particle;
        float radius;
        float restitution;
        sf::Vector2f gravity = {0.f, 980.f};
        sf::Color color = sf::Color::Red; // Default to red for better visibility
        int stationaryFrames = 0; // Counter for tracking stationary frames

        /**
         * Creates a ball with physics properties
         * @param position Initial position of the ball
         * @param velocity Initial velocity of the ball
         * @param r Radius of the ball
         * @param rest Coefficient of restitution (bounciness)
         * @param ballColor Color of the ball
         */
        Ball2D(const sf::Vector2f& position,
               const sf::Vector2f& velocity,
               float r,
               float rest = 0.8f,
               const sf::Color& ballColor = sf::Color::Red);

        /**
         * Updates the ball's physics
         * @param dt Delta time in seconds
         */
        void update(float dt) override;
        
        /**
         * Renders the ball to the window
         * @param window Target rendering window
         */
        void render(sf::RenderWindow& window) override;
    };
}