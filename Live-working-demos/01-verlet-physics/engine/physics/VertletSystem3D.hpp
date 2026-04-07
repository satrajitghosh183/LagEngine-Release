
#pragma once

#include <vector>
#include <memory>
#include "Particle3D.hpp"
#include "Spring3D.hpp"
#include "Constraint3D.hpp"

namespace engine::physics {

class VertletSystem3D {
public:
    void addParticle(const std::shared_ptr<Particle3D>& particle);
    void addSpring(const std::shared_ptr<Spring3D>& spring);
    void addConstraint(const std::shared_ptr<Constraint3D>& constraint);

    void update(float dt, const glm::vec3& gravity, int solverIterations = 5);

    const std::vector<std::shared_ptr<Particle3D>>& getParticles() const;
    const std::vector<std::shared_ptr<Spring3D>>& getSprings() const;
    const std::vector<std::shared_ptr<Constraint3D>>& getConstraints() const;

private:
    std::vector<std::shared_ptr<Particle3D>> particles;
    std::vector<std::shared_ptr<Spring3D>> springs;
    std::vector<std::shared_ptr<Constraint3D>> constraints;
};

} // namespace engine::physics
