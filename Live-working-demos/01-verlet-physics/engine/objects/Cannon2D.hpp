// Cannon2D.hpp
#pragma once

#include <glm/glm.hpp>
#include "engine/scene/Object2D.hpp"

namespace engine::objects {

    class Cannon2D : public engine::scene::Object2D {
    public:
        glm::vec2 position;
        float angle = -45.0f;
        float power = 600.0f;

        Cannon2D(const glm::vec2& pos);

        void rotate(float degrees);

        void update(float dt) override;

        glm::vec2 getFiringVelocity() const;

        glm::vec2 getMuzzlePosition() const;

        void adjustPower(float amount);

        std::vector<Vertex2D> getTriangleVertices() const override;
    };
}
