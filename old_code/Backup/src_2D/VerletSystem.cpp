#include "VerletSystem.hpp"

VerletSystem::VerletSystem() { }

void VerletSystem::addParticle(const sf::Vector2f& position) {
    particles.emplace_back(position);
}

void VerletSystem::addParticle(const sf::Vector2f& position, const sf::Vector2f& initialVelocity) {
    particles.emplace_back(position, initialVelocity);
}

void VerletSystem::update(float dt, const sf::Vector2f& acceleration, const sf::RenderWindow& window) {
    for (auto &p : particles) {
        p.update(dt, acceleration);
        p.constrainToWindow(window);
    }
}

void VerletSystem::draw(sf::RenderWindow& window) {
    for (const auto &p : particles) {
        p.draw(window);
    }
}
