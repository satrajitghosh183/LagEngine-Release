
#include "Particle3D.hpp"

namespace engine::physics {

Particle3D::Particle3D(const glm::vec3& pos, float m, bool pin)
    : position(pos), previousPosition(pos), accumulatedForce(0.0f), mass(m), pinned(pin) {}

void Particle3D::applyForce(const glm::vec3& force) {
    if (!pinned) {
        accumulatedForce += force;
    }
}

void Particle3D::integrate(float dt, float globalDamping) {
    if (pinned) {
        previousPosition = position; // stay locked
        accumulatedForce = glm::vec3(0.0f);
        return;
    }

    glm::vec3 velocity = (position - previousPosition) * globalDamping;
    previousPosition = position;
    position += velocity + (accumulatedForce / mass) * dt * dt;

    accumulatedForce = glm::vec3(0.0f);
}

void Particle3D::pin() {
    pinned = true;
}

void Particle3D::unpin() {
    pinned = false;
}

bool Particle3D::isPinned() const {
    return pinned;
}

void Particle3D::setPinned(bool value) {
    pinned = value;
}

const glm::vec3& Particle3D::getPosition() const {
    return position;
}

void Particle3D::setPosition(const glm::vec3& pos) {
    position = pos;
}

const glm::vec3& Particle3D::getPreviousPosition() const {
    return previousPosition;
}

float Particle3D::getMass() const {
    return mass;
}

} // namespace engine::physics
