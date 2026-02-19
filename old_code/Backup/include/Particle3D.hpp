#pragma once
#include <glm/glm.hpp>

class Particle3D {
public:
    glm::vec3 pos;
    glm::vec3 oldPos;
    bool locked;

    Particle3D(const glm::vec3 &position);
    Particle3D(const glm::vec3 &position, const glm::vec3 &initialVelocity);

    // Update using Verlet integration.
    // Optional damping factor (default 0.99 for a bit of energy loss).
    void update(float dt, const glm::vec3 &acceleration, float damping = 0.99f);
};
