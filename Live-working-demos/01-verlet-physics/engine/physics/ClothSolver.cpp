// // engine/physics/ClothSolver.cpp
// #include "engine/physics/ClothSolver.hpp"

// namespace engine::physics {

//     void ClothSolver::update(float dt, const glm::vec3& acceleration) {
//         for (auto& p : particles) {
//             p.update(dt, acceleration);
//         }
//     }

//     void ClothSolver::solve(int iterations) {
//         for (int i = 0; i < iterations; ++i) {
//             for (const auto& c : constraints) {
//                 c.satisfy(particles);
//             }
//         }
//     }

//     void ClothSolver::addParticle(const glm::vec3& position, const glm::vec3& velocity, bool locked) {
//         Particle3D p(position, velocity);
//         p.locked = locked;
//         particles.push_back(p);
//     }

//     void ClothSolver::addConstraint(int i1, int i2, float restLength) {
//         constraints.emplace_back(i1, i2, restLength);
//     }

// } // namespace engine::physics



// engine/physics/ClothSolver.cpp
#include "engine/physics/ClothSolver.hpp"
#include <iostream>

namespace engine::physics {

    void ClothSolver::update(float dt, const glm::vec3& acceleration) {
        // Scale time for stability
        dt *= timeScale;
        
        // Cap dt to avoid instability with large time steps
        const float maxDt = 1.0f / 60.0f;
        if (dt > maxDt) dt = maxDt;
        
        // Use provided acceleration or default gravity
        glm::vec3 acc = (acceleration != glm::vec3(0.0f)) ? acceleration : gravity;
        
        // Update all particles (apply forces)
        for (auto& p : particles) {
            if (!p.locked) {
                // Apply damping
                glm::vec3 velocity = p.position - p.oldPosition;
                velocity *= damping;
                p.oldPosition = p.position - velocity;
                
                // Update position using Verlet integration
                p.update(dt, acc);
            }
        }
    }

    void ClothSolver::solve(int iterations) {
        // Use provided iterations or default
        int iters = (iterations > 0) ? iterations : solverIterations;
        
        // Solve constraints multiple times for stability
        for (int i = 0; i < iters; ++i) {
            for (auto& c : constraints) {
                c.satisfy(particles);
            }
        }
    }

    int ClothSolver::addParticle(const glm::vec3& position, const glm::vec3& velocity, bool locked) {
        Particle3D p(position, velocity);
        p.locked = locked;
        particles.push_back(p);
        return particles.size() - 1;
    }

    int ClothSolver::addConstraint(int i1, int i2, float restLength, float stiffness) {
        // Validate indices
        if (i1 >= particles.size() || i2 >= particles.size() || i1 < 0 || i2 < 0) {
            std::cerr << "Invalid particle indices for constraint: " << i1 << ", " << i2 << std::endl;
            return -1;
        }
        
        // If rest length not specified, calculate from current positions
        if (restLength <= 0.0f) {
            glm::vec3 delta = particles[i2].position - particles[i1].position;
            restLength = glm::length(delta);
        }
        
        constraints.emplace_back(i1, i2, restLength, stiffness);
        return constraints.size() - 1;
    }
    
    int ClothSolver::createGrid(int width, int height, float spacing, 
                             const glm::vec3& position, bool fixTopRow) {
        if (width < 2 || height < 2) {
            std::cerr << "Grid dimensions must be at least 2x2" << std::endl;
            return 0;
        }
        
        // Store starting particle count
        int startCount = particles.size();
        
        // Create particles
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                glm::vec3 pos = position + glm::vec3(x * spacing, 0, y * spacing);
                bool locked = fixTopRow && (y == 0);
                addParticle(pos, glm::vec3(0.0f), locked);
            }
        }
        
        // Create structural constraints (horizontal and vertical)
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = startCount + y * width + x;
                
                // Horizontal constraints
                if (x < width - 1) {
                    addConstraint(idx, idx + 1, spacing);
                }
                
                // Vertical constraints
                if (y < height - 1) {
                    addConstraint(idx, idx + width, spacing);
                }
                
                // Diagonal constraints (optional, for better stability)
                if (x < width - 1 && y < height - 1) {
                    float diagLength = spacing * 1.41421356f; // sqrt(2)
                    addConstraint(idx, idx + width + 1, diagLength, 0.8f);
                    addConstraint(idx + 1, idx + width, diagLength, 0.8f);
                }
            }
        }
        
        return particles.size() - startCount;
    }
    
    void ClothSolver::applyImpulse(int index, const glm::vec3& impulse) {
        if (index >= 0 && index < particles.size()) {
            Particle3D& p = particles[index];
            if (!p.locked) {
                // Apply impulse by changing oldPosition (which affects velocity)
                glm::vec3 velocity = p.position - p.oldPosition;
                velocity += impulse;
                p.oldPosition = p.position - velocity;
            }
        }
    }

} // namespace engine::physics