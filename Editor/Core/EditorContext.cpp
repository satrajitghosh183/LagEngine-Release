#include "EditorContext.hpp"
#include "../../Engine/Scene/Scene.hpp"
#include "../../Engine/Scene/SceneSerializer.hpp"
#include "../../Engine/Core/UUID.hpp"
#include "../../Engine/Core/Logger.hpp"
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

    void EditorContext::SetState(EditorState newState) {
        if (m_State == newState) return;
        
        EditorState oldState = m_State;
        m_State = newState;
        
        if (m_StateChangeCallback) {
            m_StateChangeCallback(oldState, newState);
        }
    }

    void EditorContext::Play() {
        if (m_State == EditorState::Play) return;
        
        if (m_State == EditorState::Edit) {
            // Save scene state before playing
            SaveSceneState();
            GE_CORE_INFO("Entering Play mode");
        }
        
        SetState(EditorState::Play);
        
        // Start the scene if coming from edit mode
        if (m_ActiveScene) {
            m_ActiveScene->OnStart();
        }
    }

    void EditorContext::Pause() {
        if (m_State != EditorState::Play) return;
        
        GE_CORE_INFO("Pausing Play mode");
        SetState(EditorState::Pause);
        
        if (m_ActiveScene) {
            m_ActiveScene->SetPaused(true);
        }
    }

    void EditorContext::Resume() {
        if (m_State != EditorState::Pause) return;
        
        GE_CORE_INFO("Resuming Play mode");
        SetState(EditorState::Play);
        
        if (m_ActiveScene) {
            m_ActiveScene->SetPaused(false);
        }
    }

    void EditorContext::Stop() {
        if (m_State == EditorState::Edit) return;
        
        GE_CORE_INFO("Stopping Play mode");
        
        // Stop the scene
        if (m_ActiveScene) {
            m_ActiveScene->OnStop();
        }
        
        // Restore scene state
        RestoreSceneState();
        
        SetState(EditorState::Edit);
    }

    void EditorContext::SaveSceneState() {
        if (!m_ActiveScene) return;
        
        SceneSerializer serializer(m_ActiveScene);
        m_SavedSceneState = serializer.SerializeToString();
        GE_CORE_INFO("Scene state saved ({0} bytes)", m_SavedSceneState.size());
    }

    void EditorContext::RestoreSceneState() {
        if (m_SavedSceneState.empty() || !m_ActiveScene) return;
        
        // Create a new scene and deserialize into it
        Ref<Scene> restoredScene = CreateRef<Scene>(m_ActiveScene->GetName());
        SceneSerializer serializer(restoredScene);
        
        if (serializer.DeserializeFromString(m_SavedSceneState)) {
            m_ActiveScene = restoredScene;
            GE_CORE_INFO("Scene state restored");
        } else {
            GE_CORE_ERROR("Failed to restore scene state");
        }
        
        m_SavedSceneState.clear();
    }

}

