#include "Camera2D.hpp"
#include <cmath>
#include <cstdlib>

namespace GameEngine {

    Camera2D::Camera2D(float viewWidth, float viewHeight)
        : m_ViewWidth(viewWidth), m_ViewHeight(viewHeight) {}

    glm::mat4 Camera2D::getViewMatrix() const {
        glm::vec2 pos = m_Position + m_ShakeOffset;
        glm::mat4 view = glm::mat4(1.0f);
        view = glm::translate(view, glm::vec3(-pos, 0.0f));
        if (std::abs(m_Rotation) > 1e-6f) {
            view = glm::rotate(view, -m_Rotation, glm::vec3(0.0f, 0.0f, 1.0f));
        }
        return view;
    }

    glm::mat4 Camera2D::getProjectionMatrix() const {
        float halfW = (m_ViewWidth * 0.5f) / m_Zoom;
        float halfH = (m_ViewHeight * 0.5f) / m_Zoom;
        return glm::ortho(-halfW, halfW, -halfH, halfH, -1.0f, 1.0f);
    }

    glm::mat4 Camera2D::getViewProjectionMatrix() const {
        return getProjectionMatrix() * getViewMatrix();
    }

    void Camera2D::update(float dt) {
        // Follow target
        if (m_HasFollowTarget) {
            glm::vec2 diff = m_FollowTarget - m_Position;
            m_Position += diff * glm::min(m_FollowSmoothing * dt, 1.0f);
        }

        // Screen shake
        if (m_ShakeTimer > 0.0f) {
            m_ShakeTimer -= dt;
            float t = m_ShakeTimer / m_ShakeDuration;
            float intensity = m_ShakeIntensity * t;
            m_ShakeOffset.x = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * intensity;
            m_ShakeOffset.y = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * intensity;
        } else {
            m_ShakeOffset = glm::vec2(0.0f);
        }

        applyBounds();
    }

    void Camera2D::shake(float intensity, float duration) {
        m_ShakeIntensity = intensity;
        m_ShakeDuration = duration;
        m_ShakeTimer = duration;
    }

    glm::vec2 Camera2D::screenToWorld(const glm::vec2& screenPos, const glm::vec2& screenSize) const {
        // Normalize to [-1, 1]
        glm::vec2 ndc;
        ndc.x = (screenPos.x / screenSize.x) * 2.0f - 1.0f;
        ndc.y = 1.0f - (screenPos.y / screenSize.y) * 2.0f; // Flip Y

        // Inverse projection
        float halfW = (m_ViewWidth * 0.5f) / m_Zoom;
        float halfH = (m_ViewHeight * 0.5f) / m_Zoom;

        glm::vec2 worldPos;
        worldPos.x = ndc.x * halfW + m_Position.x;
        worldPos.y = ndc.y * halfH + m_Position.y;
        return worldPos;
    }

    glm::vec2 Camera2D::worldToScreen(const glm::vec2& worldPos, const glm::vec2& screenSize) const {
        float halfW = (m_ViewWidth * 0.5f) / m_Zoom;
        float halfH = (m_ViewHeight * 0.5f) / m_Zoom;

        glm::vec2 ndc;
        ndc.x = (worldPos.x - m_Position.x) / halfW;
        ndc.y = (worldPos.y - m_Position.y) / halfH;

        glm::vec2 screenPos;
        screenPos.x = (ndc.x + 1.0f) * 0.5f * screenSize.x;
        screenPos.y = (1.0f - ndc.y) * 0.5f * screenSize.y;
        return screenPos;
    }

    glm::vec2 Camera2D::getParallaxOffset(float depth) const {
        return m_Position * (1.0f - depth);
    }

    void Camera2D::applyBounds() {
        if (!m_HasBounds) return;
        float halfW = (m_ViewWidth * 0.5f) / m_Zoom;
        float halfH = (m_ViewHeight * 0.5f) / m_Zoom;

        m_Position.x = glm::clamp(m_Position.x, m_BoundsMin.x + halfW, m_BoundsMax.x - halfW);
        m_Position.y = glm::clamp(m_Position.y, m_BoundsMin.y + halfH, m_BoundsMax.y - halfH);
    }

}
