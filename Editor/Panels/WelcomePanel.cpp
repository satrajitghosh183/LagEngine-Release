#include "WelcomePanel.hpp"
#include "../../Engine/Core/Logger.hpp"
#include <imgui.h>

namespace GameEngine {

    void WelcomePanel::OnImGuiRender() {
        if (!m_Visible) return;
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(480, 360), ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24, 24));
        if (ImGui::Begin("Welcome to GameEngine", &m_Visible, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
            ImGui::TextWrapped("Create a new project from a template, open an existing project, or open a scene to get started.");
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            if (ImGui::Button("New Project from Template...", ImVec2(-1, 0)) && m_OnNewProject) m_OnNewProject();
            if (ImGui::Button("Open Project...", ImVec2(-1, 0)) && m_OnOpenProject) m_OnOpenProject();
            if (ImGui::Button("Open Scene...", ImVec2(-1, 0)) && m_OnOpenScene) m_OnOpenScene();
            ImGui::Spacing();
            if (!m_RecentProjects.empty()) {
                ImGui::Text("Recent Projects:");
                for (const auto& p : m_RecentProjects) {
                    if (ImGui::Selectable(p.c_str(), false, 0, ImVec2(-1, 0)) && m_OnOpenProject) {
                        m_OnOpenProject();
                    }
                }
            }
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("Documentation: Getting Started | Editor Guide | Scripting API");
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }
}
