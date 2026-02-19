#include "Particle3D.hpp"

Particle3D::Particle3D(const glm::vec3 &position)
    : pos(position), oldPos(position), locked(false) {}

Particle3D::Particle3D(const glm::vec3 &position, const glm::vec3 &initialVelocity)
    : pos(position), locked(false)
{
    oldPos = position - initialVelocity;
}

void Particle3D::update(float dt, const glm::vec3 &acceleration, float damping) {
    if (locked)
        return;
    glm::vec3 velocity = (pos - oldPos) * damping;
    glm::vec3 newPos = pos + velocity + acceleration * (dt * dt);
    oldPos = pos;
    pos = newPos;
}
