// Particle.cpp
#include "engine/physics/Particle.hpp"
#include <iostream>

namespace engine::physics {

    Particle::Particle(const glm::vec2& position)
        : pos(position), oldPos(position) {
    }

    Particle::Particle(const glm::vec2& position, const glm::vec2& velocity)
        : pos(position), oldPos(position - velocity) {
    }

    void Particle::update(float dt, const glm::vec2& acceleration) {
        if (locked) return;

        glm::vec2 temp = pos;

        glm::vec2 velocity = pos - oldPos;

        pos = pos + velocity + acceleration * dt * dt;

        oldPos = temp;
    }

    glm::vec2 Particle::getVelocity() const {
        return pos - oldPos;
    }

    void Particle::setVelocity(const glm::vec2& velocity) {
        oldPos = pos - velocity;
    }

}
