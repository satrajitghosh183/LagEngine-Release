#include <SFML/Graphics.hpp>
#include "Cloth.hpp"
#include "Ball.hpp"
#include "math.hpp"
#include "number_generator.hpp"  // For RNG functionality.
#include <vector>

int main() {
    sf::RenderWindow window(sf::VideoMode(800, 600), "Cloth Simulation with Bouncing Balls");
    window.setFramerateLimit(60);

    // Create the cloth.
    int gridWidth = 30;
    int gridHeight = 20;
    float spacing = 10.f;
    sf::Vector2f clothOrigin(100.f, 50.f);
    Cloth cloth(gridWidth, gridHeight, spacing, clothOrigin);

    // Create a number of bouncing balls.
    std::vector<Ball> balls;
    const int numBalls = 50; // Adjust for a "legendary" number.
    for (int i = 0; i < numBalls; ++i) {
        float x = RNG::RNGf::getRange(300.f, 750.f);
        float y = RNG::RNGf::getRange(300.f, 550.f);
        sf::Vector2f pos(x, y);
        float angle = RNG::RNGf::getRange(0.f, 2 * Math::PI);
        float speed = RNG::RNGf::getRange(50.f, 200.f);
        sf::Vector2f vel(std::cos(angle) * speed, std::sin(angle) * speed);
        float radius = RNG::RNGf::getRange(5.f, 15.f);
        balls.emplace_back(pos, vel, radius);
    }

    // Gravity.
   // Gravity.
sf::Vector2f gravity(0.f, 500.f);
sf::Vector2f wind(50.f, 0.f);  // Constant horizontal wind blowing to the right.
sf::Clock clock;

while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
        if(event.type == sf::Event::Closed)
            window.close();
    }
    float dt = clock.restart().asSeconds();

    // Update the cloth with gravity.
    cloth.update(dt, gravity, 20);
    // Apply constant wind force.
    cloth.applyWind(wind, dt);

    // Update the balls.
    for (auto &ball : balls) {
        ball.update(dt, gravity, window);
    }

    // Handle collisions between balls and cloth particles.
    float particleRadius = 3.f;
    for (auto &ball : balls) {
        for (auto &particle : cloth.particles) {
            sf::Vector2f diff = particle.pos - ball.particle.pos;
            float dist = Math::length(diff);
            float minDist = ball.radius + particleRadius;
            if (dist < minDist && dist > 0.f) {
                float penetration = minDist - dist;
                sf::Vector2f correction = Math::normalize(diff) * penetration;
                if (!particle.locked) {
                    particle.pos += correction;
                }
            }
        }
    }

    window.clear(sf::Color::Black);
    cloth.draw(window);
    for (const auto &ball : balls) {
        ball.draw(window);
    }
    window.display();
}

    return 0;
}