#include "Cloth.hpp"
#include "math.hpp"
#include <SFML/Graphics.hpp>
#include <cmath>

Cloth::Cloth(int width, int height, float spacing, const sf::Vector2f& origin)
    : clothWidth(width), clothHeight(height), spacing(spacing)
{
    // Create a grid of particles.
    for (int j = 0; j < clothHeight; ++j) {
        for (int i = 0; i < clothWidth; ++i) {
            sf::Vector2f pos(origin.x + i * spacing, origin.y + j * spacing);
            Particle p(pos);
            // Lock the top row to simulate an anchored cloth.
            if (j == 0)
                p.lock();
            particles.push_back(p);
        }
    }

    // Create horizontal constraints.
    for (int j = 0; j < clothHeight; ++j) {
        for (int i = 0; i < clothWidth - 1; ++i) {
            int index = j * clothWidth + i;
            Constraint c;
            c.p1 = index;
            c.p2 = index + 1;
            c.restLength = spacing;
            constraints.push_back(c);
        }
    }

    // Create vertical constraints.
    for (int j = 0; j < clothHeight - 1; ++j) {
        for (int i = 0; i < clothWidth; ++i) {
            int index = j * clothWidth + i;
            Constraint c;
            c.p1 = index;
            c.p2 = index + clothWidth;
            c.restLength = spacing;
            constraints.push_back(c);
        }
    }

    // Create shear constraints (diagonals).
    for (int j = 0; j < clothHeight - 1; ++j) {
        for (int i = 0; i < clothWidth - 1; ++i) {
            int index = j * clothWidth + i;
            // Diagonal from top-left to bottom-right.
            Constraint c1;
            c1.p1 = index;
            c1.p2 = index + clothWidth + 1;
            c1.restLength = spacing * std::sqrt(2.f);
            constraints.push_back(c1);

            // Diagonal from top-right to bottom-left.
            Constraint c2;
            c2.p1 = index + 1;
            c2.p2 = index + clothWidth;
            c2.restLength = spacing * std::sqrt(2.f);
            constraints.push_back(c2);
        }
    }
}

void Cloth::update(float dt, const sf::Vector2f& gravity, int iterations) {
    // Update each particle using Verlet integration with gravity.
    for (auto &p : particles) {
        p.update(dt, gravity);
    }

    // Relax constraints several times to maintain structural integrity.
    for (int iter = 0; iter < iterations; ++iter) {
        for (const auto &c : constraints) {
            Particle& p1 = particles[c.p1];
            Particle& p2 = particles[c.p2];
            sf::Vector2f delta = p2.pos - p1.pos;
            float deltaLength = Math::length(delta);
            float diff = (deltaLength - c.restLength) / (deltaLength + 1e-6f); // Avoid division by zero.

            if (!p1.locked && !p2.locked) {
                p1.pos += delta * 0.5f * diff;
                p2.pos -= delta * 0.5f * diff;
            } else if (p1.locked && !p2.locked) {
                p2.pos -= delta * diff;
            } else if (!p1.locked && p2.locked) {
                p1.pos += delta * diff;
            }
        }
    }
}

void Cloth::applyWind(const sf::Vector2f& wind, float dt) {
    // Apply wind force only to non-locked particles.
    // Here, we add a displacement proportional to the wind and dt^2
    // to mimic acceleration (consistent with Verlet integration).
    for (auto &p : particles) {
        if (!p.locked) {
            p.pos += wind * (dt * dt);
        }
    }
}

void Cloth::draw(sf::RenderWindow& window) {
    // Draw constraints as lines.
    sf::VertexArray lines(sf::Lines);
    for (const auto &c : constraints) {
        sf::Vertex v1(particles[c.p1].pos, sf::Color::White);
        sf::Vertex v2(particles[c.p2].pos, sf::Color::White);
        lines.append(v1);
        lines.append(v2);
    }
    window.draw(lines);

    // Draw particles.
    for (const auto &p : particles) {
        p.draw(window);
    }
}
