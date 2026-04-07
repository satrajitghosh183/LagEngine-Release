// // engine/physics/Constraint2D.cpp
// #include "engine/physics/Constraint2D.hpp"
// #include <cmath> // ✅ This gives you std::sqrt

// namespace engine::physics {

//     void Constraint2D::satisfy(std::vector<Particle>& particles) {
//         Particle& p1 = particles[p1Index];
//         Particle& p2 = particles[p2Index];

//         sf::Vector2f delta = p2.pos - p1.pos;
//         float dist = std::sqrt(delta.x * delta.x + delta.y * delta.y);
//         if (dist == 0.0f) return;

//         float diff = (dist - restLength) / dist;
//         sf::Vector2f correction = delta * 0.5f * diff;

//         if (!p1.locked && !p2.locked) {
//             p1.pos += correction;
//             p2.pos -= correction;
//         } else if (p1.locked && !p2.locked) {
//             p2.pos -= correction * 2.0f;
//         } else if (!p1.locked && p2.locked) {
//             p1.pos += correction * 2.0f;
//         }
//     }

// }
// engine/physics/Constraint2D.cpp
#include "engine/physics/Constraint2D.hpp"
#include <cmath>
#include <iostream>

namespace engine::physics {

    bool Constraint2D::satisfy(std::vector<Particle>& particles) {
        if (!active) return false;
        
        // Bounds checking
        if (p1Index >= particles.size() || p2Index >= particles.size()) {
            std::cerr << "Constraint index out of bounds: " << p1Index << ", " << p2Index << std::endl;
            return false;
        }
        
        Particle& p1 = particles[p1Index];
        Particle& p2 = particles[p2Index];

        // Calculate displacement vector
        sf::Vector2f delta = p2.pos - p1.pos;
        
        // Calculate current distance
        float dist = std::hypot(delta.x, delta.y); // More accurate than sqrt
        
        // Avoid division by zero
        if (dist < 0.0001f) return false;

        // Calculate displacement ratio (how much to move)
        float diff = (dist - restLength) / dist;
        
        // Apply stiffness factor
        diff *= stiffness;
        
        // Calculate correction vectors
        sf::Vector2f correction = delta * 0.5f * diff;

        // Apply correction based on which particles are locked
        if (!p1.locked && !p2.locked) {
            p1.pos += correction;
            p2.pos -= correction;
        } else if (p1.locked && !p2.locked) {
            p2.pos -= correction * 2.0f;
        } else if (!p1.locked && p2.locked) {
            p1.pos += correction * 2.0f;
        } else {
            // Both particles locked, constraint can't be satisfied
            return false;
        }
        
        return true;
    }
    
    void Constraint2D::draw(sf::RenderWindow& window, const std::vector<Particle>& particles, 
                           const sf::Color& color) const {
        if (!active || p1Index >= particles.size() || p2Index >= particles.size()) 
            return;
            
        // Create a line between the two particles
        sf::Vertex line[] = { 
            sf::Vertex(particles[p1Index].pos, color),
            sf::Vertex(particles[p2Index].pos, color)
        };
        
        window.draw(line, 2, sf::Lines);
    }
}