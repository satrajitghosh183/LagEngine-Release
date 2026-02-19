#include "ComponentsWindow.hpp"
#include "../Core/EditorContext.hpp"
#include "../../Engine/Scene/Components/TransformComponent.hpp"
#include "../../Engine/Scene/Components/MeshRendererComponent.hpp"
#include "../../Engine/Scene/Components/LightComponent.hpp"
#include "../../Engine/Scene/Components/CameraComponent.hpp"
#include "../../Engine/Scene/Components/RigidBodyComponent.hpp"
#include "../../Engine/Scene/Components/ColliderComponent.hpp"
#include "../../Engine/Scene/Components/ScriptComponent.hpp"
#include "../../Engine/Scene/Components/GPUParticleComponent.hpp"
#include "../../Engine/Scene/Components/RobotArmComponent.hpp"
#include "../../Engine/Scene/Components/SoftBodyComponent.hpp"
#include "../../Engine/Scene/Components/SpriteComponent.hpp"
#include "../../Engine/Scene/Components/SpriteAnimatorComponent.hpp"
#include "../../Engine/Graphics/Material.hpp"
#include "../../Engine/Graphics/Mesh3D.hpp"
#include "../../Engine/Assets/AssetDatabase.hpp"
#include "../../Engine/Assets/AssetImporter.hpp"
#include "../../Engine/Physics/RigidBody.hpp"
#include "../../Engine/Physics/Shapes/BoxShape.hpp"
#include "../../Engine/Physics/Shapes/SphereShape.hpp"
#include "../../Engine/Physics/Shapes/CapsuleShape.hpp"
#include "../UI/UIRenderer.hpp"
#include "../../Engine/Platform/FileDialog.hpp"
#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace GameEngine {

    ComponentsWindow::ComponentsWindow() {
    }

    void ComponentsWindow::OnImGuiRender() {
        ImGui::Begin("Components");

        if (!m_SelectedEntity.IsValid()) {
            ImGui::Text("No entity selected");
            ImGui::End();
            return;
        }

        // Name and Tag
        std::string name = m_SelectedEntity.GetName();
        char nameBuffer[256];
        memset(nameBuffer, 0, sizeof(nameBuffer));
        strncpy(nameBuffer, name.c_str(), sizeof(nameBuffer) - 1);
        if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
            m_SelectedEntity.SetName(std::string(nameBuffer));
        }

        std::string tag = m_SelectedEntity.GetTag();
        char tagBuffer[256];
        memset(tagBuffer, 0, sizeof(tagBuffer));
        strncpy(tagBuffer, tag.c_str(), sizeof(tagBuffer) - 1);
        if (ImGui::InputText("Tag", tagBuffer, sizeof(tagBuffer))) {
            m_SelectedEntity.SetTag(std::string(tagBuffer));
        }

        bool active = m_SelectedEntity.IsActive();
        if (ImGui::Checkbox("Active", &active)) {
            m_SelectedEntity.SetActive(active);
        }

        ImGui::Separator();

        // Render component windows
        if (m_SelectedEntity.HasComponent<TransformComponent>()) {
            RenderComponentWindow(&m_TransformWindow, m_SelectedEntity);
        }

        if (m_SelectedEntity.HasComponent<MeshRendererComponent>()) {
            RenderComponentWindow(&m_MeshWindow, m_SelectedEntity);
            RenderComponentWindow(&m_MaterialWindow, m_SelectedEntity);
        }

        if (m_SelectedEntity.HasComponent<LightComponent>()) {
            RenderComponentWindow(&m_LightWindow, m_SelectedEntity);
        }

        if (m_SelectedEntity.HasComponent<CameraComponent>()) {
            RenderComponentWindow(&m_CameraWindow, m_SelectedEntity);
        }

        if (m_SelectedEntity.HasComponent<RigidBodyComponent>()) {
            RenderComponentWindow(&m_RigidBodyWindow, m_SelectedEntity);
        }

        if (m_SelectedEntity.HasComponent<ColliderComponent>()) {
            RenderComponentWindow(&m_ColliderWindow, m_SelectedEntity);
        }

        if (m_SelectedEntity.HasComponent<ScriptComponent>()) {
            RenderComponentWindow(&m_ScriptWindow, m_SelectedEntity);
        }

        if (m_SelectedEntity.HasComponent<GPUParticleComponent>()) {
            RenderComponentWindow(&m_GPUParticleWindow, m_SelectedEntity);
        }

        if (m_SelectedEntity.HasComponent<RobotArmComponent>()) {
            RenderComponentWindow(&m_RobotArmWindow, m_SelectedEntity);
        }

        if (m_SelectedEntity.HasComponent<SoftBodyComponent>()) {
            RenderComponentWindow(&m_SoftBodyWindow, m_SelectedEntity);
        }

        if (m_SelectedEntity.HasComponent<SpriteComponent>()) {
            RenderComponentWindow(&m_SpriteWindow, m_SelectedEntity);
        }

        if (m_SelectedEntity.HasComponent<SpriteAnimatorComponent>()) {
            RenderComponentWindow(&m_SpriteAnimatorWindow, m_SelectedEntity);
        }

        ImGui::Separator();

        // Add Component button
        if (ImGui::Button("+ Add Component")) {
            ImGui::OpenPopup("AddComponent");
        }

        RenderAddComponentMenu(m_SelectedEntity);

        ImGui::End();
    }

    void ComponentsWindow::RenderComponentWindow(ComponentWindow* window, Entity entity) {
        bool open = true;
        if (ImGui::CollapsingHeader(window->GetName(), &open, ImGuiTreeNodeFlags_DefaultOpen)) {
            window->Render(entity);
        }
        
        // Handle component removal (if close button clicked)
        if (!open && window->TryRemoveComponent(entity)) {
            // Component removed
        }
    }

    void ComponentsWindow::RenderAddComponentMenu(Entity entity) {
        if (ImGui::BeginPopup("AddComponent")) {
            if (!entity.HasComponent<TransformComponent>()) {
                if (ImGui::MenuItem("Transform")) {
                    entity.AddComponent<TransformComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }

            if (!entity.HasComponent<MeshRendererComponent>()) {
                if (ImGui::MenuItem("Mesh Renderer")) {
                    entity.AddComponent<MeshRendererComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }

            if (!entity.HasComponent<CameraComponent>()) {
                if (ImGui::MenuItem("Camera")) {
                    entity.AddComponent<CameraComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }

            if (!entity.HasComponent<LightComponent>()) {
                if (ImGui::MenuItem("Light")) {
                    entity.AddComponent<LightComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }

            if (!entity.HasComponent<RigidBodyComponent>()) {
                if (ImGui::MenuItem("Rigid Body")) {
                    entity.AddComponent<RigidBodyComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }

            if (!entity.HasComponent<ColliderComponent>()) {
                if (ImGui::BeginMenu("Collider")) {
                    if (ImGui::MenuItem("Box Collider")) {
                        auto& collider = entity.AddComponent<ColliderComponent>();
                        collider.SetShape(CreateRef<Physics::BoxShape>(glm::vec3(0.5f)));
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::MenuItem("Sphere Collider")) {
                        auto& collider = entity.AddComponent<ColliderComponent>();
                        collider.SetShape(CreateRef<Physics::SphereShape>(0.5f));
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::MenuItem("Capsule Collider")) {
                        auto& collider = entity.AddComponent<ColliderComponent>();
                        collider.SetShape(CreateRef<Physics::CapsuleShape>(0.5f, 2.0f));
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndMenu();
                }
            }

            if (!entity.HasComponent<ScriptComponent>()) {
                if (ImGui::MenuItem("Script")) {
                    entity.AddComponent<ScriptComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::Separator();

            if (!entity.HasComponent<GPUParticleComponent>()) {
                if (ImGui::MenuItem("GPU Particles")) {
                    entity.AddComponent<GPUParticleComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }

            if (!entity.HasComponent<RobotArmComponent>()) {
                if (ImGui::MenuItem("Robot Arm")) {
                    entity.AddComponent<RobotArmComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }

            if (!entity.HasComponent<SoftBodyComponent>()) {
                if (ImGui::MenuItem("Soft Body (XPBD)")) {
                    entity.AddComponent<SoftBodyComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }

            if (!entity.HasComponent<SpriteComponent>()) {
                if (ImGui::MenuItem("Sprite")) {
                    entity.AddComponent<SpriteComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }

            if (!entity.HasComponent<SpriteAnimatorComponent>()) {
                if (ImGui::MenuItem("Sprite Animator")) {
                    entity.AddComponent<SpriteAnimatorComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::EndPopup();
        }
    }

    // ==================== TransformWindow ====================

    void TransformWindow::Render(Entity entity) {
        if (!entity.HasComponent<TransformComponent>()) return;

        auto& transform = entity.GetComponent<TransformComponent>();

        // Position
        if (UIRenderer::Vec3Control("Position", transform.Position)) {
            transform.SetDirty();  // Mark transform cache as dirty so GetWorldTransform() recalculates
        }

        // Rotation (as Euler angles)
        glm::vec3 rotation = glm::degrees(glm::eulerAngles(transform.Rotation));
        if (UIRenderer::Vec3Control("Rotation", rotation)) {
            transform.Rotation = glm::quat(glm::radians(rotation));
            transform.SetDirty();  // Mark transform cache as dirty
        }

        // Scale
        if (UIRenderer::Vec3Control("Scale", transform.Scale, 1.0f)) {
            transform.SetDirty();  // Mark transform cache as dirty
        }
    }

    bool TransformWindow::TryRemoveComponent(Entity entity) {
        if (entity.HasComponent<TransformComponent>()) {
            entity.RemoveComponent<TransformComponent>();
            return true;
        }
        return false;
    }

    // ==================== MaterialWindow ====================

    void MaterialWindow::Render(Entity entity) {
        if (!entity.HasComponent<MeshRendererComponent>()) return;

        auto& meshRenderer = entity.GetComponent<MeshRendererComponent>();
        auto material = meshRenderer.GetMaterial();
        
        if (!material) {
            ImGui::Text("No material assigned");
            return;
        }

        // Albedo
        glm::vec3 albedo = material->GetAlbedo();
        if (ImGui::ColorEdit3("Albedo", glm::value_ptr(albedo))) {
            material->SetAlbedo(albedo);
        }

        // Metallic
        float metallic = material->GetMetallic();
        if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f)) {
            material->SetMetallic(metallic);
        }

        // Roughness
        float roughness = material->GetRoughness();
        if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f)) {
            material->SetRoughness(roughness);
        }

        // AO
        float ao = material->GetAO();
        if (ImGui::SliderFloat("AO", &ao, 0.0f, 1.0f)) {
            material->SetAO(ao);
        }

        // Emission (via generic SetVec3 if needed - Material doesn't have dedicated emission yet)
    }

    bool MaterialWindow::TryRemoveComponent(Entity entity) {
        // Material is a sub-property of MeshRendererComponent, not a standalone component.
        // Removing it would destroy the entire MeshRenderer. Return false to prevent removal.
        return false;
    }

    // ==================== MeshWindow ====================

    void MeshWindow::Render(Entity entity) {
        if (!entity.HasComponent<MeshRendererComponent>()) return;

        auto& meshRenderer = entity.GetComponent<MeshRendererComponent>();
        auto mesh = meshRenderer.GetMesh();

        if (mesh) {
            ImGui::Text("Mesh: %s", mesh->GetName().c_str());
            ImGui::Text("Vertices: %u", mesh->GetVertexCount());
            ImGui::Text("Indices: %u", mesh->GetIndexCount());
        } else {
            ImGui::Text("No mesh assigned");
        }

        if (ImGui::Button("Select Mesh...")) {
            ImGui::OpenPopup("MeshPicker");
        }
        if (ImGui::BeginPopup("MeshPicker")) {
            const auto& all = AssetDatabase::GetAll();
            int n = 0;
            for (const auto& [guid, meta] : all) {
                if (meta.Type != AssetType::Mesh) continue;
                if (ImGui::Selectable(meta.Path.c_str())) {
                    auto result = AssetImporter::ImportMesh(meta.Path);
                    if (result.Success && !result.Meshes.empty()) {
                        meshRenderer.SetMesh(result.Meshes[0]);
                    }
                    ImGui::CloseCurrentPopup();
                    break;
                }
                n++;
            }
            if (n == 0) ImGui::Text("No meshes in asset database");
            ImGui::EndPopup();
        }
    }

    bool MeshWindow::TryRemoveComponent(Entity entity) {
        if (entity.HasComponent<MeshRendererComponent>()) {
            entity.RemoveComponent<MeshRendererComponent>();
            return true;
        }
        return false;
    }

    // ==================== LightWindow ====================

    void LightWindow::Render(Entity entity) {
        if (!entity.HasComponent<LightComponent>()) return;

        auto& light = entity.GetComponent<LightComponent>();

        // Light Type
        const char* lightTypes[] = { "Directional", "Point", "Spot" };
        int currentType = static_cast<int>(light.Type);
        if (ImGui::Combo("Type", &currentType, lightTypes, 3)) {
            light.Type = static_cast<LightType>(currentType);
            light.SyncToLight();
        }

        // Color
        glm::vec3 color = light.Color;
        if (ImGui::ColorEdit3("Color", glm::value_ptr(color))) {
            light.Color = color;
            light.SyncToLight();
        }

        // Intensity
        float intensity = light.Intensity;
        if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 10.0f)) {
            light.Intensity = intensity;
            light.SyncToLight();
        }

        // Range (for Point/Spot)
        if (light.Type == LightType::Point || light.Type == LightType::Spot) {
            float range = light.Range;
            if (ImGui::SliderFloat("Range", &range, 0.1f, 100.0f)) {
                light.Range = range;
                light.SyncToLight();
            }
        }

        // Spot light specific
        if (light.Type == LightType::Spot) {
            float innerCutoff = glm::degrees(light.InnerConeAngle);
            if (ImGui::SliderFloat("Inner Cone Angle", &innerCutoff, 0.0f, 90.0f)) {
                light.InnerConeAngle = glm::radians(innerCutoff);
                light.SyncToLight();
            }

            float outerCutoff = glm::degrees(light.OuterConeAngle);
            if (ImGui::SliderFloat("Outer Cone Angle", &outerCutoff, 0.0f, 90.0f)) {
                light.OuterConeAngle = glm::radians(outerCutoff);
                light.SyncToLight();
            }
        }

        // Cast Shadows
        bool castShadows = light.CastShadows;
        if (ImGui::Checkbox("Cast Shadows", &castShadows)) {
            light.CastShadows = castShadows;
            light.SyncToLight();
        }
    }

    bool LightWindow::TryRemoveComponent(Entity entity) {
        if (entity.HasComponent<LightComponent>()) {
            entity.RemoveComponent<LightComponent>();
            return true;
        }
        return false;
    }

    // ==================== CameraComponentWindow ====================

    void CameraComponentWindow::Render(Entity entity) {
        if (!entity.HasComponent<CameraComponent>()) return;

        auto& camera = entity.GetComponent<CameraComponent>();

        // Projection Type
        const char* projectionTypes[] = { "Perspective", "Orthographic" };
        int currentProj = static_cast<int>(camera.ProjectionType);
        if (ImGui::Combo("Projection", &currentProj, projectionTypes, 2)) {
            camera.ProjectionType = static_cast<CameraProjectionType>(currentProj);
        }

        // FOV (Perspective)
        if (camera.ProjectionType == CameraProjectionType::Perspective) {
            float fov = camera.FOV;
            if (ImGui::SliderFloat("FOV", &fov, 10.0f, 120.0f)) {
                camera.FOV = fov;
            }
        }

        // Aspect Ratio
        float aspect = camera.AspectRatio;
        if (ImGui::InputFloat("Aspect Ratio", &aspect)) {
            camera.AspectRatio = aspect;
        }

        // Near/Far Clip
        float nearClip = camera.NearClip;
        if (ImGui::InputFloat("Near Clip", &nearClip)) {
            camera.NearClip = nearClip;
        }
        float farClip = camera.FarClip;
        if (ImGui::InputFloat("Far Clip", &farClip)) {
            camera.FarClip = farClip;
        }

        // Main Camera toggle
        bool isMain = camera.IsMainCamera;
        if (ImGui::Checkbox("Main Camera", &isMain)) {
            camera.IsMainCamera = isMain;
        }
    }

    bool CameraComponentWindow::TryRemoveComponent(Entity entity) {
        if (entity.HasComponent<CameraComponent>()) {
            entity.RemoveComponent<CameraComponent>();
            return true;
        }
        return false;
    }

    // ==================== RigidBodyWindow ====================

    void RigidBodyWindow::Render(Entity entity) {
        if (!entity.HasComponent<RigidBodyComponent>()) return;

        auto& rb = entity.GetComponent<RigidBodyComponent>();

        // Edit the component's public properties (these are used when physics body is created in OnCreate)
        
        // Body Type
        const char* bodyTypes[] = { "Static", "Kinematic", "Dynamic" };
        int currentType = static_cast<int>(rb.Type);
        if (ImGui::Combo("Body Type", &currentType, bodyTypes, 3)) {
            rb.Type = static_cast<RigidBodyComponent::BodyType>(currentType);
            // Also update runtime body if it exists
            if (auto rigidBody = rb.GetRigidBody()) {
                rigidBody->SetBodyType(static_cast<Physics::RigidBody::BodyType>(currentType));
            }
        }

        // Mass
        if (ImGui::InputFloat("Mass", &rb.Mass, 0.1f, 1.0f)) {
            rb.Mass = glm::max(rb.Mass, 0.001f);  // Prevent zero/negative mass
            if (auto rigidBody = rb.GetRigidBody()) {
                rigidBody->SetMass(rb.Mass);
            }
        }

        // Linear Damping
        if (ImGui::SliderFloat("Linear Damping", &rb.LinearDamping, 0.0f, 1.0f)) {
            if (auto rigidBody = rb.GetRigidBody()) {
                rigidBody->SetLinearDamping(rb.LinearDamping);
            }
        }

        // Angular Damping
        if (ImGui::SliderFloat("Angular Damping", &rb.AngularDamping, 0.0f, 1.0f)) {
            if (auto rigidBody = rb.GetRigidBody()) {
                rigidBody->SetAngularDamping(rb.AngularDamping);
            }
        }

        // Use Gravity
        if (ImGui::Checkbox("Use Gravity", &rb.UseGravity)) {
            if (auto rigidBody = rb.GetRigidBody()) {
                rigidBody->SetUseGravity(rb.UseGravity);
            }
        }

        // Show runtime state if physics body exists (during play mode)
        auto rigidBody = rb.GetRigidBody();
        if (rigidBody) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Runtime State:");
            
            glm::vec3 linVel = rigidBody->GetLinearVelocity();
            ImGui::Text("Linear Velocity: (%.2f, %.2f, %.2f)", linVel.x, linVel.y, linVel.z);
            
            glm::vec3 angVel = rigidBody->GetAngularVelocity();
            ImGui::Text("Angular Velocity: (%.2f, %.2f, %.2f)", angVel.x, angVel.y, angVel.z);
            
            ImGui::Text("Awake: %s", rigidBody->IsAwake() ? "Yes" : "No");
        }
    }

    bool RigidBodyWindow::TryRemoveComponent(Entity entity) {
        if (entity.HasComponent<RigidBodyComponent>()) {
            entity.RemoveComponent<RigidBodyComponent>();
            return true;
        }
        return false;
    }

    // ==================== ColliderWindow ====================

    void ColliderWindow::Render(Entity entity) {
        if (!entity.HasComponent<ColliderComponent>()) return;

        auto& collider = entity.GetComponent<ColliderComponent>();
        auto shape = collider.GetShape();

        if (!shape) {
            ImGui::Text("No shape assigned");
            return;
        }

        // Shape type display
        const char* shapeType = "Unknown";
        if (dynamic_cast<Physics::BoxShape*>(shape.get())) {
            shapeType = "Box";
            auto* boxShape = static_cast<Physics::BoxShape*>(shape.get());
            glm::vec3 halfExtents = boxShape->GetHalfExtents();
            if (UIRenderer::Vec3Control("Half Extents", halfExtents)) {
                boxShape->SetHalfExtents(halfExtents);
            }
        } else if (dynamic_cast<Physics::SphereShape*>(shape.get())) {
            shapeType = "Sphere";
            auto* sphereShape = static_cast<Physics::SphereShape*>(shape.get());
            float radius = sphereShape->GetRadius();
            if (ImGui::InputFloat("Radius", &radius)) {
                sphereShape->SetRadius(radius);
            }
        } else if (dynamic_cast<Physics::CapsuleShape*>(shape.get())) {
            shapeType = "Capsule";
            auto* capsuleShape = static_cast<Physics::CapsuleShape*>(shape.get());
            float radius = capsuleShape->GetRadius();
            float height = capsuleShape->GetHeight();
            if (ImGui::InputFloat("Radius", &radius)) {
                capsuleShape->SetRadius(radius);
            }
            if (ImGui::InputFloat("Height", &height)) {
                capsuleShape->SetHeight(height);
            }
        }

        ImGui::Text("Shape Type: %s", shapeType);

        // Is Trigger
        bool isTrigger = collider.IsTrigger;
        if (ImGui::Checkbox("Is Trigger", &isTrigger)) {
            collider.IsTrigger = isTrigger;
        }
    }

    bool ColliderWindow::TryRemoveComponent(Entity entity) {
        if (entity.HasComponent<ColliderComponent>()) {
            entity.RemoveComponent<ColliderComponent>();
            return true;
        }
        return false;
    }

    // ==================== ScriptWindow ====================

    void ScriptWindow::Render(Entity entity) {
        if (!entity.HasComponent<ScriptComponent>()) return;

        auto& script = entity.GetComponent<ScriptComponent>();

        // Script path
        std::string scriptPath = script.GetScriptPath();
        char pathBuffer[512];
        memset(pathBuffer, 0, sizeof(pathBuffer));
        strncpy(pathBuffer, scriptPath.c_str(), sizeof(pathBuffer) - 1);
        
        if (ImGui::InputText("Script Path", pathBuffer, sizeof(pathBuffer))) {
            script.SetScriptPath(std::string(pathBuffer));
        }

        ImGui::SameLine();
        if (ImGui::Button("...")) {
            auto result = FileDialog::OpenFile("Select Lua Script",
                { FileFilter("Lua scripts", "*.lua") }, "");
            if (result.has_value() && !result->empty()) {
                script.SetScriptPath(*result);
            }
        }

        // Reload button
        if (ImGui::Button("Reload Script")) {
            script.ReloadScript();
        }

        // Show script status
        if (script.IsLoaded()) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Script loaded");
        } else {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Script not loaded");
        }
    }

    bool ScriptWindow::TryRemoveComponent(Entity entity) {
        if (entity.HasComponent<ScriptComponent>()) {
            entity.RemoveComponent<ScriptComponent>();
            return true;
        }
        return false;
    }

    // ==================== GPUParticleWindow ====================

    void GPUParticleWindow::Render(Entity entity) {
        if (!entity.HasComponent<GPUParticleComponent>()) return;

        auto& particles = entity.GetComponent<GPUParticleComponent>();

        ImGui::DragInt("Max Particles", &particles.MaxParticles, 1000, 1000, 1000000);
        ImGui::DragFloat3("Gravity", glm::value_ptr(particles.Gravity), 0.1f);
        ImGui::DragFloat("Restitution", &particles.Restitution, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat("Damping", &particles.Damping, 0.001f, 0.9f, 1.0f);
        ImGui::DragFloat("Emit Height", &particles.EmitHeight, 0.1f, 0.0f, 50.0f);
        ImGui::DragFloat("Floor Y", &particles.FloorY, 0.1f, -10.0f, 10.0f);
        ImGui::DragFloat("Particle Size", &particles.ParticleSize, 0.001f, 0.001f, 1.0f);
        ImGui::DragInt("Emit Rate", &particles.EmitRate, 100, 0, 100000);
    }

    bool GPUParticleWindow::TryRemoveComponent(Entity entity) {
        if (entity.HasComponent<GPUParticleComponent>()) {
            entity.RemoveComponent<GPUParticleComponent>();
            return true;
        }
        return false;
    }

    // ==================== RobotArmWindow ====================

    void RobotArmWindow::Render(Entity entity) {
        if (!entity.HasComponent<RobotArmComponent>()) return;

        auto& arm = entity.GetComponent<RobotArmComponent>();

        // Joint angles
        if (ImGui::CollapsingHeader("Joint Angles", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (size_t i = 0; i < arm.JointAngles.size(); i++) {
                std::string label = "Joint " + std::to_string(i);
                float degrees = glm::degrees(arm.JointAngles[i]);
                if (ImGui::SliderFloat(label.c_str(), &degrees, -180.0f, 180.0f)) {
                    arm.JointAngles[i] = glm::radians(degrees);
                }
            }
        }

        // IK settings
        ImGui::Checkbox("IK Enabled", &arm.IKEnabled);
        if (arm.IKEnabled) {
            ImGui::DragFloat3("Target Position", glm::value_ptr(arm.TargetPosition), 0.01f);
            ImGui::DragFloat("IK Tolerance", &arm.IKTolerance, 0.0001f, 0.0001f, 0.1f, "%.4f");
            ImGui::DragInt("IK Max Iterations", &arm.IKMaxIterations, 1, 1, 1000);
        }

        // PID gains
        if (ImGui::CollapsingHeader("PID Control")) {
            ImGui::DragFloat("Kp", &arm.Kp, 0.01f, 0.0f, 20.0f);
            ImGui::DragFloat("Ki", &arm.Ki, 0.001f, 0.0f, 5.0f);
            ImGui::DragFloat("Kd", &arm.Kd, 0.01f, 0.0f, 10.0f);
        }

        // DH Parameters
        if (ImGui::CollapsingHeader("DH Parameters")) {
            for (size_t i = 0; i < arm.DHParams.size(); i++) {
                ImGui::PushID(static_cast<int>(i));
                std::string header = "Link " + std::to_string(i);
                if (ImGui::TreeNode(header.c_str())) {
                    ImGui::DragFloat("a (link length)", &arm.DHParams[i].a, 0.01f);
                    ImGui::DragFloat("d (link offset)", &arm.DHParams[i].d, 0.01f);
                    float alpha_deg = glm::degrees(arm.DHParams[i].alpha);
                    if (ImGui::DragFloat("alpha", &alpha_deg, 0.5f)) {
                        arm.DHParams[i].alpha = glm::radians(alpha_deg);
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
        }

        // End effector info
        glm::vec3 eePos = arm.GetEndEffectorPosition();
        ImGui::Text("End Effector: (%.3f, %.3f, %.3f)", eePos.x, eePos.y, eePos.z);
    }

    bool RobotArmWindow::TryRemoveComponent(Entity entity) {
        if (entity.HasComponent<RobotArmComponent>()) {
            entity.RemoveComponent<RobotArmComponent>();
            return true;
        }
        return false;
    }

    // ==================== SoftBodyWindow ====================

    void SoftBodyWindow::Render(Entity entity) {
        if (!entity.HasComponent<SoftBodyComponent>()) return;

        auto& softBody = entity.GetComponent<SoftBodyComponent>();

        ImGui::DragFloat("Mass", &softBody.Mass, 0.1f, 0.01f, 100.0f);
        ImGui::DragFloat("Damping", &softBody.Damping, 0.001f, 0.9f, 1.0f);
        ImGui::DragInt("Sub Steps", &softBody.SubSteps, 1, 1, 32);

        ImGui::Separator();
        ImGui::Text("Constraints");
        ImGui::Checkbox("Distance", &softBody.EnableDistanceConstraints);
        ImGui::Checkbox("Bending", &softBody.EnableBendConstraints);
        ImGui::Checkbox("Volume", &softBody.EnableVolumeConstraints);
        ImGui::Checkbox("Collision", &softBody.EnableCollisionConstraints);

        if (softBody.EnableDistanceConstraints) {
            ImGui::DragFloat("Distance Compliance", &softBody.DistanceCompliance, 0.0001f, 0.0f, 0.1f, "%.4f");
        }
        if (softBody.EnableBendConstraints) {
            ImGui::DragFloat("Bend Compliance", &softBody.BendCompliance, 0.0001f, 0.0f, 0.1f, "%.4f");
        }

        ImGui::Separator();
        ImGui::Text("Grid");
        ImGui::DragInt("Res X", &softBody.GridResX, 1, 2, 100);
        ImGui::DragInt("Res Y", &softBody.GridResY, 1, 2, 100);
        ImGui::DragFloat("Width", &softBody.Width, 0.1f, 0.1f, 50.0f);
        ImGui::DragFloat("Height", &softBody.Height, 0.1f, 0.1f, 50.0f);

        ImGui::Checkbox("Simulating", &softBody.IsSimulating);
    }

    bool SoftBodyWindow::TryRemoveComponent(Entity entity) {
        if (entity.HasComponent<SoftBodyComponent>()) {
            entity.RemoveComponent<SoftBodyComponent>();
            return true;
        }
        return false;
    }

    // ==================== SpriteWindow ====================

    void SpriteWindow::Render(Entity entity) {
        if (!entity.HasComponent<SpriteComponent>()) return;

        auto& sprite = entity.GetComponent<SpriteComponent>();

        ImGui::ColorEdit4("Color", glm::value_ptr(sprite.Color));

        char texPathBuffer[512];
        memset(texPathBuffer, 0, sizeof(texPathBuffer));
        strncpy(texPathBuffer, sprite.TexturePath.c_str(), sizeof(texPathBuffer) - 1);
        if (ImGui::InputText("Texture Path", texPathBuffer, sizeof(texPathBuffer))) {
            sprite.TexturePath = std::string(texPathBuffer);
        }

        ImGui::DragFloat2("UV Min", glm::value_ptr(sprite.UVMin), 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat2("UV Max", glm::value_ptr(sprite.UVMax), 0.01f, 0.0f, 1.0f);
        ImGui::DragInt("Sort Order", &sprite.SortOrder);
    }

    bool SpriteWindow::TryRemoveComponent(Entity entity) {
        if (entity.HasComponent<SpriteComponent>()) {
            entity.RemoveComponent<SpriteComponent>();
            return true;
        }
        return false;
    }

    // ==================== SpriteAnimatorWindow ====================

    void SpriteAnimatorWindow::Render(Entity entity) {
        if (!entity.HasComponent<SpriteAnimatorComponent>()) return;

        auto& animator = entity.GetComponent<SpriteAnimatorComponent>();

        ImGui::DragInt("Frame Columns", &animator.FrameColumns, 1, 1, 64);
        ImGui::DragInt("Frame Rows", &animator.FrameRows, 1, 1, 64);
        ImGui::DragInt("Total Frames", &animator.TotalFrames, 1, 1, 4096);
        ImGui::DragFloat("Frame Duration", &animator.FrameDuration, 0.01f, 0.01f, 5.0f);
        ImGui::Checkbox("Looping", &animator.Looping);

        if (ImGui::Checkbox("Playing", &animator.Playing)) {
            auto& anim = animator.GetAnimation();
            if (animator.Playing) anim.Play();
            else anim.Stop();
        }

        // Current frame info
        auto& anim = animator.GetAnimation();
        ImGui::Text("Current Frame: %d / %d", anim.GetCurrentFrameIndex(), animator.TotalFrames);
    }

    bool SpriteAnimatorWindow::TryRemoveComponent(Entity entity) {
        if (entity.HasComponent<SpriteAnimatorComponent>()) {
            entity.RemoveComponent<SpriteAnimatorComponent>();
            return true;
        }
        return false;
    }
}
