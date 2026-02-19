#pragma once
#include <SFML/Graphics.hpp>
#include "Particle.hpp"

class Ball {
public:
    Particle particle;
    float radius;
    float restitution; // Bounce factor (0 < restitution < 1)

    Ball(const sf::Vector2f& position, const sf::Vector2f& initialVelocity, float radius, float restitution = 0.8f);

    // Update ball physics and handle collisions with window boundaries.
    void update(float dt, const sf::Vector2f& acceleration, const sf::RenderWindow& window);

    // Render the ball.
    void draw(sf::RenderWindow& window) const;
};

