
// // engine/physics/Constraint2D.hpp
// #pragma once
// #include <vector>
// #include <SFML/Graphics.hpp>
// #include "engine/physics/Particle.hpp"

// namespace engine::physics {

//     class Constraint2D {
//     public:
//         int p1Index, p2Index;
//         float restLength;

//         Constraint2D(int i1, int i2, float length)
//             : p1Index(i1), p2Index(i2), restLength(length) {}

//         void satisfy(std::vector<Particle>& particles);
//     };

// } // namespace engine::physics
// engine/physics/Constraint2D.hpp
#pragma once
#include <vector>
#include <SFML/Graphics.hpp>
#include "engine/physics/Particle.hpp"

namespace engine::physics {

    /**
     * @brief Distance constraint between two particles for cloth simulation
     */
    class Constraint2D {
    public:
        int p1Index;        // Index of first particle
        int p2Index;        // Index of second particle
        float restLength;   // Rest length of the constraint (relaxed state)
        float stiffness = 1.0f; // Constraint stiffness (1.0 = fully stiff)
        bool active = true; // Whether constraint is active or broken

        /**
         * @brief Create a new constraint between two particles
         * @param i1 Index of first particle
         * @param i2 Index of second particle
         * @param length Rest length of the constraint
         * @param stiff Stiffness factor (default 1.0)
         */
        Constraint2D(int i1, int i2, float length, float stiff = 1.0f)
            : p1Index(i1), p2Index(i2), restLength(length), stiffness(stiff) {}

        /**
         * @brief Satisfy the constraint by moving particles
         * @param particles Vector of particles to modify
         * @return True if constraint was successfully satisfied
         */
        bool satisfy(std::vector<Particle>& particles);
        
        /**
         * @brief Draw the constraint for visualization
         * @param window Target window
         * @param particles Vector of particles for position data
         * @param color Color of the constraint line
         */
        void draw(sf::RenderWindow& window, const std::vector<Particle>& particles, 
                 const sf::Color& color = sf::Color::White) const;
    };

} // namespace engine::physics