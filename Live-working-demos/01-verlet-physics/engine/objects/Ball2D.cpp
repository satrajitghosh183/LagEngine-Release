// Ball2D.cpp
#include "engine/objects/Ball2D.hpp"
#include <iostream>
#include <cmath>

namespace engine::objects {

    Ball2D::Ball2D(const glm::vec2& position,
                   const glm::vec2& velocity,
                   float r,
                   float rest,
                   const glm::vec3& ballColor)
        : particle(position, velocity), radius(r), restitution(rest), color(ballColor) {
        active = true;
        visible = true;

        std::cout << "Ball created at: (" << position.x << ", " << position.y << ")" << std::endl;
        std::cout << "With velocity: (" << velocity.x << ", " << velocity.y << ")" << std::endl;
    }

    void Ball2D::update(float dt) {
        if (!active) return;

        glm::vec2 adjustedGravity = gravity * 0.5f;

        const int substeps = 3;
        float subDt = dt / substeps;

        for (int i = 0; i < substeps; i++) {
            particle.update(subDt, adjustedGravity);

            // Ground collision
            if (particle.pos.y > 720 - radius) {
                particle.pos.y = 720 - radius;
                glm::vec2 velocity = particle.pos - particle.oldPos;
                velocity.y = -velocity.y * restitution;
                velocity.x *= 0.95f;
                particle.oldPos = particle.pos - velocity;
            }

            // Wall collisions
            if (particle.pos.x < radius) {
                particle.pos.x = radius;
                glm::vec2 velocity = particle.pos - particle.oldPos;
                velocity.x = -velocity.x * restitution;
                particle.oldPos = particle.pos - velocity;
            }

            if (particle.pos.x > 1280 - radius) {
                particle.pos.x = 1280 - radius;
                glm::vec2 velocity = particle.pos - particle.oldPos;
                velocity.x = -velocity.x * restitution;
                particle.oldPos = particle.pos - velocity;
            }
        }

        // Force the ball to stay on screen
        if (particle.pos.y < radius) particle.pos.y = radius;
        if (particle.pos.y > 720 - radius) particle.pos.y = 720 - radius;
        if (particle.pos.x < radius) particle.pos.x = radius;
        if (particle.pos.x > 1280 - radius) particle.pos.x = 1280 - radius;

        // Check for stationary ball
        glm::vec2 vel = particle.pos - particle.oldPos;
        float speed = glm::length(vel);
        bool onGround = (particle.pos.y > 720 - radius - 1.0f);

        if (onGround && speed < 1.0f) {
            stationaryFrames++;
            if (stationaryFrames > 60) {
                active = false;
                visible = false;
                std::cout << "Ball deactivated due to being stationary" << std::endl;
            }
        } else {
            stationaryFrames = 0;
        }

        // Deactivate if the ball goes way off screen
        if (particle.pos.y > 2000 || particle.pos.x < -1000 || particle.pos.x > 2000) {
            active = false;
            visible = false;
            std::cout << "Ball went out of bounds, deactivated" << std::endl;
        }
    }

    std::vector<scene::Object2D::Vertex2D> Ball2D::getTriangleVertices() const {
        if (!visible) return {};

        // Approximate circle with triangle fan
        const int segments = 24;
        std::vector<Vertex2D> verts;
        verts.reserve(segments * 3);

        glm::vec2 center = particle.pos;
        for (int i = 0; i < segments; ++i) {
            float a0 = (float)i / segments * 2.0f * 3.14159265f;
            float a1 = (float)(i + 1) / segments * 2.0f * 3.14159265f;

            verts.push_back({{center.x, center.y, 0.0f}, color});
            verts.push_back({{center.x + radius * std::cos(a0), center.y + radius * std::sin(a0), 0.0f}, color});
            verts.push_back({{center.x + radius * std::cos(a1), center.y + radius * std::sin(a1), 0.0f}, color});
        }

        // Inner dot (yellow)
        float innerR = radius * 0.5f;
        glm::vec3 yellow = {1.0f, 1.0f, 0.0f};
        for (int i = 0; i < segments; ++i) {
            float a0 = (float)i / segments * 2.0f * 3.14159265f;
            float a1 = (float)(i + 1) / segments * 2.0f * 3.14159265f;

            verts.push_back({{center.x, center.y, 0.0f}, yellow});
            verts.push_back({{center.x + innerR * std::cos(a0), center.y + innerR * std::sin(a0), 0.0f}, yellow});
            verts.push_back({{center.x + innerR * std::cos(a1), center.y + innerR * std::sin(a1), 0.0f}, yellow});
        }

        return verts;
    }

}
