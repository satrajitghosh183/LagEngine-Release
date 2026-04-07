#include "VertletSystem3D.hpp"

namespace engine::physics {

void VertletSystem3D::addParticle(const std::shared_ptr<Particle3D>& particle) {
    particles.push_back(particle);
}

void VertletSystem3D::addSpring(const std::shared_ptr<Spring3D>& spring) {
    springs.push_back(spring);
}

void VertletSystem3D::addConstraint(const std::shared_ptr<Constraint3D>& constraint) {
    constraints.push_back(constraint);
}

void VertletSystem3D::update(float dt, const glm::vec3& gravity, int solverIterations) {
    // Step 1: Apply gravity
    for (auto& particle : particles) {
        if (!particle->isPinned())
            particle->applyForce(gravity * particle->getMass());
    }

    // Step 2: Integrate motion
    for (auto& particle : particles) {
        particle->integrate(dt, 0.98f); // Damping coefficient here
    }

    // Step 3: Solve springs and constraints
    for (int i = 0; i < solverIterations; ++i) {
        for (auto& spring : springs) {
            spring->solve();
        }
        for (auto& constraint : constraints) {
            constraint->enforce();
        }
    }
}

const std::vector<std::shared_ptr<Particle3D>>& VertletSystem3D::getParticles() const {
    return particles;
}

const std::vector<std::shared_ptr<Spring3D>>& VertletSystem3D::getSprings() const {
    return springs;
}

const std::vector<std::shared_ptr<Constraint3D>>& VertletSystem3D::getConstraints() const {
    return constraints;
}

} // namespace engine::physics
