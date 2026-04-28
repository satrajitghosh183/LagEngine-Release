#include "ViewportPanel.hpp"
#include "../core/EditorTheme.hpp"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

namespace editor {

ViewportPanel::ViewportPanel()
    : EditorPanel("Viewport", true) {
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
    m_viewportWidth = static_cast<int>(viewportSize.x);
    m_viewportHeight = static_cast<int>(viewportSize.y);

    // Update viewport state
    m_viewportFocused = ImGui::IsWindowFocused();
    m_viewportHovered = ImGui::IsWindowHovered();

    // Render the wireframe scene preview via ImGui draw list
    renderViewport();

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

// ---- 3D projection helpers ------------------------------------------------

ImVec2 ViewportPanel::projectPoint(const glm::mat4& mvp, const glm::vec3& point,
                                    const ImVec2& origin, const ImVec2& size) const {
    glm::vec4 clip = mvp * glm::vec4(point, 1.0f);
    if (clip.w <= 0.0001f) {
        return ImVec2(-10000, -10000); // behind camera
    }
    glm::vec3 ndc = glm::vec3(clip) / clip.w;
    float sx = origin.x + (ndc.x * 0.5f + 0.5f) * size.x;
    float sy = origin.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * size.y;
    return ImVec2(sx, sy);
}

bool ViewportPanel::isPointVisible(const glm::mat4& mvp, const glm::vec3& point) const {
    glm::vec4 clip = mvp * glm::vec4(point, 1.0f);
    if (clip.w <= 0.0001f) return false;
    glm::vec3 ndc = glm::vec3(clip) / clip.w;
    return ndc.x >= -1.2f && ndc.x <= 1.2f &&
           ndc.y >= -1.2f && ndc.y <= 1.2f &&
           ndc.z >= -0.1f && ndc.z <= 1.1f;
}

void ViewportPanel::drawLine3D(ImDrawList* drawList, const glm::mat4& mvp,
                                const glm::vec3& a, const glm::vec3& b,
                                const ImVec2& origin, const ImVec2& size,
                                ImU32 color, float thickness) {
    glm::vec4 clipA = mvp * glm::vec4(a, 1.0f);
    glm::vec4 clipB = mvp * glm::vec4(b, 1.0f);
    if (clipA.w <= 0.001f && clipB.w <= 0.001f) return;

    // Simple near-plane clipping: if one point is behind camera, skip
    if (clipA.w <= 0.001f || clipB.w <= 0.001f) return;

    ImVec2 pa = projectPoint(mvp, a, origin, size);
    ImVec2 pb = projectPoint(mvp, b, origin, size);
    drawList->AddLine(pa, pb, color, thickness);
}

// ---- Wireframe drawing helpers --------------------------------------------

void ViewportPanel::drawGrid(ImDrawList* drawList, const ImVec2& origin, const ImVec2& size) {
    if (!m_sceneRenderer) return;

    EditorCamera& cam = m_sceneRenderer->getCamera();
    float aspect = size.x / size.y;
    glm::mat4 vp = cam.getProjectionMatrix(aspect) * cam.getViewMatrix();

    int gridLines = 20;
    float spacing = 1.0f;
    float halfSize = gridLines * spacing * 0.5f;

    ImU32 gridColor = IM_COL32(80, 80, 80, 100);
    ImU32 xAxisColor = IM_COL32(200, 50, 50, 180);
    ImU32 zAxisColor = IM_COL32(50, 50, 200, 180);

    for (int i = -gridLines / 2; i <= gridLines / 2; i++) {
        float pos = i * spacing;
        ImU32 colorX = (i == 0) ? zAxisColor : gridColor;
        ImU32 colorZ = (i == 0) ? xAxisColor : gridColor;
        float thick = (i == 0) ? 2.0f : 1.0f;

        // Lines parallel to X axis
        drawLine3D(drawList, vp,
                   glm::vec3(-halfSize, 0.0f, pos),
                   glm::vec3(halfSize, 0.0f, pos),
                   origin, size, colorX, thick);

        // Lines parallel to Z axis
        drawLine3D(drawList, vp,
                   glm::vec3(pos, 0.0f, -halfSize),
                   glm::vec3(pos, 0.0f, halfSize),
                   origin, size, colorZ, thick);
    }
}

void ViewportPanel::drawWireframeCube(ImDrawList* drawList, const ImVec2& origin, const ImVec2& size,
                                       const glm::mat4& mvp, const glm::vec3& halfExtents, ImU32 color) {
    float hx = halfExtents.x, hy = halfExtents.y, hz = halfExtents.z;

    // 8 corners of the cube
    glm::vec3 corners[8] = {
        {-hx, -hy, -hz}, { hx, -hy, -hz}, { hx,  hy, -hz}, {-hx,  hy, -hz},
        {-hx, -hy,  hz}, { hx, -hy,  hz}, { hx,  hy,  hz}, {-hx,  hy,  hz}
    };

    // 12 edges
    int edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0},  // back face
        {4,5},{5,6},{6,7},{7,4},  // front face
        {0,4},{1,5},{2,6},{3,7}   // connecting edges
    };

    for (auto& e : edges) {
        drawLine3D(drawList, mvp, corners[e[0]], corners[e[1]], origin, size, color, 1.5f);
    }
}

void ViewportPanel::drawWireframeSphere(ImDrawList* drawList, const ImVec2& origin, const ImVec2& size,
                                         const glm::mat4& mvp, float radius, ImU32 color) {
    const int segments = 24;
    const float pi = 3.14159265f;

    // Draw 3 circles (XY, XZ, YZ planes)
    auto drawCircle = [&](int axis0, int axis1, int axis2) {
        for (int i = 0; i < segments; i++) {
            float a0 = (2.0f * pi * i) / segments;
            float a1 = (2.0f * pi * (i + 1)) / segments;

            glm::vec3 p0(0), p1(0);
            p0[axis0] = cos(a0) * radius;
            p0[axis1] = sin(a0) * radius;
            p0[axis2] = 0.0f;

            p1[axis0] = cos(a1) * radius;
            p1[axis1] = sin(a1) * radius;
            p1[axis2] = 0.0f;

            drawLine3D(drawList, mvp, p0, p1, origin, size, color, 1.0f);
        }
    };

    drawCircle(0, 1, 2); // XY
    drawCircle(0, 2, 1); // XZ
    drawCircle(1, 2, 0); // YZ
}

void ViewportPanel::drawWireframePlane(ImDrawList* drawList, const ImVec2& origin, const ImVec2& size,
                                        const glm::mat4& mvp, const glm::vec3& halfExtents, ImU32 color) {
    float hx = halfExtents.x, hz = halfExtents.z;

    glm::vec3 corners[4] = {
        {-hx, 0, -hz}, { hx, 0, -hz},
        { hx, 0,  hz}, {-hx, 0,  hz}
    };

    // 4 edges + 2 diagonals
    drawLine3D(drawList, mvp, corners[0], corners[1], origin, size, color, 1.5f);
    drawLine3D(drawList, mvp, corners[1], corners[2], origin, size, color, 1.5f);
    drawLine3D(drawList, mvp, corners[2], corners[3], origin, size, color, 1.5f);
    drawLine3D(drawList, mvp, corners[3], corners[0], origin, size, color, 1.5f);
    drawLine3D(drawList, mvp, corners[0], corners[2], origin, size, color & 0x80FFFFFF, 1.0f);
    drawLine3D(drawList, mvp, corners[1], corners[3], origin, size, color & 0x80FFFFFF, 1.0f);
}

// ---- Viewport rendering ---------------------------------------------------

void ViewportPanel::renderViewport() {
    ImVec2 vpSize = ImGui::GetContentRegionAvail();
    ImVec2 vpPos = ImGui::GetCursorScreenPos();

    // Draw background
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(vpPos,
                            ImVec2(vpPos.x + vpSize.x, vpPos.y + vpSize.y),
                            IM_COL32(15, 15, 20, 255));

    // Clipping rectangle for the viewport area
    drawList->PushClipRect(vpPos, ImVec2(vpPos.x + vpSize.x, vpPos.y + vpSize.y), true);

    // Draw grid
    if (m_showGrid) {
        drawGrid(drawList, vpPos, vpSize);
    }

    // Draw scene objects
    if (m_sceneRenderer) {
        EditorCamera& cam = m_sceneRenderer->getCamera();
        float aspect = vpSize.x / vpSize.y;
        glm::mat4 vp = cam.getProjectionMatrix(aspect) * cam.getViewMatrix();

        const auto& objects = m_sceneRenderer->getObjects();
        m_drawCalls = 0;
        m_triangles = 0;

        for (size_t i = 0; i < objects.size(); i++) {
            const auto& obj = objects[i];
            if (!obj.visible) continue;

            glm::mat4 model = obj.transform.getMatrix();
            glm::mat4 mvp = vp * model;

            // Color: convert vec3 to ImU32
            ImU32 objColor = IM_COL32(
                static_cast<int>(obj.color.x * 255),
                static_cast<int>(obj.color.y * 255),
                static_cast<int>(obj.color.z * 255),
                220);

            // Brighten selected objects
            if (obj.selected) {
                objColor = IM_COL32(255, 200, 50, 255);
            }

            if (obj.type == "Cube") {
                drawWireframeCube(drawList, vpPos, vpSize, mvp, glm::vec3(0.5f), objColor);
                m_triangles += 12;
            } else if (obj.type == "Sphere") {
                drawWireframeSphere(drawList, vpPos, vpSize, mvp, 0.5f, objColor);
                m_triangles += 24;
            } else if (obj.type == "Plane") {
                drawWireframePlane(drawList, vpPos, vpSize, mvp, glm::vec3(0.5f, 0.0f, 0.5f), objColor);
                m_triangles += 2;
            }

            m_drawCalls++;
        }
    }

    drawList->PopClipRect();

    // Make the viewport area interactive (for mouse input)
    ImGui::InvisibleButton("##ViewportArea", vpSize);
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

    // Simple selection: cycle through objects or deselect
    // In a full implementation, this would do ray casting
    int currentIdx = m_sceneRenderer->getSelectedIndex();
    int objectCount = static_cast<int>(m_sceneRenderer->getObjects().size());

    if (objectCount > 0) {
        // Simple cycling selection for now
        int newIdx = (currentIdx + 1) % objectCount;
        m_sceneRenderer->selectObject(newIdx);
    }
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
