#pragma once

#include "../../Engine/Core/Base.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace GameEngine {

    struct ProjectSettings {
        // General
        std::string ProjectName = "My Project";
        std::string CompanyName = "";
        std::string Version = "1.0.0";
        std::string Description = "";
        
        // Graphics
        int DefaultWidth = 1920;
        int DefaultHeight = 1080;
        bool Fullscreen = false;
        bool VSync = true;
        int TargetFPS = 60;
        int MSAASamples = 4;
        bool HDR = false;
        float RenderScale = 1.0f;
        
        // Physics
        float Gravity = -9.81f;
        int PhysicsSubsteps = 4;
        float FixedTimestep = 1.0f / 60.0f;
        bool PhysicsDebugDraw = false;
        
        // Audio
        float MasterVolume = 1.0f;
        float MusicVolume = 1.0f;
        float SFXVolume = 1.0f;
        int MaxAudioSources = 32;
        
        // Input
        float MouseSensitivity = 1.0f;
        bool InvertY = false;
        float DeadZone = 0.1f;
        
        // Quality
        int ShadowMapResolution = 2048;
        int ShadowCascades = 4;
        float ShadowDistance = 100.0f;
        bool SSAO = true;
        bool Bloom = true;
        int TextureQuality = 2;  // 0=Low, 1=Medium, 2=High
        
        // Build
        std::string OutputDirectory = "Build";
        bool IncludeDebugSymbols = false;
        bool CompressAssets = true;
        std::vector<std::string> ExcludedDirectories;
        
        // Scenes
        std::string StartupScene;
        std::vector<std::string> IncludedScenes;
    };

    class ProjectSettingsPanel {
    public:
        ProjectSettingsPanel();
        ~ProjectSettingsPanel() = default;

        void OnImGuiRender();

        // Visibility control
        void Toggle() { m_IsOpen = !m_IsOpen; }
        void SetOpen(bool open) { m_IsOpen = open; }
        bool IsOpen() const { return m_IsOpen; }

        // Settings management
        void LoadSettings(const std::string& path);
        void SaveSettings(const std::string& path);
        void ResetToDefaults();

        ProjectSettings& GetSettings() { return m_Settings; }
        const ProjectSettings& GetSettings() const { return m_Settings; }

        // Callbacks
        void SetOnSettingsChangedCallback(std::function<void()> callback) {
            m_OnSettingsChangedCallback = callback;
        }

    private:
        void RenderGeneralSettings();
        void RenderGraphicsSettings();
        void RenderPhysicsSettings();
        void RenderAudioSettings();
        void RenderInputSettings();
        void RenderQualitySettings();
        void RenderBuildSettings();
        void RenderScenesSettings();

    private:
        ProjectSettings m_Settings;
        std::string m_SettingsPath;
        bool m_Modified = false;

        // UI state
        int m_SelectedCategory = 0;
        const char* m_Categories[8] = {
            "General", "Graphics", "Physics", "Audio",
            "Input", "Quality", "Build", "Scenes"
        };

        // Callbacks
        std::function<void()> m_OnSettingsChangedCallback;

        // Visibility
        bool m_IsOpen = true;
    };

}
