#include "Particle.hpp"
#include <cmath>

// Constructor without initial velocity.
Particle::Particle(const sf::Vector2f& position)
    : pos(position), oldPos(position), locked(false) {}

// Constructor with initial velocity.
Particle::Particle(const sf::Vector2f& position, const sf::Vector2f& initialVelocity)
    : pos(position), locked(false)
{
    // Set oldPos such that the difference equals the initial velocity.
    oldPos = position - initialVelocity;
}

void Particle::update(float dt, const sf::Vector2f& acceleration) {
    if (locked)
        return;
    // Verlet integration: newPos = pos + (pos - oldPos) + acceleration * dt^2
    sf::Vector2f velocity = pos - oldPos;
    sf::Vector2f newPos = pos + velocity + acceleration * (dt * dt);
    oldPos = pos;
    pos = newPos;
}

void Particle::constrainToWindow(const sf::RenderWindow& window) {
    sf::Vector2u size = window.getSize();
    if (pos.x < 0) pos.x = 0;
    if (pos.y < 0) pos.y = 0;
    if (pos.x > size.x) pos.x = size.x;
    if (pos.y > size.y) pos.y = size.y;
}

void Particle::lock() {
    locked = true;
}

void Particle::unlock() {
    locked = false;
}

void Particle::draw(sf::RenderWindow& window) const {
    sf::CircleShape circle(3.f); // Draw as a 3-pixel radius circle.
    circle.setOrigin(3.f, 3.f);
    circle.setPosition(pos);
    circle.setFillColor(sf::Color::White);
    window.draw(circle);
}
