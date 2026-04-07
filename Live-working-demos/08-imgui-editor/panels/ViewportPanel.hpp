#pragma once

#include "EditorPanel.hpp"
#include "../core/SceneRenderer.hpp"
#include <memory>

namespace editor {

// Forward declaration
class SceneRenderer;

/**
 * @brief Viewport Panel - renders the 3D scene view
 * Similar to LumixEngine's Scene View with gizmo support
 */
class ViewportPanel : public EditorPanel {
public:
    ViewportPanel();
    ~ViewportPanel();
    
    void render() override;
    void update(float dt) override;
    
    // Scene renderer access
    void setSceneRenderer(SceneRenderer* renderer) { m_sceneRenderer = renderer; }
    SceneRenderer* getSceneRenderer() { return m_sceneRenderer; }
    
    // Viewport state
    bool isFocused() const { return m_viewportFocused; }
    bool isHovered() const { return m_viewportHovered; }
    
    // Gizmo mode
    enum class GizmoOperation { Translate, Rotate, Scale };
    enum class GizmoSpace { Local, World };
    
    void setGizmoOperation(GizmoOperation op) { m_gizmoOp = op; }
    GizmoOperation getGizmoOperation() const { return m_gizmoOp; }
    
    void setGizmoSpace(GizmoSpace space) { m_gizmoSpace = space; }
    GizmoSpace getGizmoSpace() const { return m_gizmoSpace; }
    
    // Viewport settings
    void setShowGrid(bool show) { m_showGrid = show; }
    void setShowGizmos(bool show) { m_showGizmos = show; }
    void setShowStats(bool show) { m_showStats = show; }

private:
    void renderToolbar();
    void renderViewport();
    void renderOverlay();
    void renderGizmos();
    void handleCameraInput(float dt);
    void handleSelection();
    
    // Framebuffer (for rendering scene to texture)
    unsigned int m_framebuffer = 0;
    unsigned int m_colorTexture = 0;
    unsigned int m_depthTexture = 0;
    int m_viewportWidth = 1280;
    int m_viewportHeight = 720;
    
    // Scene renderer reference
    SceneRenderer* m_sceneRenderer = nullptr;
    
    // Viewport state
    bool m_viewportFocused = false;
    bool m_viewportHovered = false;
    
    // Camera control
    float m_cameraSpeed = 5.0f;
    float m_cameraSensitivity = 0.1f;
    bool m_cameraControlActive = false;
    float m_lastMouseX = 0.0f;
    float m_lastMouseY = 0.0f;
    
    // Gizmo state
    GizmoOperation m_gizmoOp = GizmoOperation::Translate;
    GizmoSpace m_gizmoSpace = GizmoSpace::Local;
    
    // Display options
    bool m_showGrid = true;
    bool m_showGizmos = true;
    bool m_showStats = true;
    bool m_showWireframe = false;
    
    // Stats
    float m_fps = 0.0f;
    int m_drawCalls = 0;
    int m_triangles = 0;
    
    void createFramebuffer(int width, int height);
    void resizeFramebuffer(int width, int height);
    void deleteFramebuffer();
};

} // namespace editor
