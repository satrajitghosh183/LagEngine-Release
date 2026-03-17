#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

glm::vec3 Camera::position() const {
    float pitchR = glm::radians(pitch);
    float yawR   = glm::radians(yaw);
    glm::vec3 offset = {
        distance * std::cos(pitchR) * std::sin(yawR),
        distance * std::sin(pitchR),
        distance * std::cos(pitchR) * std::cos(yawR)
    };
    return target + offset;
}

glm::mat4 Camera::viewMatrix(float /*aspect*/) const {
    return glm::lookAt(position(), target, glm::vec3(0,1,0));
}

glm::mat4 Camera::projMatrix(float aspect) const {
    return glm::perspective(glm::radians(fovY), aspect, nearZ, farZ);
}

void Camera::orbit(float dx, float dy) {
    yaw   -= dx * 0.4f;
    pitch += dy * 0.4f;
    pitch  = std::clamp(pitch, -89.0f, 89.0f);
}

void Camera::pan(float dx, float dy) {
    glm::vec3 pos   = position();
    glm::vec3 fwd   = glm::normalize(target - pos);
    glm::vec3 right = glm::normalize(glm::cross(fwd, glm::vec3(0,1,0)));
    glm::vec3 up    = glm::cross(right, fwd);
    float speed = distance * 0.001f;
    target     -= right * (dx * speed);
    target     += up    * (dy * speed);
}

void Camera::zoom(float delta) {
    distance -= delta * 0.5f;
    distance  = std::clamp(distance, 1.0f, 100.0f);
}
