#pragma once
#include <vector>
#include <SFML/Graphics.hpp>
#include "Particle.hpp"

class VerletSystem {
public:
    std::vector<Particle> particles;

    VerletSystem();

    // Add a particle with zero initial velocity.
    void addParticle(const sf::Vector2f& position);

    // New: Add a particle with a specified initial velocity.
    void addParticle(const sf::Vector2f& position, const sf::Vector2f& initialVelocity);

    // Update all particles.
    void update(float dt, const sf::Vector2f& acceleration, const sf::RenderWindow& window);

    // Draw all particles.
    void draw(sf::RenderWindow& window);
};
