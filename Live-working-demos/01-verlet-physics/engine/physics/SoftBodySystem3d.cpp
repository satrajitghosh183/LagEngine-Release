#include "SoftBodySystem3D.hpp"

namespace engine::physics {

void SoftBodySystem3D::addParticle(const std::shared_ptr<Particle3D>& particle) {
    particles.push_back(particle);
}

void SoftBodySystem3D::addSpring(const std::shared_ptr<Spring3D>& spring) {
    springs.push_back(spring);
}

void SoftBodySystem3D::applyForceToAll(const glm::vec3& force) {
    for (auto& particle : particles) {
        if (!particle->isPinned()) {
            particle->applyForce(force);
        }
    }
}

void SoftBodySystem3D::simulate(float dt, int solverIterations) {
    // Integrate
    for (auto& particle : particles) {
        particle->integrate(dt);
    }

    // Solve constraints
    for (int i = 0; i < solverIterations; ++i) {
        for (auto& spring : springs) {
            spring->solve();
        }
    }
}

const std::vector<std::shared_ptr<Particle3D>>& SoftBodySystem3D::getParticles() const {
    return particles;
}

const std::vector<std::shared_ptr<Spring3D>>& SoftBodySystem3D::getSprings() const {
    return springs;
}

} // namespace engine::physics
