#pragma once

#include "../../Engine/Core/Base.hpp"
#include "../../Engine/Scene/Scene.hpp"
#include "../../Engine/Graphics/Framebuffer.hpp"
#include "../../Engine/Graphics/Camera3D.hpp"
#include "../../Engine/Graphics/Frustum.hpp"
#include "../../Engine/Graphics/RenderGraph.hpp"
#include "../../Engine/Graphics/RenderPath.hpp"
#include "../../Engine/Graphics/ShadowMap.hpp"
#include "../Gizmos/TransformGizmo.hpp"
#include <imgui.h>
#include <glm/glm.hpp>
#include <functional>

namespace GameEngine {

    /**
     * @brief Gizmo operation mode (for backward compatibility)
     */
    enum class GizmoOperation {
        None = 0,
        Translate,
        Rotate,
        Scale
    };

    /**
     * @brief Editor camera for viewport navigation
     * 
     * Supports orbit, pan, and zoom controls
     */
    class EditorCamera {
    public:
        EditorCamera();
        
        void OnUpdate(float deltaTime);
        void OnEvent(/* Event& e */);
        
        // Orbit around focus point
        void Orbit(float deltaX, float deltaY);
        
        // Pan camera (move focus point)
        void Pan(float deltaX, float deltaY);
        
        // Zoom (move closer/further from focus point)
        void Zoom(float delta);
        
        // Focus on position
        void FocusOn(const glm::vec3& position);
        
        // Reset camera
        void Reset();
        
        // Getters
        const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
        const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
        glm::mat4 GetViewProjectionMatrix() const { return m_ProjectionMatrix * m_ViewMatrix; }
        
        glm::vec3 GetPosition() const { return m_Position; }
        glm::vec3 GetForward() const;
        glm::vec3 GetRight() const;
        glm::vec3 GetUp() const;
        
        float GetFOV() const { return m_FOV; }
        void SetFOV(float fov);
        
        void SetViewportSize(float width, float height);
        
        // Camera state getters/setters for scene tab management
        glm::vec3 GetFocalPoint() const { return m_FocalPoint; }
        void SetFocalPoint(const glm::vec3& point) { m_FocalPoint = point; RecalculateViewMatrix(); }
        float GetDistance() const { return m_Distance; }
        void SetDistance(float distance) { m_Distance = distance; RecalculateViewMatrix(); }
        float GetPitch() const { return m_Pitch; }
        void SetPitch(float pitch) { m_Pitch = pitch; RecalculateViewMatrix(); }
        float GetYaw() const { return m_Yaw; }
        void SetYaw(float yaw) { m_Yaw = yaw; RecalculateViewMatrix(); }
        
    private:
        void RecalculateViewMatrix();
        void RecalculateProjectionMatrix();
        glm::vec3 CalculatePosition() const;
        
    private:
        glm::vec3 m_FocalPoint = glm::vec3(0.0f);
        float m_Distance = 10.0f;
        
        float m_Pitch = 30.0f;  // Degrees
        float m_Yaw = 45.0f;    // Degrees
        
        float m_FOV = 45.0f;
        float m_AspectRatio = 16.0f / 9.0f;
        float m_NearClip = 0.1f;
        float m_FarClip = 1000.0f;
        
        float m_ViewportWidth = 1280.0f;
        float m_ViewportHeight = 720.0f;
        
        glm::vec3 m_Position;
        glm::mat4 m_ViewMatrix = glm::mat4(1.0f);
        glm::mat4 m_ProjectionMatrix = glm::mat4(1.0f);
        
        // Mouse state - public for input handling
        glm::vec2 m_LastMousePos = glm::vec2(0.0f);
        
        friend class ViewportPanel;
    };

    /**
     * @brief Viewport panel - 3D scene view
     * 
     * Features:
     * - Framebuffer rendering
     * - Editor camera with orbit/pan/zoom
     * - Gizmos for transform manipulation
     * - Entity picking
     * - Grid overlay
     */
    class ViewportPanel {
    public:
        ViewportPanel();
        ~ViewportPanel() = default;
        
        void SetContext(const Ref<Scene>& scene) { m_Context = scene; }
        void SetSelectedEntity(Entity entity) { m_SelectedEntity = entity; }
        Entity GetSelectedEntity() const { return m_SelectedEntity; }
        
        void OnUpdate(float deltaTime);
        void OnImGuiRender();
        
        // Gizmo operations
        void SetGizmoOperation(GizmoOperation op) { m_GizmoOperation = op; }
        GizmoOperation GetGizmoOperation() const { return m_GizmoOperation; }
        
        // Camera
        EditorCamera& GetEditorCamera() { return m_EditorCamera; }
        const EditorCamera& GetEditorCamera() const { return m_EditorCamera; }
        
        // Viewport info
        glm::vec2 GetViewportSize() const { return m_ViewportSize; }
        bool IsViewportFocused() const { return m_ViewportFocused; }
        bool IsViewportHovered() const { return m_ViewportHovered; }

        // Asset drop callback: called when an asset is dropped into the viewport
        using AssetDropCallback = std::function<void(const std::string& assetPath)>;
        void SetAssetDropCallback(AssetDropCallback cb) { m_AssetDropCallback = std::move(cb); }
        
        // RenderPath management
        void SetRenderPath(const Ref<RenderPath>& path) { m_ActiveRenderPath = path; }
        Ref<RenderPath> GetRenderPath() const { return m_ActiveRenderPath; }
        
    private:
        void HandleInput();
        void RenderSceneToFramebuffer();
        void RenderGizmos();
        void DrawGrid();
        void DrawSelectionOutline();
        void PickEntity(const glm::vec2& mousePos);
        
    private:
        Ref<Scene> m_Context;
        Entity m_SelectedEntity;
        
        Scope<Framebuffer> m_Framebuffer;
        EditorCamera m_EditorCamera;
        TransformGizmo m_TransformGizmo;
        
        glm::vec2 m_ViewportSize = glm::vec2(0.0f);
        glm::vec2 m_ViewportBounds[2];
        
        bool m_ViewportFocused = false;
        bool m_ViewportHovered = false;
        
        GizmoOperation m_GizmoOperation = GizmoOperation::Translate;
        GizmoMode m_GizmoMode = GizmoMode::World;
        
        // Grid settings
        bool m_ShowGrid = true;
        float m_GridSize = 10.0f;
        int m_GridDivisions = 10;

        // Frustum culling
        Frustum m_Frustum;

        // Render graph
        RenderGraph m_RenderGraph;

        // Shadow mapping
        Scope<ShadowMapManager> m_ShadowMapManager;

        void SetupRenderGraph();

        AssetDropCallback m_AssetDropCallback;
        
        // Active render path
        Ref<RenderPath> m_ActiveRenderPath;
    };

}
