#include "Ball3D.hpp"

namespace engine::objects {

Ball3D::Ball3D(const glm::vec3& pos, float r, float m)
    : position(pos), velocity(0.0f), accumulatedForce(0.0f), radius(r), mass(m) {}

void Ball3D::applyForce(const glm::vec3& force) {
    accumulatedForce += force;
}

void Ball3D::setVelocity(const glm::vec3& vel) {
    velocity = vel;
}

void Ball3D::setPosition(const glm::vec3& pos) {
    position = pos;
}

void Ball3D::setMass(float m) {
    mass = m;
}

const glm::vec3& Ball3D::getPosition() const {
    return position;
}

const glm::vec3& Ball3D::getVelocity() const {
    return velocity;
}

float Ball3D::getRadius() const {
    return radius;
}

float Ball3D::getMass() const {
    return mass;
}

void Ball3D::update(float dt) {
    if (mass <= 0.0f) return;

    glm::vec3 acceleration = accumulatedForce / mass;
    velocity += acceleration * dt;
    position += velocity * dt;

    accumulatedForce = glm::vec3(0.0f); // Clear after each frame
}

void Ball3D::resolveGroundCollision(float groundY, float restitution) {
    if (position.y - radius < groundY) {
        position.y = groundY + radius;
        velocity.y = -velocity.y * restitution;
    }
}

} // namespace engine::objects
