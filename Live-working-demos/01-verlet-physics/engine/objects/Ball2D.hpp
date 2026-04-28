// Ball2D.hpp
#pragma once

#include <glm/glm.hpp>
#include "engine/physics/Particle.hpp"
#include "engine/scene/Object2D.hpp"

namespace engine::objects {

    class Ball2D : public engine::scene::Object2D {
    public:
        engine::physics::Particle particle;
        float radius;
        float restitution;
        glm::vec2 gravity = {0.f, 980.f};
        glm::vec3 color = {1.0f, 0.0f, 0.0f}; // Default red

        int stationaryFrames = 0;

        Ball2D(const glm::vec2& position,
               const glm::vec2& velocity,
               float r,
               float rest = 0.8f,
               const glm::vec3& ballColor = {1.0f, 0.0f, 0.0f});

        void update(float dt) override;

        std::vector<Vertex2D> getTriangleVertices() const override;
    };
}
