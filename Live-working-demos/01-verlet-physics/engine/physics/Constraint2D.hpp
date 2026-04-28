// engine/physics/Constraint2D.hpp
#pragma once
#include <vector>
#include <glm/glm.hpp>
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

        Constraint2D(int i1, int i2, float length, float stiff = 1.0f)
            : p1Index(i1), p2Index(i2), restLength(length), stiffness(stiff) {}

        bool satisfy(std::vector<Particle>& particles);
    };

} // namespace engine::physics
