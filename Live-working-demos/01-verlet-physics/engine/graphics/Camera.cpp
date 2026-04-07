#include "Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

namespace engine::graphics {

Camera::Camera(float fovDeg, float aspect, float nearC, float farC)
    : fov(fovDeg), aspectRatio(aspect), nearClip(nearC), farClip(farC),
      yaw(-90.0f), pitch(0.0f) {

    position = glm::vec3(0.0f, 0.0f, 3.0f);
    target = glm::vec3(0.0f, 0.0f, 0.0f);
    up = glm::vec3(0.0f, 1.0f, 0.0f);

    updateVectors();
}

void Camera::setPosition(const glm::vec3& pos) {
    position = pos;
    updateVectors();
}

void Camera::setTarget(const glm::vec3& tgt) {
    target = tgt;
    updateVectors();
}

void Camera::setUpDirection(const glm::vec3& upDir) {
    up = upDir;
    updateVectors();
}

void Camera::move(const glm::vec3& delta) {
    position += delta;
    target += delta;
    updateVectors();
}

void Camera::rotate(float yawDegrees, float pitchDegrees) {
    yaw += yawDegrees;
    pitch += pitchDegrees;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    updateVectors();
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(position, position + forward, up);
}

glm::mat4 Camera::getProjectionMatrix() const {
    return glm::perspective(glm::radians(fov), aspectRatio, nearClip, farClip);
}

const glm::vec3& Camera::getPosition() const {
    return position;
}

const glm::vec3& Camera::getTarget() const {
    return target;
}

const glm::vec3& Camera::getUp() const {
    return up;
}

void Camera::updateVectors() {
    forward.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    forward.y = sin(glm::radians(pitch));
    forward.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    forward = glm::normalize(forward);

    right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    up = glm::normalize(glm::cross(right, forward));
}

} // namespace engine::graphics
