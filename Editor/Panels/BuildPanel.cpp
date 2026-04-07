#include "BuildPanel.hpp"
#include "../../Engine/Core/Logger.hpp"
#include <imgui.h>
#include <chrono>
#include <ctime>
#include <filesystem>

namespace GameEngine {

    BuildPanel::BuildPanel() {
        // Check environment
        m_CMakeFound = m_BuildManager.DetectCMake();
        m_CompilerFound = m_BuildManager.DetectCompiler();
        m_OllamaAvailable = m_BuildManager.IsOllamaAvailable();

        if (m_CMakeFound) {
            m_CMakeVersion = "CMake found";
        }
        if (m_CompilerFound) {
            m_CompilerInfo = "Compiler: ";
            switch (m_BuildManager.GetDetectedCompiler()) {
                case CompilerType::MSVC: m_CompilerInfo += "MSVC "; break;
                case CompilerType::GCC: m_CompilerInfo += "GCC "; break;
                case CompilerType::Clang: m_CompilerInfo += "Clang "; break;
                case CompilerType::MinGW: m_CompilerInfo += "MinGW "; break;
                default: m_CompilerInfo += "Unknown "; break;
            }
            m_CompilerInfo += m_BuildManager.GetCompilerVersion();
        }

        // Set up output callback
        m_BuildManager.SetOutputCallback([this](const std::string& line) {
            AddOutputLine(line);
        });
    }

    void BuildPanel::OnImGuiRender() {
        if (!m_IsOpen) return;

        // Periodically re-check Ollama availability (every OLLAMA_CHECK_INTERVAL seconds)
        m_OllamaCheckTimer += ImGui::GetIO().DeltaTime;
        if (m_OllamaCheckTimer >= OLLAMA_CHECK_INTERVAL) {
            m_OllamaAvailable = m_BuildManager.IsOllamaAvailable();
            m_OllamaCheckTimer = 0.0f;
        }

        ImGui::Begin("Build", nullptr, ImGuiWindowFlags_MenuBar);

        RenderToolbar();

        // Environment status bar
        ImGui::Separator();
        ImGui::BeginGroup();
        {
            // CMake status
            if (m_CMakeFound) {
                ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), "CMake");
            } else {
                ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "CMake Not Found");
            }
            ImGui::SameLine();
            ImGui::Text(" | ");
            ImGui::SameLine();

            // Compiler status
            if (m_CompilerFound) {
                ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), "%s", m_CompilerInfo.c_str());
            } else {
                ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "No Compiler");
            }
            ImGui::SameLine();
            ImGui::Text(" | ");
            ImGui::SameLine();

            // Ollama status
            if (m_OllamaAvailable) {
                ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), "Ollama Ready");
            } else {
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.2f, 1.0f), "Ollama Offline");
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Refresh")) {
                m_OllamaAvailable = m_BuildManager.IsOllamaAvailable();
                m_OllamaCheckTimer = 0.0f;
            }
        }
        ImGui::EndGroup();
        ImGui::Separator();

        // Tab bar
        if (ImGui::BeginTabBar("BuildTabs")) {
            if (ImGui::BeginTabItem("Configuration")) {
                RenderConfiguration();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Output")) {
                RenderBuildOutput();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Errors")) {
                RenderErrorList();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("CMake Generator")) {
                RenderCMakeGenerator();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::End();
    }

    void BuildPanel::RenderToolbar() {
        if (ImGui::BeginMenuBar()) {
            // Build buttons
            bool canBuild = m_CMakeFound && m_CompilerFound && !m_IsBuilding && !m_ProjectPath.empty();
            
            ImGui::BeginDisabled(!canBuild);
            if (ImGui::Button("Build")) {
                StartBuild();
            }
            ImGui::EndDisabled();

            ImGui::SameLine();

            ImGui::BeginDisabled(!canBuild);
            if (ImGui::Button("Rebuild")) {
                StartClean();
                // Will start build after clean
            }
            ImGui::EndDisabled();

            ImGui::SameLine();

            ImGui::BeginDisabled(m_IsBuilding || m_ProjectPath.empty());
            if (ImGui::Button("Clean")) {
                StartClean();
            }
            ImGui::EndDisabled();

            ImGui::SameLine();
            ImGui::Separator();
            ImGui::SameLine();

            // Build target dropdown
            ImGui::SetNextItemWidth(120);
            if (ImGui::Combo("##Target", &m_SelectedTarget, m_TargetNames, 4)) {
                // Target changed
            }

            ImGui::SameLine();
            ImGui::Separator();
            ImGui::SameLine();

            // Progress indicator
            if (m_IsBuilding) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Building...");
                ImGui::SameLine();
                ImGui::ProgressBar(m_BuildProgress, ImVec2(100, 0));
            } else if (m_LastBuildTime > 0) {
                if (m_Errors.empty()) {
                    ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), 
                        "Build succeeded (%.1fs)", m_LastBuildTime);
                } else {
                    ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f),
                        "Build failed (%zu errors)", m_Errors.size());
                }
            }

            ImGui::EndMenuBar();
        }
    }

    void BuildPanel::RenderConfiguration() {
        ImGui::BeginChild("ConfigChild", ImVec2(0, 0), true);

        ImGui::Text("Project Configuration");
        ImGui::Separator();

        // Project name
        char nameBuffer[256];
        strncpy(nameBuffer, m_ProjectName.c_str(), sizeof(nameBuffer) - 1);
        nameBuffer[sizeof(nameBuffer) - 1] = '\0';
        if (ImGui::InputText("Project Name", nameBuffer, sizeof(nameBuffer))) {
            m_ProjectName = nameBuffer;
        }

        // Project path
        char pathBuffer[512];
        strncpy(pathBuffer, m_ProjectPath.c_str(), sizeof(pathBuffer) - 1);
        pathBuffer[sizeof(pathBuffer) - 1] = '\0';
        ImGui::InputText("Project Path", pathBuffer, sizeof(pathBuffer), ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine();
        if (ImGui::Button("Browse...##ProjectPath")) {
            // TODO: Open folder dialog
        }

        // Output directory
        char outputBuffer[512];
        std::string outputDir = m_OutputDirectory.empty() ? 
            (m_ProjectPath.empty() ? "" : m_ProjectPath + "/build") : m_OutputDirectory;
        strncpy(outputBuffer, outputDir.c_str(), sizeof(outputBuffer) - 1);
        outputBuffer[sizeof(outputBuffer) - 1] = '\0';
        if (ImGui::InputText("Output Directory", outputBuffer, sizeof(outputBuffer))) {
            m_OutputDirectory = outputBuffer;
        }

        ImGui::Separator();
        ImGui::Text("Build Options");

        // Build target
        ImGui::Combo("Build Target", &m_SelectedTarget, m_TargetNames, 4);

        // Parallel jobs
        static int parallelJobs = 0;
        ImGui::SliderInt("Parallel Jobs", &parallelJobs, 0, 16, 
            parallelJobs == 0 ? "Auto" : "%d");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("0 = Auto-detect based on CPU cores");
        }

        ImGui::Separator();
        ImGui::Text("Advanced");

        // Additional defines
        static char definesBuffer[1024] = "";
        ImGui::InputTextMultiline("Defines", definesBuffer, sizeof(definesBuffer),
            ImVec2(-1, 60));
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("One define per line, e.g.:\nDEBUG_MODE=1\nUSE_OPENGL");
        }

        ImGui::EndChild();
    }

    void BuildPanel::RenderBuildOutput() {
        // Options bar
        ImGui::Checkbox("Auto-scroll", &m_AutoScroll);
        ImGui::SameLine();
        ImGui::Checkbox("Timestamps", &m_ShowTimestamps);
        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            ClearOutput();
        }
        ImGui::Separator();

        // Output window
        ImGui::BeginChild("OutputScroll", ImVec2(0, 0), true,
            ImGuiWindowFlags_HorizontalScrollbar);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 1));

        for (const auto& line : m_OutputLines) {
            // Color based on content
            ImVec4 color(0.8f, 0.8f, 0.8f, 1.0f);  // Default gray
            
            if (line.find("error") != std::string::npos ||
                line.find("Error") != std::string::npos ||
                line.find("ERROR") != std::string::npos) {
                color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);  // Red
            } else if (line.find("warning") != std::string::npos ||
                       line.find("Warning") != std::string::npos ||
                       line.find("WARNING") != std::string::npos) {
                color = ImVec4(1.0f, 0.8f, 0.0f, 1.0f);  // Yellow
            } else if (line.find("Building") != std::string::npos ||
                       line.find("Compiling") != std::string::npos ||
                       line.find("Linking") != std::string::npos) {
                color = ImVec4(0.5f, 0.8f, 1.0f, 1.0f);  // Blue
            } else if (line.find("success") != std::string::npos ||
                       line.find("Success") != std::string::npos ||
                       line.find("Built target") != std::string::npos) {
                color = ImVec4(0.3f, 1.0f, 0.3f, 1.0f);  // Green
            }

            ImGui::TextColored(color, "%s", line.c_str());
        }

        ImGui::PopStyleVar();

        if (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }

        ImGui::EndChild();
    }

    void BuildPanel::RenderErrorList() {
        // Summary
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%zu Errors", m_Errors.size());
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "%zu Warnings", m_Warnings.size());
        ImGui::Separator();

        // Error list
        ImGui::BeginChild("ErrorList", ImVec2(0, 0), true);

        // Errors first
        for (size_t i = 0; i < m_Errors.size(); i++) {
            const auto& error = m_Errors[i];
            
            ImGui::PushID(static_cast<int>(i));
            
            bool selected = (m_SelectedError == static_cast<int>(i));
            
            // Format: icon file:line message
            std::string label = std::string("[E] ") + 
                std::filesystem::path(error.File).filename().string() + 
                ":" + std::to_string(error.Line) + " - " + error.Message;
            
            if (ImGui::Selectable(label.c_str(), selected)) {
                m_SelectedError = static_cast<int>(i);
                // TODO: Jump to file location
            }
            
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s\nLine %d, Column %d", 
                    error.File.c_str(), error.Line, error.Column);
            }
            
            ImGui::PopID();
        }

        // Then warnings
        for (size_t i = 0; i < m_Warnings.size(); i++) {
            const auto& warning = m_Warnings[i];
            
            ImGui::PushID(static_cast<int>(m_Errors.size() + i));
            
            std::string label = std::string("[W] ") +
                std::filesystem::path(warning.File).filename().string() +
                ":" + std::to_string(warning.Line) + " - " + warning.Message;
            
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));
            ImGui::Selectable(label.c_str());
            ImGui::PopStyleColor();
            
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s\nLine %d, Column %d",
                    warning.File.c_str(), warning.Line, warning.Column);
            }
            
            ImGui::PopID();
        }

        ImGui::EndChild();
    }

    void BuildPanel::RenderCMakeGenerator() {
        ImGui::Text("AI-Powered CMakeLists.txt Generator");
        ImGui::Separator();

        // Ollama status
        if (m_OllamaAvailable) {
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), 
                "Ollama is available - AI generation enabled");
        } else {
            ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.2f, 1.0f),
                "Ollama not running - using template generation");
            ImGui::TextWrapped("Start Ollama with 'ollama serve' for AI-powered CMake generation.");
        }

        ImGui::Separator();

        // Model selection
        char modelBuffer[128];
        strncpy(modelBuffer, m_OllamaModel.c_str(), sizeof(modelBuffer) - 1);
        modelBuffer[sizeof(modelBuffer) - 1] = '\0';
        if (ImGui::InputText("Ollama Model", modelBuffer, sizeof(modelBuffer))) {
            m_OllamaModel = modelBuffer;
            m_BuildManager.SetOllamaModel(m_OllamaModel);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Recommended models:\n- codellama:13b\n- codellama:7b\n- deepseek-coder:6.7b");
        }

        ImGui::Separator();

        // Generate button
        ImGui::BeginDisabled(m_IsGeneratingCMake || m_ProjectPath.empty());
        if (ImGui::Button("Generate CMakeLists.txt", ImVec2(200, 30))) {
            GenerateCMake();
        }
        ImGui::EndDisabled();

        if (m_IsGeneratingCMake) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Generating...");
        }

        ImGui::Separator();

        // Preview
        ImGui::Text("Generated CMakeLists.txt:");
        
        ImGui::BeginChild("CMakePreview", ImVec2(0, -40), true,
            ImGuiWindowFlags_HorizontalScrollbar);
        
        if (!m_GeneratedCMake.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.9f, 0.7f, 1.0f));
            ImGui::TextUnformatted(m_GeneratedCMake.c_str());
            ImGui::PopStyleColor();
        } else {
            ImGui::TextDisabled("No CMakeLists.txt generated yet.\nConfigure your project and click 'Generate'.");
        }
        
        ImGui::EndChild();

        // Action buttons
        ImGui::BeginDisabled(m_GeneratedCMake.empty());
        if (ImGui::Button("Copy to Clipboard")) {
            ImGui::SetClipboardText(m_GeneratedCMake.c_str());
        }
        ImGui::SameLine();
        if (ImGui::Button("Save to Project")) {
            if (!m_ProjectPath.empty()) {
                std::string path = m_ProjectPath + "/CMakeLists.txt";
                std::ofstream file(path);
                if (file.is_open()) {
                    file << m_GeneratedCMake;
                    file.close();
                    AddOutputLine("CMakeLists.txt saved to: " + path);
                }
            }
        }
        ImGui::EndDisabled();
    }

    void BuildPanel::StartBuild() {
        if (m_IsBuilding || m_ProjectPath.empty()) return;

        m_IsBuilding = true;
        m_Errors.clear();
        m_Warnings.clear();
        ClearOutput();

        AddOutputLine("=== Starting Build ===");
        AddOutputLine("Project: " + m_ProjectName);
        AddOutputLine("Target: " + std::string(m_TargetNames[m_SelectedTarget]));
        AddOutputLine("");

        BuildConfig config;
        config.ProjectName = m_ProjectName;
        config.ProjectPath = m_ProjectPath;
        config.OutputDirectory = m_OutputDirectory;
        config.Target = static_cast<BuildTarget>(m_SelectedTarget);

        // First configure
        m_BuildManager.ConfigureAsync(config, [this, config](bool success, const std::string& output) {
            if (!success) {
                AddOutputLine("=== Configuration Failed ===");
                m_IsBuilding = false;
                return;
            }

            AddOutputLine("=== Configuration Complete ===");
            AddOutputLine("");

            // Then build
            m_BuildManager.BuildAsync(config, [this](const BuildResult& result) {
                m_Errors = result.Errors;
                m_Warnings = result.Warnings;
                m_LastBuildTime = result.ElapsedSeconds;
                m_IsBuilding = false;

                AddOutputLine("");
                if (result.Success) {
                    AddOutputLine("=== Build Succeeded ===");
                } else {
                    AddOutputLine("=== Build Failed ===");
                }
                AddOutputLine("Time: " + std::to_string(result.ElapsedSeconds) + "s");
                AddOutputLine("Errors: " + std::to_string(m_Errors.size()));
                AddOutputLine("Warnings: " + std::to_string(m_Warnings.size()));
            });
        });
    }

    void BuildPanel::StartClean() {
        if (m_IsCleaning) return;

        m_IsCleaning = true;
        std::string buildDir = m_OutputDirectory.empty() ?
            m_ProjectPath + "/build" : m_OutputDirectory;

        AddOutputLine("=== Cleaning Build Directory ===");

        m_BuildManager.CleanAsync(buildDir, [this](bool success) {
            m_IsCleaning = false;
            if (success) {
                AddOutputLine("Clean completed successfully");
            } else {
                AddOutputLine("Clean failed");
            }
        });
    }

    void BuildPanel::GenerateCMake() {
        if (m_IsGeneratingCMake) return;

        m_IsGeneratingCMake = true;
        m_GeneratedCMake.clear();

        BuildConfig config;
        config.ProjectName = m_ProjectName;
        config.ProjectPath = m_ProjectPath;
        config.Target = static_cast<BuildTarget>(m_SelectedTarget);

        // Scan for source files
        if (!m_ProjectPath.empty() && std::filesystem::exists(m_ProjectPath)) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(m_ProjectPath)) {
                if (entry.is_regular_file()) {
                    std::string ext = entry.path().extension().string();
                    if (ext == ".cpp" || ext == ".c" || ext == ".cxx") {
                        config.SourceFiles.push_back(
                            std::filesystem::relative(entry.path(), m_ProjectPath).string());
                    }
                }
            }
        }

        // Add standard directories
        config.IncludeDirectories.push_back("${CMAKE_CURRENT_SOURCE_DIR}");
        config.Libraries.push_back("GameEngine::GameEngine");

        m_BuildManager.GenerateCMakeListsAsync(config, [this](const std::string& cmake) {
            m_GeneratedCMake = cmake;
            m_IsGeneratingCMake = false;
        });
    }

    void BuildPanel::AddOutputLine(const std::string& line) {
        std::string formattedLine = line;
        
        if (m_ShowTimestamps) {
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            char timeStr[32];
            std::strftime(timeStr, sizeof(timeStr), "[%H:%M:%S] ", std::localtime(&time));
            formattedLine = timeStr + line;
        }

        m_OutputLines.push_back(formattedLine);
        
        while (m_OutputLines.size() > MAX_OUTPUT_LINES) {
            m_OutputLines.pop_front();
        }
    }

    void BuildPanel::ClearOutput() {
        m_OutputLines.clear();
    }

}
