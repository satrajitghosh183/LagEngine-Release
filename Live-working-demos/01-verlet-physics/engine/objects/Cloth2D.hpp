#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "engine/scene/Object2D.hpp"
#include "engine/physics/Particle.hpp"
#include "engine/physics/Constraint2D.hpp"

namespace engine::objects {

    class Ball2D;

    class Cloth2D : public scene::Object2D {
    public:
        int width, height;
        float spacing;
        std::vector<physics::Particle> particles;
        std::vector<physics::Constraint2D> constraints;

        Cloth2D(int w, int h, float s, const glm::vec2& origin);

        void update(float dt) override;

        void update(float dt, const glm::vec2& gravity, int iterations);

        void setProjectiles(std::vector<Ball2D*>* projectiles);

        std::vector<Vertex2D> getLineVertices() const override;

    private:
        std::vector<Ball2D*>* projectiles = nullptr;
        void checkCollisionAndTear();
    };

}
