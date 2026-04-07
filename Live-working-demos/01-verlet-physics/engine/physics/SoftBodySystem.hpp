#pragma once
#include <cstddef>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Particle3D.hpp"
#include "Constraint3D.hpp"

namespace engine::physics {

    // Soft body system (generalized physics blob or deformable object)
    class SoftBodySystem {
    public:
        std::vector<Particle3D> particles;
        std::vector<Constraint3D> constraints;

        void update(float dt, const glm::vec3& acceleration);
        void solveConstraints(int iterations = 10);
        void addParticle(const glm::vec3& position, const glm::vec3& velocity = glm::vec3(0), bool locked = false);
        void connect(int i1, int i2, float restLength);
    };

} // namespace engine::physics

