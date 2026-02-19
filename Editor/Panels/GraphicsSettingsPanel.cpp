#include "GraphicsSettingsPanel.hpp"
#include "../../Engine/Core/Application.hpp"
#include "../../Engine/Core/Logger.hpp"
#include "../../Engine/Graphics/RenderPath.hpp"
#include <imgui.h>

namespace GameEngine {

    GraphicsSettingsPanel::GraphicsSettingsPanel() {
        // Initialize with current runtime config values
        auto* app = Application::GetPtr();
        if (app) {
            const auto& config = app->GetConfig();
            m_ResolutionWidth = static_cast<int>(config.width);
            m_ResolutionHeight = static_cast<int>(config.height);
            m_VSync = config.vsync;
        }
    }

    void GraphicsSettingsPanel::OnImGuiRender() {
        ImGui::Begin("Graphics Settings", &m_IsOpen);

        if (ImGui::CollapsingHeader("Backend Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            RenderBackendSettings();
        }

        if (ImGui::CollapsingHeader("Debug Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            RenderDebugSettings();
        }

        if (ImGui::CollapsingHeader("Post-Process Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            RenderPostProcessSettings();
        }

        if (ImGui::CollapsingHeader("Display Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            RenderDisplaySettings();
        }

        ImGui::Separator();

        if (ImGui::Button("Apply Settings")) {
            auto* app = Application::GetPtr();
            if (app) {
                if (Window* win = app->GetWindowPtr()) {
                    win->SetVSync(m_VSync);
                    win->SetFullscreen(m_Fullscreen);
                }
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Reset to Defaults")) {
            m_CurrentBackend = "OpenGL";
            m_DebugDevice = false;
            m_GPUValidation = false;
            m_GPUVerbose = false;
            m_PreferredGPU = "Auto";
            m_ResolutionWidth = 1920;
            m_ResolutionHeight = 1080;
            m_VSync = true;
            m_Fullscreen = false;
            
            if (m_RenderPath) {
                m_RenderPath->SetFXAAEnabled(false);
                m_RenderPath->SetBloomEnabled(false);
                m_RenderPath->SetSSAOEnabled(false);
            }
        }

        ImGui::End();
    }

    void GraphicsSettingsPanel::RenderBackendSettings() {
        const char* backends[] = { "OpenGL", "Vulkan", "DirectX 12" };
        int currentBackendIndex = 0;
        
        // Find current backend index
        for (int i = 0; i < IM_ARRAYSIZE(backends); i++) {
            if (m_CurrentBackend == backends[i]) {
                currentBackendIndex = i;
                break;
            }
        }

        if (ImGui::Combo("Graphics Backend", &currentBackendIndex, backends, IM_ARRAYSIZE(backends))) {
            m_CurrentBackend = backends[currentBackendIndex];
            GE_CORE_INFO("Graphics backend changed to: {}", m_CurrentBackend);
            // TODO: Apply backend change (requires application restart)
        }

        ImGui::TextWrapped("Note: Backend changes require application restart");

        ImGui::Separator();

        const char* gpuOptions[] = { "Auto", "Integrated", "Nvidia", "AMD", "Intel" };
        int currentGPUIndex = 0;
        
        for (int i = 0; i < IM_ARRAYSIZE(gpuOptions); i++) {
            if (m_PreferredGPU == gpuOptions[i]) {
                currentGPUIndex = i;
                break;
            }
        }

        if (ImGui::Combo("Preferred GPU", &currentGPUIndex, gpuOptions, IM_ARRAYSIZE(gpuOptions))) {
            m_PreferredGPU = gpuOptions[currentGPUIndex];
        }

        ImGui::TextWrapped("Selects which GPU to use when multiple are available");
    }

    void GraphicsSettingsPanel::RenderDebugSettings() {
        if (ImGui::Checkbox("Debug Device", &m_DebugDevice)) {
            GE_CORE_INFO("Debug device: {}", m_DebugDevice ? "enabled" : "disabled");
            // TODO: Apply debug device setting (requires backend reinitialization)
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Enables graphics API validation layer.\nPerformance will be degraded, but graphics warnings and errors will be written to the Output window.");
        }

        if (ImGui::Checkbox("GPU Validation", &m_GPUValidation)) {
            GE_CORE_INFO("GPU validation: {}", m_GPUValidation ? "enabled" : "disabled");
            // TODO: Apply GPU validation setting
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Enables GPU-based validation.\nMust be used together with Debug Device.\nCurrently DX12 only.");
        }

        if (ImGui::Checkbox("GPU Verbose", &m_GPUVerbose)) {
            GE_CORE_INFO("GPU verbose: {}", m_GPUVerbose ? "enabled" : "disabled");
            // TODO: Apply GPU verbose setting
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Enables verbose GPU validation mode.");
        }

        ImGui::Separator();

        ImGui::TextWrapped("Note: Debug settings require application restart to take effect");
    }

    void GraphicsSettingsPanel::RenderPostProcessSettings() {
        if (!m_RenderPath) {
            ImGui::TextDisabled("No active render path");
            return;
        }

        bool fxaa = m_RenderPath->IsFXAAEnabled();
        if (ImGui::Checkbox("FXAA (Fast Approximate Anti-Aliasing)", &fxaa)) {
            m_RenderPath->SetFXAAEnabled(fxaa);
            GE_CORE_INFO("FXAA: {}", fxaa ? "enabled" : "disabled");
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Fast Approximate Anti-Aliasing reduces jagged edges.\nLower performance cost than MSAA.");
        }

        bool bloom = m_RenderPath->IsBloomEnabled();
        if (ImGui::Checkbox("Bloom", &bloom)) {
            m_RenderPath->SetBloomEnabled(bloom);
            GE_CORE_INFO("Bloom: {}", bloom ? "enabled" : "disabled");
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Bloom creates a glowing effect around bright objects.");
        }

        bool ssao = m_RenderPath->IsSSAOEnabled();
        if (ImGui::Checkbox("SSAO (Screen-Space Ambient Occlusion)", &ssao)) {
            m_RenderPath->SetSSAOEnabled(ssao);
            GE_CORE_INFO("SSAO: {}", ssao ? "enabled" : "disabled");
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("SSAO adds subtle shadows in corners and crevices.\nImproves depth perception.");
        }

        ImGui::Separator();
        ImGui::TextWrapped("Post-process effects can be toggled at runtime");
    }

    void GraphicsSettingsPanel::RenderDisplaySettings() {
        auto* app = Application::GetPtr();
        if (!app) {
            ImGui::TextDisabled("Application not available");
            return;
        }

        // Resolution
        int resolution[2] = { m_ResolutionWidth, m_ResolutionHeight };
        if (ImGui::InputInt2("Resolution", resolution)) {
            m_ResolutionWidth = std::max(100, std::min(7680, resolution[0]));
            m_ResolutionHeight = std::max(100, std::min(4320, resolution[1]));
        }

        // Common resolutions
        if (ImGui::Button("1280x720")) {
            m_ResolutionWidth = 1280;
            m_ResolutionHeight = 720;
        }
        ImGui::SameLine();
        if (ImGui::Button("1920x1080")) {
            m_ResolutionWidth = 1920;
            m_ResolutionHeight = 1080;
        }
        ImGui::SameLine();
        if (ImGui::Button("2560x1440")) {
            m_ResolutionWidth = 2560;
            m_ResolutionHeight = 1440;
        }
        ImGui::SameLine();
        if (ImGui::Button("3840x2160")) {
            m_ResolutionWidth = 3840;
            m_ResolutionHeight = 2160;
        }

        ImGui::Separator();

        // VSync
        if (ImGui::Checkbox("VSync", &m_VSync)) {
            auto* app = Application::GetPtr();
            if (app && app->GetWindowPtr()) app->GetWindowPtr()->SetVSync(m_VSync);
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Vertical synchronization limits frame rate to monitor refresh rate.\nReduces screen tearing but may increase input latency.");
        }

        // Fullscreen
        if (ImGui::Checkbox("Fullscreen", &m_Fullscreen)) {
            auto* app = Application::GetPtr();
            if (app && app->GetWindowPtr()) app->GetWindowPtr()->SetFullscreen(m_Fullscreen);
        }

        ImGui::Separator();
        ImGui::TextWrapped("Display settings can be applied at runtime");
    }

}