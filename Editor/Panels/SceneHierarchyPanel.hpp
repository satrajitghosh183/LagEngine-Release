#pragma once

#include "../../Engine/Core/Base.hpp"
#include "../../Engine/Scene/Scene.hpp"
#include "../Core/Command.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <typeinfo>
#include <functional>

namespace GameEngine {

    class SceneHierarchyPanel {
    public:
        SceneHierarchyPanel() = default;

        void SetContext(const Ref<Scene>& scene) { m_Context = scene; }
        void SetCommandHistory(CommandHistory* history) { m_CommandHistory = history; }

        void OnImGuiRender();

        Entity GetSelectedEntity() const { return m_SelectionContext; }
        void SetSelectedEntity(Entity entity) { m_SelectionContext = entity; }

        // Callback for creating mesh entities (requires access to default material/shader)
        using CreateMeshEntityCallback = std::function<void(const std::string& meshType)>;
        void SetCreateMeshEntityCallback(CreateMeshEntityCallback cb) { m_CreateMeshEntityCallback = std::move(cb); }

    private:
        void DrawEntityNode(Entity entity);
        void DrawComponents(Entity entity);

        template<typename T, typename UIFunction>
        void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction);

        // Undo/redo helper: records property change on deactivation
        template<typename T>
        bool TrackedDragFloat(const char* label, T* value, float speed = 0.1f,
                              float min = 0.0f, float max = 0.0f, const char* desc = "Set Property");
        template<typename T>
        bool TrackedColorEdit3(const char* label, T* value, const char* desc = "Set Color");

    private:
        Ref<Scene> m_Context;
        Entity m_SelectionContext;
        CommandHistory* m_CommandHistory = nullptr;
        CreateMeshEntityCallback m_CreateMeshEntityCallback;

        // Snapshot of value when drag/edit starts
        float m_DragStartFloat = 0.0f;
        glm::vec3 m_DragStartVec3 = glm::vec3(0.0f);
    };

}
