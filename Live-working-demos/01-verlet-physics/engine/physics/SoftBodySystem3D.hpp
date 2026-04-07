#pragma once

#include <vector>
#include <memory>
#include "Particle3D.hpp"
#include "Spring3D.hpp"

namespace engine::physics {

class SoftBodySystem3D {
public:
    SoftBodySystem3D() = default;

    void addParticle(const std::shared_ptr<Particle3D>& particle);
    void addSpring(const std::shared_ptr<Spring3D>& spring);

    void applyForceToAll(const glm::vec3& force);
    void simulate(float dt, int solverIterations = 5);

    const std::vector<std::shared_ptr<Particle3D>>& getParticles() const;
    const std::vector<std::shared_ptr<Spring3D>>& getSprings() const;

private:
    std::vector<std::shared_ptr<Particle3D>> particles;
    std::vector<std::shared_ptr<Spring3D>> springs;
};

} // namespace engine::physics
