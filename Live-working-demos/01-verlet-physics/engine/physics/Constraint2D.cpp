// engine/physics/Constraint2D.cpp
#include "engine/physics/Constraint2D.hpp"
#include <cmath>
#include <iostream>

namespace engine::physics {

    bool Constraint2D::satisfy(std::vector<Particle>& particles) {
        if (!active) return false;

        if (p1Index >= static_cast<int>(particles.size()) ||
            p2Index >= static_cast<int>(particles.size())) {
            std::cerr << "Constraint index out of bounds: " << p1Index << ", " << p2Index << std::endl;
            return false;
        }

        Particle& p1 = particles[p1Index];
        Particle& p2 = particles[p2Index];

        glm::vec2 delta = p2.pos - p1.pos;

        float dist = glm::length(delta);

        if (dist < 0.0001f) return false;

        float diff = (dist - restLength) / dist;

        diff *= stiffness;

        glm::vec2 correction = delta * 0.5f * diff;

        if (!p1.locked && !p2.locked) {
            p1.pos += correction;
            p2.pos -= correction;
        } else if (p1.locked && !p2.locked) {
            p2.pos -= correction * 2.0f;
        } else if (!p1.locked && p2.locked) {
            p1.pos += correction * 2.0f;
        } else {
            return false;
        }

        return true;
    }

}
