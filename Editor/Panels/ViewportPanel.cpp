#include "ViewportPanel.hpp"
#include "../../Engine/Graphics/RenderCommand.hpp"
#include "../../Engine/Graphics/Renderer3D.hpp"
#include "../../Engine/Graphics/RenderPath.hpp"
#include "../../Engine/Core/Application.hpp"
#include "../../Engine/Scene/Components/TransformComponent.hpp"
#include "../../Engine/Scene/Components/MeshRendererComponent.hpp"
#include "../../Engine/Scene/Components/LightComponent.hpp"
#include "../../Engine/Utilities/Debug/DebugDraw.hpp"
#include "../../Engine/Core/Logger.hpp"
#include <imgui.h>
#include <imgui_internal.h>
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
        spec.HasColorAttachment = true;
        spec.HasDepthAttachment = true;
        m_Framebuffer = CreateScope<Framebuffer>(spec);
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
        m_ViewportHovered = ImGui::IsWindowHovered();
        
        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        float dx = std::abs(m_ViewportSize.x - viewportPanelSize.x);
        float dy = std::abs(m_ViewportSize.y - viewportPanelSize.y);
        // Only resize when difference is significant (>10 pixels) to avoid constant resize spam
        if ((dx > 10.0f || dy > 10.0f) && viewportPanelSize.x > 0 && viewportPanelSize.y > 0) {
            m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };
            m_Framebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
            m_EditorCamera.SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
        }
        
        // Render scene to framebuffer
        RenderSceneToFramebuffer();
        
        // Display framebuffer as image
        uint32_t textureID = m_Framebuffer->GetColorAttachment();
        ImGui::Image((ImTextureID)(uintptr_t)textureID, ImVec2(m_ViewportSize.x, m_ViewportSize.y), ImVec2(0, 1), ImVec2(1, 0));

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
        
        // RenderPath info (top-right) - Editor always uses EditorCamera rendering
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImGui::SetCursorPos(ImVec2(windowSize.x - 120, 30));
        ImGui::BeginGroup();
        
        // Show current render mode - Editor mode uses EditorCamera
        ImGui::Text("Editor Mode");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Editor uses EditorCamera for viewport rendering.\nRenderPaths are for runtime/play mode.");
        }
        
        ImGui::EndGroup();
        
        ImGui::End();
        ImGui::PopStyleVar();
    }

    void ViewportPanel::HandleInput() {
        ImGuiIO& io = ImGui::GetIO();
        
        if (!m_ViewportHovered) return;
        
        glm::vec2 mousePos(io.MousePos.x, io.MousePos.y);
        glm::vec2 delta = mousePos - m_EditorCamera.m_LastMousePos;
        m_EditorCamera.m_LastMousePos = mousePos;
        
        // Calculate mouse position relative to viewport
        glm::vec2 viewportMousePos = glm::vec2(io.MousePos.x, io.MousePos.y) - m_ViewportBounds[0];
        
        // Update gizmo interaction (left mouse button)
        bool gizmoUsed = false;
        if (m_SelectedEntity.IsValid() && m_GizmoOperation != GizmoOperation::None) {
            // Sync gizmo target, type, and mode before update
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

            // Scale gizmo based on camera distance so it stays a consistent screen size
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
                ImGui::IsMouseDown(ImGuiMouseButton_Left)
            );
        }

        // Camera controls only when not using gizmo
        if (!gizmoUsed) {
            // Right mouse button - orbit
            if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
                m_EditorCamera.Orbit(delta.x, delta.y);
            }

            // Middle mouse button - pan
            if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
                m_EditorCamera.Pan(delta.x, delta.y);
            }

            // Left click - pick entity (skip if gizmo axis is hovered)
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)
                && !ImGui::IsMouseDragging(ImGuiMouseButton_Left)
                && !m_TransformGizmo.IsHovered()) {
                PickEntity(viewportMousePos);
            }
        }
        
        // Scroll wheel - zoom
        if (io.MouseWheel != 0.0f) {
            m_EditorCamera.Zoom(io.MouseWheel);
        }
        
        // Keyboard shortcuts for gizmo modes
        if (!io.WantCaptureKeyboard) {
            if (ImGui::IsKeyPressed(ImGuiKey_W)) m_GizmoOperation = GizmoOperation::Translate;
            if (ImGui::IsKeyPressed(ImGuiKey_E)) m_GizmoOperation = GizmoOperation::Rotate;
            if (ImGui::IsKeyPressed(ImGuiKey_R)) m_GizmoOperation = GizmoOperation::Scale;
            if (ImGui::IsKeyPressed(ImGuiKey_Q)) m_GizmoOperation = GizmoOperation::None;
            
            // Toggle gizmo mode (local/world)
            if (ImGui::IsKeyPressed(ImGuiKey_X)) {
                m_GizmoMode = (m_GizmoMode == GizmoMode::World) ? GizmoMode::Local : GizmoMode::World;
            }
            
            if (ImGui::IsKeyPressed(ImGuiKey_F) && m_SelectedEntity.IsValid()) {
                // Focus on selected entity
                if (m_SelectedEntity.HasComponent<TransformComponent>()) {
                    auto& transform = m_SelectedEntity.GetComponent<TransformComponent>();
                    m_EditorCamera.FocusOn(transform.Position);
                }
            }
        }
    }

    void ViewportPanel::SetupRenderGraph() {
        m_RenderGraph.Clear();
        m_ShadowMapManager = CreateScope<ShadowMapManager>(4);

        // Shadow pass: renders scene depth from light perspective
        m_RenderGraph.AddPass("ShadowPass")
            .SetExecuteFunc([this]() {
                auto& lights = Renderer3D::GetLights();
                m_ShadowMapManager->BeginShadowPass();

                int shadowIndex = 0;
                for (size_t i = 0; i < lights.size() && shadowIndex < 4; i++) {
                    const auto& light = lights[i];
                    if (light.Properties.Type == LightType::Directional) {
                        m_ShadowMapManager->RenderDirectionalShadow(
                            shadowIndex, light.Direction,
                            glm::vec3(0.0f), 15.0f,
                            [](const glm::mat4& lsm) {
                                Renderer3D::FlushShadowPass(ShadowMap::GetDepthShader(), lsm);
                            });
                        shadowIndex++;
                    } else if (light.Properties.Type == LightType::Spot) {
                        m_ShadowMapManager->RenderSpotShadow(
                            shadowIndex, light.Position, light.Direction,
                            light.Properties.OuterConeAngle, light.Properties.Range,
                            [](const glm::mat4& lsm) {
                                Renderer3D::FlushShadowPass(ShadowMap::GetDepthShader(), lsm);
                            });
                        shadowIndex++;
                    }
                }

                m_ShadowMapManager->EndShadowPass();
            });

        // Forward pass: renders scene with lighting + shadows
        m_RenderGraph.AddPass("ForwardPass")
            .AddDependency("ShadowPass")
            .SetExecuteFunc([this]() {
                // Bind shadow maps to texture slots 4+
                m_ShadowMapManager->BindShadowMaps(4);

                // EndScene sorts and flushes the render queue
                Renderer3D::EndScene();
            });

        // Overlay pass: grid + gizmos + selection outline (drawn on top)
        m_RenderGraph.AddPass("OverlayPass")
            .AddDependency("ForwardPass")
            .SetExecuteFunc([this]() {
                // Pass 1: Grid + gizmos (with depth testing)
                if (m_ShowGrid) {
                    DrawGrid();
                }
                if (m_SelectedEntity.IsValid()) {
                    RenderGizmos();
                }
                DebugDraw::SetDepthTest(true);
                DebugDraw::Render(m_EditorCamera.GetViewProjectionMatrix());
                DebugDraw::FlushSingleFrame();

                // Pass 2: Selection outline (no depth testing, visible through objects)
                if (m_SelectedEntity.IsValid()) {
                    DrawSelectionOutline();
                }
                DebugDraw::SetDepthTest(false);
                DebugDraw::Render(m_EditorCamera.GetViewProjectionMatrix());
                DebugDraw::FlushSingleFrame();

                DebugDraw::SetDepthTest(true);
            });
    }

    void ViewportPanel::RenderSceneToFramebuffer() {
        m_Framebuffer->Bind();

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
            // Fallback to direct rendering
            // Update frustum from camera VP
            glm::mat4 vp = m_EditorCamera.GetViewProjectionMatrix();
            m_Frustum.Update(vp);

            // Begin scene (clears queue)
            Renderer3D::BeginScene(vp, m_EditorCamera.GetPosition());

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

                if (!mesh || !material || !material->GetShader()) continue;

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

        m_Framebuffer->Unbind();
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
