#pragma once

#include "../../Engine/Core/Base.hpp"
#include "../../Engine/Build/BuildManager.hpp"
#include <string>
#include <vector>
#include <deque>

namespace GameEngine {

    class BuildPanel {
    public:
        BuildPanel();
        ~BuildPanel() = default;

        void OnImGuiRender();

        void SetProjectPath(const std::string& path) { m_ProjectPath = path; }
        void SetProjectName(const std::string& name) { m_ProjectName = name; }

        // Visibility control
        void Toggle() { m_IsOpen = !m_IsOpen; }
        void SetOpen(bool open) { m_IsOpen = open; }
        bool IsOpen() const { return m_IsOpen; }

    private:
        void RenderToolbar();
        void RenderConfiguration();
        void RenderBuildOutput();
        void RenderErrorList();
        void RenderCMakeGenerator();

        void StartBuild();
        void StartClean();
        void GenerateCMake();

        void AddOutputLine(const std::string& line);
        void ClearOutput();

    private:
        BuildManager m_BuildManager;
        BuildConfig m_Config;

        std::string m_ProjectPath;
        std::string m_ProjectName = "MyProject";
        std::string m_OutputDirectory;

        // Build target selection
        int m_SelectedTarget = 1;  // 0=Debug, 1=Release, 2=RelWithDebInfo, 3=MinSizeRel
        const char* m_TargetNames[4] = { "Debug", "Release", "RelWithDebInfo", "MinSizeRel" };

        // Build output
        std::deque<std::string> m_OutputLines;
        static constexpr size_t MAX_OUTPUT_LINES = 1000;
        bool m_AutoScroll = true;
        bool m_ShowTimestamps = false;

        // Error list
        std::vector<BuildError> m_Errors;
        std::vector<BuildError> m_Warnings;
        int m_SelectedError = -1;

        // Build state
        bool m_IsBuilding = false;
        bool m_IsCleaning = false;
        bool m_IsGeneratingCMake = false;
        float m_BuildProgress = 0.0f;
        float m_LastBuildTime = 0.0f;

        // CMake generator
        std::string m_GeneratedCMake;
        bool m_ShowCMakePreview = false;

        // Environment info
        bool m_CMakeFound = false;
        bool m_CompilerFound = false;
        bool m_OllamaAvailable = false;
        std::string m_CMakeVersion;
        std::string m_CompilerInfo;
        std::string m_OllamaModel = "codellama:13b";

        // Ollama status refresh
        float m_OllamaCheckTimer = 0.0f;
        static constexpr float OLLAMA_CHECK_INTERVAL = 5.0f; // seconds

        // Tabs
        int m_SelectedTab = 0;  // 0=Output, 1=Errors, 2=CMake

        // Visibility
        bool m_IsOpen = true;
    };

}
