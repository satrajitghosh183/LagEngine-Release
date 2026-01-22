#pragma once

#include "../../Engine/Core/Base.hpp"
#include "../../Engine/Scene/Entity.hpp"
#include <vector>
#include <unordered_set>

namespace GameEngine {

    /**
     * @brief Editor context - tracks selection and undo/redo
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

    private:
        std::vector<UUID> m_Selection;
        std::unordered_set<UUID> m_SelectionSet; // For fast lookup
        Ref<Scene> m_ActiveScene;
    };

}

