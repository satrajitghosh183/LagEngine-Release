#include "Cloth3D.hpp"
#include <glm/gtx/norm.hpp>
#include <cmath>

Cloth3D::Cloth3D(int w, int h, float s, const glm::vec3 &origin)
    : width(w), height(h), spacing(s)
{
    // Build a grid of particles.
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            glm::vec3 pos = origin + glm::vec3(i * spacing, -j * spacing, 0.0f);
            Particle3D p(pos);
            if (j == 0)
                p.locked = true;  
            particles.push_back(p);
        }
    }
    
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            int index = j * width + i;
            if (i < width - 1) {
                Constraint3D c;
                c.p1 = index;
                c.p2 = index + 1;
                c.restLength = spacing;
                constraints.push_back(c);
            }
            if (j < height - 1) {
                Constraint3D c;
                c.p1 = index;
                c.p2 = index + width;
                c.restLength = spacing;
                constraints.push_back(c);
            }
            
            if (i < width - 1 && j < height - 1) {
                Constraint3D c;
                c.p1 = index;
                c.p2 = index + width + 1;
                c.restLength = spacing * std::sqrt(2.0f);
                constraints.push_back(c);

                Constraint3D c2;
                c2.p1 = index + 1;
                c2.p2 = index + width;
                c2.restLength = spacing * std::sqrt(2.0f);
                constraints.push_back(c2);
            }
        }
    }
}

void Cloth3D::update(float dt, const glm::vec3 &acceleration, int iterations) {
    for (auto &p : particles) {
        p.update(dt, acceleration);
    }
    // Constraint relaxation.
    for (int iter = 0; iter < iterations; ++iter) {
        for (auto &c : constraints) {
            Particle3D &p1 = particles[c.p1];
            Particle3D &p2 = particles[c.p2];
            glm::vec3 delta = p2.pos - p1.pos;
            float deltaLength = glm::length(delta);
            float diff = (deltaLength - c.restLength) / (deltaLength + 1e-6f);
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

void Cloth3D::applyWind(const glm::vec3 &windForce) {
    for (auto &p : particles) {
        if (!p.locked)
            p.pos += windForce;  
    }
}
