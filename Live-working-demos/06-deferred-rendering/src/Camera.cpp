#include "Camera.h"
#include <algorithm>
#include <cmath>

Camera::Camera()
    : m_position(0.0f, 15.0f, 25.0f)  // Start further back and higher
    , m_forward(0.0f, -0.5f, -1.0f)
    , m_up(0.0f, 1.0f, 0.0f)
    , m_fov(60.0f)  // Wider FOV to see more
    , m_aspect(16.0f / 9.0f)
    , m_near(0.1f)
    , m_far(200.0f)  // Further far plane
    , m_yaw(-90.0f)
    , m_pitch(-20.0f)
    , m_speed(5.0f)
    , m_sensitivity(0.1f)
{
    update(0.0f);
    // Initialize to look at origin
    m_forward = glm::normalize(glm::vec3(0.0f, 0.0f, 0.0f) - m_position);
    m_right = glm::normalize(glm::cross(m_forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    m_up = glm::normalize(glm::cross(m_right, m_forward));
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(m_position, m_position + m_forward, m_up);
}

glm::mat4 Camera::getProjectionMatrix() const {
    return glm::perspective(glm::radians(m_fov), m_aspect, m_near, m_far);
}

glm::mat4 Camera::getViewProjectionMatrix() const {
    return getProjectionMatrix() * getViewMatrix();
}

void Camera::setAspect(float aspect) {
    m_aspect = aspect;
}

void Camera::update(float deltaTime) {
    // Update camera vectors from yaw and pitch
    glm::vec3 front;
    front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    front.y = sin(glm::radians(m_pitch));
    front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_forward = glm::normalize(front);
    
    m_right = glm::normalize(glm::cross(m_forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    m_up = glm::normalize(glm::cross(m_right, m_forward));
    
    // DISABLED: Automatic orbital camera - user can control manually with WASD
    // If you want automatic movement, uncomment this:
    /*
    static float time = 0.0f;
    time += deltaTime;
    float radius = 25.0f;  // Much further back
    m_position.x = cos(time * 0.3f) * radius;
    m_position.z = sin(time * 0.3f) * radius;
    m_position.y = 15.0f + sin(time * 0.2f) * 3.0f;  // Higher up
    
    // Look at origin
    m_forward = glm::normalize(glm::vec3(0.0f, 0.0f, 0.0f) - m_position);
    m_right = glm::normalize(glm::cross(m_forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    m_up = glm::normalize(glm::cross(m_right, m_forward));
    */
    
    // Always look at origin for now (can be changed later)
    m_forward = glm::normalize(glm::vec3(0.0f, 0.0f, 0.0f) - m_position);
    m_right = glm::normalize(glm::cross(m_forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    m_up = glm::normalize(glm::cross(m_right, m_forward));
}

