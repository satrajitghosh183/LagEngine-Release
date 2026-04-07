#include "Spring3D.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>

namespace engine::physics {

Spring3D::Spring3D(std::shared_ptr<Particle3D> p1, std::shared_ptr<Particle3D> p2, float k, float rest)
    : particleA(std::move(p1)), particleB(std::move(p2)), stiffness(k) {

    if (rest > 0.0f) {
        restLength = rest;
    } else {
        restLength = glm::length(particleA->getPosition() - particleB->getPosition());
    }
}

void Spring3D::solve() {
    if (particleA->isPinned() && particleB->isPinned()) return;

    glm::vec3 delta = particleB->getPosition() - particleA->getPosition();
    float currentLength = glm::length(delta);

    if (glm::epsilonEqual(currentLength, 0.0f, 1e-6f)) return;

    glm::vec3 direction = delta / currentLength;
    float displacement = currentLength - restLength;

    // Mass-weighted solve
    float invMassA = particleA->isPinned() ? 0.0f : (1.0f / particleA->getMass());
    float invMassB = particleB->isPinned() ? 0.0f : (1.0f / particleB->getMass());
    float invMassSum = invMassA + invMassB;

    if (invMassSum == 0.0f) return;

    glm::vec3 correction = direction * (stiffness * displacement / invMassSum);

    if (!particleA->isPinned())
        particleA->setPosition(particleA->getPosition() + correction * invMassA);

    if (!particleB->isPinned())
        particleB->setPosition(particleB->getPosition() - correction * invMassB);
}

float Spring3D::getCurrentLength() const {
    return glm::length(particleA->getPosition() - particleB->getPosition());
}

} // namespace engine::physics
