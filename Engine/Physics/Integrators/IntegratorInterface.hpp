#pragma once

#include <glm/glm.hpp>

namespace GameEngine::Physics {

    class IntegratorInterface {
    public:
        virtual ~IntegratorInterface() = default;
        virtual void Integrate(glm::vec3& position, glm::vec3& velocity,
                              const glm::vec3& acceleration, float dt) = 0;
        virtual const char* GetName() const = 0;
    };

    class EulerIntegrator : public IntegratorInterface {
    public:
        void Integrate(glm::vec3& position, glm::vec3& velocity,
                      const glm::vec3& acceleration, float dt) override {
            velocity += acceleration * dt;
            position += velocity * dt;
        }
        const char* GetName() const override { return "Euler"; }
    };

}
