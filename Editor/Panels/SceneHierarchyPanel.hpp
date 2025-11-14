#pragma once

#include <GameEngine/Core/Base.hpp>
#include <GameEngine/Scene/Scene.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <typeinfo>

namespace GameEngine {

    class SceneHierarchyPanel {
    public:
        SceneHierarchyPanel() = default;
        
        void SetContext(const Ref<Scene>& scene) { m_Context = scene; }
        
        void OnImGuiRender();
        
        Entity GetSelectedEntity() const { return m_SelectionContext; }
        void SetSelectedEntity(Entity entity) { m_SelectionContext = entity; }
        
    private:
        void DrawEntityNode(Entity entity);
        void DrawComponents(Entity entity);
        
        template<typename T, typename UIFunction>
        void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction) {
            const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
            
            if (!entity.HasComponent<T>()) {
                return;
            }
            
            auto& component = entity.GetComponent<T>();
            ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
            
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
            float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
            ImGui::Separator();
            bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());
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
        
    private:
        Ref<Scene> m_Context;
        Entity m_SelectionContext;
    };

}

