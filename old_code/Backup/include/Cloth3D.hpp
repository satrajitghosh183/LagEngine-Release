#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "Particle3D.hpp"

struct Constraint3D {
    int p1, p2;
    float restLength;
};

class Cloth3D {
public:
    std::vector<Particle3D> particles;
    std::vector<Constraint3D> constraints;
    int width, height;
    float spacing;

    // Create a cloth grid (particles arranged in the X-Y plane, for example).
    // The top row is locked.
    Cloth3D(int width, int height, float spacing, const glm::vec3 &origin);

    // dt: delta time, acceleration: gravity (plus wind added later),
    // iterations: number of constraint relaxations.
    void update(float dt, const glm::vec3 &acceleration, int iterations);

    // A simple wind method: add a force to every non-locked particle.
    void applyWind(const glm::vec3 &windForce);
};
