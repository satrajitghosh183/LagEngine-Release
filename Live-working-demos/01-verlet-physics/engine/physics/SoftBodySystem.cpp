
#include "engine/physics/SoftBodySystem.hpp"

namespace engine::physics {

    void SoftBodySystem::update(float dt, const glm::vec3& acceleration) {
        for (auto& p : particles)
            p.update(dt, acceleration);
    }

    void SoftBodySystem::solveConstraints(int iterations) {
        for (int i = 0; i < iterations; ++i) {
            for (auto& c : constraints)
                c.satisfy(particles);
        }
    }

    void SoftBodySystem::addParticle(const glm::vec3& position, const glm::vec3& velocity, bool locked) {
        Particle3D p(position, velocity);
        p.locked = locked;
        particles.push_back(p);
    }

    void SoftBodySystem::connect(int i1, int i2, float restLength) {
        constraints.emplace_back(i1, i2, restLength);
    }

} // namespace engine::physics
