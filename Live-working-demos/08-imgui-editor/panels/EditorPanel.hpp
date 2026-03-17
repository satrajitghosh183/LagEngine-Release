#pragma once

#include <string>
#include <imgui.h>

namespace editor {

/**
 * @brief Base class for all editor panels
 * Provides common functionality for dockable, toggleable UI panels
 */
class EditorPanel {
public:
    EditorPanel(const std::string& name, bool visible = true)
        : m_name(name), m_visible(visible) {}
    
    virtual ~EditorPanel() = default;
    
    // Core interface
    virtual void render() = 0;
    virtual void update(float /*dt*/) {}
    
    // Panel properties
    const std::string& getName() const { return m_name; }
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }
    void toggleVisible() { m_visible = !m_visible; }
    
    bool isFocused() const { return m_focused; }
    bool isHovered() const { return m_hovered; }
    
protected:
    // Helper to begin a panel window with consistent styling
    bool beginPanel(ImGuiWindowFlags flags = 0) {
        if (!m_visible) return false;
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
        bool open = ImGui::Begin(m_name.c_str(), &m_visible, flags);
        m_focused = ImGui::IsWindowFocused();
        m_hovered = ImGui::IsWindowHovered();
        ImGui::PopStyleVar();
        
        return open;
    }
    
    void endPanel() {
        ImGui::End();
    }
    
    // UI Helpers
    void drawSeparator() {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }
    
    void drawHeader(const char* text) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.85f, 1.0f, 1.0f));
        ImGui::Text("%s", text);
        ImGui::PopStyleColor();
        ImGui::Separator();
    }
    
    bool drawCollapsingHeader(const char* label, bool defaultOpen = true) {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | 
                                   ImGuiTreeNodeFlags_Framed |
                                   ImGuiTreeNodeFlags_AllowItemOverlap;
        if (!defaultOpen) flags &= ~ImGuiTreeNodeFlags_DefaultOpen;
        return ImGui::CollapsingHeader(label, flags);
    }
    
    std::string m_name;
    bool m_visible = true;
    bool m_focused = false;
    bool m_hovered = false;
};

} // namespace editor

