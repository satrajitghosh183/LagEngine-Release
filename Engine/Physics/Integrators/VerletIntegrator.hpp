#pragma once

#include "IntegratorInterface.hpp"
#include <glm/glm.hpp>
#include <unordered_map>

namespace GameEngine::Physics {

    class VelocityVerletIntegrator : public IntegratorInterface {
    public:
        void Integrate(glm::vec3& position, glm::vec3& velocity,
                      const glm::vec3& acceleration, float dt) override {
            position += velocity * dt + 0.5f * acceleration * dt * dt;
            velocity += acceleration * dt;
        }
        const char* GetName() const override { return "VelocityVerlet"; }
    };

    class StormerVerletIntegrator : public IntegratorInterface {
    public:
        void Integrate(glm::vec3& position, glm::vec3& velocity,
                      const glm::vec3& acceleration, float dt) override {
            glm::vec3 newPos = 2.0f * position - m_PrevPosition + acceleration * dt * dt;
            velocity = (newPos - m_PrevPosition) / (2.0f * dt);
            m_PrevPosition = position;
            position = newPos;
        }
        const char* GetName() const override { return "StormerVerlet"; }

        void SetPreviousPosition(const glm::vec3& prev) { m_PrevPosition = prev; }

    private:
        glm::vec3 m_PrevPosition{0.0f};
    };

}
