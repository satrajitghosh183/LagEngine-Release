#include "SceneHierarchyPanel.hpp"
#include "../../Engine/Scene/Components/TransformComponent.hpp"
#include "../../Engine/Scene/Components/MeshRendererComponent.hpp"
#include "../../Engine/Scene/Components/CameraComponent.hpp"
#include "../../Engine/Scene/Components/LightComponent.hpp"
#include "../../Engine/Scene/Components/RigidBodyComponent.hpp"
#include "../../Engine/Scene/Components/ColliderComponent.hpp"
#include "../../Engine/Scene/Components/ScriptComponent.hpp"
#include "../../Engine/Physics/Shapes/SphereShape.hpp"
#include "../../Engine/Physics/Shapes/BoxShape.hpp"
#include "../../Engine/Physics/Shapes/CapsuleShape.hpp"
#include "../../Engine/Physics/Shapes/PlaneShape.hpp"
#include "../UI/UIRenderer.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <cstring>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace GameEngine {

    void SceneHierarchyPanel::OnImGuiRender() {
        ImGui::Begin("Scene Hierarchy");
        
        if (m_Context) {
            auto entities = m_Context->GetAllEntities();
            
            if (ImGui::Button("+ Add Entity")) {
                ImGui::OpenPopup("AddEntityPopup");
            }

            if (ImGui::BeginPopup("AddEntityPopup")) {
                if (ImGui::MenuItem("Empty Entity")) {
                    // Scene::CreateEntity already adds TransformComponent
                    Entity e = m_Context->CreateEntity("New Entity");
                    m_SelectionContext = e;
                }
                if (ImGui::MenuItem("Cube")) {
                    if (m_CreateMeshEntityCallback) m_CreateMeshEntityCallback("Cube");
                }
                if (ImGui::MenuItem("Sphere")) {
                    if (m_CreateMeshEntityCallback) m_CreateMeshEntityCallback("Sphere");
                }
                if (ImGui::MenuItem("Plane")) {
                    if (m_CreateMeshEntityCallback) m_CreateMeshEntityCallback("Plane");
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Directional Light")) {
                    Entity light = m_Context->CreateEntity("Directional Light");
                    // TransformComponent already added by CreateEntity
                    auto& tf = light.GetComponent<TransformComponent>();
                    tf.Position = glm::vec3(0.0f, 5.0f, 0.0f);
                    auto& lc = light.AddComponent<LightComponent>(LightType::Directional);
                    lc.Intensity = 1.0f;
                    lc.Color = glm::vec3(1.0f);
                    lc.SyncToLight();
                    m_SelectionContext = light;
                }
                if (ImGui::MenuItem("Point Light")) {
                    Entity light = m_Context->CreateEntity("Point Light");
                    // TransformComponent already added by CreateEntity
                    auto& tf = light.GetComponent<TransformComponent>();
                    tf.Position = glm::vec3(0.0f, 3.0f, 0.0f);
                    auto& lc = light.AddComponent<LightComponent>(LightType::Point);
                    lc.Intensity = 1.0f;
                    lc.Range = 10.0f;
                    lc.SyncToLight();
                    m_SelectionContext = light;
                }
                if (ImGui::MenuItem("Camera")) {
                    Entity cam = m_Context->CreateEntity("Camera");
                    // TransformComponent already added by CreateEntity
                    cam.AddComponent<CameraComponent>();
                    m_SelectionContext = cam;
                }
                ImGui::EndPopup();
            }

            ImGui::Separator();

            for (auto entity : entities) {
                DrawEntityNode(entity);
            }

            // Right-click on blank space
            if (ImGui::BeginPopupContextWindow("SceneHierarchyContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
                if (ImGui::MenuItem("Create Empty Entity")) {
                    // Scene::CreateEntity already adds TransformComponent
                    Entity e = m_Context->CreateEntity("New Entity");
                    m_SelectionContext = e;
                }
                ImGui::EndPopup();
            }
        }
        
        ImGui::End();
    }
    
    void SceneHierarchyPanel::DrawEntityNode(Entity entity) {
        const std::string& name = entity.GetName();
        
        ImGuiTreeNodeFlags flags = ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
        flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
        
        uint64_t entityUUID = static_cast<uint64_t>(entity.GetUUID());
        bool opened = ImGui::TreeNodeEx((void*)(uintptr_t)(entityUUID | 1), flags, "%s", name.c_str());
        
        if (ImGui::IsItemClicked()) {
            m_SelectionContext = entity;
        }
        
        bool entityDeleted = false;
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Delete Entity")) {
                entityDeleted = true;
            }
            ImGui::EndPopup();
        }
        
        if (opened) {
            ImGui::TreePop();
        }
        
        if (entityDeleted) {
            if (m_SelectionContext == entity) {
                m_SelectionContext = {};
            }
            m_Context->DestroyEntity(entity);
        }
    }
    
    void SceneHierarchyPanel::DrawComponents(Entity entity) {
        // Name and Tag
        std::string name = entity.GetName();
        char nameBuffer[256];
        memset(nameBuffer, 0, sizeof(nameBuffer));
        strncpy(nameBuffer, name.c_str(), sizeof(nameBuffer) - 1);
        if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
            entity.SetName(std::string(nameBuffer));
        }
        
        std::string tag = entity.GetTag();
        char tagBuffer[256];
        memset(tagBuffer, 0, sizeof(tagBuffer));
        strncpy(tagBuffer, tag.c_str(), sizeof(tagBuffer) - 1);
        if (ImGui::InputText("Tag", tagBuffer, sizeof(tagBuffer))) {
            entity.SetTag(std::string(tagBuffer));
        }
        
        bool active = entity.IsActive();
        if (ImGui::Checkbox("Active", &active)) {
            entity.SetActive(active);
        }
        
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        
        if (ImGui::Button("Add Component")) {
            ImGui::OpenPopup("AddComponent");
        }
        
        if (ImGui::BeginPopup("AddComponent")) {
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
            }
            
            if (!entity.HasComponent<ScriptComponent>()) {
                if (ImGui::MenuItem("Script")) {
                    entity.AddComponent<ScriptComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }
            
            ImGui::EndPopup();
        }
        
        ImGui::PopItemWidth();
        
        DrawComponent<TransformComponent>("Transform", entity, [this](auto& component) {
            // Snapshot before edit
            glm::vec3 oldPos = component.Position;
            if (UIRenderer::Vec3Control("Translation", component.Position)) {
                component.SetDirty();  // Mark transform cache as dirty so changes take effect
                if (m_CommandHistory && ImGui::IsItemDeactivatedAfterEdit()) {
                    m_CommandHistory->Execute(CreateScope<SetPropertyCommand<glm::vec3>>(
                        &component.Position, oldPos, component.Position, "Move Entity"));
                }
            }

            glm::vec3 rotation = glm::degrees(glm::eulerAngles(component.Rotation));
            glm::vec3 oldRot = rotation;
            if (UIRenderer::Vec3Control("Rotation", rotation)) {
                component.Rotation = glm::quat(glm::radians(rotation));
                component.SetDirty();  // Mark transform cache as dirty so changes take effect
                if (m_CommandHistory && ImGui::IsItemDeactivatedAfterEdit()) {
                    glm::vec3 newRot = rotation;
                    m_CommandHistory->Execute(CreateScope<SetPropertyCallbackCommand<glm::vec3>>(
                        [&component](const glm::vec3& r) { component.Rotation = glm::quat(glm::radians(r)); component.SetDirty(); },
                        oldRot, newRot, "Rotate Entity"));
                }
            }

            glm::vec3 oldScale = component.Scale;
            if (UIRenderer::Vec3Control("Scale", component.Scale, 1.0f)) {
                component.SetDirty();  // Mark transform cache as dirty so changes take effect
                if (m_CommandHistory && ImGui::IsItemDeactivatedAfterEdit()) {
                    m_CommandHistory->Execute(CreateScope<SetPropertyCommand<glm::vec3>>(
                        &component.Scale, oldScale, component.Scale, "Scale Entity"));
                }
            }
        });
        
        DrawComponent<MeshRendererComponent>("Mesh Renderer", entity, [](auto& component) {
            // Mesh and material selection would go here
            ImGui::Text("Mesh: %s", component.GetMesh() ? "Assigned" : "None");
            ImGui::Text("Material: %s", component.GetMaterial() ? "Assigned" : "None");
        });
        
        DrawComponent<CameraComponent>("Camera", entity, [](auto& component) {
            auto camera = component.GetCamera();
            if (!camera) return;
            
            float fov = camera->GetFOV();
            if (ImGui::DragFloat("FOV", &fov, 1.0f, 10.0f, 120.0f)) {
                camera->SetFOV(fov);
            }
            
            float nearPlane = camera->GetNearClip();
            ImGui::DragFloat("Near", &nearPlane, 0.1f, 0.01f, 10.0f);
            
            float farPlane = camera->GetFarClip();
            ImGui::DragFloat("Far", &farPlane, 1.0f, 10.0f, 10000.0f);
        });
        
        DrawComponent<RigidBodyComponent>("Rigid Body", entity, [](auto& component) {
            const char* bodyTypeStrings[] = { "Static", "Kinematic", "Dynamic" };
            const char* currentBodyType = bodyTypeStrings[(int)component.Type];
            
            if (ImGui::BeginCombo("Type", currentBodyType)) {
                for (int i = 0; i < 3; i++) {
                    bool isSelected = currentBodyType == bodyTypeStrings[i];
                    if (ImGui::Selectable(bodyTypeStrings[i], isSelected)) {
                        component.Type = (RigidBodyComponent::BodyType)i;
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            
            ImGui::DragFloat("Mass", &component.Mass, 0.1f, 0.0f, 1000.0f);
            ImGui::DragFloat("Linear Damping", &component.LinearDamping, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("Angular Damping", &component.AngularDamping, 0.01f, 0.0f, 1.0f);
            ImGui::Checkbox("Use Gravity", &component.UseGravity);
        });
        
        DrawComponent<ColliderComponent>("Collider", entity, [](auto& component) {
            auto shape = component.GetShape();
            
            // Shape type display
            if (shape) {
                const char* shapeTypeStrings[] = { "Sphere", "Box", "Capsule", "Plane", "Mesh" };
                auto shapeType = shape->GetType();
                ImGui::Text("Shape Type: %s", shapeTypeStrings[(int)shapeType]);
                
                // Shape-specific properties
                switch (shapeType) {
                    case Physics::CollisionShape::ShapeType::Sphere: {
                        auto sphere = std::static_pointer_cast<Physics::SphereShape>(shape);
                        float radius = sphere->GetRadius();
                        if (ImGui::DragFloat("Radius", &radius, 0.01f, 0.01f, 100.0f)) {
                            sphere->SetRadius(radius);
                        }
                        break;
                    }
                    case Physics::CollisionShape::ShapeType::Box: {
                        auto box = std::static_pointer_cast<Physics::BoxShape>(shape);
                        glm::vec3 halfExtents = box->GetHalfExtents();
                        if (UIRenderer::Vec3Control("Half Extents", halfExtents, 0.5f)) {
                            box->SetHalfExtents(halfExtents);
                        }
                        break;
                    }
                    case Physics::CollisionShape::ShapeType::Capsule: {
                        auto capsule = std::static_pointer_cast<Physics::CapsuleShape>(shape);
                        float radius = capsule->GetRadius();
                        float height = capsule->GetHeight();
                        if (ImGui::DragFloat("Radius", &radius, 0.01f, 0.01f, 100.0f)) {
                            capsule->SetRadius(radius);
                        }
                        if (ImGui::DragFloat("Height", &height, 0.01f, 0.01f, 100.0f)) {
                            capsule->SetHeight(height);
                        }
                        break;
                    }
                    default:
                        break;
                }
            } else {
                ImGui::Text("No shape assigned");
            }
            
            ImGui::Separator();
            
            // Physics material properties
            ImGui::Checkbox("Is Trigger", &component.IsTrigger);
            ImGui::DragFloat("Friction", &component.Friction, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("Restitution", &component.Restitution, 0.01f, 0.0f, 1.0f);
        });
        
        DrawComponent<LightComponent>("Light", entity, [this](auto& component) {
            // Light type
            const char* lightTypeStrings[] = { "Directional", "Point", "Spot" };
            int currentType = static_cast<int>(component.Type);

            if (ImGui::BeginCombo("Light Type", lightTypeStrings[currentType])) {
                for (int i = 0; i < 3; i++) {
                    bool isSelected = (currentType == i);
                    if (ImGui::Selectable(lightTypeStrings[i], isSelected)) {
                        int oldType = currentType;
                        component.Type = static_cast<LightType>(i);
                        if (m_CommandHistory) {
                            m_CommandHistory->Execute(CreateScope<SetPropertyCallbackCommand<int>>(
                                [&component](const int& t) { component.Type = static_cast<LightType>(t); },
                                oldType, i, "Change Light Type"));
                        }
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            // Color
            glm::vec3 oldColor = component.Color;
            if (ImGui::ColorEdit3("Color", &component.Color.x)) {
                if (ImGui::IsItemDeactivatedAfterEdit() && m_CommandHistory) {
                    m_CommandHistory->Execute(CreateScope<SetPropertyCommand<glm::vec3>>(
                        &component.Color, oldColor, component.Color, "Set Light Color"));
                }
            }

            // Intensity
            float oldIntensity = component.Intensity;
            if (ImGui::DragFloat("Intensity", &component.Intensity, 0.1f, 0.0f, 100.0f)) {
                if (ImGui::IsItemDeactivatedAfterEdit() && m_CommandHistory) {
                    m_CommandHistory->Execute(CreateScope<SetPropertyCommand<float>>(
                        &component.Intensity, oldIntensity, component.Intensity, "Set Light Intensity"));
                }
            }

            // Range (for point and spot lights)
            if (component.Type != LightType::Directional) {
                float oldRange = component.Range;
                if (ImGui::DragFloat("Range", &component.Range, 0.5f, 0.1f, 500.0f)) {
                    if (ImGui::IsItemDeactivatedAfterEdit() && m_CommandHistory) {
                        m_CommandHistory->Execute(CreateScope<SetPropertyCommand<float>>(
                            &component.Range, oldRange, component.Range, "Set Light Range"));
                    }
                }
            }

            // Spot angle (for spot lights)
            if (component.Type == LightType::Spot) {
                float innerAngle = glm::degrees(component.InnerConeAngle);
                float outerAngle = glm::degrees(component.OuterConeAngle);

                if (ImGui::DragFloat("Inner Angle", &innerAngle, 1.0f, 1.0f, 89.0f)) {
                    component.InnerConeAngle = glm::radians(innerAngle);
                }
                if (ImGui::DragFloat("Outer Angle", &outerAngle, 1.0f, 1.0f, 90.0f)) {
                    component.OuterConeAngle = glm::radians(outerAngle);
                }
            }

            ImGui::Checkbox("Cast Shadows", &component.CastShadows);
        });
        
        DrawComponent<ScriptComponent>("Script", entity, [](auto& component) {
            // Script path
            std::string path = component.GetScriptPath();
            char pathBuffer[512];
            memset(pathBuffer, 0, sizeof(pathBuffer));
            strncpy(pathBuffer, path.c_str(), sizeof(pathBuffer) - 1);
            
            if (ImGui::InputText("Script Path", pathBuffer, sizeof(pathBuffer))) {
                component.SetScriptPath(std::string(pathBuffer));
            }
            
            // Status
            if (component.IsLoaded()) {
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Loaded");
            } else if (!component.GetLastError().empty()) {
                ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "Error: %s", component.GetLastError().c_str());
            } else {
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Not loaded");
            }
            
            // Reload button
            if (ImGui::Button("Reload Script")) {
                component.ReloadScript();
            }
            
            // Exposed variables
            auto& variables = component.GetVariables();
            if (!variables.empty()) {
                ImGui::Separator();
                ImGui::Text("Exposed Variables:");
                
                for (auto& var : variables) {
                    switch (var.type) {
                        case ScriptVariable::Type::Float:
                            if (ImGui::DragFloat(var.name.c_str(), &var.floatValue, 0.1f)) {
                                component.SetVariableFloat(var.name, var.floatValue);
                            }
                            break;
                        case ScriptVariable::Type::Int:
                            if (ImGui::DragInt(var.name.c_str(), &var.intValue)) {
                                component.SetVariableInt(var.name, var.intValue);
                            }
                            break;
                        case ScriptVariable::Type::Bool:
                            if (ImGui::Checkbox(var.name.c_str(), &var.boolValue)) {
                                component.SetVariableBool(var.name, var.boolValue);
                            }
                            break;
                        case ScriptVariable::Type::String: {
                            char strBuffer[256];
                            strncpy(strBuffer, var.stringValue.c_str(), sizeof(strBuffer) - 1);
                            if (ImGui::InputText(var.name.c_str(), strBuffer, sizeof(strBuffer))) {
                                var.stringValue = strBuffer;
                                component.SetVariableString(var.name, var.stringValue);
                            }
                            break;
                        }
                        case ScriptVariable::Type::Vec3: {
                            glm::vec3 vec(var.vec3Value[0], var.vec3Value[1], var.vec3Value[2]);
                            if (UIRenderer::Vec3Control(var.name.c_str(), vec)) {
                                var.vec3Value[0] = vec.x;
                                var.vec3Value[1] = vec.y;
                                var.vec3Value[2] = vec.z;
                                component.SetVariableVec3(var.name, vec.x, vec.y, vec.z);
                            }
                            break;
                        }
                    }
                }
            }
        });
    }
    
    template<typename T, typename UIFunction>
    void SceneHierarchyPanel::DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction) {
        const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
        
        if (!entity.HasComponent<T>()) {
            return;
        }
        
        auto& component = entity.GetComponent<T>();
        ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
        
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
        float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImGui::Separator();
        bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, "%s", name.c_str());
        ImGui::PopStyleVar();
        
        bool removeComponent = false;
        ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
        if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight })) {
            ImGui::OpenPopup("ComponentSettings");
        }
        
        if (ImGui::BeginPopup("ComponentSettings")) {
            if (ImGui::MenuItem("Remove component")) {
                removeComponent = true;
            }
            ImGui::EndPopup();
        }
        
        if (open) {
            uiFunction(component);
            ImGui::TreePop();
        }
        
        if (removeComponent) {
            entity.RemoveComponent<T>();
        }
    }

    // Explicit template instantiations
    template void SceneHierarchyPanel::DrawComponent<TransformComponent>(const std::string&, Entity, std::function<void(TransformComponent&)>);
    template void SceneHierarchyPanel::DrawComponent<MeshRendererComponent>(const std::string&, Entity, std::function<void(MeshRendererComponent&)>);
    template void SceneHierarchyPanel::DrawComponent<CameraComponent>(const std::string&, Entity, std::function<void(CameraComponent&)>);
    template void SceneHierarchyPanel::DrawComponent<RigidBodyComponent>(const std::string&, Entity, std::function<void(RigidBodyComponent&)>);
    template void SceneHierarchyPanel::DrawComponent<ColliderComponent>(const std::string&, Entity, std::function<void(ColliderComponent&)>);
    template void SceneHierarchyPanel::DrawComponent<LightComponent>(const std::string&, Entity, std::function<void(LightComponent&)>);
    template void SceneHierarchyPanel::DrawComponent<ScriptComponent>(const std::string&, Entity, std::function<void(ScriptComponent&)>);

}
