#pragma once

#include "../../Engine/Core/Base.hpp"
#include "../../Engine/Scene/Scene.hpp"
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
        void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction);
        
    private:
        Ref<Scene> m_Context;
        Entity m_SelectionContext;
    };

}
