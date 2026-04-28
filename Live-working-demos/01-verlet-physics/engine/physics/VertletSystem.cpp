// engine/physics/VerletSystem.cpp
//
// 2D Verlet particle system. All types use GLM.
#include "engine/physics/VertletSystem.hpp"
#include <iostream>
#include <cmath>

namespace engine::physics {

    int VerletSystem::addParticle(const glm::vec2& position) {
        particles.emplace_back(position);
        return static_cast<int>(particles.size()) - 1;
    }

    int VerletSystem::addParticle(const glm::vec2& position, const glm::vec2& initialVelocity) {
        particles.emplace_back(position, initialVelocity);
        return static_cast<int>(particles.size()) - 1;
    }

    int VerletSystem::addConstraint(int i1, int i2, float restLength) {
        // Validate indices
        if (i1 >= static_cast<int>(particles.size()) ||
            i2 >= static_cast<int>(particles.size()) || i1 < 0 || i2 < 0) {
            std::cerr << "Invalid particle indices for constraint: " << i1 << ", " << i2 << std::endl;
            return -1;
        }

        // If rest length not specified, use current distance
        if (restLength <= 0.0f) {
            glm::vec2 delta = particles[i2].pos - particles[i1].pos;
            restLength = glm::length(delta);
        }

        constraints.emplace_back(i1, i2, restLength);
        return static_cast<int>(constraints.size()) - 1;
    }

    void VerletSystem::update(float dt, const glm::uvec2& windowBounds) {
        // Scale time for stability
        dt *= timeScale;

        // Cap dt to avoid instability with large time steps
        const float maxDt = 1.0f / 60.0f;
        if (dt > maxDt) dt = maxDt;

        // Update all particles (apply forces)
        for (auto& p : particles) {
            if (!p.locked) {
                // Apply damping to velocity (stored as difference between pos and oldPos)
                glm::vec2 velocity = p.pos - p.oldPos;
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
                    if (p.pos.x > static_cast<float>(windowBounds.x))
                        p.pos.x = static_cast<float>(windowBounds.x);
                    if (p.pos.y > static_cast<float>(windowBounds.y))
                        p.pos.y = static_cast<float>(windowBounds.y);
                }
            }
        }
    }

    void VerletSystem::clear() {
        particles.clear();
        constraints.clear();
    }

    void VerletSystem::applyImpulse(int index, const glm::vec2& impulse) {
        if (index >= 0 && index < static_cast<int>(particles.size())) {
            Particle& p = particles[index];
            if (!p.locked) {
                // Apply impulse by changing oldPos (which affects velocity)
                glm::vec2 velocity = p.pos - p.oldPos;
                velocity += impulse;
                p.oldPos = p.pos - velocity;
            }
        }
    }

} // namespace engine::physics
