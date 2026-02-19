#pragma once

#include "../../Engine/Core/Base.hpp"
#include "../../Engine/Scene/Entity.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace GameEngine {

    /**
     * @brief Gizmo mode
     */
    enum class GizmoMode {
        Local,  // Transform relative to object's local axes
        World   // Transform relative to world axes
    };

    /**
     * @brief Gizmo type
     */
    enum class GizmoType {
        Translate,
        Rotate,
        Scale
    };

    /**
     * @brief Transform gizmo for visual manipulation
     * 
     * Provides interactive transformation handles for:
     * - Translation (arrows)
     * - Rotation (circles)
     * - Scale (boxes)
     */
    class TransformGizmo {
    public:
        TransformGizmo();
        
        /**
         * @brief Update gizmo interaction
         * @param viewMatrix Camera view matrix
         * @param projMatrix Camera projection matrix
         * @param viewportSize Viewport dimensions
         * @param mousePos Mouse position in screen space
         * @param mouseDown Is mouse button down
         * @return true if gizmo is being manipulated
         */
        bool Update(
            const glm::mat4& viewMatrix,
            const glm::mat4& projMatrix,
            const glm::vec2& viewportSize,
            const glm::vec2& mousePos,
            bool mouseDown
        );
        
        /**
         * @brief Draw gizmo handles
         * @param viewMatrix Camera view matrix
         * @param projMatrix Camera projection matrix
         */
        void Draw(const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
        
        /**
         * @brief Set target entity
         */
        void SetTarget(Entity entity) { m_Target = entity; }
        Entity GetTarget() const { return m_Target; }
        
        /**
         * @brief Set gizmo type
         */
        void SetType(GizmoType type) { m_Type = type; }
        GizmoType GetType() const { return m_Type; }
        
        /**
         * @brief Set gizmo mode
         */
        void SetMode(GizmoMode mode) { m_Mode = mode; }
        GizmoMode GetMode() const { return m_Mode; }
        
        /**
         * @brief Check if gizmo is being used
         */
        bool IsUsing() const { return m_IsUsing; }

        /**
         * @brief Check if a gizmo axis is hovered
         */
        bool IsHovered() const { return m_HoveredAxis != None; }

        /**
         * @brief Set gizmo size scale
         */
        void SetSize(float size) { m_Size = size; }
        
    private:
        // Axis identifiers
        enum Axis {
            None = 0,
            X = 1,
            Y = 2,
            Z = 3,
            XY = 4,
            XZ = 5,
            YZ = 6,
            XYZ = 7  // Uniform scale
        };
        
        // Hit testing
        Axis HitTest(const glm::vec3& rayOrigin, const glm::vec3& rayDir);
        bool HitTestAxis(const glm::vec3& rayOrigin, const glm::vec3& rayDir, 
                        const glm::vec3& axisDir, float& t);
        bool HitTestPlane(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                         const glm::vec3& planeNormal, const glm::vec3& planePoint, float& t);
        
        // Apply transformation
        void ApplyTranslation(const glm::vec3& delta, Axis axis);
        void ApplyRotation(float angle, Axis axis);
        void ApplyScale(const glm::vec3& delta, Axis axis);
        
        // Get world space ray from screen coordinates
        void ScreenToWorldRay(const glm::vec2& screenPos, const glm::mat4& viewMatrix,
                             const glm::mat4& projMatrix, const glm::vec2& viewportSize,
                             glm::vec3& outOrigin, glm::vec3& outDirection);
        
        // Draw helpers
        void DrawTranslateGizmo(const glm::vec3& position, const glm::quat& rotation);
        void DrawRotateGizmo(const glm::vec3& position, const glm::quat& rotation);
        void DrawScaleGizmo(const glm::vec3& position, const glm::quat& rotation);
        
    private:
        Entity m_Target;
        GizmoType m_Type = GizmoType::Translate;
        GizmoMode m_Mode = GizmoMode::World;
        
        bool m_IsUsing = false;
        Axis m_HoveredAxis = None;
        Axis m_ActiveAxis = None;
        
        glm::vec3 m_InitialPosition;
        glm::quat m_InitialRotation;
        glm::vec3 m_InitialScale;
        glm::vec2 m_InitialMousePos;
        glm::vec3 m_DragStartPoint;
        
        float m_Size = 1.0f;
        
        // Colors
        static constexpr glm::vec3 X_AXIS_COLOR = glm::vec3(0.9f, 0.2f, 0.2f);
        static constexpr glm::vec3 Y_AXIS_COLOR = glm::vec3(0.2f, 0.9f, 0.2f);
        static constexpr glm::vec3 Z_AXIS_COLOR = glm::vec3(0.2f, 0.2f, 0.9f);
        static constexpr glm::vec3 HOVER_COLOR = glm::vec3(1.0f, 1.0f, 0.0f);
    };

}
