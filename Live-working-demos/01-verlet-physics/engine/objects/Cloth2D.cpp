
#include "engine/objects/Cloth2D.hpp"
#include "engine/objects/Ball2D.hpp"
#include <cmath>
#include <iostream>
namespace engine::objects {

    using namespace physics;

    Cloth2D::Cloth2D(int w, int h, float s, const sf::Vector2f& origin)
        : width(w), height(h), spacing(s) {
        particles.resize(width * height);

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                sf::Vector2f pos = origin + sf::Vector2f(x * spacing, y * spacing);
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

    void Cloth2D::update(float dt, const sf::Vector2f& gravity, int iterations) {
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

    void Cloth2D::render(sf::RenderWindow& window) {
        for (auto& c : constraints) {
            const auto& p1 = particles[c.p1Index].pos;
            const auto& p2 = particles[c.p2Index].pos;
            sf::Vertex line[] = { sf::Vertex(p1), sf::Vertex(p2) };
            window.draw(line, 2, sf::Lines);
        }
    }

    void Cloth2D::setProjectiles(std::vector<Ball2D*>* proj) { // ✅ Fix: use pointer
        projectiles = proj;
    }

//     void Cloth2D::checkCollisionAndTear() {
//         if (!projectiles) return;

//         for (auto* ball : *projectiles) {
//             sf::Vector2f ballPos = ball->particle.pos;
//             float radius = ball->radius;

//             constraints.erase(std::remove_if(constraints.begin(), constraints.end(),
//                 [&](const Constraint2D& c) {
//                     const auto& p1 = particles[c.p1Index].pos;
//                     const auto& p2 = particles[c.p2Index].pos;
//                     return (std::hypot(p1.x - ballPos.x, p1.y - ballPos.y) < radius) ||
//                            (std::hypot(p2.x - ballPos.x, p2.y - ballPos.y) < radius);
//                 }),
//                 constraints.end());
//         }
//     }

// }
void Cloth2D::checkCollisionAndTear() {
    if (!projectiles) return;

    for (auto* ball : *projectiles) {
        if (!ball->active || !ball->visible) continue;
        
        sf::Vector2f ballPos = ball->particle.pos;
        float radius = ball->radius;
        
        // Calculate ball velocity for impact force
        sf::Vector2f ballVel = ball->particle.pos - ball->particle.oldPos;
        float speed = std::sqrt(ballVel.x * ballVel.x + ballVel.y * ballVel.y);
        
        // Debugging output
        bool hitCloth = false;

        // Check for collisions with cloth points
        for (auto& p : particles) {
            float dist = std::hypot(p.pos.x - ballPos.x, p.pos.y - ballPos.y);
            if (dist < radius + 5.0f) {  // 5.0f is a small buffer
                // Push the point
                sf::Vector2f dir = p.pos - ballPos;
                // Normalize direction
                float len = std::hypot(dir.x, dir.y);
                if (len > 0) {
                    dir.x /= len;
                    dir.y /= len;
                    
                    // Push with force proportional to ball speed
                    float pushForce = std::min(speed * 0.8f, 20.0f);
                    
                    if (!p.locked) {
                        p.pos += dir * pushForce;
                    }
                    
                    hitCloth = true;
                }
            }
        }

        // Check for constraint breaking
        constraints.erase(std::remove_if(constraints.begin(), constraints.end(),
            [&](const Constraint2D& c) {
                const auto& p1 = particles[c.p1Index].pos;
                const auto& p2 = particles[c.p2Index].pos;
                
                // Break if either point is too close to the ball
                bool breakConstraint = (std::hypot(p1.x - ballPos.x, p1.y - ballPos.y) < radius + 2.0f) ||
                                     (std::hypot(p2.x - ballPos.x, p2.y - ballPos.y) < radius + 2.0f);
                
                // Also check if the constraint is stretched too much
                float currentLength = std::hypot(p1.x - p2.x, p1.y - p2.y);
                bool tooStretched = (currentLength > c.restLength * 1.5f);
                
                return breakConstraint || (tooStretched && speed > 15.0f);
            }),
            constraints.end());
            
        if (hitCloth) {
            // Slow down the ball when it hits cloth
            sf::Vector2f ballVelocity = ball->particle.pos - ball->particle.oldPos;
            ball->particle.oldPos = ball->particle.pos - (ballVelocity * 0.8f);
            
            // Debugging
            std::cout << "Ball hit cloth! Speed before: " << speed << std::endl;
        }
    }
}
}