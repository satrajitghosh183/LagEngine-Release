#include "ProjectSettingsPanel.hpp"
#include "../../Engine/Core/Logger.hpp"
#include <imgui.h>
#include <fstream>
#include <nlohmann/json.hpp>

namespace GameEngine {

    ProjectSettingsPanel::ProjectSettingsPanel() {
    }

    void ProjectSettingsPanel::OnImGuiRender() {
        if (!m_IsOpen) return;
        
        ImGui::Begin("Project Settings", nullptr, ImGuiWindowFlags_MenuBar);

        // Menu bar
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Save", "Ctrl+S")) {
                    if (!m_SettingsPath.empty()) {
                        SaveSettings(m_SettingsPath);
                    }
                }
                if (ImGui::MenuItem("Reset to Defaults")) {
                    ResetToDefaults();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // Modified indicator
        if (m_Modified) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "* Unsaved changes");
        }

        ImGui::Separator();

        // Two-panel layout: categories on left, settings on right
        float categoryWidth = 150.0f;

        // Category list
        ImGui::BeginChild("Categories", ImVec2(categoryWidth, 0), true);
        for (int i = 0; i < 8; i++) {
            bool selected = (m_SelectedCategory == i);
            if (ImGui::Selectable(m_Categories[i], selected)) {
                m_SelectedCategory = i;
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();

        // Settings panel
        ImGui::BeginChild("SettingsContent", ImVec2(0, 0), true);
        
        switch (m_SelectedCategory) {
            case 0: RenderGeneralSettings(); break;
            case 1: RenderGraphicsSettings(); break;
            case 2: RenderPhysicsSettings(); break;
            case 3: RenderAudioSettings(); break;
            case 4: RenderInputSettings(); break;
            case 5: RenderQualitySettings(); break;
            case 6: RenderBuildSettings(); break;
            case 7: RenderScenesSettings(); break;
        }
        
        ImGui::EndChild();

        ImGui::End();
    }

    void ProjectSettingsPanel::RenderGeneralSettings() {
        ImGui::Text("General Settings");
        ImGui::Separator();

        bool changed = false;

        char buffer[256];
        
        strncpy(buffer, m_Settings.ProjectName.c_str(), sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';
        if (ImGui::InputText("Project Name", buffer, sizeof(buffer))) {
            m_Settings.ProjectName = buffer;
            changed = true;
        }

        strncpy(buffer, m_Settings.CompanyName.c_str(), sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';
        if (ImGui::InputText("Company Name", buffer, sizeof(buffer))) {
            m_Settings.CompanyName = buffer;
            changed = true;
        }

        strncpy(buffer, m_Settings.Version.c_str(), sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';
        if (ImGui::InputText("Version", buffer, sizeof(buffer))) {
            m_Settings.Version = buffer;
            changed = true;
        }

        char descBuffer[1024];
        strncpy(descBuffer, m_Settings.Description.c_str(), sizeof(descBuffer) - 1);
        descBuffer[sizeof(descBuffer) - 1] = '\0';
        if (ImGui::InputTextMultiline("Description", descBuffer, sizeof(descBuffer), ImVec2(-1, 100))) {
            m_Settings.Description = descBuffer;
            changed = true;
        }

        if (changed) {
            m_Modified = true;
            if (m_OnSettingsChangedCallback) m_OnSettingsChangedCallback();
        }
    }

    void ProjectSettingsPanel::RenderGraphicsSettings() {
        ImGui::Text("Graphics Settings");
        ImGui::Separator();

        bool changed = false;

        ImGui::Text("Resolution");
        changed |= ImGui::InputInt("Width", &m_Settings.DefaultWidth);
        changed |= ImGui::InputInt("Height", &m_Settings.DefaultHeight);
        
        ImGui::Separator();

        changed |= ImGui::Checkbox("Fullscreen", &m_Settings.Fullscreen);
        changed |= ImGui::Checkbox("VSync", &m_Settings.VSync);
        
        if (!m_Settings.VSync) {
            changed |= ImGui::SliderInt("Target FPS", &m_Settings.TargetFPS, 30, 240);
        }

        ImGui::Separator();
        ImGui::Text("Anti-Aliasing");
        
        const char* msaaOptions[] = { "Off", "2x", "4x", "8x" };
        int msaaIndex = 0;
        if (m_Settings.MSAASamples == 2) msaaIndex = 1;
        else if (m_Settings.MSAASamples == 4) msaaIndex = 2;
        else if (m_Settings.MSAASamples == 8) msaaIndex = 3;
        
        if (ImGui::Combo("MSAA", &msaaIndex, msaaOptions, 4)) {
            const int msaaValues[] = { 0, 2, 4, 8 };
            m_Settings.MSAASamples = msaaValues[msaaIndex];
            changed = true;
        }

        ImGui::Separator();
        
        changed |= ImGui::Checkbox("HDR Rendering", &m_Settings.HDR);
        changed |= ImGui::SliderFloat("Render Scale", &m_Settings.RenderScale, 0.25f, 2.0f, "%.2f");

        if (changed) {
            m_Modified = true;
            if (m_OnSettingsChangedCallback) m_OnSettingsChangedCallback();
        }
    }

    void ProjectSettingsPanel::RenderPhysicsSettings() {
        ImGui::Text("Physics Settings");
        ImGui::Separator();

        bool changed = false;

        changed |= ImGui::DragFloat("Gravity", &m_Settings.Gravity, 0.1f, -20.0f, 0.0f);
        changed |= ImGui::SliderInt("Physics Substeps", &m_Settings.PhysicsSubsteps, 1, 16);
        
        float fps = 1.0f / m_Settings.FixedTimestep;
        if (ImGui::SliderFloat("Fixed Update Rate", &fps, 30.0f, 240.0f, "%.0f Hz")) {
            m_Settings.FixedTimestep = 1.0f / fps;
            changed = true;
        }

        ImGui::Separator();
        changed |= ImGui::Checkbox("Physics Debug Draw", &m_Settings.PhysicsDebugDraw);

        if (changed) {
            m_Modified = true;
            if (m_OnSettingsChangedCallback) m_OnSettingsChangedCallback();
        }
    }

    void ProjectSettingsPanel::RenderAudioSettings() {
        ImGui::Text("Audio Settings");
        ImGui::Separator();

        bool changed = false;

        changed |= ImGui::SliderFloat("Master Volume", &m_Settings.MasterVolume, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Music Volume", &m_Settings.MusicVolume, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("SFX Volume", &m_Settings.SFXVolume, 0.0f, 1.0f);
        
        ImGui::Separator();
        
        changed |= ImGui::SliderInt("Max Audio Sources", &m_Settings.MaxAudioSources, 8, 128);

        if (changed) {
            m_Modified = true;
            if (m_OnSettingsChangedCallback) m_OnSettingsChangedCallback();
        }
    }

    void ProjectSettingsPanel::RenderInputSettings() {
        ImGui::Text("Input Settings");
        ImGui::Separator();

        bool changed = false;

        changed |= ImGui::SliderFloat("Mouse Sensitivity", &m_Settings.MouseSensitivity, 0.1f, 5.0f);
        changed |= ImGui::Checkbox("Invert Y-Axis", &m_Settings.InvertY);
        changed |= ImGui::SliderFloat("Controller Dead Zone", &m_Settings.DeadZone, 0.0f, 0.5f);

        if (changed) {
            m_Modified = true;
            if (m_OnSettingsChangedCallback) m_OnSettingsChangedCallback();
        }
    }

    void ProjectSettingsPanel::RenderQualitySettings() {
        ImGui::Text("Quality Settings");
        ImGui::Separator();

        bool changed = false;

        ImGui::Text("Shadows");
        
        const char* shadowResOptions[] = { "512", "1024", "2048", "4096" };
        int shadowResIndex = 2;
        if (m_Settings.ShadowMapResolution == 512) shadowResIndex = 0;
        else if (m_Settings.ShadowMapResolution == 1024) shadowResIndex = 1;
        else if (m_Settings.ShadowMapResolution == 2048) shadowResIndex = 2;
        else if (m_Settings.ShadowMapResolution == 4096) shadowResIndex = 3;
        
        if (ImGui::Combo("Shadow Resolution", &shadowResIndex, shadowResOptions, 4)) {
            const int resValues[] = { 512, 1024, 2048, 4096 };
            m_Settings.ShadowMapResolution = resValues[shadowResIndex];
            changed = true;
        }

        changed |= ImGui::SliderInt("Shadow Cascades", &m_Settings.ShadowCascades, 1, 4);
        changed |= ImGui::SliderFloat("Shadow Distance", &m_Settings.ShadowDistance, 10.0f, 500.0f);

        ImGui::Separator();
        ImGui::Text("Post Processing");

        changed |= ImGui::Checkbox("SSAO", &m_Settings.SSAO);
        changed |= ImGui::Checkbox("Bloom", &m_Settings.Bloom);

        ImGui::Separator();
        ImGui::Text("Textures");

        const char* texQualityOptions[] = { "Low", "Medium", "High" };
        changed |= ImGui::Combo("Texture Quality", &m_Settings.TextureQuality, texQualityOptions, 3);

        if (changed) {
            m_Modified = true;
            if (m_OnSettingsChangedCallback) m_OnSettingsChangedCallback();
        }
    }

    void ProjectSettingsPanel::RenderBuildSettings() {
        ImGui::Text("Build Settings");
        ImGui::Separator();

        bool changed = false;

        char buffer[256];
        strncpy(buffer, m_Settings.OutputDirectory.c_str(), sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';
        if (ImGui::InputText("Output Directory", buffer, sizeof(buffer))) {
            m_Settings.OutputDirectory = buffer;
            changed = true;
        }

        ImGui::Separator();

        changed |= ImGui::Checkbox("Include Debug Symbols", &m_Settings.IncludeDebugSymbols);
        changed |= ImGui::Checkbox("Compress Assets", &m_Settings.CompressAssets);

        ImGui::Separator();
        ImGui::Text("Excluded Directories:");
        
        for (size_t i = 0; i < m_Settings.ExcludedDirectories.size(); i++) {
            ImGui::PushID(static_cast<int>(i));
            
            char dirBuffer[256];
            strncpy(dirBuffer, m_Settings.ExcludedDirectories[i].c_str(), sizeof(dirBuffer) - 1);
            dirBuffer[sizeof(dirBuffer) - 1] = '\0';
            
            if (ImGui::InputText("##excluded", dirBuffer, sizeof(dirBuffer))) {
                m_Settings.ExcludedDirectories[i] = dirBuffer;
                changed = true;
            }
            
            ImGui::SameLine();
            if (ImGui::Button("X")) {
                m_Settings.ExcludedDirectories.erase(m_Settings.ExcludedDirectories.begin() + i);
                changed = true;
                ImGui::PopID();
                break;
            }
            
            ImGui::PopID();
        }

        if (ImGui::Button("Add Excluded Directory")) {
            m_Settings.ExcludedDirectories.push_back("");
            changed = true;
        }

        if (changed) {
            m_Modified = true;
            if (m_OnSettingsChangedCallback) m_OnSettingsChangedCallback();
        }
    }

    void ProjectSettingsPanel::RenderScenesSettings() {
        ImGui::Text("Scene Settings");
        ImGui::Separator();

        bool changed = false;

        char buffer[256];
        strncpy(buffer, m_Settings.StartupScene.c_str(), sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';
        if (ImGui::InputText("Startup Scene", buffer, sizeof(buffer))) {
            m_Settings.StartupScene = buffer;
            changed = true;
        }

        ImGui::Separator();
        ImGui::Text("Included Scenes in Build:");

        for (size_t i = 0; i < m_Settings.IncludedScenes.size(); i++) {
            ImGui::PushID(static_cast<int>(i));

            char sceneBuffer[256];
            strncpy(sceneBuffer, m_Settings.IncludedScenes[i].c_str(), sizeof(sceneBuffer) - 1);
            sceneBuffer[sizeof(sceneBuffer) - 1] = '\0';
            
            if (ImGui::InputText("##scene", sceneBuffer, sizeof(sceneBuffer))) {
                m_Settings.IncludedScenes[i] = sceneBuffer;
                changed = true;
            }

            ImGui::SameLine();
            if (ImGui::Button("X")) {
                m_Settings.IncludedScenes.erase(m_Settings.IncludedScenes.begin() + i);
                changed = true;
                ImGui::PopID();
                break;
            }

            ImGui::PopID();
        }

        if (ImGui::Button("Add Scene")) {
            m_Settings.IncludedScenes.push_back("");
            changed = true;
        }

        if (changed) {
            m_Modified = true;
            if (m_OnSettingsChangedCallback) m_OnSettingsChangedCallback();
        }
    }

    void ProjectSettingsPanel::LoadSettings(const std::string& path) {
        try {
            std::ifstream file(path);
            if (!file.is_open()) {
                GE_CORE_ERROR("Failed to open project settings: {}", path);
                return;
            }

            nlohmann::json j;
            file >> j;

            // General
            m_Settings.ProjectName = j.value("projectName", "My Project");
            m_Settings.CompanyName = j.value("companyName", "");
            m_Settings.Version = j.value("version", "1.0.0");
            m_Settings.Description = j.value("description", "");

            // Graphics
            if (j.contains("graphics")) {
                auto& g = j["graphics"];
                m_Settings.DefaultWidth = g.value("width", 1920);
                m_Settings.DefaultHeight = g.value("height", 1080);
                m_Settings.Fullscreen = g.value("fullscreen", false);
                m_Settings.VSync = g.value("vsync", true);
                m_Settings.TargetFPS = g.value("targetFPS", 60);
                m_Settings.MSAASamples = g.value("msaa", 4);
                m_Settings.HDR = g.value("hdr", false);
                m_Settings.RenderScale = g.value("renderScale", 1.0f);
            }

            // Physics
            if (j.contains("physics")) {
                auto& p = j["physics"];
                m_Settings.Gravity = p.value("gravity", -9.81f);
                m_Settings.PhysicsSubsteps = p.value("substeps", 4);
                m_Settings.FixedTimestep = p.value("fixedTimestep", 1.0f / 60.0f);
                m_Settings.PhysicsDebugDraw = p.value("debugDraw", false);
            }

            // Audio
            if (j.contains("audio")) {
                auto& a = j["audio"];
                m_Settings.MasterVolume = a.value("masterVolume", 1.0f);
                m_Settings.MusicVolume = a.value("musicVolume", 1.0f);
                m_Settings.SFXVolume = a.value("sfxVolume", 1.0f);
                m_Settings.MaxAudioSources = a.value("maxSources", 32);
            }

            // Quality
            if (j.contains("quality")) {
                auto& q = j["quality"];
                m_Settings.ShadowMapResolution = q.value("shadowRes", 2048);
                m_Settings.ShadowCascades = q.value("shadowCascades", 4);
                m_Settings.ShadowDistance = q.value("shadowDistance", 100.0f);
                m_Settings.SSAO = q.value("ssao", true);
                m_Settings.Bloom = q.value("bloom", true);
                m_Settings.TextureQuality = q.value("textureQuality", 2);
            }

            // Build
            if (j.contains("build")) {
                auto& b = j["build"];
                m_Settings.OutputDirectory = b.value("outputDir", "Build");
                m_Settings.IncludeDebugSymbols = b.value("debugSymbols", false);
                m_Settings.CompressAssets = b.value("compressAssets", true);
                if (b.contains("excludedDirs")) {
                    m_Settings.ExcludedDirectories = b["excludedDirs"].get<std::vector<std::string>>();
                }
            }

            // Scenes
            if (j.contains("scenes")) {
                auto& s = j["scenes"];
                m_Settings.StartupScene = s.value("startup", "");
                if (s.contains("included")) {
                    m_Settings.IncludedScenes = s["included"].get<std::vector<std::string>>();
                }
            }

            m_SettingsPath = path;
            m_Modified = false;

            GE_CORE_INFO("Loaded project settings from: {}", path);
        } catch (const std::exception& e) {
            GE_CORE_ERROR("Failed to load project settings: {}", e.what());
        }
    }

    void ProjectSettingsPanel::SaveSettings(const std::string& path) {
        try {
            nlohmann::json j;

            // General
            j["projectName"] = m_Settings.ProjectName;
            j["companyName"] = m_Settings.CompanyName;
            j["version"] = m_Settings.Version;
            j["description"] = m_Settings.Description;

            // Graphics
            j["graphics"]["width"] = m_Settings.DefaultWidth;
            j["graphics"]["height"] = m_Settings.DefaultHeight;
            j["graphics"]["fullscreen"] = m_Settings.Fullscreen;
            j["graphics"]["vsync"] = m_Settings.VSync;
            j["graphics"]["targetFPS"] = m_Settings.TargetFPS;
            j["graphics"]["msaa"] = m_Settings.MSAASamples;
            j["graphics"]["hdr"] = m_Settings.HDR;
            j["graphics"]["renderScale"] = m_Settings.RenderScale;

            // Physics
            j["physics"]["gravity"] = m_Settings.Gravity;
            j["physics"]["substeps"] = m_Settings.PhysicsSubsteps;
            j["physics"]["fixedTimestep"] = m_Settings.FixedTimestep;
            j["physics"]["debugDraw"] = m_Settings.PhysicsDebugDraw;

            // Audio
            j["audio"]["masterVolume"] = m_Settings.MasterVolume;
            j["audio"]["musicVolume"] = m_Settings.MusicVolume;
            j["audio"]["sfxVolume"] = m_Settings.SFXVolume;
            j["audio"]["maxSources"] = m_Settings.MaxAudioSources;

            // Quality
            j["quality"]["shadowRes"] = m_Settings.ShadowMapResolution;
            j["quality"]["shadowCascades"] = m_Settings.ShadowCascades;
            j["quality"]["shadowDistance"] = m_Settings.ShadowDistance;
            j["quality"]["ssao"] = m_Settings.SSAO;
            j["quality"]["bloom"] = m_Settings.Bloom;
            j["quality"]["textureQuality"] = m_Settings.TextureQuality;

            // Build
            j["build"]["outputDir"] = m_Settings.OutputDirectory;
            j["build"]["debugSymbols"] = m_Settings.IncludeDebugSymbols;
            j["build"]["compressAssets"] = m_Settings.CompressAssets;
            j["build"]["excludedDirs"] = m_Settings.ExcludedDirectories;

            // Scenes
            j["scenes"]["startup"] = m_Settings.StartupScene;
            j["scenes"]["included"] = m_Settings.IncludedScenes;

            std::ofstream file(path);
            file << j.dump(2);
            file.close();

            m_SettingsPath = path;
            m_Modified = false;

            GE_CORE_INFO("Saved project settings to: {}", path);
        } catch (const std::exception& e) {
            GE_CORE_ERROR("Failed to save project settings: {}", e.what());
        }
    }

    void ProjectSettingsPanel::ResetToDefaults() {
        m_Settings = ProjectSettings();
        m_Modified = true;
        
        if (m_OnSettingsChangedCallback) {
            m_OnSettingsChangedCallback();
        }
    }

}
