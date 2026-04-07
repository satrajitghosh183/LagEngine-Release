#pragma once
/**
 * Physics2D.hpp
 *
 * 2D cloth/ball/cannon physics from old_code/engine/ — converted from
 * SFML types to GLM so it compiles inside GameEngine.
 *
 * Mechanical type substitution only:
 *   sf::Vector2f  → glm::vec2
 *   sf::Vector2f(0,0) → glm::vec2(0.f)
 * All physics equations (Verlet integration, constraint satisfaction,
 * collision detection, cannon firing) are 1:1 from old_code/engine.
 *
 * Reference files:
 *   old_code/engine/physics/Particle.hpp / .cpp
 *   old_code/engine/physics/Constraint2D.hpp / .cpp
 *   old_code/engine/objects/Cloth2D.hpp / .cpp
 *   old_code/engine/objects/Ball2D.hpp / .cpp
 *   old_code/engine/objects/Cannon2D.hpp / .cpp
 */

#include <vector>
#include <list>
#include <cmath>
#include <algorithm>
#include <glm/glm.hpp>

namespace p2d {

// ---------------------------------------------------------------------------
// Particle — mirrors engine/physics/Particle.hpp (sf::Vector2f → glm::vec2)
// ---------------------------------------------------------------------------
struct Particle {
    glm::vec2 pos    = glm::vec2(0.f);
    glm::vec2 oldPos = glm::vec2(0.f);
    bool locked      = false;

    Particle() = default;
    Particle(const glm::vec2& position)
        : pos(position), oldPos(position) {}
    Particle(const glm::vec2& position, const glm::vec2& velocity)
        : pos(position), oldPos(position - velocity) {}

    // Verlet integration — 1:1 from Particle::update() in old_code/engine
    void update(float dt, const glm::vec2& acceleration) {
        if (locked) return;
        glm::vec2 velocity = pos - oldPos;
        oldPos = pos;
        pos += velocity + acceleration * (dt * dt);
    }

    glm::vec2 getVelocity() const { return pos - oldPos; }
};

// ---------------------------------------------------------------------------
// Constraint2D — mirrors engine/physics/Constraint2D.hpp
// ---------------------------------------------------------------------------
struct Constraint2D {
    int   p1Index;
    int   p2Index;
    float restLength;
    float stiffness = 1.0f;
    bool  active    = true;

    Constraint2D(int i1, int i2, float length, float stiff = 1.0f)
        : p1Index(i1), p2Index(i2), restLength(length), stiffness(stiff) {}

    // Constraint satisfaction — 1:1 from Constraint2D::satisfy() in old_code/engine
    void satisfy(std::vector<Particle>& particles) {
        if (!active) return;
        auto& p1 = particles[p1Index];
        auto& p2 = particles[p2Index];

        glm::vec2 delta = p2.pos - p1.pos;
        float dist = glm::length(delta);
        if (dist < 1e-6f) return;

        float diff = (dist - restLength) / dist * stiffness;
        glm::vec2 correction = delta * (0.5f * diff);

        if (!p1.locked) p1.pos += correction;
        if (!p2.locked) p2.pos -= correction;
    }
};

// ---------------------------------------------------------------------------
// Ball2D — mirrors engine/objects/Ball2D.hpp
// ---------------------------------------------------------------------------
struct Ball2D {
    Particle    particle;
    float       radius;
    glm::vec3   color;
    bool        active  = true;
    bool        visible = true;

    Ball2D(const glm::vec2& pos, const glm::vec2& velocity, float r,
           float restitution = 0.8f, const glm::vec3& col = glm::vec3(1.f))
        : particle(pos, velocity), radius(r), color(col), m_Restitution(restitution) {}

    // Update — 1:1 from Ball2D::update() in old_code/engine
    void update(float dt, const glm::vec2& gravity, float worldHeight) {
        if (!active) return;
        particle.update(dt, gravity);

        // Floor bounce
        if (particle.pos.y + radius > worldHeight) {
            particle.pos.y = worldHeight - radius;
            glm::vec2 vel = particle.getVelocity();
            particle.oldPos.y = particle.pos.y + vel.y * m_Restitution;
        }

        // Fade out off-screen
        if (particle.pos.y > worldHeight + 200.f ||
            particle.pos.x < -500.f || particle.pos.x > 2000.f) {
            active = false;
        }
    }

    float m_Restitution;
};

// ---------------------------------------------------------------------------
// Cannon2D — mirrors engine/objects/Cannon2D.hpp
// ---------------------------------------------------------------------------
struct Cannon2D {
    glm::vec2 position;
    float     angle       = -45.0f; // degrees, -45 = upper-right
    float     power       = 300.0f; // initial velocity magnitude
    float     barrelLength = 50.0f;

    Cannon2D(const glm::vec2& pos) : position(pos) {}

    // 1:1 from Cannon2D::getMuzzlePosition()
    glm::vec2 getMuzzlePosition() const {
        float rad = glm::radians(angle);
        return position + glm::vec2(std::cos(rad), std::sin(rad)) * barrelLength;
    }

    // 1:1 from Cannon2D::getFiringVelocity()
    glm::vec2 getFiringVelocity() const {
        float rad = glm::radians(angle);
        return glm::vec2(std::cos(rad), std::sin(rad)) * power;
    }

    void rotate(float degrees) { angle += degrees; }
    void adjustPower(float delta) { power = std::max(50.0f, power + delta); }
};

// ---------------------------------------------------------------------------
// Cloth2D — mirrors engine/objects/Cloth2D.hpp/.cpp
// ---------------------------------------------------------------------------
class Cloth2D {
public:
    std::vector<Particle>     particles;
    std::vector<Constraint2D> constraints;
    std::list<Ball2D*>*       projectiles = nullptr;
    bool                      active = true;

    Cloth2D(int w, int h, float spacing, const glm::vec2& origin)
        : width(w), height(h), spacing(spacing)
    {
        particles.resize(w * h);
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                glm::vec2 pos = origin + glm::vec2(x * spacing, y * spacing);
                bool locked = (y == 0 && (x % 5 == 0));
                particles[y * w + x] = Particle(pos, glm::vec2(0.f));
                particles[y * w + x].locked = locked;

                if (x > 0) constraints.emplace_back(y*w + x-1, y*w + x, spacing);
                if (y > 0) constraints.emplace_back((y-1)*w + x, y*w + x, spacing);
            }
        }
    }

    // update — 1:1 from Cloth2D::update() in old_code/engine
    void update(float dt, const glm::vec2& gravity = glm::vec2(0.f, 980.f),
                int iterations = 5)
    {
        for (auto& p : particles) p.update(dt, gravity);
        for (int i = 0; i < iterations; ++i)
            for (auto& c : constraints) c.satisfy(particles);

        if (projectiles) checkCollisionAndTear();
    }

    void setProjectiles(std::list<Ball2D*>* proj) { projectiles = proj; }

private:
    int   width, height;
    float spacing;

    // checkCollisionAndTear — 1:1 from old_code/engine Cloth2D::checkCollisionAndTear()
    void checkCollisionAndTear() {
        if (!projectiles) return;
        for (auto* ball : *projectiles) {
            if (!ball->active || !ball->visible) continue;

            glm::vec2 ballPos = ball->particle.pos;
            float     radius  = ball->radius;
            glm::vec2 ballVel = ball->particle.pos - ball->particle.oldPos;
            float     speed   = glm::length(ballVel);

            bool hitCloth = false;
            for (auto& p : particles) {
                float dist = glm::length(p.pos - ballPos);
                if (dist < radius + 5.0f) {
                    glm::vec2 dir = p.pos - ballPos;
                    float len = glm::length(dir);
                    if (len > 0.f) {
                        dir /= len;
                        float pushForce = std::min(speed * 0.8f, 20.0f);
                        if (!p.locked) p.pos += dir * pushForce;
                        hitCloth = true;
                    }
                }
            }

            constraints.erase(
                std::remove_if(constraints.begin(), constraints.end(),
                    [&](const Constraint2D& c) {
                        const auto& p1 = particles[c.p1Index].pos;
                        const auto& p2 = particles[c.p2Index].pos;
                        bool breakC = (glm::length(p1 - ballPos) < radius + 2.0f) ||
                                      (glm::length(p2 - ballPos) < radius + 2.0f);
                        float curLen = glm::length(p1 - p2);
                        bool  torn   = (curLen > c.restLength * 1.5f) && (speed > 15.f);
                        return breakC || torn;
                    }),
                constraints.end());

            if (hitCloth) {
                glm::vec2 bv = ball->particle.pos - ball->particle.oldPos;
                ball->particle.oldPos = ball->particle.pos - bv * 0.8f;
            }
        }
    }
};

} // namespace p2d
