#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "Particle3D.hpp"
#include "Spring3D.hpp"
#include "VertletSystem3D.hpp"

namespace engine::physics {

class ClothSolver3D {
public:
    ClothSolver3D(VertletSystem3D& system, int width, int height, float particleDistance, float stiffnessStructural = 1.0f, float stiffnessShear = 0.8f, float stiffnessBend = 0.5f);

    void createCloth(const glm::vec3& origin, const glm::vec3& rightDir, const glm::vec3& downDir);

    const std::vector<std::shared_ptr<Particle3D>>& getParticles() const;
    const std::vector<std::shared_ptr<Spring3D>>& getSprings() const;

private:
    int width;
    int height;
    float particleDistance;

    float stiffnessStructural;
    float stiffnessShear;
    float stiffnessBend;

    VertletSystem3D& system;

    std::vector<std::shared_ptr<Particle3D>> particles;
    std::vector<std::shared_ptr<Spring3D>> springs;

    std::shared_ptr<Particle3D> getParticle(int x, int y) const;
    void connectParticles(int x1, int y1, int x2, int y2, float stiffness);
};

} // namespace engine::physics
