#pragma once

#include <imgui.h>
#include <string>

namespace editor {

/**
 * @brief Editor theming and styling
 * Provides different visual themes for the editor
 */
class EditorTheme {
public:
    enum class Theme {
        Dark,           // Default dark theme
        DarkPro,        // Professional dark theme
        Light,          // Light theme
        Nord,           // Nord color scheme
        Dracula,        // Dracula color scheme
        CyberPunk,      // Cyberpunk neon theme
        Forest          // Nature-inspired green theme
    };

    EditorTheme();
    
    void applyTheme(Theme theme);
    void applyDarkTheme();
    void applyDarkProTheme();
    void applyLightTheme();
    void applyNordTheme();
    void applyDraculaTheme();
    void applyCyberPunkTheme();
    void applyForestTheme();
    
    Theme getCurrentTheme() const { return m_currentTheme; }
    const std::string& getThemeName() const;
    
    // Custom colors
    static ImVec4 getAccentColor();
    static ImVec4 getWarningColor();
    static ImVec4 getErrorColor();
    static ImVec4 getSuccessColor();
    
    // Panel specific colors
    static ImVec4 getHierarchyItemColor(bool selected, bool hovered);
    static ImVec4 getInspectorHeaderColor();
    static ImVec4 getViewportBorderColor();

private:
    Theme m_currentTheme = Theme::DarkPro;
    
    void setupFonts();
    void setupStyle();
};

// Scoped style helpers
class ScopedStyleColor {
public:
    ScopedStyleColor(ImGuiCol idx, const ImVec4& col) {
        ImGui::PushStyleColor(idx, col);
    }
    ~ScopedStyleColor() {
        ImGui::PopStyleColor();
    }
};

class ScopedStyleVar {
public:
    ScopedStyleVar(ImGuiStyleVar idx, float val) {
        ImGui::PushStyleVar(idx, val);
    }
    ScopedStyleVar(ImGuiStyleVar idx, const ImVec2& val) {
        ImGui::PushStyleVar(idx, val);
    }
    ~ScopedStyleVar() {
        ImGui::PopStyleVar();
    }
};

} // namespace editor

