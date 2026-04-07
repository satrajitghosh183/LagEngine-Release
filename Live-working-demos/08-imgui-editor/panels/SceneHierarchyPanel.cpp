#include "SceneHierarchyPanel.hpp"
#include "../core/EditorTheme.hpp"
#include <imgui.h>
#include <algorithm>
#include <cstring>

namespace editor {

SceneHierarchyPanel::SceneHierarchyPanel() 
    : EditorPanel("Scene Hierarchy", true) {
}

void SceneHierarchyPanel::render() {
    if (!beginPanel(ImGuiWindowFlags_MenuBar)) {
        endPanel();
        return;
    }
    
    // Menu bar
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Add")) {
            renderCreateMenu();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            static bool showHidden = false;
            ImGui::MenuItem("Show Hidden", nullptr, &showHidden);
            ImGui::MenuItem("Expand All", nullptr, false);
            ImGui::MenuItem("Collapse All", nullptr, false);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    
    renderSearchBar();
    renderToolbar();
    
    ImGui::Separator();
    
    // Scene tree
    ImGui::BeginChild("SceneTree", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    renderSceneTree();
    ImGui::EndChild();
    
    // Context menu
    if (ImGui::BeginPopupContextWindow("HierarchyContextMenu")) {
        renderContextMenu();
        ImGui::EndPopup();
    }
    
    endPanel();
}

void SceneHierarchyPanel::renderSearchBar() {
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.12f, 0.15f, 1.0f));
    
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputTextWithHint("##Search", "Search objects...", 
                                  m_searchBuffer, sizeof(m_searchBuffer))) {
        // Filter objects based on search
    }
    
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void SceneHierarchyPanel::renderToolbar() {
    ImGui::Spacing();
    
    // Quick add buttons
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.3f, 1.0f));
    
    if (ImGui::Button("+##AddObject")) {
        m_showCreateMenu = true;
        ImGui::OpenPopup("CreateObjectMenu");
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add Object");
    
    // Create menu popup
    if (ImGui::BeginPopup("CreateObjectMenu")) {
        renderCreateMenu();
        ImGui::EndPopup();
    }
    
    ImGui::PopStyleColor();
    
    ImGui::SameLine();
    
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.25f, 0.25f, 1.0f));
    if (ImGui::Button("-##DeleteObject")) {
        if (m_sceneRenderer && m_selectedIndex >= 0) {
            m_sceneRenderer->removeObject(m_selectedIndex);
            m_selectedIndex = -1;
            if (onSelectionChanged) {
                onSelectionChanged(-1);
            }
        }
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Delete Selected");
    ImGui::PopStyleColor();
    
    ImGui::SameLine();
    
    if (ImGui::Button("D##Duplicate")) {
        if (m_sceneRenderer && m_selectedIndex >= 0) {
            SceneObject* obj = m_sceneRenderer->getObject(m_selectedIndex);
            if (obj) {
                int newIdx = m_sceneRenderer->addObject(obj->name + "_copy", obj->type);
                SceneObject* newObj = m_sceneRenderer->getObject(newIdx);
                if (newObj) {
                    newObj->transform = obj->transform;
                    newObj->transform.position.x += 1.0f;
                    newObj->color = obj->color;
                }
            }
        }
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Duplicate Selected");
    
    ImGui::PopStyleVar();
}

void SceneHierarchyPanel::renderSceneTree() {
    if (!m_sceneRenderer) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No scene loaded");
        return;
    }
    
    std::string searchStr = m_searchBuffer;
    std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);
    
    auto& objects = m_sceneRenderer->getObjects();
    
    for (size_t i = 0; i < objects.size(); ++i) {
        const auto& obj = objects[i];
        
        // Skip if doesn't match search
        if (!searchStr.empty()) {
            std::string nameLower = obj.name;
            std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
            if (nameLower.find(searchStr) == std::string::npos) {
                continue;
            }
        }
        
        renderObjectNode(static_cast<int>(i));
    }
}

void SceneHierarchyPanel::renderObjectNode(int index) {
    if (!m_sceneRenderer) return;
    
    auto& objects = m_sceneRenderer->getObjects();
    if (index < 0 || index >= static_cast<int>(objects.size())) return;
    
    auto& obj = objects[index];
    
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | 
                                ImGuiTreeNodeFlags_SpanAvailWidth |
                                ImGuiTreeNodeFlags_NoTreePushOnOpen;
    
    if (m_sceneRenderer->getSelectedIndex() == index) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    
    // Icon based on type
    const char* icon = "[C]"; // Cube
    ImVec4 iconColor = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
    
    if (obj.type == "Sphere") {
        icon = "[S]";
        iconColor = ImVec4(0.4f, 0.8f, 0.4f, 1.0f);
    } else if (obj.type == "Plane") {
        icon = "[P]";
        iconColor = ImVec4(0.5f, 0.5f, 0.7f, 1.0f);
    } else if (obj.type == "Cube") {
        icon = "[C]";
        iconColor = ImVec4(0.8f, 0.5f, 0.3f, 1.0f);
    }
    
    ImGui::PushID(index);
    
    // Visibility toggle
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
    
    ImVec4 visColor = obj.visible ? ImVec4(0.7f, 0.7f, 0.7f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, visColor);
    if (ImGui::SmallButton(obj.visible ? "O" : "-")) {
        objects[index].visible = !objects[index].visible;
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleColor(2);
    
    ImGui::SameLine();
    
    // Icon
    ImGui::PushStyleColor(ImGuiCol_Text, iconColor);
    ImGui::Text("%s", icon);
    ImGui::PopStyleColor();
    ImGui::SameLine();
    
    // Renaming mode
    if (m_renaming && m_renamingIndex == index) {
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::InputText("##rename", m_renameBuffer, sizeof(m_renameBuffer), 
                            ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
            objects[index].name = m_renameBuffer;
            m_renaming = false;
            m_renamingIndex = -1;
        }
        if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(0)) {
            m_renaming = false;
            m_renamingIndex = -1;
        }
    } else {
        // Tree node (just for selection)
        ImGui::TreeNodeEx(obj.name.c_str(), flags);
        
        // Selection handling
        if (ImGui::IsItemClicked()) {
            m_selectedIndex = index;
            m_sceneRenderer->selectObject(index);
            
            if (onSelectionChanged) {
                onSelectionChanged(index);
            }
        }
        
        // Double-click to rename
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
            m_renaming = true;
            m_renamingIndex = index;
            strncpy(m_renameBuffer, obj.name.c_str(), sizeof(m_renameBuffer) - 1);
        }
    }
    
    // Context menu
    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Rename")) {
            m_renaming = true;
            m_renamingIndex = index;
            strncpy(m_renameBuffer, obj.name.c_str(), sizeof(m_renameBuffer) - 1);
        }
        if (ImGui::MenuItem("Duplicate")) {
            int newIdx = m_sceneRenderer->addObject(obj.name + "_copy", obj.type);
            SceneObject* newObj = m_sceneRenderer->getObject(newIdx);
            if (newObj) {
                newObj->transform = obj.transform;
                newObj->transform.position.x += 1.0f;
                newObj->color = obj.color;
            }
        }
        if (ImGui::MenuItem("Delete")) {
            m_sceneRenderer->removeObject(index);
            if (m_selectedIndex == index) {
                m_selectedIndex = -1;
                if (onSelectionChanged) {
                    onSelectionChanged(-1);
                }
            }
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Focus (F)")) {
            m_sceneRenderer->getCamera().focusOn(obj.transform.position);
        }
        ImGui::EndPopup();
    }
    
    ImGui::PopID();
}

void SceneHierarchyPanel::renderContextMenu() {
    if (ImGui::MenuItem("Create Empty")) {
        if (m_sceneRenderer) {
            m_sceneRenderer->addObject("Empty", "Cube");
        }
    }
    ImGui::Separator();
    if (ImGui::BeginMenu("3D Objects")) {
        if (ImGui::MenuItem("Cube")) {
            if (m_sceneRenderer) {
                int idx = m_sceneRenderer->addObject("Cube", "Cube");
                SceneObject* obj = m_sceneRenderer->getObject(idx);
                if (obj) obj->color = glm::vec3(0.8f, 0.4f, 0.3f);
            }
        }
        if (ImGui::MenuItem("Sphere")) {
            if (m_sceneRenderer) {
                int idx = m_sceneRenderer->addObject("Sphere", "Sphere");
                SceneObject* obj = m_sceneRenderer->getObject(idx);
                if (obj) obj->color = glm::vec3(0.3f, 0.7f, 0.4f);
            }
        }
        if (ImGui::MenuItem("Plane")) {
            if (m_sceneRenderer) {
                int idx = m_sceneRenderer->addObject("Plane", "Plane");
                SceneObject* obj = m_sceneRenderer->getObject(idx);
                if (obj) {
                    obj->color = glm::vec3(0.5f, 0.5f, 0.6f);
                    obj->transform.scale = glm::vec3(5.0f, 1.0f, 5.0f);
                }
            }
        }
        ImGui::EndMenu();
    }
}

void SceneHierarchyPanel::renderCreateMenu() {
    if (ImGui::MenuItem("Empty Object")) {
        if (m_sceneRenderer) {
            m_sceneRenderer->addObject("Empty", "Cube");
        }
    }
    ImGui::Separator();
    if (ImGui::BeginMenu("3D Objects")) {
        if (ImGui::MenuItem("Cube")) {
            if (m_sceneRenderer) {
                int idx = m_sceneRenderer->addObject("Cube", "Cube");
                SceneObject* obj = m_sceneRenderer->getObject(idx);
                if (obj) {
                    obj->color = glm::vec3(0.8f, 0.4f, 0.3f);
                    obj->transform.position.y = 0.5f;
                }
            }
        }
        if (ImGui::MenuItem("Sphere")) {
            if (m_sceneRenderer) {
                int idx = m_sceneRenderer->addObject("Sphere", "Sphere");
                SceneObject* obj = m_sceneRenderer->getObject(idx);
                if (obj) {
                    obj->color = glm::vec3(0.3f, 0.7f, 0.4f);
                    obj->transform.position.y = 0.5f;
                }
            }
        }
        if (ImGui::MenuItem("Plane")) {
            if (m_sceneRenderer) {
                int idx = m_sceneRenderer->addObject("Plane", "Plane");
                SceneObject* obj = m_sceneRenderer->getObject(idx);
                if (obj) {
                    obj->color = glm::vec3(0.5f, 0.5f, 0.6f);
                    obj->transform.scale = glm::vec3(5.0f, 1.0f, 5.0f);
                }
            }
        }
        ImGui::EndMenu();
    }
}

void SceneHierarchyPanel::update(float /*dt*/) {
    // Update logic if needed
}

} // namespace editor
