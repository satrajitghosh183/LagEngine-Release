#include "ViewportPanel.hpp"
#include "../../Engine/Graphics/RenderCommand.hpp"
#include "../../Engine/Graphics/Renderer3D.hpp"
#include "../../Engine/Graphics/RenderPath.hpp"
#include "../../Engine/Graphics/Vulkan/VulkanDevice.hpp"
#include "../../Engine/Core/Application.hpp"
#include "../../Engine/Scene/Components/TransformComponent.hpp"
#include "../../Engine/Scene/Components/MeshRendererComponent.hpp"
#include "../../Engine/Scene/Components/LightComponent.hpp"
#include "../../Engine/Scene/Components/CameraComponent.hpp"
#include "../../Engine/Utilities/Debug/DebugDraw.hpp"
#include "../../Engine/Core/Logger.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_vulkan.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

namespace GameEngine {

    // ==================== EditorCamera Implementation ====================

    EditorCamera::EditorCamera() {
        RecalculateProjectionMatrix();
        RecalculateViewMatrix();
    }

    void EditorCamera::OnUpdate(float deltaTime) {
        // Camera updates handled through explicit calls to Orbit/Pan/Zoom
    }

    void EditorCamera::Orbit(float deltaX, float deltaY) {
        m_Yaw += deltaX * 0.15f;
        m_Pitch += deltaY * 0.15f;

        // Clamp pitch to avoid gimbal lock
        m_Pitch = glm::clamp(m_Pitch, -89.0f, 89.0f);

        RecalculateViewMatrix();
    }

    void EditorCamera::Pan(float deltaX, float deltaY) {
        float panSpeed = 0.003f * m_Distance;

        glm::vec3 right = GetRight();
        glm::vec3 up = GetUp();

        m_FocalPoint -= right * deltaX * panSpeed;
        m_FocalPoint += up * deltaY * panSpeed;

        RecalculateViewMatrix();
    }

    void EditorCamera::Zoom(float delta) {
        // Proportional zoom: moves faster when far, slower when close
        float zoomSpeed = m_Distance * 0.1f;
        m_Distance -= delta * zoomSpeed;
        m_Distance = glm::clamp(m_Distance, 0.5f, 500.0f);
        RecalculateViewMatrix();
    }

    void EditorCamera::Fly(float forward, float right, float up) {
        // Move the focal point (and thus the whole camera) in view-space directions
        m_FocalPoint += GetForward() * forward;
        m_FocalPoint += GetRight()   * right;
        m_FocalPoint += glm::vec3(0.0f, 1.0f, 0.0f) * up;
        RecalculateViewMatrix();
    }

    void EditorCamera::FocusOn(const glm::vec3& position) {
        m_FocalPoint = position;
        m_Distance = 5.0f;
        RecalculateViewMatrix();
    }

    void EditorCamera::Reset() {
        m_FocalPoint = glm::vec3(0.0f);
        m_Distance = 10.0f;
        m_Pitch = 30.0f;
        m_Yaw = 45.0f;
        RecalculateViewMatrix();
    }

    glm::vec3 EditorCamera::GetForward() const {
        float pitchRad = glm::radians(m_Pitch);
        float yawRad = glm::radians(m_Yaw);
        
        return glm::normalize(glm::vec3(
            cos(pitchRad) * sin(yawRad),
            -sin(pitchRad),
            cos(pitchRad) * cos(yawRad)
        ));
    }

    glm::vec3 EditorCamera::GetRight() const {
        float yawRad = glm::radians(m_Yaw);
        return glm::normalize(glm::vec3(cos(yawRad), 0.0f, -sin(yawRad)));
    }

    glm::vec3 EditorCamera::GetUp() const {
        return glm::normalize(glm::cross(GetRight(), GetForward()));
    }

    void EditorCamera::SetFOV(float fov) {
        m_FOV = fov;
        RecalculateProjectionMatrix();
    }

    void EditorCamera::SetViewportSize(float width, float height) {
        if (width > 0 && height > 0) {
            m_ViewportWidth = width;
            m_ViewportHeight = height;
            m_AspectRatio = width / height;
            RecalculateProjectionMatrix();
        }
    }

    void EditorCamera::RecalculateViewMatrix() {
        m_Position = CalculatePosition();
        
        glm::vec3 direction = glm::normalize(m_FocalPoint - m_Position);
        glm::vec3 right = glm::normalize(glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 up = glm::normalize(glm::cross(right, direction));
        
        m_ViewMatrix = glm::lookAt(m_Position, m_FocalPoint, up);
    }

    void EditorCamera::RecalculateProjectionMatrix() {
        m_ProjectionMatrix = glm::perspective(
            glm::radians(m_FOV),
            m_AspectRatio,
            m_NearClip,
            m_FarClip
        );
    }

    glm::vec3 EditorCamera::CalculatePosition() const {
        float pitchRad = glm::radians(m_Pitch);
        float yawRad = glm::radians(m_Yaw);
        
        glm::vec3 offset(
            m_Distance * cos(pitchRad) * sin(yawRad),
            m_Distance * sin(pitchRad),
            m_Distance * cos(pitchRad) * cos(yawRad)
        );
        
        return m_FocalPoint + offset;
    }

    // ==================== ViewportPanel Implementation ====================

    ViewportPanel::ViewportPanel() {
        FramebufferSpec spec;
        spec.Width = 1280;
        spec.Height = 720;
        spec.ColorAttachments.push_back({ FramebufferAttachmentFormat::RGBA8 });
        spec.DepthAttachment = { FramebufferAttachmentFormat::Depth32F };
        m_Framebuffer = CreateScope<Framebuffer>(spec);

        // Small framebuffer for the camera-preview PIP (320×180 = 16:9)
        FramebufferSpec previewSpec;
        previewSpec.Width = 320;
        previewSpec.Height = 180;
        previewSpec.ColorAttachments.push_back({ FramebufferAttachmentFormat::RGBA8 });
        previewSpec.DepthAttachment = { FramebufferAttachmentFormat::Depth32F };
        m_CameraPreviewFramebuffer = CreateScope<Framebuffer>(previewSpec);
    }

    void ViewportPanel::OnUpdate(float deltaTime) {
        m_EditorCamera.OnUpdate(deltaTime);
        
        // Handle camera input when viewport is focused
        if (m_ViewportFocused || m_ViewportHovered) {
            HandleInput();
        }
    }

    void ViewportPanel::OnImGuiRender() {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        
        // Use NoMove flag when gizmo is being used to prevent window from being dragged
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_None;
        if (m_TransformGizmo.IsUsing()) {
            windowFlags |= ImGuiWindowFlags_NoMove;
        }
        
        ImGui::Begin("Viewport", nullptr, windowFlags);
        
        auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
        auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
        auto viewportOffset = ImGui::GetWindowPos();
        m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
        m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };
        
        m_ViewportFocused = ImGui::IsWindowFocused();
        // Allow hover detection even when another ImGui item is active (e.g. a drag elsewhere)
        m_ViewportHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        
        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        float dx = std::abs(m_ViewportSize.x - viewportPanelSize.x);
        float dy = std::abs(m_ViewportSize.y - viewportPanelSize.y);
        // Only resize when difference is significant (>10 pixels) to avoid constant resize spam
        if ((dx > 10.0f || dy > 10.0f) && viewportPanelSize.x > 0 && viewportPanelSize.y > 0) {
            m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };
            m_Framebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
            // The framebuffer's image view changed — drop the cached ImGui
            // descriptor so it will be re-registered next display.
            if (m_ViewportImGuiDescriptor != VK_NULL_HANDLE) {
                ImGui_ImplVulkan_RemoveTexture(m_ViewportImGuiDescriptor);
                m_ViewportImGuiDescriptor = VK_NULL_HANDLE;
            }
            m_EditorCamera.SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
        }
        
        // Render scene to framebuffer
        RenderSceneToFramebuffer();
        
        // Display the offscreen framebuffer through an ImGui-Vulkan descriptor.
        // Created lazily; recreated when the framebuffer is resized.
        if (m_ViewportImGuiDescriptor == VK_NULL_HANDLE && m_Framebuffer) {
            VkImageView view = m_Framebuffer->GetColorAttachmentView(0);
            if (view != VK_NULL_HANDLE) {
                if (m_ViewportSampler == VK_NULL_HANDLE) {
                    VkSamplerCreateInfo sci{};
                    sci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                    sci.magFilter = VK_FILTER_LINEAR;
                    sci.minFilter = VK_FILTER_LINEAR;
                    sci.addressModeU = sci.addressModeV = sci.addressModeW =
                        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                    sci.minLod = 0.0f; sci.maxLod = 1.0f;
                    vkCreateSampler(VulkanDevice::Get().GetDevice(), &sci, nullptr, &m_ViewportSampler);
                }
                m_ViewportImGuiDescriptor = ImGui_ImplVulkan_AddTexture(
                    m_ViewportSampler, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }
        }
        if (m_ViewportImGuiDescriptor != VK_NULL_HANDLE) {
            ImGui::Image((ImTextureID)m_ViewportImGuiDescriptor,
                         ImVec2(m_ViewportSize.x, m_ViewportSize.y));
        } else {
            ImGui::Dummy(ImVec2(m_ViewportSize.x, m_ViewportSize.y));
        }

        // Accept asset drag-drop onto viewport
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH")) {
                std::string droppedPath(static_cast<const char*>(payload->Data));
                if (m_AssetDropCallback) {
                    m_AssetDropCallback(droppedPath);
                }
            }
            ImGui::EndDragDropTarget();
        }

        // Gizmo toolbar
        ImGui::SetCursorPos(ImVec2(10, 30));
        ImGui::BeginGroup();
        
        bool translateActive = (m_GizmoOperation == GizmoOperation::Translate);
        bool rotateActive = (m_GizmoOperation == GizmoOperation::Rotate);
        bool scaleActive = (m_GizmoOperation == GizmoOperation::Scale);
        
        if (translateActive) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.8f, 1.0f));
        if (ImGui::Button("T", ImVec2(25, 25))) m_GizmoOperation = GizmoOperation::Translate;
        if (translateActive) ImGui::PopStyleColor();
        
        ImGui::SameLine();
        if (rotateActive) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.8f, 1.0f));
        if (ImGui::Button("R", ImVec2(25, 25))) m_GizmoOperation = GizmoOperation::Rotate;
        if (rotateActive) ImGui::PopStyleColor();
        
        ImGui::SameLine();
        if (scaleActive) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.8f, 1.0f));
        if (ImGui::Button("S", ImVec2(25, 25))) m_GizmoOperation = GizmoOperation::Scale;
        if (scaleActive) ImGui::PopStyleColor();
        
        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();
        
        if (ImGui::Button("Grid", ImVec2(40, 25))) {
            m_ShowGrid = !m_ShowGrid;
        }
        
        ImGui::EndGroup();
        
        // Top-right overlay: play mode indicator + camera help
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImGui::SetCursorPos(ImVec2(windowSize.x - 130, 30));
        ImGui::BeginGroup();
        if (m_IsPlayMode) {
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), "  PLAYING  ");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Viewport is showing the scene camera.\nStop play mode to return to editor view.");
        } else {
            ImGui::TextDisabled("Editor Mode");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Editor camera active.\nSelect a Camera entity to preview its view.");
        }
        ImGui::EndGroup();
        
        // Bottom-left: Camera controls help (compact)
        ImGui::SetCursorPos(ImVec2(10, windowSize.y - 85));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 0.8f));
        ImGui::BeginGroup();
        ImGui::TextUnformatted("Camera Controls:");
        ImGui::TextUnformatted("Alt+LMB: Orbit | Alt+MMB: Pan | Alt+RMB: Zoom");
        ImGui::TextUnformatted("RMB+WASD: Fly | MMB: Pan | Scroll: Zoom | F: Focus");
        ImGui::EndGroup();
        ImGui::PopStyleColor();

        // Bottom-right: camera preview PIP when a camera entity is selected (edit mode only)
        if (!m_IsPlayMode && m_SelectedEntity.IsValid()
                && m_SelectedEntity.HasComponent<CameraComponent>()) {
            constexpr float kPreviewW = 256.0f;
            constexpr float kPreviewH = 144.0f;  // 16:9
            constexpr float kPad      = 10.0f;

            RenderCameraPreview();

            // Overlay the preview image in the bottom-right corner
            ImGui::SetCursorPos(ImVec2(windowSize.x - kPreviewW - kPad,
                                       windowSize.y - kPreviewH - 20.0f - kPad));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.85f));
            ImGui::BeginChild("##CamPreview", ImVec2(kPreviewW, kPreviewH + 18.0f), false,
                              ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs);
            ImGui::TextUnformatted("Camera Preview");
            // Camera-preview image: register an ImGui Vulkan descriptor for
            // the preview framebuffer's color attachment.
            if (m_PreviewImGuiDescriptor == VK_NULL_HANDLE && m_CameraPreviewFramebuffer) {
                VkImageView view = m_CameraPreviewFramebuffer->GetColorAttachmentView(0);
                if (view != VK_NULL_HANDLE && m_ViewportSampler != VK_NULL_HANDLE) {
                    m_PreviewImGuiDescriptor = ImGui_ImplVulkan_AddTexture(
                        m_ViewportSampler, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                }
            }
            if (m_PreviewImGuiDescriptor != VK_NULL_HANDLE) {
                ImGui::Image((ImTextureID)m_PreviewImGuiDescriptor,
                             ImVec2(kPreviewW, kPreviewH));
            } else {
                ImGui::Dummy(ImVec2(kPreviewW, kPreviewH));
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void ViewportPanel::HandleInput() {
        ImGuiIO& io = ImGui::GetIO();

        if (!m_ViewportHovered) return;

        // Use ImGui's pre-computed per-frame mouse delta — avoids huge jump when
        // the mouse first enters the viewport (no initialisation of m_LastMousePos needed)
        glm::vec2 delta(io.MouseDelta.x, io.MouseDelta.y);
        glm::vec2 viewportMousePos = glm::vec2(io.MousePos.x, io.MousePos.y) - m_ViewportBounds[0];

        bool leftDown   = ImGui::IsMouseDown(ImGuiMouseButton_Left);
        bool rightDown  = ImGui::IsMouseDown(ImGuiMouseButton_Right);
        bool middleDown = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
        bool altDown    = io.KeyAlt;

        // ---- Gizmo interaction (disabled while camera controls are active) ----
        bool gizmoUsed = false;
        bool cameraActive = rightDown || (altDown && (leftDown || rightDown || middleDown));
        
        if (!cameraActive && m_SelectedEntity.IsValid() && m_GizmoOperation != GizmoOperation::None) {
            m_TransformGizmo.SetTarget(m_SelectedEntity);
            switch (m_GizmoOperation) {
                case GizmoOperation::Translate:
                    m_TransformGizmo.SetType(GizmoType::Translate); break;
                case GizmoOperation::Rotate:
                    m_TransformGizmo.SetType(GizmoType::Rotate); break;
                case GizmoOperation::Scale:
                    m_TransformGizmo.SetType(GizmoType::Scale); break;
                default: break;
            }
            m_TransformGizmo.SetMode(m_GizmoMode);

            if (m_SelectedEntity.HasComponent<TransformComponent>()) {
                glm::vec3 entityPos = m_SelectedEntity.GetComponent<TransformComponent>().GetWorldPosition();
                float dist = glm::length(m_EditorCamera.GetPosition() - entityPos);
                m_TransformGizmo.SetSize(dist * 0.15f);
            }

            gizmoUsed = m_TransformGizmo.Update(
                m_EditorCamera.GetViewMatrix(),
                m_EditorCamera.GetProjectionMatrix(),
                m_ViewportSize,
                viewportMousePos,
                leftDown
            );
        }

        // ================ UNITY-STYLE CAMERA CONTROLS ================
        // 
        // Alt + Left Mouse:   Orbit around focus point (tumble)
        // Alt + Middle Mouse: Pan (track)
        // Alt + Right Mouse:  Zoom (dolly)
        // Middle Mouse:       Pan
        // Right Mouse:        FPS-style look + WASD fly
        // Scroll Wheel:       Zoom
        // F:                  Frame/focus selected object
        // ============================================================

        if (!gizmoUsed) {
            bool hasDelta = (delta.x != 0.0f || delta.y != 0.0f);

            // --- Alt + Mouse combinations (Maya/Unity style) ---
            if (altDown) {
                if (leftDown && hasDelta) {
                    // Alt + Left Mouse: ORBIT (tumble around focus point)
                    m_EditorCamera.Orbit(delta.x, delta.y);
                }
                else if (middleDown && hasDelta) {
                    // Alt + Middle Mouse: PAN (track)
                    m_EditorCamera.Pan(delta.x, delta.y);
                }
                else if (rightDown && hasDelta) {
                    // Alt + Right Mouse: ZOOM (dolly) - vertical drag zooms
                    float zoomDelta = -delta.y * 0.02f;
                    m_EditorCamera.Zoom(zoomDelta);
                }
            }
            // --- Non-Alt controls ---
            else {
                if (rightDown) {
                    // RIGHT MOUSE held → free-fly mode (Unity/Unreal style)
                    // Mouse look (rotate view)
                    if (hasDelta) {
                        m_EditorCamera.Orbit(delta.x, delta.y);
                    }

                    // WASD + Q/E to fly through the scene.
                    // Speed scales with current camera distance so it stays intuitive
                    // at any scale; Shift gives a 4x turbo boost.
                    float speed = m_EditorCamera.GetDistance() * 0.9f * io.DeltaTime;
                    if (io.KeyShift) speed *= 4.0f;

                    if (ImGui::IsKeyDown(ImGuiKey_W) || ImGui::IsKeyDown(ImGuiKey_UpArrow))
                        m_EditorCamera.Fly( speed,    0.0f, 0.0f);
                    if (ImGui::IsKeyDown(ImGuiKey_S) || ImGui::IsKeyDown(ImGuiKey_DownArrow))
                        m_EditorCamera.Fly(-speed,    0.0f, 0.0f);
                    if (ImGui::IsKeyDown(ImGuiKey_A) || ImGui::IsKeyDown(ImGuiKey_LeftArrow))
                        m_EditorCamera.Fly( 0.0f,   -speed, 0.0f);
                    if (ImGui::IsKeyDown(ImGuiKey_D) || ImGui::IsKeyDown(ImGuiKey_RightArrow))
                        m_EditorCamera.Fly( 0.0f,    speed, 0.0f);
                    if (ImGui::IsKeyDown(ImGuiKey_E))
                        m_EditorCamera.Fly( 0.0f,    0.0f,  speed);
                    if (ImGui::IsKeyDown(ImGuiKey_Q))
                        m_EditorCamera.Fly( 0.0f,    0.0f, -speed);
                }

                // MIDDLE mouse → pan (without Alt)
                if (middleDown && hasDelta) {
                    m_EditorCamera.Pan(delta.x, delta.y);
                }

                // LEFT click → pick entity (only when not dragging and no Alt)
                if (!rightDown
                    && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
                    && !ImGui::IsMouseDragging(ImGuiMouseButton_Left)
                    && !m_TransformGizmo.IsHovered()) {
                    PickEntity(viewportMousePos);
                }
            }
        }

        // Scroll wheel → zoom (always works, regardless of other buttons)
        if (io.MouseWheel != 0.0f) {
            m_EditorCamera.Zoom(io.MouseWheel);
        }

        // ---- Keyboard shortcuts (only when NOT in fly mode to avoid conflicts) ----
        if (!rightDown && !io.WantCaptureKeyboard) {
            if (ImGui::IsKeyPressed(ImGuiKey_W)) m_GizmoOperation = GizmoOperation::Translate;
            if (ImGui::IsKeyPressed(ImGuiKey_E)) m_GizmoOperation = GizmoOperation::Rotate;
            if (ImGui::IsKeyPressed(ImGuiKey_R)) m_GizmoOperation = GizmoOperation::Scale;
            if (ImGui::IsKeyPressed(ImGuiKey_Q)) m_GizmoOperation = GizmoOperation::None;

            if (ImGui::IsKeyPressed(ImGuiKey_X)) {
                m_GizmoMode = (m_GizmoMode == GizmoMode::World) ? GizmoMode::Local : GizmoMode::World;
            }

            // F → frame / focus on selected entity
            if (ImGui::IsKeyPressed(ImGuiKey_F)
                && m_SelectedEntity.IsValid()
                && m_SelectedEntity.HasComponent<TransformComponent>()) {
                glm::vec3 pos = m_SelectedEntity.GetComponent<TransformComponent>().GetWorldPosition();
                m_EditorCamera.FocusOn(pos);
            }
            
            // Home → reset camera to default view
            if (ImGui::IsKeyPressed(ImGuiKey_Home)) {
                m_EditorCamera.Reset();
            }
            
            // Numpad views (like Blender/Unity)
            if (ImGui::IsKeyPressed(ImGuiKey_Keypad1)) {
                // Front view
                m_EditorCamera.SetYaw(0.0f);
                m_EditorCamera.SetPitch(0.0f);
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Keypad3)) {
                // Right view
                m_EditorCamera.SetYaw(90.0f);
                m_EditorCamera.SetPitch(0.0f);
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Keypad7)) {
                // Top view
                m_EditorCamera.SetYaw(0.0f);
                m_EditorCamera.SetPitch(89.0f);
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Keypad5)) {
                // Toggle perspective/orthographic (not implemented yet, just a placeholder)
                GE_CORE_INFO("Orthographic toggle not yet implemented");
            }
        }
    }

    void ViewportPanel::SetupRenderGraph() {
        m_RenderGraph.Clear();
        m_ShadowMapManager = CreateScope<ShadowMapManager>(4);

        // Shadow pass: renders scene depth from light perspective
        m_RenderGraph.AddPass("ShadowPass")
            .SetExecuteFunc([this]() {
                // Shadow pass — record into the active command buffer.
                VkCommandBuffer cmd = RenderCommand::GetCommandBuffer();
                if (cmd == VK_NULL_HANDLE) return;

                auto& lights = Renderer3D::GetLights();
                int shadowIndex = 0;
                for (size_t i = 0; i < lights.size() && shadowIndex < 4; i++) {
                    const auto& light = lights[i];
                    if (light.Properties.Type == LightType::Directional ||
                        light.Properties.Type == LightType::Spot) {
                        // The Vulkan ShadowMapManager API has been moved to
                        // a render-pass-based flow handled by RenderPath3D.
                        // The editor delegates shadow rendering there.
                        shadowIndex++;
                    }
                }
            });

        // Forward pass: renders scene with lighting + shadows
        m_RenderGraph.AddPass("ForwardPass")
            .AddDependency("ShadowPass")
            .SetExecuteFunc([this]() {
                // Shadow-map descriptor binding is handled by the active
                // render path under Vulkan. The editor render graph just
                // delegates to Renderer3D::EndScene which records draw
                // commands to the active command buffer.
                Renderer3D::EndScene();
            });

        // Overlay pass: grid + gizmos + selection outline (editor-only, skipped in play mode)
        m_RenderGraph.AddPass("OverlayPass")
            .AddDependency("ForwardPass")
            .SetExecuteFunc([this]() {
                if (m_IsPlayMode) return;

                // Pass 1: Grid + gizmos (with depth testing)
                if (m_ShowGrid) {
                    DrawGrid();
                }
                // Camera frustum / body gizmos for every camera in the scene
                DrawCameraGizmos();
                if (m_SelectedEntity.IsValid()) {
                    RenderGizmos();
                }
                // Record debug-draw lines into the active command buffer.
                if (VkCommandBuffer cmd = RenderCommand::GetCommandBuffer()) {
                    DebugDraw::Render(cmd, m_EditorCamera.GetViewProjectionMatrix());
                }
                DebugDraw::FlushSingleFrame();

                // Pass 2: Selection outline (no depth testing, visible through objects)
                if (m_SelectedEntity.IsValid()) {
                    DrawSelectionOutline();
                }
                DebugDraw::SetDepthTest(false);
                if (VkCommandBuffer cmd = RenderCommand::GetCommandBuffer()) {
                    DebugDraw::Render(cmd, m_EditorCamera.GetViewProjectionMatrix());
                }
                DebugDraw::FlushSingleFrame();

                DebugDraw::SetDepthTest(true);
            });
    }

    void ViewportPanel::RenderSceneToFramebuffer() {
        // Begin the offscreen viewport render pass on the active command
        // buffer if available. Skip silently when no cmd is active —
        // the engine's render path will pick up the queued scene state.
        VkCommandBuffer cmd = RenderCommand::GetCommandBuffer();
        if (cmd != VK_NULL_HANDLE && m_Framebuffer) {
            std::vector<VkClearValue> clears(2);
            clears[0].color = { {0.15f, 0.15f, 0.18f, 1.0f} };
            clears[1].depthStencil = { 1.0f, 0 };
            m_Framebuffer->BeginRenderPass(cmd, clears);
        }

        RenderCommand::SetViewport(0, 0,
            (uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
        RenderCommand::SetClearColor({0.15f, 0.15f, 0.18f, 1.0f});
        RenderCommand::Clear();
        RenderCommand::SetDepthTest(true);

        // Use active render path if set, otherwise render directly
        auto& app = Application::Get();
        auto activePath = app.GetActiveRenderPath();
        
        if (activePath && m_Context) {
            // Set scene on RenderPath3D if needed
            auto path3D = std::dynamic_pointer_cast<RenderPath3D>(activePath);
            if (path3D && !path3D->GetScene()) {
                path3D->SetScene(m_Context);
            }
            
            // Render using render path
            activePath->Render();
            activePath->Compose();
        } else if (m_Context) {
            // Determine which camera to render from
            glm::mat4 vp = m_EditorCamera.GetViewProjectionMatrix();
            glm::vec3 camPos = m_EditorCamera.GetPosition();

            if (m_IsPlayMode) {
                FindMainCameraVP(vp, camPos);
            }

            // Update frustum from camera VP
            m_Frustum.Update(vp);

            // Begin scene (clears queue)
            Renderer3D::BeginScene(vp, camPos);

            // Set ambient light for proper PBR rendering (fallback if no IBL)
            Renderer3D::SetAmbientLight(glm::vec3(1.0f), 0.3f);

            // Submit lights
            auto lightEntities = m_Context->GetEntitiesWith<TransformComponent, LightComponent>();
            for (auto& entity : lightEntities) {
                auto& transform = entity.GetComponent<TransformComponent>();
                auto& lightComp = entity.GetComponent<LightComponent>();
                lightComp.SyncToLight();

                glm::vec3 position = transform.GetWorldPosition();
                glm::mat4 worldTf = transform.GetWorldTransform();
                glm::vec3 direction = glm::normalize(glm::vec3(worldTf * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f)));

                Renderer3D::SubmitLight(lightComp.GetLight(), position, direction);
            }

            // Submit renderable meshes with frustum culling
            auto renderEntities = m_Context->GetEntitiesWith<TransformComponent, MeshRendererComponent>();
            for (auto& entity : renderEntities) {
                auto& meshRenderer = entity.GetComponent<MeshRendererComponent>();
                auto mesh = meshRenderer.GetMesh();
                auto material = meshRenderer.GetMaterial();

                if (!mesh || !material) continue;

                auto& transform = entity.GetComponent<TransformComponent>();
                glm::mat4 worldTransform = transform.GetWorldTransform();

                // Frustum cull using mesh AABB
                if (!m_Frustum.IsAABBVisible(mesh->GetAABB(), worldTransform)) {
                    continue;
                }

                Renderer3D::Submit(mesh, material, worldTransform);
            }

            // Set up render graph if not done
            if (m_RenderGraph.GetExecutionOrder().empty()) {
                SetupRenderGraph();
            }

            // Execute render graph (shadow -> forward -> overlay)
            m_RenderGraph.Execute();
        }

        // End the offscreen render pass we began at the top of this method.
        if (cmd != VK_NULL_HANDLE && m_Framebuffer) {
            m_Framebuffer->EndRenderPass(cmd);
        }
    }

    void ViewportPanel::RenderGizmos() {
        if (!m_SelectedEntity.IsValid()) return;
        if (!m_SelectedEntity.HasComponent<TransformComponent>()) return;
        
        // Update gizmo target
        m_TransformGizmo.SetTarget(m_SelectedEntity);
        
        // Sync gizmo type with operation mode
        switch (m_GizmoOperation) {
            case GizmoOperation::Translate:
                m_TransformGizmo.SetType(GizmoType::Translate);
                break;
            case GizmoOperation::Rotate:
                m_TransformGizmo.SetType(GizmoType::Rotate);
                break;
            case GizmoOperation::Scale:
                m_TransformGizmo.SetType(GizmoType::Scale);
                break;
            default:
                break;
        }
        
        m_TransformGizmo.SetMode(m_GizmoMode);
        
        // Draw gizmo
        m_TransformGizmo.Draw(
            m_EditorCamera.GetViewMatrix(),
            m_EditorCamera.GetProjectionMatrix()
        );
    }

    void ViewportPanel::DrawGrid() {
        DebugDraw::DrawGrid(m_GridSize, m_GridDivisions, glm::vec3(0.3f, 0.3f, 0.3f));
    }

    void ViewportPanel::PickEntity(const glm::vec2& mousePos) {
        if (!m_Context) return;
        if (m_ViewportSize.x <= 0 || m_ViewportSize.y <= 0) return;

        // Convert mouse position to NDC [-1, 1]
        float ndcX = (2.0f * mousePos.x) / m_ViewportSize.x - 1.0f;
        float ndcY = 1.0f - (2.0f * mousePos.y) / m_ViewportSize.y;

        // Unproject ray from camera
        glm::mat4 invVP = glm::inverse(m_EditorCamera.GetViewProjectionMatrix());

        glm::vec4 rayClipNear(ndcX, ndcY, -1.0f, 1.0f);
        glm::vec4 rayClipFar(ndcX, ndcY, 1.0f, 1.0f);

        glm::vec4 rayWorldNear = invVP * rayClipNear;
        glm::vec4 rayWorldFar = invVP * rayClipFar;
        rayWorldNear /= rayWorldNear.w;
        rayWorldFar /= rayWorldFar.w;

        glm::vec3 rayOrigin = glm::vec3(rayWorldNear);
        glm::vec3 rayDir = glm::normalize(glm::vec3(rayWorldFar - rayWorldNear));

        // Test ray against all renderable entities' AABBs
        float closestT = std::numeric_limits<float>::max();
        Entity closestEntity;

        auto renderEntities = m_Context->GetEntitiesWith<TransformComponent, MeshRendererComponent>();
        for (auto& entity : renderEntities) {
            auto& meshRenderer = entity.GetComponent<MeshRendererComponent>();
            auto mesh = meshRenderer.GetMesh();
            if (!mesh) continue;

            auto& transform = entity.GetComponent<TransformComponent>();
            glm::mat4 worldTransform = transform.GetWorldTransform();
            glm::mat4 invWorld = glm::inverse(worldTransform);

            // Transform ray into local space
            glm::vec3 localOrigin = glm::vec3(invWorld * glm::vec4(rayOrigin, 1.0f));
            glm::vec3 localDir = glm::normalize(glm::vec3(invWorld * glm::vec4(rayDir, 0.0f)));

            // Ray-AABB intersection (slab method)
            auto aabb = mesh->GetAABB();
            glm::vec3 tMin = (aabb.Min - localOrigin) / localDir;
            glm::vec3 tMax = (aabb.Max - localOrigin) / localDir;

            glm::vec3 t1 = glm::min(tMin, tMax);
            glm::vec3 t2 = glm::max(tMin, tMax);

            float tNear = std::max({t1.x, t1.y, t1.z});
            float tFar = std::min({t2.x, t2.y, t2.z});

            if (tNear <= tFar && tFar > 0.0f) {
                float hitT = (tNear > 0.0f) ? tNear : tFar;
                if (hitT < closestT) {
                    closestT = hitT;
                    closestEntity = entity;
                }
            }
        }

        // Only deselect if no gizmo operation is active; otherwise keep current selection
        if (closestEntity.IsValid() || m_GizmoOperation == GizmoOperation::None) {
            m_SelectedEntity = closestEntity;
        }
    }

    void ViewportPanel::DrawCameraGizmos() {
        if (!m_Context) return;

        auto cameraEntities = m_Context->GetEntitiesWith<TransformComponent, CameraComponent>();
        for (auto& entity : cameraEntities) {
            auto& camComp = entity.GetComponent<CameraComponent>();
            auto& tf      = entity.GetComponent<TransformComponent>();

            bool      isSelected = (entity == m_SelectedEntity);
            glm::vec3 col        = isSelected ? glm::vec3(1.00f, 0.95f, 0.30f)   // yellow
                                              : glm::vec3(0.50f, 0.85f, 1.00f);  // light-blue

            glm::vec3 pos = tf.GetWorldPosition();
            glm::quat rot = tf.GetWorldRotation();

            // Small camera body box
            DebugDraw::DrawBox(pos, glm::vec3(0.22f, 0.14f, 0.28f), rot, col);

            // Forward direction indicator (short line from camera)
            auto cam = camComp.GetCamera();
            if (cam) {
                glm::vec3 fwd = cam->GetForward();
                DebugDraw::DrawLine(pos, pos + fwd * 0.6f, col);
                // Full frustum wireframe (near + far rectangles + 4 connecting edges)
                DebugDraw::DrawFrustum(cam->GetViewProjectionMatrix(), col);
            } else {
                // Camera3D not yet initialised — derive forward from entity transform
                glm::mat4 worldTf = tf.GetWorldTransform();
                glm::vec3 fwd = glm::normalize(glm::vec3(worldTf * glm::vec4(0.f, 0.f, -1.f, 0.f)));
                DebugDraw::DrawLine(pos, pos + fwd * 0.6f, col);
            }
        }
    }

    bool ViewportPanel::FindMainCameraVP(glm::mat4& outVP, glm::vec3& outPos) {
        if (!m_Context) return false;

        auto cameraEntities = m_Context->GetEntitiesWith<TransformComponent, CameraComponent>();

        // Pass 1: prefer the entity flagged as main camera
        for (auto& entity : cameraEntities) {
            auto& camComp = entity.GetComponent<CameraComponent>();
            if (!camComp.IsMainCamera) continue;
            auto cam = camComp.GetCamera();
            if (!cam) continue;
            if (m_ViewportSize.x > 0 && m_ViewportSize.y > 0)
                cam->SetAspectRatio(m_ViewportSize.x / m_ViewportSize.y);
            outVP  = cam->GetViewProjectionMatrix();
            outPos = cam->GetPosition();
            return true;
        }

        // Pass 2: fall back to any camera in the scene
        for (auto& entity : cameraEntities) {
            auto& camComp = entity.GetComponent<CameraComponent>();
            auto cam = camComp.GetCamera();
            if (!cam) continue;
            if (m_ViewportSize.x > 0 && m_ViewportSize.y > 0)
                cam->SetAspectRatio(m_ViewportSize.x / m_ViewportSize.y);
            outVP  = cam->GetViewProjectionMatrix();
            outPos = cam->GetPosition();
            return true;
        }

        return false;
    }

    void ViewportPanel::RenderCameraPreview() {
        if (!m_Context || !m_SelectedEntity.IsValid()) return;
        if (!m_SelectedEntity.HasComponent<CameraComponent>()) return;
        if (!m_CameraPreviewFramebuffer) return;

        auto& camComp = m_SelectedEntity.GetComponent<CameraComponent>();

        // Build view / projection from the component's Camera3D (updated each frame by OnUpdate)
        constexpr float kPreviewAspect = 320.0f / 180.0f;
        glm::mat4 viewProj;
        glm::vec3 camPos;

        auto cam = camComp.GetCamera();
        if (cam) {
            cam->SetAspectRatio(kPreviewAspect);
            viewProj = cam->GetViewProjectionMatrix();
            camPos   = cam->GetPosition();
        } else {
            // Camera3D not yet initialised — compute from entity transform directly
            if (!m_SelectedEntity.HasComponent<TransformComponent>()) return;
            auto& tf = m_SelectedEntity.GetComponent<TransformComponent>();
            Camera3D tmpCam(camComp.FOV, kPreviewAspect, camComp.NearClip, camComp.FarClip);
            tmpCam.SetFromWorldTransform(tf.GetWorldTransform());
            viewProj = tmpCam.GetViewProjectionMatrix();
            camPos   = tmpCam.GetPosition();
        }

        // Render the scene into the small preview framebuffer
        // Camera preview framebuffer activation deferred to render thread.
        RenderCommand::SetViewport(0, 0, 320, 180);
        RenderCommand::SetClearColor({0.05f, 0.05f, 0.08f, 1.0f});
        RenderCommand::Clear();
        RenderCommand::SetDepthTest(true);

        Frustum previewFrustum;
        previewFrustum.Update(viewProj);

        Renderer3D::BeginScene(viewProj, camPos);
        Renderer3D::SetAmbientLight(glm::vec3(1.0f), 0.3f);

        auto lightEntities = m_Context->GetEntitiesWith<TransformComponent, LightComponent>();
        for (auto& entity : lightEntities) {
            auto& transform = entity.GetComponent<TransformComponent>();
            auto& lightComp = entity.GetComponent<LightComponent>();
            lightComp.SyncToLight();
            glm::vec3 position  = transform.GetWorldPosition();
            glm::mat4 worldTf   = transform.GetWorldTransform();
            glm::vec3 direction = glm::normalize(glm::vec3(worldTf * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f)));
            Renderer3D::SubmitLight(lightComp.GetLight(), position, direction);
        }

        auto renderEntities = m_Context->GetEntitiesWith<TransformComponent, MeshRendererComponent>();
        for (auto& entity : renderEntities) {
            auto& meshRenderer = entity.GetComponent<MeshRendererComponent>();
            auto mesh     = meshRenderer.GetMesh();
            auto material = meshRenderer.GetMaterial();
            if (!mesh || !material) continue;
            auto& transform      = entity.GetComponent<TransformComponent>();
            glm::mat4 worldTransform = transform.GetWorldTransform();
            if (!previewFrustum.IsAABBVisible(mesh->GetAABB(), worldTransform)) continue;
            Renderer3D::Submit(mesh, material, worldTransform);
        }

        Renderer3D::EndScene();
        // (Deferred — see Bind comment above.)
    }

    void ViewportPanel::DrawSelectionOutline() {
        if (!m_SelectedEntity.IsValid()) return;
        if (!m_SelectedEntity.HasComponent<TransformComponent>()) return;

        auto& transform = m_SelectedEntity.GetComponent<TransformComponent>();
        glm::vec3 outlineColor(1.0f, 0.6f, 0.0f);  // Orange

        if (m_SelectedEntity.HasComponent<MeshRendererComponent>()) {
            auto& meshRenderer = m_SelectedEntity.GetComponent<MeshRendererComponent>();
            auto mesh = meshRenderer.GetMesh();
            if (mesh) {
                auto aabb = mesh->GetAABB();
                glm::mat4 worldTransform = transform.GetWorldTransform();

                // 8 corners of the local AABB
                glm::vec3 lc[8] = {
                    {aabb.Min.x, aabb.Min.y, aabb.Min.z},
                    {aabb.Max.x, aabb.Min.y, aabb.Min.z},
                    {aabb.Max.x, aabb.Max.y, aabb.Min.z},
                    {aabb.Min.x, aabb.Max.y, aabb.Min.z},
                    {aabb.Min.x, aabb.Min.y, aabb.Max.z},
                    {aabb.Max.x, aabb.Min.y, aabb.Max.z},
                    {aabb.Max.x, aabb.Max.y, aabb.Max.z},
                    {aabb.Min.x, aabb.Max.y, aabb.Max.z},
                };

                // Transform to world space
                glm::vec3 wc[8];
                for (int i = 0; i < 8; i++) {
                    wc[i] = glm::vec3(worldTransform * glm::vec4(lc[i], 1.0f));
                }

                // 12 edges of the oriented bounding box
                DebugDraw::DrawLine(wc[0], wc[1], outlineColor);
                DebugDraw::DrawLine(wc[1], wc[2], outlineColor);
                DebugDraw::DrawLine(wc[2], wc[3], outlineColor);
                DebugDraw::DrawLine(wc[3], wc[0], outlineColor);
                DebugDraw::DrawLine(wc[4], wc[5], outlineColor);
                DebugDraw::DrawLine(wc[5], wc[6], outlineColor);
                DebugDraw::DrawLine(wc[6], wc[7], outlineColor);
                DebugDraw::DrawLine(wc[7], wc[4], outlineColor);
                DebugDraw::DrawLine(wc[0], wc[4], outlineColor);
                DebugDraw::DrawLine(wc[1], wc[5], outlineColor);
                DebugDraw::DrawLine(wc[2], wc[6], outlineColor);
                DebugDraw::DrawLine(wc[3], wc[7], outlineColor);
                return;
            }
        }

        // Fallback: unit box at entity position
        glm::vec3 position = transform.GetWorldPosition();
        DebugDraw::DrawBox(position, glm::vec3(1.0f), transform.GetWorldRotation(), outlineColor);
    }

}
