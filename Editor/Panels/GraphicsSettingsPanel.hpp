#pragma once

#include "../../Engine/Core/Base.hpp"
#include "../../Engine/Core/RuntimeConfig.hpp"
#include "../../Engine/Graphics/RenderPath.hpp"
#include <imgui.h>

namespace GameEngine {

    /**
     * @brief Graphics settings panel
     * 
     * Features:
     * - Graphics backend selection
     * - Debug/validation toggles
     * - Post-process settings
     * - Resolution and display options
     */
    class GraphicsSettingsPanel {
    public:
        GraphicsSettingsPanel();
        ~GraphicsSettingsPanel() = default;

        void OnImGuiRender();

        /**
         * @brief Set active render path (for post-process settings)
         */
        void SetRenderPath(const Ref<RenderPath3D>& path) { m_RenderPath = path; }

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

    private:
        void RenderBackendSettings();
        void RenderDebugSettings();
        void RenderPostProcessSettings();
        void RenderDisplaySettings();

    private:
        bool m_IsOpen = false;
        Ref<RenderPath3D> m_RenderPath;
        
        // Graphics options
        std::string m_CurrentBackend = "OpenGL";
        bool m_DebugDevice = false;
        bool m_GPUValidation = false;
        bool m_GPUVerbose = false;
        std::string m_PreferredGPU = "Auto";
        
        // Display settings
        int m_ResolutionWidth = 1920;
        int m_ResolutionHeight = 1080;
        bool m_VSync = true;
        bool m_Fullscreen = false;
    };

}
