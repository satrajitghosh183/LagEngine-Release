#include "Ball.hpp"
#include <cmath>

Ball::Ball(const sf::Vector2f& position, const sf::Vector2f& initialVelocity, float r, float rest)
    : particle(position, initialVelocity), radius(r), restitution(rest) {}

void Ball::update(float dt, const sf::Vector2f& acceleration, const sf::RenderWindow& window) {
    // Update physics via Verlet integration.
    particle.update(dt, acceleration);

    // Retrieve window dimensions.
    sf::Vector2u winSize = window.getSize();

    // Floor collision.
    if (particle.pos.y + radius > winSize.y) {
        particle.pos.y = winSize.y - radius;
        float vy = particle.pos.y - particle.oldPos.y;
        particle.oldPos.y = particle.pos.y + vy * restitution;
    }
    // Ceiling collision.
    if (particle.pos.y - radius < 0) {
        particle.pos.y = radius;
        float vy = particle.pos.y - particle.oldPos.y;
        particle.oldPos.y = particle.pos.y + vy * restitution;
    }
    // Left wall.
    if (particle.pos.x - radius < 0) {
        particle.pos.x = radius;
        float vx = particle.pos.x - particle.oldPos.x;
        particle.oldPos.x = particle.pos.x + vx * restitution;
    }
    // Right wall.
    if (particle.pos.x + radius > winSize.x) {
        particle.pos.x = winSize.x - radius;
        float vx = particle.pos.x - particle.oldPos.x;
        particle.oldPos.x = particle.pos.x + vx * restitution;
    }
}

void Ball::draw(sf::RenderWindow& window) const {
    sf::CircleShape circle(radius);
    circle.setOrigin(radius, radius);
    circle.setPosition(particle.pos);
    circle.setFillColor(sf::Color::Cyan);
    window.draw(circle);
}
