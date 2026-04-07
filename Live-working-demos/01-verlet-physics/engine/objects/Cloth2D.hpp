#pragma once
#include <SFML/Graphics.hpp>
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

        Cloth2D(int w, int h, float s, const sf::Vector2f& origin);

        void update(float dt) override;
        void render(sf::RenderWindow& window) override;

        void update(float dt, const sf::Vector2f& gravity, int iterations);

        void setProjectiles(std::vector<Ball2D*>* projectiles); // ✅ Fix: pass pointer!

    private:
        std::vector<Ball2D*>* projectiles = nullptr; // ✅ Pointer to live list
        void checkCollisionAndTear();
    };

}
