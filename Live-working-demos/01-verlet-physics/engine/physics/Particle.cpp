// #include "engine/physics/Particle.hpp"

// namespace engine::physics {

//     Particle::Particle(const sf::Vector2f& position) : pos(position), oldPos(position) {}

//     Particle::Particle(const sf::Vector2f& position, const sf::Vector2f& velocity)
//         : pos(position), oldPos(position - velocity) {}

//     void Particle::update(float dt, const sf::Vector2f& acceleration) {
//         if (locked) return;
//         sf::Vector2f temp = pos;
//         pos += pos - oldPos + acceleration * dt * dt;
//         oldPos = temp;
//     }

//     void Particle::constrainToWindow(const sf::RenderWindow& window) {
//         if (pos.x < 0) pos.x = 0;
//         if (pos.y < 0) pos.y = 0;
//         if (pos.x > window.getSize().x) pos.x = window.getSize().x;
//         if (pos.y > window.getSize().y) pos.y = window.getSize().y;
//     }

//     void Particle::draw(sf::RenderWindow& window) const {
//         sf::CircleShape circle(2.f);
//         circle.setFillColor(sf::Color::White);
//         circle.setPosition(pos - sf::Vector2f(2.f, 2.f));
//         window.draw(circle);
//     }

// }

// Particle.cpp
#include "engine/physics/Particle.hpp"
#include <iostream>

namespace engine::physics {

    Particle::Particle(const sf::Vector2f& position) 
        : pos(position), oldPos(position) {
        // Initialize with zero velocity
    }

    Particle::Particle(const sf::Vector2f& position, const sf::Vector2f& velocity)
        : pos(position), oldPos(position - velocity) {
        // Initialize with specified velocity (stored as difference between pos and oldPos)
    }

    void Particle::update(float dt, const sf::Vector2f& acceleration) {
        if (locked) return;
        
        // Store current position for velocity calculation
        sf::Vector2f temp = pos;
        
        // Verlet integration: x(t+dt) = 2*x(t) - x(t-dt) + a*dt²
        // Which simplifies to: x(t+dt) = x(t) + (x(t) - x(t-dt)) + a*dt²
        // Where (x(t) - x(t-dt)) is the current velocity
        
        // Get current velocity
        sf::Vector2f velocity = pos - oldPos;
        
        // Update position with velocity and acceleration
        pos = pos + velocity + acceleration * dt * dt;
        
        // Update old position for next velocity calculation
        oldPos = temp;
    }

    sf::Vector2f Particle::getVelocity() const {
        return pos - oldPos;
    }

    void Particle::setVelocity(const sf::Vector2f& velocity) {
        oldPos = pos - velocity;
    }

    void Particle::constrainToWindow(const sf::RenderWindow& window) {
        sf::Vector2u size = window.getSize();
        sf::Vector2f velocity = pos - oldPos;
        
        // Left boundary
        if (pos.x < 0) {
            pos.x = 0;
            oldPos.x = pos.x + velocity.x; // Reverse velocity.x
        }
        
        // Top boundary
        if (pos.y < 0) {
            pos.y = 0;
            oldPos.y = pos.y + velocity.y; // Reverse velocity.y
        }
        
        // Right boundary
        if (pos.x > size.x) {
            pos.x = size.x;
            oldPos.x = pos.x + velocity.x; // Reverse velocity.x
        }
        
        // Bottom boundary
        if (pos.y > size.y) {
            pos.y = size.y;
            oldPos.y = pos.y + velocity.y; // Reverse velocity.y
        }
    }

    void Particle::draw(sf::RenderWindow& window) const {
        // Draw a small circle to represent the particle
        sf::CircleShape circle(3.f);
        circle.setFillColor(locked ? sf::Color::Red : sf::Color::White);
        circle.setOrigin(3.f, 3.f);
        circle.setPosition(pos);
        window.draw(circle);
        
        // Optionally draw a line showing velocity
        sf::Vector2f velocity = pos - oldPos;
        if (velocity.x != 0 || velocity.y != 0) {
            sf::Vertex line[] = {
                sf::Vertex(pos, sf::Color::Yellow),
                sf::Vertex(pos + velocity * 5.f, sf::Color::Yellow)
            };
            window.draw(line, 2, sf::Lines);
        }
    }
}