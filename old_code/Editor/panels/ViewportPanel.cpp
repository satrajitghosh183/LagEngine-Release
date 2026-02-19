#include "ViewportPanel.hpp"
#include "../core/EditorTheme.hpp"
#include <imgui.h>
#include <glad/glad.h>

namespace editor {

ViewportPanel::ViewportPanel() 
    : EditorPanel("Viewport", true) {
    createFramebuffer(m_viewportWidth, m_viewportHeight);
}

ViewportPanel::~ViewportPanel() {
    deleteFramebuffer();
}

void ViewportPanel::createFramebuffer(int width, int height) {
    if (width <= 0 || height <= 0) return;
    
    // Delete existing framebuffer if any
    deleteFramebuffer();
    
    m_viewportWidth = width;
    m_viewportHeight = height;
    
    // Create framebuffer
    glGenFramebuffers(1, &m_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    
    // Color texture
    glGenTextures(1, &m_colorTexture);
    glBindTexture(GL_TEXTURE_2D, m_colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTexture, 0);
    
    // Depth texture
    glGenTextures(1, &m_depthTexture);
    glBindTexture(GL_TEXTURE_2D, m_depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_depthTexture, 0);
    
    // Check completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        // Error handling
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ViewportPanel::resizeFramebuffer(int width, int height) {
    if (width != m_viewportWidth || height != m_viewportHeight) {
        createFramebuffer(width, height);
    }
}

void ViewportPanel::deleteFramebuffer() {
    if (m_framebuffer) {
        glDeleteFramebuffers(1, &m_framebuffer);
        m_framebuffer = 0;
    }
    if (m_colorTexture) {
        glDeleteTextures(1, &m_colorTexture);
        m_colorTexture = 0;
    }
    if (m_depthTexture) {
        glDeleteTextures(1, &m_depthTexture);
        m_depthTexture = 0;
    }
}

void ViewportPanel::render() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    
    if (!beginPanel(ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        ImGui::PopStyleVar();
        endPanel();
        return;
    }
    
    ImGui::PopStyleVar();
    
    // Render toolbar overlay
    renderToolbar();
    
    // Get available size for viewport
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    
    // Resize framebuffer if needed
    if (viewportSize.x > 0 && viewportSize.y > 0) {
        resizeFramebuffer(static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y));
    }
    
    // Update viewport state
    m_viewportFocused = ImGui::IsWindowFocused();
    m_viewportHovered = ImGui::IsWindowHovered();
    
    // Render scene to framebuffer
    renderViewport();
    
    // Display the framebuffer texture
    ImGui::Image(
        (ImTextureID)(intptr_t)m_colorTexture,
        viewportSize,
        ImVec2(0, 1),  // UV flipped for OpenGL
        ImVec2(1, 0)
    );
    
    // Handle selection via mouse click
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsKeyDown(ImGuiKey_LeftAlt)) {
        handleSelection();
    }
    
    // Render overlay stats
    if (m_showStats) {
        renderOverlay();
    }
    
    // Handle camera input
    if (m_viewportHovered) {
        handleCameraInput(ImGui::GetIO().DeltaTime);
    }
    
    endPanel();
}

void ViewportPanel::renderToolbar() {
    ImGui::SetCursorPos(ImVec2(10, 30));
    
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.18f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.30f, 1.0f));
    
    // Gizmo operation buttons
    ImVec4 activeColor = ImVec4(0.3f, 0.6f, 0.8f, 1.0f);
    
    ImGui::PushStyleColor(ImGuiCol_Button, m_gizmoOp == GizmoOperation::Translate ? activeColor : ImVec4(0.15f, 0.15f, 0.18f, 0.9f));
    if (ImGui::Button("T##Translate", ImVec2(28, 28))) {
        m_gizmoOp = GizmoOperation::Translate;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Translate (W)");
    ImGui::PopStyleColor();
    
    ImGui::SameLine();
    
    ImGui::PushStyleColor(ImGuiCol_Button, m_gizmoOp == GizmoOperation::Rotate ? activeColor : ImVec4(0.15f, 0.15f, 0.18f, 0.9f));
    if (ImGui::Button("R##Rotate", ImVec2(28, 28))) {
        m_gizmoOp = GizmoOperation::Rotate;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Rotate (E)");
    ImGui::PopStyleColor();
    
    ImGui::SameLine();
    
    ImGui::PushStyleColor(ImGuiCol_Button, m_gizmoOp == GizmoOperation::Scale ? activeColor : ImVec4(0.15f, 0.15f, 0.18f, 0.9f));
    if (ImGui::Button("S##Scale", ImVec2(28, 28))) {
        m_gizmoOp = GizmoOperation::Scale;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Scale (R)");
    ImGui::PopStyleColor();
    
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(10, 0));
    ImGui::SameLine();
    
    // Space toggle
    if (ImGui::Button(m_gizmoSpace == GizmoSpace::Local ? "Local" : "World", ImVec2(50, 28))) {
        m_gizmoSpace = (m_gizmoSpace == GizmoSpace::Local) ? GizmoSpace::World : GizmoSpace::Local;
    }
    
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(10, 0));
    ImGui::SameLine();
    
    // Display options
    ImGui::PushStyleColor(ImGuiCol_Button, m_showGrid ? activeColor : ImVec4(0.15f, 0.15f, 0.18f, 0.9f));
    if (ImGui::Button("Grid##Grid", ImVec2(40, 28))) {
        m_showGrid = !m_showGrid;
        if (m_sceneRenderer) {
            m_sceneRenderer->setShowGrid(m_showGrid);
        }
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle Grid");
    ImGui::PopStyleColor();
    
    ImGui::SameLine();
    
    ImGui::PushStyleColor(ImGuiCol_Button, m_showWireframe ? activeColor : ImVec4(0.15f, 0.15f, 0.18f, 0.9f));
    if (ImGui::Button("Wire##Wire", ImVec2(40, 28))) {
        m_showWireframe = !m_showWireframe;
        if (m_sceneRenderer) {
            m_sceneRenderer->setShowWireframe(m_showWireframe);
        }
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle Wireframe");
    ImGui::PopStyleColor();
    
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

void ViewportPanel::renderViewport() {
    // Bind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    glViewport(0, 0, m_viewportWidth, m_viewportHeight);
    
    // Clear with a gradient-ish background
    glClearColor(0.12f, 0.12f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Render scene using SceneRenderer
    if (m_sceneRenderer) {
        m_sceneRenderer->render(m_viewportWidth, m_viewportHeight);
        
        // Update stats
        m_drawCalls = static_cast<int>(m_sceneRenderer->getObjects().size());
        m_triangles = m_drawCalls * 12; // Rough estimate
    }
    
    // Unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ViewportPanel::renderOverlay() {
    // Position overlay in top-right corner of viewport
    ImVec2 windowSize = ImGui::GetWindowSize();
    
    ImGui::SetCursorPos(ImVec2(windowSize.x - 180, 35));
    
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.12f, 0.85f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
    
    ImGui::BeginChild("##Stats", ImVec2(170, 120), true);
    
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 0.95f, 1.0f));
    ImGui::Text("Viewport Stats");
    ImGui::PopStyleColor();
    
    ImGui::Separator();
    
    ImGui::Text("FPS: %.1f", m_fps > 0 ? m_fps : 60.0f);
    ImGui::Text("Resolution: %dx%d", m_viewportWidth, m_viewportHeight);
    ImGui::Text("Draw Calls: %d", m_drawCalls);
    ImGui::Text("Triangles: %d", m_triangles);
    
    if (m_sceneRenderer) {
        ImGui::Text("Objects: %d", static_cast<int>(m_sceneRenderer->getObjects().size()));
    }
    
    ImGui::EndChild();
    
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

void ViewportPanel::renderGizmos() {
    if (!m_showGizmos || !m_sceneRenderer) return;
    
    SceneObject* selected = m_sceneRenderer->getSelectedObject();
    if (!selected) return;
    
    // Gizmo rendering would happen here (requires ImGuizmo or custom implementation)
}

void ViewportPanel::handleCameraInput(float dt) {
    if (!m_sceneRenderer) return;
    
    ImGuiIO& io = ImGui::GetIO();
    EditorCamera& camera = m_sceneRenderer->getCamera();
    
    // Right-click to start camera control
    if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        if (!m_cameraControlActive) {
            m_cameraControlActive = true;
            m_lastMouseX = io.MousePos.x;
            m_lastMouseY = io.MousePos.y;
        }
        
        // Mouse delta for camera rotation
        float deltaX = io.MousePos.x - m_lastMouseX;
        float deltaY = io.MousePos.y - m_lastMouseY;
        m_lastMouseX = io.MousePos.x;
        m_lastMouseY = io.MousePos.y;
        
        // Apply rotation
        camera.processMouseMovement(deltaX, -deltaY);
        
        // WASD movement
        float speed = m_cameraSpeed * dt;
        if (ImGui::IsKeyDown(ImGuiKey_LeftShift)) speed *= 2.0f;
        
        if (ImGui::IsKeyDown(ImGuiKey_W)) {
            camera.moveForward(speed);
        }
        if (ImGui::IsKeyDown(ImGuiKey_S)) {
            camera.moveForward(-speed);
        }
        if (ImGui::IsKeyDown(ImGuiKey_A)) {
            camera.moveRight(-speed);
        }
        if (ImGui::IsKeyDown(ImGuiKey_D)) {
            camera.moveRight(speed);
        }
        if (ImGui::IsKeyDown(ImGuiKey_Q)) {
            camera.moveUp(-speed);
        }
        if (ImGui::IsKeyDown(ImGuiKey_E)) {
            camera.moveUp(speed);
        }
    } else {
        m_cameraControlActive = false;
    }
    
    // Scroll to zoom
    if (io.MouseWheel != 0) {
        camera.processMouseScroll(io.MouseWheel);
    }
    
    // Focus on selection (F key)
    if (ImGui::IsKeyPressed(ImGuiKey_F)) {
        SceneObject* selected = m_sceneRenderer->getSelectedObject();
        if (selected) {
            camera.focusOn(selected->transform.position);
        }
    }
    
    // Gizmo shortcuts (only when not typing)
    if (!io.WantTextInput && !m_cameraControlActive) {
        if (ImGui::IsKeyPressed(ImGuiKey_W)) {
            m_gizmoOp = GizmoOperation::Translate;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_E)) {
            m_gizmoOp = GizmoOperation::Rotate;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_R)) {
            m_gizmoOp = GizmoOperation::Scale;
        }
    }
}

void ViewportPanel::handleSelection() {
    if (!m_sceneRenderer) return;
    
    // Get mouse position relative to viewport
    ImVec2 mousePos = ImGui::GetMousePos();
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();
    
    float relX = mousePos.x - windowPos.x;
    float relY = mousePos.y - windowPos.y - 30; // Account for title bar
    
    // Check if click is within viewport
    if (relX < 0 || relY < 0 || relX > windowSize.x || relY > windowSize.y - 30) {
        return;
    }
    
    // Convert to normalized device coordinates
    float ndcX = (2.0f * relX / m_viewportWidth) - 1.0f;
    float ndcY = 1.0f - (2.0f * relY / m_viewportHeight);
    
    // Simple selection: cycle through objects or deselect
    // In a full implementation, this would do ray casting
    int currentIdx = m_sceneRenderer->getSelectedIndex();
    int objectCount = static_cast<int>(m_sceneRenderer->getObjects().size());
    
    if (objectCount > 0) {
        // Simple cycling selection for now
        int newIdx = (currentIdx + 1) % objectCount;
        m_sceneRenderer->selectObject(newIdx);
    }
    
    (void)ndcX;
    (void)ndcY;
}

void ViewportPanel::update(float dt) {
    // Update FPS
    static float fpsTimer = 0.0f;
    static int frameCount = 0;
    
    fpsTimer += dt;
    frameCount++;
    
    if (fpsTimer >= 0.5f) {
        m_fps = frameCount / fpsTimer;
        frameCount = 0;
        fpsTimer = 0.0f;
    }
}

} // namespace editor
