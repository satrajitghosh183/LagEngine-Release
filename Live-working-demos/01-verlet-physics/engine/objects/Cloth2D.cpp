#include "engine/objects/Cloth2D.hpp"
#include "engine/objects/Ball2D.hpp"
#include <cmath>
#include <iostream>
#include <algorithm>

namespace engine::objects {

    using namespace physics;

    Cloth2D::Cloth2D(int w, int h, float s, const glm::vec2& origin)
        : width(w), height(h), spacing(s) {
        particles.resize(width * height);

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                glm::vec2 pos = origin + glm::vec2(x * spacing, y * spacing);
                bool locked = (y == 0 && (x % 5 == 0));
                particles[y * width + x] = Particle(pos, {0, 0});
                particles[y * width + x].locked = locked;

                if (x > 0)
                    constraints.emplace_back((y * width + x - 1), (y * width + x), spacing);
                if (y > 0)
                    constraints.emplace_back(((y - 1) * width + x), (y * width + x), spacing);
            }
        }
    }

    void Cloth2D::update(float dt, const glm::vec2& gravity, int iterations) {
        for (auto& p : particles) p.update(dt, gravity);
        for (int i = 0; i < iterations; ++i)
            for (auto& c : constraints) c.satisfy(particles);

        if (projectiles) {
            checkCollisionAndTear();
        }
    }

    void Cloth2D::update(float dt) {
        update(dt, {0.f, 980.f}, 5);
    }

    std::vector<scene::Object2D::Vertex2D> Cloth2D::getLineVertices() const {
        std::vector<Vertex2D> verts;
        verts.reserve(constraints.size() * 2);

        glm::vec3 white = {1.0f, 1.0f, 1.0f};
        for (auto& c : constraints) {
            if (!c.active) continue;
            const auto& p1 = particles[c.p1Index].pos;
            const auto& p2 = particles[c.p2Index].pos;
            verts.push_back({{p1.x, p1.y, 0.0f}, white});
            verts.push_back({{p2.x, p2.y, 0.0f}, white});
        }

        return verts;
    }

    void Cloth2D::setProjectiles(std::vector<Ball2D*>* proj) {
        projectiles = proj;
    }

    void Cloth2D::checkCollisionAndTear() {
        if (!projectiles) return;

        for (auto* ball : *projectiles) {
            if (!ball->active || !ball->visible) continue;

            glm::vec2 ballPos = ball->particle.pos;
            float radius = ball->radius;

            glm::vec2 ballVel = ball->particle.pos - ball->particle.oldPos;
            float speed = glm::length(ballVel);

            bool hitCloth = false;

            for (auto& p : particles) {
                float dist = glm::length(p.pos - ballPos);
                if (dist < radius + 5.0f) {
                    glm::vec2 dir = p.pos - ballPos;
                    float len = glm::length(dir);
                    if (len > 0) {
                        dir /= len;

                        float pushForce = std::min(speed * 0.8f, 20.0f);

                        if (!p.locked) {
                            p.pos += dir * pushForce;
                        }

                        hitCloth = true;
                    }
                }
            }

            constraints.erase(std::remove_if(constraints.begin(), constraints.end(),
                [&](const Constraint2D& c) {
                    const auto& p1 = particles[c.p1Index].pos;
                    const auto& p2 = particles[c.p2Index].pos;

                    bool breakConstraint = (glm::length(p1 - ballPos) < radius + 2.0f) ||
                                         (glm::length(p2 - ballPos) < radius + 2.0f);

                    float currentLength = glm::length(p1 - p2);
                    bool tooStretched = (currentLength > c.restLength * 1.5f);

                    return breakConstraint || (tooStretched && speed > 15.0f);
                }),
                constraints.end());

            if (hitCloth) {
                glm::vec2 ballVelocity = ball->particle.pos - ball->particle.oldPos;
                ball->particle.oldPos = ball->particle.pos - (ballVelocity * 0.8f);

                std::cout << "Ball hit cloth! Speed before: " << speed << std::endl;
            }
        }
    }

}
