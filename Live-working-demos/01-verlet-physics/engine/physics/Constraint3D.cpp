
#include "Constraint3D.hpp"
#include <glm/glm.hpp>

namespace engine::physics {

Constraint3D::Constraint3D(std::shared_ptr<Particle3D> p1, std::shared_ptr<Particle3D> p2, float distance)
    : particleA(std::move(p1)), particleB(std::move(p2)), restDistance(distance) {}

void Constraint3D::enforce() const {
    if (particleA->isPinned() && particleB->isPinned()) return;

    glm::vec3 delta = particleB->getPosition() - particleA->getPosition();
    float currentDistance = glm::length(delta);

    if (currentDistance == 0.0f) return;

    glm::vec3 direction = delta / currentDistance;
    float displacement = currentDistance - restDistance;

    float invMassA = particleA->isPinned() ? 0.0f : (1.0f / particleA->getMass());
    float invMassB = particleB->isPinned() ? 0.0f : (1.0f / particleB->getMass());
    float invMassSum = invMassA + invMassB;

    if (invMassSum == 0.0f) return;

    glm::vec3 correction = direction * (displacement / invMassSum);

    if (!particleA->isPinned())
        particleA->setPosition(particleA->getPosition() + correction * invMassA);

    if (!particleB->isPinned())
        particleB->setPosition(particleB->getPosition() - correction * invMassB);
}

} // namespace engine::physics
