#include "SceneHierarchyPanel.hpp"
#include "../../Engine/Scene/Components/TransformComponent.hpp"
#include "../../Engine/Scene/Components/MeshRendererComponent.hpp"
#include "../../Engine/Scene/Components/CameraComponent.hpp"
#include "../../Engine/Scene/Components/LightComponent.hpp"
#include "../../Engine/Scene/Components/RigidBodyComponent.hpp"
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
                m_Context->CreateEntity("New Entity");
            }
            
            ImGui::Separator();
            
            for (auto entity : entities) {
                DrawEntityNode(entity);
            }
            
            // Right-click on blank space
            if (ImGui::BeginPopupContextWindow(0, 1)) {
                if (ImGui::MenuItem("Create Empty Entity")) {
                    m_Context->CreateEntity("New Entity");
                }
                ImGui::EndPopup();
            }
        }
        
        ImGui::End();
        
        // Inspector
        ImGui::Begin("Inspector");
        
        if (m_SelectionContext.IsValid()) {
            DrawComponents(m_SelectionContext);
        } else {
            ImGui::Text("No entity selected");
        }
        
        ImGui::End();
    }
    
    void SceneHierarchyPanel::DrawEntityNode(Entity entity) {
        const std::string& name = entity.GetName();
        
        ImGuiTreeNodeFlags flags = ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
        flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
        
        bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, "%s", name.c_str());
        
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
            
            ImGui::EndPopup();
        }
        
        ImGui::PopItemWidth();
        
        DrawComponent<TransformComponent>("Transform", entity, [](auto& component) {
            UIRenderer::Vec3Control("Translation", component.Position);
            
            glm::vec3 rotation = glm::degrees(glm::eulerAngles(component.Rotation));
            if (UIRenderer::Vec3Control("Rotation", rotation)) {
                component.Rotation = glm::quat(glm::radians(rotation));
            }
            
            UIRenderer::Vec3Control("Scale", component.Scale, 1.0f);
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

}
