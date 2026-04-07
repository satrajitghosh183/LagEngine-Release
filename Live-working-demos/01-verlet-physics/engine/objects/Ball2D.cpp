
// // engine/objects/Ball2D.cpp
// #include "engine/objects/Ball2D.hpp"

// namespace engine::objects {

//     Ball2D::Ball2D(const sf::Vector2f& position,
//                    const sf::Vector2f& velocity,
//                    float r,
//                    float rest)
//         : particle(position, velocity), radius(r), restitution(rest) {}

//     void Ball2D::update(float dt) {
//         if (!active) return;

//         particle.update(dt, gravity);

//         // Soft delete if out of bounds
//         if (particle.pos.y > 1000 || particle.pos.x < -100 || particle.pos.x > 1400) {
//             active = false;
//             visible = false;
//         }
//     }

//     void Ball2D::render(sf::RenderWindow& window) {
//         if (!visible) return;
//         sf::CircleShape shape(radius);
//         shape.setOrigin(radius, radius);
//         shape.setPosition(particle.pos);
//         shape.setFillColor(sf::Color::White);
//         window.draw(shape);
//     }

// }
// Ball2D.cpp
#include "engine/objects/Ball2D.hpp"
#include <iostream>
#include <cmath>

namespace engine::objects {

    Ball2D::Ball2D(const sf::Vector2f& position,
                   const sf::Vector2f& velocity,
                   float r,
                   float rest,
                   const sf::Color& ballColor)
        : particle(position, velocity), radius(r), restitution(rest), color(ballColor) {
        
        // Explicitly set these to ensure they're true
        active = true;
        visible = true;
        
        // Debug output at creation
        std::cout << "Ball created at: (" << position.x << ", " << position.y << ")" << std::endl;
        std::cout << "With velocity: (" << velocity.x << ", " << velocity.y << ")" << std::endl;
    }

    void Ball2D::update(float dt) {
        if (!active) return;

        // Debug output of current position and velocity
        sf::Vector2f vel = particle.pos - particle.oldPos;
        std::cout << "Ball position before update: (" << particle.pos.x << ", " << particle.pos.y 
                  << "), velocity: (" << vel.x << ", " << vel.y << ")" << std::endl;

        // Reduce the gravity to make the balls fall slower
        sf::Vector2f adjustedGravity = gravity * 0.5f;

        // Apply physics with smaller timestep
        const int substeps = 3;
        float subDt = dt / substeps;
        
        for (int i = 0; i < substeps; i++) {
            // Update particle position
            particle.update(subDt, adjustedGravity);
            
            // Handle boundary collisions with more visibility
            // Ground collision
            if (particle.pos.y > 720 - radius) {
                particle.pos.y = 720 - radius;
                sf::Vector2f velocity = particle.pos - particle.oldPos;
                velocity.y = -velocity.y * restitution;
                velocity.x *= 0.95f; // Add friction
                particle.oldPos = particle.pos - velocity;
                std::cout << "Ball hit ground!" << std::endl;
            }
            
            // Wall collisions
            if (particle.pos.x < radius) {
                particle.pos.x = radius;
                sf::Vector2f velocity = particle.pos - particle.oldPos;
                velocity.x = -velocity.x * restitution;
                particle.oldPos = particle.pos - velocity;
                std::cout << "Ball hit left wall!" << std::endl;
            }
            
            if (particle.pos.x > 1280 - radius) {
                particle.pos.x = 1280 - radius;
                sf::Vector2f velocity = particle.pos - particle.oldPos;
                velocity.x = -velocity.x * restitution;
                particle.oldPos = particle.pos - velocity;
                std::cout << "Ball hit right wall!" << std::endl;
            }
        }
        
        // Force the ball to stay on screen
        if (particle.pos.y < radius) particle.pos.y = radius;
        if (particle.pos.y > 720 - radius) particle.pos.y = 720 - radius;
        if (particle.pos.x < radius) particle.pos.x = radius;
        if (particle.pos.x > 1280 - radius) particle.pos.x = 1280 - radius;
        
        // Debug output after update
        vel = particle.pos - particle.oldPos;
        std::cout << "Ball position after update: (" << particle.pos.x << ", " << particle.pos.y 
                  << "), velocity: (" << vel.x << ", " << vel.y << ")" << std::endl;
        
        // Check for stationary ball
        sf::Vector2f velocity = particle.pos - particle.oldPos;
        float speed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
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
        
        // Deactivate if the ball goes way off screen (fail-safe)
        if (particle.pos.y > 2000 || particle.pos.x < -1000 || particle.pos.x > 2000) {
            active = false;
            visible = false;
            std::cout << "Ball went out of bounds, deactivated" << std::endl;
        }
    }

    void Ball2D::render(sf::RenderWindow& window) {
        if (!visible) return;
        
        // Debug output - uncomment if needed
        std::cout << "Rendering ball at position: (" << particle.pos.x << ", " << particle.pos.y << ")" << std::endl;
        
        // Make ball more visible with brighter color
        sf::CircleShape shape(radius);
        shape.setOrigin(radius, radius);
        shape.setPosition(particle.pos);
        
        // Use bright colors that stand out against dark background
        shape.setFillColor(sf::Color(255, 0, 0, 255)); // Bright red
        
        // Add thick white outline
        shape.setOutlineThickness(3.0f);
        shape.setOutlineColor(sf::Color::White);
        
        window.draw(shape);
        
        // Also draw a smaller inner circle in yellow to make it even more visible
        sf::CircleShape innerDot(radius * 0.5f);
        innerDot.setOrigin(radius * 0.5f, radius * 0.5f);
        innerDot.setPosition(particle.pos);
        innerDot.setFillColor(sf::Color::Yellow);
        window.draw(innerDot);
    }
}