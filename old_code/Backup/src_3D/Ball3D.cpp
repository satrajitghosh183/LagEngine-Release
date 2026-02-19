#include "Ball3D.hpp"

Ball3D::Ball3D(const glm::vec3 &position, const glm::vec3 &initialVelocity, float r, float rest)
    : particle(position, initialVelocity), radius(r), restitution(rest) {}

void Ball3D::update(float dt, const glm::vec3 &acceleration, const glm::vec3 &minBounds, const glm::vec3 &maxBounds) {
    particle.update(dt, acceleration);
    // X-axis collisions.
    if (particle.pos.x - radius < minBounds.x) {
        particle.pos.x = minBounds.x + radius;
        particle.oldPos.x = particle.pos.x + (particle.pos.x - particle.oldPos.x) * -restitution;
    }
    if (particle.pos.x + radius > maxBounds.x) {
        particle.pos.x = maxBounds.x - radius;
        particle.oldPos.x = particle.pos.x + (particle.pos.x - particle.oldPos.x) * -restitution;
    }
    // Y-axis collisions.
    if (particle.pos.y - radius < minBounds.y) {
        particle.pos.y = minBounds.y + radius;
        particle.oldPos.y = particle.pos.y + (particle.pos.y - particle.oldPos.y) * -restitution;
    }
    if (particle.pos.y + radius > maxBounds.y) {
        particle.pos.y = maxBounds.y - radius;
        particle.oldPos.y = particle.pos.y + (particle.pos.y - particle.oldPos.y) * -restitution;
    }
    // Z-axis collisions.
    if (particle.pos.z - radius < minBounds.z) {
        particle.pos.z = minBounds.z + radius;
        particle.oldPos.z = particle.pos.z + (particle.pos.z - particle.oldPos.z) * -restitution;
    }
    if (particle.pos.z + radius > maxBounds.z) {
        particle.pos.z = maxBounds.z - radius;
        particle.oldPos.z = particle.pos.z + (particle.pos.z - particle.oldPos.z) * -restitution;
    }
}
