#pragma once

#include "../../Engine/Core/Base.hpp"
#include "../../Engine/Scene/Entity.hpp"
#include <vector>
#include <unordered_set>
#include <functional>

namespace GameEngine {

    /**
     * @brief Editor state enum
     */
    enum class EditorState {
        Edit,   // Normal editing mode
        Play,   // Game is running
        Pause   // Game is paused
    };

    /**
     * @brief Editor context - tracks selection, state, and undo/redo
     */
    class EditorContext {
    public:
        EditorContext();
        ~EditorContext() = default;

        /**
         * @brief Selection
         */
        void SetSelection(const std::vector<UUID>& entities);
        void AddToSelection(UUID entity);
        void RemoveFromSelection(UUID entity);
        void ClearSelection();
        
        const std::vector<UUID>& GetSelection() const { return m_Selection; }
        bool IsSelected(UUID entity) const;

        /**
         * @brief Active scene
         */
        void SetActiveScene(Ref<Scene> scene);
        Ref<Scene> GetActiveScene() const { return m_ActiveScene; }

        /**
         * @brief Editor state machine (Play/Pause/Stop)
         */
        EditorState GetState() const { return m_State; }
        bool IsEditing() const { return m_State == EditorState::Edit; }
        bool IsPlaying() const { return m_State == EditorState::Play; }
        bool IsPaused() const { return m_State == EditorState::Pause; }
        
        // State transitions
        void Play();
        void Pause();
        void Resume();
        void Stop();
        
        // Callbacks for state changes
        using StateChangeCallback = std::function<void(EditorState oldState, EditorState newState)>;
        void SetStateChangeCallback(StateChangeCallback callback) { m_StateChangeCallback = callback; }

    private:
        void SetState(EditorState newState);
        void SaveSceneState();
        void RestoreSceneState();

    private:
        std::vector<UUID> m_Selection;
        std::unordered_set<UUID> m_SelectionSet; // For fast lookup
        Ref<Scene> m_ActiveScene;
        
        // State machine
        EditorState m_State = EditorState::Edit;
        StateChangeCallback m_StateChangeCallback;
        
        // Saved scene state for play mode
        std::string m_SavedSceneState;
    };

}

