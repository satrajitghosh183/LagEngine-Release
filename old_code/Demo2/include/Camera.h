#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    Camera();
    
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;
    glm::mat4 getViewProjectionMatrix() const;
    
    glm::vec3 getPosition() const { return m_position; }
    glm::vec3 getForward() const { return m_forward; }
    glm::vec3 getUp() const { return m_up; }
    
    void setPosition(const glm::vec3& pos) { m_position = pos; }
    void setAspect(float aspect);
    void update(float deltaTime);

private:
    glm::vec3 m_position;
    glm::vec3 m_forward;
    glm::vec3 m_up;
    glm::vec3 m_right;
    
    float m_fov;
    float m_aspect;
    float m_near;
    float m_far;
    
    float m_yaw;
    float m_pitch;
    float m_speed;
    float m_sensitivity;
};

