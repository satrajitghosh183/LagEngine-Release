#include "ClothSolver3D.hpp"

namespace engine::physics {

ClothSolver3D::ClothSolver3D(VertletSystem3D& sys, int w, int h, float dist, float stiffStruct, float stiffShear, float stiffBend)
    : system(sys), width(w), height(h), particleDistance(dist),
      stiffnessStructural(stiffStruct), stiffnessShear(stiffShear), stiffnessBend(stiffBend) {}

void ClothSolver3D::createCloth(const glm::vec3& origin, const glm::vec3& rightDir, const glm::vec3& downDir) {
    particles.clear();
    springs.clear();

    // Create grid of particles
    for (int y = 0; y <= height; ++y) {
        for (int x = 0; x <= width; ++x) {
            glm::vec3 pos = origin + rightDir * (float)x * particleDistance + downDir * (float)y * particleDistance;
            auto particle = std::make_shared<Particle3D>(pos, 1.0f);
            if (y == 0) {
                particle->pin(); // Top row pinned (can later adjust)
            }
            particles.push_back(particle);
            system.addParticle(particle);
        }
    }

    // Connect springs: Structural
    for (int y = 0; y <= height; ++y) {
        for (int x = 0; x <= width; ++x) {
            if (x < width)
                connectParticles(x, y, x + 1, y, stiffnessStructural);
            if (y < height)
                connectParticles(x, y, x, y + 1, stiffnessStructural);

            // Shear springs
            if (x < width && y < height)
                connectParticles(x, y, x + 1, y + 1, stiffnessShear);
            if (x > 0 && y < height)
                connectParticles(x, y, x - 1, y + 1, stiffnessShear);

            // Bend springs (two apart)
            if (x < width - 1)
                connectParticles(x, y, x + 2, y, stiffnessBend);
            if (y < height - 1)
                connectParticles(x, y, x, y + 2, stiffnessBend);
        }
    }
}

std::shared_ptr<Particle3D> ClothSolver3D::getParticle(int x, int y) const {
    return particles[y * (width + 1) + x];
}

void ClothSolver3D::connectParticles(int x1, int y1, int x2, int y2, float stiffness) {
    if (x2 > width || y2 > height || x1 < 0 || y1 < 0) return;
    auto p1 = getParticle(x1, y1);
    auto p2 = getParticle(x2, y2);
    auto spring = std::make_shared<Spring3D>(p1, p2, stiffness);
    springs.push_back(spring);
    system.addSpring(spring);
}

const std::vector<std::shared_ptr<Particle3D>>& ClothSolver3D::getParticles() const {
    return particles;
}

const std::vector<std::shared_ptr<Spring3D>>& ClothSolver3D::getSprings() const {
    return springs;
}

} // namespace engine::physics
