#pragma once
#include <SFML/Graphics.hpp>

class Particle {
public:
    sf::Vector2f pos;
    sf::Vector2f oldPos;
    bool locked;

    // Constructor with no initial velocity (stationary start).
    Particle(const sf::Vector2f& position);

    // New constructor that sets an initial velocity.
    Particle(const sf::Vector2f& position, const sf::Vector2f& initialVelocity);

    // Update particle position using Verlet integration.
    void update(float dt, const sf::Vector2f& acceleration);
    
    // Constrain the particle within the window boundaries.
    void constrainToWindow(const sf::RenderWindow& window);

    // Lock/unlock particle motion.
    void lock();
    void unlock();

    // Render the particle as a small circle.
    void draw(sf::RenderWindow& window) const;
};
