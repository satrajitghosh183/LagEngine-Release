// Particle.hpp
#pragma once
#include <glm/glm.hpp>

namespace engine::physics {

    class Particle {
    public:
        glm::vec2 pos;      // Current position
        glm::vec2 oldPos;   // Previous position (for velocity calculation)
        bool locked = false; // If true, particle won't move

        Particle() = default;

        Particle(const glm::vec2& position);

        Particle(const glm::vec2& position, const glm::vec2& velocity);

        void update(float dt, const glm::vec2& acceleration);

        glm::vec2 getVelocity() const;

        void setVelocity(const glm::vec2& velocity);
    };

}
