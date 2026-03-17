#pragma once

#include "EditorPanel.hpp"
#include "../core/SceneRenderer.hpp"
#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace editor {

// Forward declaration
class SceneRenderer;

/**
 * @brief Scene Hierarchy Panel - displays all objects in the scene tree
 * Similar to Unity's Hierarchy or LumixEngine's Entity List
 */
class SceneHierarchyPanel : public EditorPanel {
public:
    SceneHierarchyPanel();
    
    void render() override;
    void update(float dt) override;
    
    // Scene renderer connection
    void setSceneRenderer(SceneRenderer* renderer) { m_sceneRenderer = renderer; }
    
    // Selection callback
    std::function<void(int)> onSelectionChanged;

private:
    void renderSearchBar();
    void renderToolbar();
    void renderSceneTree();
    void renderObjectNode(int index);
    void renderContextMenu();
    void renderCreateMenu();
    
    SceneRenderer* m_sceneRenderer = nullptr;
    
    // Selection state
    int m_selectedIndex = -1;
    
    // UI State
    char m_searchBuffer[256] = "";
    bool m_showCreateMenu = false;
    bool m_renaming = false;
    int m_renamingIndex = -1;
    char m_renameBuffer[256] = "";
};

} // namespace editor
