#include "EditorContext.hpp"
#include "../../Engine/Scene/Scene.hpp"
#include "../../Engine/Core/UUID.hpp"
#include <algorithm>

namespace GameEngine {

    EditorContext::EditorContext() {
    }

    void EditorContext::SetSelection(const std::vector<UUID>& entities) {
        m_Selection = entities;
        m_SelectionSet.clear();
        for (auto id : entities) {
            m_SelectionSet.insert(id);
        }
    }

    void EditorContext::AddToSelection(UUID entity) {
        if (m_SelectionSet.find(entity) == m_SelectionSet.end()) {
            m_Selection.push_back(entity);
            m_SelectionSet.insert(entity);
        }
    }

    void EditorContext::RemoveFromSelection(UUID entity) {
        auto it = std::find(m_Selection.begin(), m_Selection.end(), entity);
        if (it != m_Selection.end()) {
            m_Selection.erase(it);
            m_SelectionSet.erase(entity);
        }
    }

    void EditorContext::ClearSelection() {
        m_Selection.clear();
        m_SelectionSet.clear();
    }

    bool EditorContext::IsSelected(UUID entity) const {
        return m_SelectionSet.find(entity) != m_SelectionSet.end();
    }

    void EditorContext::SetActiveScene(Ref<Scene> scene) {
        m_ActiveScene = scene;
    }

}

