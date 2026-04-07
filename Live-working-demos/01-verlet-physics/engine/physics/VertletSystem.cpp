
// // VerletSystem.cpp
// #include "engine/physics/VerletSystem.hpp"

// namespace engine::physics {

//     void VerletSystem::addParticle(const sf::Vector2f& position) {
//         particles.emplace_back(position);
//     }

//     void VerletSystem::addParticle(const sf::Vector2f& position, const sf::Vector2f& initialVelocity) {
//         particles.emplace_back(position, initialVelocity);
//     }

//     void VerletSystem::update(float dt, const sf::Vector2f& acceleration, const sf::RenderWindow& window) {
//         for (auto& p : particles) {
//             p.update(dt, acceleration);
//             p.constrainToWindow(window);
//         }
//     }

//     void VerletSystem::draw(sf::RenderWindow& window) {
//         for (const auto& p : particles) {
//             p.draw(window);
//         }
//     }

// } // namespace engine::physics
// engine/physics/VerletSystem.cpp
#include "engine/physics/VerletSystem.hpp"
#include <iostream>

namespace engine::physics {

    int VerletSystem::addParticle(const sf::Vector2f& position) {
        particles.emplace_back(position);
        return particles.size() - 1;
    }

    int VerletSystem::addParticle(const sf::Vector2f& position, const sf::Vector2f& initialVelocity) {
        particles.emplace_back(position, initialVelocity);
        return particles.size() - 1;
    }
    
    int VerletSystem::addConstraint(int i1, int i2, float restLength) {
        // Validate indices
        if (i1 >= particles.size() || i2 >= particles.size() || i1 < 0 || i2 < 0) {
            std::cerr << "Invalid particle indices for constraint: " << i1 << ", " << i2 << std::endl;
            return -1;
        }
        
        // If rest length not specified, use current distance
        if (restLength <= 0.0f) {
            sf::Vector2f delta = particles[i2].pos - particles[i1].pos;
            restLength = std::hypot(delta.x, delta.y);
        }
        
        constraints.emplace_back(i1, i2, restLength);
        return constraints.size() - 1;
    }

    void VerletSystem::update(float dt, const sf::Vector2u& windowBounds) {
        // Scale time for stability
        dt *= timeScale;
        
        // Cap dt to avoid instability with large time steps
        const float maxDt = 1.0f / 60.0f;
        if (dt > maxDt) dt = maxDt;
        
        // Update all particles (apply forces)
        for (auto& p : particles) {
            if (!p.locked) {
                // Apply damping to velocity (stored as difference between pos and oldPos)
                sf::Vector2f velocity = p.pos - p.oldPos;
                velocity *= damping;
                p.oldPos = p.pos - velocity;
                
                // Update position using Verlet integration
                p.update(dt, gravity);
            }
        }
        
        // Solve constraints multiple times for stability
        for (int i = 0; i < solverIterations; ++i) {
            // Apply all constraints
            for (auto& c : constraints) {
                c.satisfy(particles);
            }
            
            // Apply window bounds constraints if provided
            if (windowBounds.x > 0 && windowBounds.y > 0) {
                for (auto& p : particles) {
                    // Constrain to window bounds
                    if (p.pos.x < 0) p.pos.x = 0;
                    if (p.pos.y < 0) p.pos.y = 0;
                    if (p.pos.x > windowBounds.x) p.pos.x = windowBounds.x;
                    if (p.pos.y > windowBounds.y) p.pos.y = windowBounds.y;
                }
            }
        }
    }

    void VerletSystem::draw(sf::RenderWindow& window) {
        // Draw all constraints first (underneath particles)
        for (const auto& c : constraints) {
            c.draw(window, particles);
        }
        
        // Draw all particles
        for (const auto& p : particles) {
            p.draw(window);
        }
    }
    
    void VerletSystem::clear() {
        particles.clear();
        constraints.clear();
    }
    
    void VerletSystem::applyImpulse(int index, const sf::Vector2f& impulse) {
        if (index >= 0 && index < particles.size()) {
            Particle& p = particles[index];
            if (!p.locked) {
                // Apply impulse by changing oldPos (which affects velocity)
                sf::Vector2f velocity = p.pos - p.oldPos;
                velocity += impulse;
                p.oldPos = p.pos - velocity;
            }
        }
    }

} // namespace engine::physics