#pragma once
#include <glm/glm.hpp>
#include "Particle3D.hpp"

class Ball3D {
public:
    Particle3D particle;
    float radius;
    float restitution; // Bounce factor.

    Ball3D(const glm::vec3 &position, const glm::vec3 &initialVelocity, float radius, float restitution = 0.95f);

    // Update ball physics and perform boundary collisions within a 3D box.
    void update(float dt, const glm::vec3 &acceleration, const glm::vec3 &minBounds, const glm::vec3 &maxBounds);
};
