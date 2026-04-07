

#pragma once

#include <glm/glm.hpp>

namespace engine::graphics {

class Camera {
public:
    Camera(float fov, float aspectRatio, float nearClip, float farClip);

    void setPosition(const glm::vec3& pos);
    void setTarget(const glm::vec3& tgt);
    void setUpDirection(const glm::vec3& up);

    void move(const glm::vec3& delta);
    void rotate(float yawDegrees, float pitchDegrees);

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;

    const glm::vec3& getPosition() const;
    const glm::vec3& getTarget() const;
    const glm::vec3& getUp() const;

private:
    void updateVectors();

    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 up;
    glm::vec3 forward;
    glm::vec3 right;

    float fov;
    float aspectRatio;
    float nearClip;
    float farClip;

    float yaw;   // Left-right
    float pitch; // Up-down
};

} // namespace engine::graphics
