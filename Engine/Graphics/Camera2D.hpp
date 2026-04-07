#pragma once

#include "../Core/Base.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace GameEngine {

    class Camera2D {
    public:
        Camera2D(float viewWidth = 1280.0f, float viewHeight = 720.0f);
        ~Camera2D() = default;

        // View/Projection
        glm::mat4 getViewMatrix() const;
        glm::mat4 getProjectionMatrix() const;
        glm::mat4 getViewProjectionMatrix() const;

        // Transform
        void setPosition(const glm::vec2& pos) { m_Position = pos; }
        glm::vec2 getPosition() const { return m_Position; }
        void setZoom(float zoom) { m_Zoom = glm::max(zoom, 0.01f); }
        float getZoom() const { return m_Zoom; }
        void setRotation(float radians) { m_Rotation = radians; }
        float getRotation() const { return m_Rotation; }

        // View size
        void setViewSize(float w, float h) { m_ViewWidth = w; m_ViewHeight = h; }
        float getViewWidth() const { return m_ViewWidth; }
        float getViewHeight() const { return m_ViewHeight; }

        // Camera follow
        void setFollowTarget(const glm::vec2& target) { m_FollowTarget = target; m_HasFollowTarget = true; }
        void clearFollowTarget() { m_HasFollowTarget = false; }
        void setFollowSmoothing(float smoothing) { m_FollowSmoothing = smoothing; }
        void update(float dt);

        // Camera bounds (world space limits)
        void setBounds(const glm::vec2& min, const glm::vec2& max) { m_BoundsMin = min; m_BoundsMax = max; m_HasBounds = true; }
        void clearBounds() { m_HasBounds = false; }

        // Screen shake
        void shake(float intensity, float duration);

        // Coordinate conversion
        glm::vec2 screenToWorld(const glm::vec2& screenPos, const glm::vec2& screenSize) const;
        glm::vec2 worldToScreen(const glm::vec2& worldPos, const glm::vec2& screenSize) const;

        // Parallax helper: returns offset for a layer at given depth (0=camera, 1=background)
        glm::vec2 getParallaxOffset(float depth) const;

    private:
        void applyBounds();

        glm::vec2 m_Position = glm::vec2(0.0f);
        float m_Zoom = 1.0f;
        float m_Rotation = 0.0f;
        float m_ViewWidth;
        float m_ViewHeight;

        // Follow
        bool m_HasFollowTarget = false;
        glm::vec2 m_FollowTarget = glm::vec2(0.0f);
        float m_FollowSmoothing = 5.0f;

        // Bounds
        bool m_HasBounds = false;
        glm::vec2 m_BoundsMin = glm::vec2(0.0f);
        glm::vec2 m_BoundsMax = glm::vec2(0.0f);

        // Shake
        float m_ShakeIntensity = 0.0f;
        float m_ShakeDuration = 0.0f;
        float m_ShakeTimer = 0.0f;
        glm::vec2 m_ShakeOffset = glm::vec2(0.0f);
    };

}
