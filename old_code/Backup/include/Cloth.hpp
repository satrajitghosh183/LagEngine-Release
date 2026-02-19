#pragma once
#include <vector>
#include "Particle.hpp"
#include <SFML/Graphics.hpp>

// A constraint connecting two particles.
struct Constraint {
    int p1;           // Index of the first particle.
    int p2;           // Index of the second particle.
    float restLength; // Resting distance between the two particles.
};

class Cloth {
public:
    std::vector<Particle> particles;
    std::vector<Constraint> constraints;

    int clothWidth;
    int clothHeight;
    float spacing;

    // Constructor: creates a cloth grid starting at 'origin' with given spacing.
    Cloth(int width, int height, float spacing, const sf::Vector2f& origin);

    // Update function applies gravity (or any base acceleration) and relaxes constraints.
    void update(float dt, const sf::Vector2f& gravity, int iterations);

    // Applies a constant wind force to each unlocked particle.
    void applyWind(const sf::Vector2f& wind, float dt);

    // Draw the cloth.
    void draw(sf::RenderWindow& window);
};
