#include "Camera3D.hpp"
#include <glm/gtc/quaternion.hpp>
#include <algorithm>

namespace GameEngine {

    Camera3D::Camera3D(float fov, float aspectRatio, float nearClip, float farClip)
        : m_ProjectionType(CameraProjectionType::Perspective)
        , m_Position(0.0f, 0.0f, 0.0f)
        , m_Pitch(0.0f)
        , m_Yaw(-90.0f)  // Start facing negative Z
        , m_Roll(0.0f)
        , m_FOV(fov)
        , m_AspectRatio(aspectRatio)
        , m_NearClip(nearClip)
        , m_FarClip(farClip) {
        
        UpdateVectors();
        RecalculateViewMatrix();
        RecalculateProjectionMatrix();
    }

    Camera3D::Camera3D(float left, float right, float bottom, float top, float nearClip, float farClip)
        : m_ProjectionType(CameraProjectionType::Orthographic)
        , m_Position(0.0f, 0.0f, 0.0f)
        , m_Pitch(0.0f)
        , m_Yaw(-90.0f)
        , m_Roll(0.0f)
        , m_OrthographicLeft(left)
        , m_OrthographicRight(right)
        , m_OrthographicBottom(bottom)
        , m_OrthographicTop(top)
        , m_NearClip(nearClip)
        , m_FarClip(farClip) {
        
        UpdateVectors();
        RecalculateViewMatrix();
        RecalculateProjectionMatrix();
    }

    void Camera3D::SetPosition(const glm::vec3& position) {
        m_Position = position;
        RecalculateViewMatrix();
    }

    void Camera3D::SetRotation(float pitch, float yaw, float roll) {
        m_Pitch = std::clamp(pitch, -89.0f, 89.0f);
        m_Yaw = yaw;
        m_Roll = roll;
        
        UpdateVectors();
        RecalculateViewMatrix();
    }

    void Camera3D::SetFromWorldTransform(const glm::mat4& worldTransform) {
        // Extract position from world transform
        m_Position = glm::vec3(worldTransform[3]);
        
        // Extract rotation basis vectors from the world transform
        // Entity convention: -Z is forward, +Y is up, +X is right
        m_Right   = glm::normalize(glm::vec3(worldTransform[0]));
        m_Up      = glm::normalize(glm::vec3(worldTransform[1]));
        m_Forward = -glm::normalize(glm::vec3(worldTransform[2])); // Negate Z for -Z forward
        
        // Calculate euler angles from the forward vector for consistency
        m_Pitch = glm::degrees(asin(glm::clamp(m_Forward.y, -1.0f, 1.0f)));
        m_Yaw = glm::degrees(atan2(m_Forward.z, m_Forward.x));
        
        // Build view matrix directly: inverse of the world transform's rotation + translation
        m_ViewMatrix = glm::lookAt(m_Position, m_Position + m_Forward, m_Up);
    }

    void Camera3D::Rotate(float deltaPitch, float deltaYaw, float deltaRoll) {
        m_Pitch = std::clamp(m_Pitch + deltaPitch, -89.0f, 89.0f);
        m_Yaw += deltaYaw;
        m_Roll += deltaRoll;
        
        // Normalize yaw to [0, 360)
        while (m_Yaw >= 360.0f) m_Yaw -= 360.0f;
        while (m_Yaw < 0.0f) m_Yaw += 360.0f;
        
        UpdateVectors();
        RecalculateViewMatrix();
    }

    void Camera3D::LookAt(const glm::vec3& target, const glm::vec3& up) {
        glm::vec3 direction = glm::normalize(target - m_Position);
        
        // Calculate yaw and pitch from direction
        m_Yaw = glm::degrees(atan2(direction.x, direction.z));
        m_Pitch = glm::degrees(asin(direction.y));
        
        UpdateVectors();
        RecalculateViewMatrix();
    }

    void Camera3D::MoveForward(float distance) {
        m_Position += m_Forward * distance;
        RecalculateViewMatrix();
    }

    void Camera3D::MoveRight(float distance) {
        m_Position += m_Right * distance;
        RecalculateViewMatrix();
    }

    void Camera3D::MoveUp(float distance) {
        m_Position += m_Up * distance;
        RecalculateViewMatrix();
    }

    void Camera3D::SetPerspective(float fov, float aspectRatio, float nearClip, float farClip) {
        m_ProjectionType = CameraProjectionType::Perspective;
        m_FOV = fov;
        m_AspectRatio = aspectRatio;
        m_NearClip = nearClip;
        m_FarClip = farClip;
        
        RecalculateProjectionMatrix();
    }

    void Camera3D::SetOrthographic(float left, float right, float bottom, float top, float nearClip, float farClip) {
        m_ProjectionType = CameraProjectionType::Orthographic;
        m_OrthographicLeft = left;
        m_OrthographicRight = right;
        m_OrthographicBottom = bottom;
        m_OrthographicTop = top;
        m_NearClip = nearClip;
        m_FarClip = farClip;
        
        RecalculateProjectionMatrix();
    }

    void Camera3D::SetFOV(float fov) {
        m_FOV = fov;
        if (m_ProjectionType == CameraProjectionType::Perspective) {
            RecalculateProjectionMatrix();
        }
    }

    void Camera3D::SetAspectRatio(float aspectRatio) {
        m_AspectRatio = aspectRatio;
        if (m_ProjectionType == CameraProjectionType::Perspective) {
            RecalculateProjectionMatrix();
        }
    }

    void Camera3D::UpdateVectors() {
        // Calculate forward vector from Euler angles
        glm::vec3 forward;
        forward.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
        forward.y = sin(glm::radians(m_Pitch));
        forward.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
        m_Forward = glm::normalize(forward);
        
        // Calculate right and up vectors
        m_Right = glm::normalize(glm::cross(m_Forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        m_Up = glm::normalize(glm::cross(m_Right, m_Forward));
        
        // Apply roll rotation
        if (abs(m_Roll) > 0.001f) {
            glm::mat4 rollMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(m_Roll), m_Forward);
            m_Right = glm::vec3(rollMatrix * glm::vec4(m_Right, 0.0f));
            m_Up = glm::vec3(rollMatrix * glm::vec4(m_Up, 0.0f));
        }
    }

    void Camera3D::RecalculateViewMatrix() {
        m_ViewMatrix = glm::lookAt(m_Position, m_Position + m_Forward, m_Up);
    }

    void Camera3D::RecalculateProjectionMatrix() {
        if (m_ProjectionType == CameraProjectionType::Perspective) {
            m_ProjectionMatrix = glm::perspective(
                glm::radians(m_FOV),
                m_AspectRatio,
                m_NearClip,
                m_FarClip
            );
        } else {
            m_ProjectionMatrix = glm::ortho(
                m_OrthographicLeft,
                m_OrthographicRight,
                m_OrthographicBottom,
                m_OrthographicTop,
                m_NearClip,
                m_FarClip
            );
        }
    }
}