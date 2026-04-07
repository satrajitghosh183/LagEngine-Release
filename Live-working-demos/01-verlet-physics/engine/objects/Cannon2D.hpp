// // engine/objects/Cannon2D.hpp
// #pragma once

// #include <SFML/Graphics.hpp>
// #include "engine/scene/Object2D.hpp"

// namespace engine::objects {

//     class Cannon2D : public engine::scene::Object2D {
//     public:
//         sf::Vector2f position;
//         float angle = -90.0f; // Facing upward initially
//         float power = 800.0f; // Firing speed

//         Cannon2D(const sf::Vector2f& pos);

//         void rotate(float degrees);
//         void update(float dt) override;
//         void render(sf::RenderWindow& window) override;

//         sf::Vector2f getFiringVelocity() const;
//     };

// }

// Cannon2D.hpp
#pragma once

#include <SFML/Graphics.hpp>
#include "engine/scene/Object2D.hpp"

namespace engine::objects {

    class Cannon2D : public engine::scene::Object2D {
    public:
        sf::Vector2f position;
        float angle = -45.0f; // Angled for better initial trajectory
        float power = 600.0f; // Firing power

        /**
         * Creates a cannon at the specified position
         * @param pos Initial position of the cannon
         */
        Cannon2D(const sf::Vector2f& pos);

        /**
         * Rotates the cannon by the specified angle in degrees
         * @param degrees Rotation amount (positive = clockwise)
         */
        void rotate(float degrees);

        /**
         * Updates the cannon (not used for physics)
         * @param dt Delta time in seconds
         */
        void update(float dt) override;

        /**
         * Renders the cannon to the window
         * @param window Target rendering window
         */
        void render(sf::RenderWindow& window) override;

        /**
         * Calculates the velocity vector for firing projectiles
         * @return Velocity vector based on current angle and power
         */
        sf::Vector2f getFiringVelocity() const;

        /**
         * Calculates the position at the end of the barrel
         * @return Position at the end of the barrel for projectile spawning
         */
        sf::Vector2f getMuzzlePosition() const;

        /**
         * Increases or decreases firing power
         * @param amount Amount to change power by
         */
        void adjustPower(float amount);
    };
}