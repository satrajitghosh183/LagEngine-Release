

#pragma once

#include <glm/glm.hpp>

namespace engine::physics {

class Particle3D {
public:
    Particle3D(const glm::vec3& position, float mass = 1.0f, bool pinned = false);

    void applyForce(const glm::vec3& force);
    void integrate(float dt, float globalDamping = 0.98f);
    void pin();
    void unpin();
    bool isPinned() const;
    void setPinned(bool value);

    const glm::vec3& getPosition() const;
    void setPosition(const glm::vec3& pos);
    const glm::vec3& getPreviousPosition() const;
    float getMass() const;

private:
    glm::vec3 position;
    glm::vec3 previousPosition;
    glm::vec3 accumulatedForce;
    float mass;
    bool pinned;
};

} // namespace engine::physics
