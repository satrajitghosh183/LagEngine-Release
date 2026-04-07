#pragma once

#include <glm/glm.hpp>

namespace engine::objects {

class Ball3D {
public:
    Ball3D(const glm::vec3& position, float radius, float mass = 1.0f);

    void applyForce(const glm::vec3& force);
    void setVelocity(const glm::vec3& vel);
    void setPosition(const glm::vec3& pos);
    void setMass(float m);

    const glm::vec3& getPosition() const;
    const glm::vec3& getVelocity() const;
    float getRadius() const;
    float getMass() const;

    void update(float dt);
    void resolveGroundCollision(float groundY = 0.0f, float restitution = 0.5f);

private:
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 accumulatedForce;
    float radius;
    float mass;
};

} // namespace engine::objects
