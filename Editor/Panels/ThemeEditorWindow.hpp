#pragma once

#include "../../Engine/Core/Base.hpp"
#include <imgui.h>
#include <string>
#include <vector>

namespace GameEngine {

    /**
     * @brief Theme editor window for customizing ImGui styles
     * 
     * Features:
     * - Color customization (background, text, borders, etc.)
     * - Sizing controls (window padding, frame padding, item spacing)
     * - Rounding controls (window rounding, frame rounding, grab rounding)
     * - Alpha values
     * - Save/load theme presets
     */
    class ThemeEditorWindow {
    public:
        ThemeEditorWindow();
        ~ThemeEditorWindow() = default;

        /**
         * @brief Render the theme editor UI
         */
        void OnImGuiRender();

        /**
         * @brief Toggle window visibility
         */
        void Toggle() { m_IsOpen = !m_IsOpen; }

        /**
         * @brief Set window visibility
         */
        void SetOpen(bool open) { m_IsOpen = open; }

        /**
         * @brief Check if window is open
         */
        bool IsOpen() const { return m_IsOpen; }

        /**
         * @brief Save current theme to file
         */
        bool SaveTheme(const std::string& filepath);

        /**
         * @brief Load theme from file
         */
        bool LoadTheme(const std::string& filepath);

        /**
         * @brief Apply a preset theme
         */
        void ApplyPreset(const std::string& presetName);

    private:
        void RenderColorEditor();
        void RenderSizingEditor();
        void RenderRoundingEditor();
        void RenderAlphaEditor();
        void RenderPresets();

        void ApplyStyle();
        void ResetToDefault();

    private:
        bool m_IsOpen = false;

        // Style data (copied from ImGui::GetStyle())
        ImGuiStyle m_Style;
        
        // Preset names
        std::vector<std::string> m_PresetNames = {
            "Default",
            "Dark",
            "Light",
            "Classic"
        };
    };

}
