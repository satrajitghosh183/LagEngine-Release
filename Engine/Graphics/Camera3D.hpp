#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace GameEngine {

    /**
     * @brief Camera projection type
     */
    enum class CameraProjectionType {
        Perspective,
        Orthographic
    };

    /**
     * @brief 3D Camera class
     * 
     * Features:
     * - Perspective and orthographic projection
     * - Position, rotation (pitch/yaw/roll)
     * - View and projection matrices
     * - Multiple camera modes (FPS, Orbital, Free)
     * - Frustum data for culling
     * 
     * Usage:
     *   Camera3D camera(45.0f, aspectRatio, 0.1f, 100.0f);
     *   camera.SetPosition(glm::vec3(0, 5, 10));
     *   camera.LookAt(glm::vec3(0, 0, 0));
     *   
     *   glm::mat4 view = camera.GetViewMatrix();
     *   glm::mat4 proj = camera.GetProjectionMatrix();
     */
    class Camera3D {
    public:
        /**
         * @brief Create perspective camera
         */
        Camera3D(float fov, float aspectRatio, float nearClip, float farClip);
        
        /**
         * @brief Create orthographic camera
         */
        Camera3D(float left, float right, float bottom, float top, float nearClip, float farClip);
        
        ~Camera3D() = default;
        
        /**
         * @brief Get view matrix
         */
        const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
        
        /**
         * @brief Get projection matrix
         */
        const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
        
        /**
         * @brief Get view-projection matrix
         */
        glm::mat4 GetViewProjectionMatrix() const { return m_ProjectionMatrix * m_ViewMatrix; }
        
        /**
         * @brief Get camera position
         */
        const glm::vec3& GetPosition() const { return m_Position; }
        
        /**
         * @brief Set camera position
         */
        void SetPosition(const glm::vec3& position);
        
        /**
         * @brief Get forward direction vector
         */
        glm::vec3 GetForward() const { return m_Forward; }
        
        /**
         * @brief Get right direction vector
         */
        glm::vec3 GetRight() const { return m_Right; }
        
        /**
         * @brief Get up direction vector
         */
        glm::vec3 GetUp() const { return m_Up; }
        
        /**
         * @brief Set camera rotation (Euler angles in degrees)
         */
        void SetRotation(float pitch, float yaw, float roll = 0.0f);
        
        /**
         * @brief Set camera transform from world matrix (position + rotation)
         * This properly handles the transform's rotation convention (-Z forward)
         */
        void SetFromWorldTransform(const glm::mat4& worldTransform);
        
        /**
         * @brief Rotate camera
         */
        void Rotate(float deltaPitch, float deltaYaw, float deltaRoll = 0.0f);
        
        /**
         * @brief Look at target position
         */
        void LookAt(const glm::vec3& target, const glm::vec3& up = glm::vec3(0, 1, 0));
        
        /**
         * @brief Move camera relative to its orientation
         */
        void MoveForward(float distance);
        void MoveRight(float distance);
        void MoveUp(float distance);
        
        /**
         * @brief Set projection parameters
         */
        void SetPerspective(float fov, float aspectRatio, float nearClip, float farClip);
        void SetOrthographic(float left, float right, float bottom, float top, float nearClip, float farClip);
        
        /**
         * @brief Get projection type
         */
        CameraProjectionType GetProjectionType() const { return m_ProjectionType; }
        
        /**
         * @brief Get FOV (perspective only)
         */
        float GetFOV() const { return m_FOV; }
        
        /**
         * @brief Set FOV (perspective only)
         */
        void SetFOV(float fov);
        
        /**
         * @brief Get aspect ratio
         */
        float GetAspectRatio() const { return m_AspectRatio; }
        
        /**
         * @brief Set aspect ratio
         */
        void SetAspectRatio(float aspectRatio);
        
        /**
         * @brief Get near clip plane
         */
        float GetNearClip() const { return m_NearClip; }
        
        /**
         * @brief Get far clip plane
         */
        float GetFarClip() const { return m_FarClip; }
        
    private:
        void RecalculateViewMatrix();
        void RecalculateProjectionMatrix();
        void UpdateVectors();
        
    private:
        CameraProjectionType m_ProjectionType;
        
        // Position and orientation
        glm::vec3 m_Position;
        float m_Pitch; // X-axis rotation (degrees)
        float m_Yaw;   // Y-axis rotation (degrees)
        float m_Roll;  // Z-axis rotation (degrees)
        
        // Direction vectors
        glm::vec3 m_Forward;
        glm::vec3 m_Right;
        glm::vec3 m_Up;
        
        // Projection parameters
        float m_FOV;
        float m_AspectRatio;
        float m_NearClip;
        float m_FarClip;
        
        // Orthographic parameters
        float m_OrthographicLeft;
        float m_OrthographicRight;
        float m_OrthographicBottom;
        float m_OrthographicTop;
        
        // Matrices
        glm::mat4 m_ViewMatrix;
        glm::mat4 m_ProjectionMatrix;
    };
}