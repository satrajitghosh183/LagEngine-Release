#pragma once

#include "../../Engine/Assets/PresetManager.hpp"
#include <imgui.h>
#include <string>
#include <functional>

namespace GameEngine {

    /**
     * @brief ImGui panel that displays loadable scene presets
     *
     * Provides a category filter, preset list with descriptions,
     * and a load button with confirmation dialog.
     */
    class PresetBrowserPanel {
    public:
        using LoadCallback = std::function<void(const std::string& scenePath)>;

        /**
         * @brief Render the panel (call every frame)
         */
        void OnImGuiRender();

        /**
         * @brief Set the callback invoked when a preset is loaded
         */
        void SetLoadCallback(const LoadCallback& callback) { m_Callback = callback; }

    private:
        int m_SelectedPreset = -1;
        int m_SelectedCategory = 0;
        LoadCallback m_Callback;
        bool m_ShowConfirmation = false;
    };
}
