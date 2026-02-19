#include "NewProjectDialog.hpp"
#include "../../Engine/Core/Logger.hpp"
#include "../../Engine/Core/ProjectFile.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace GameEngine {

    void NewProjectDialog::Open()
    {
        m_IsOpen = true;
        m_SelectedTemplate = -1;
        ImGui::OpenPopup("New Project from Template");
    }

    void NewProjectDialog::OnImGuiRender()
    {
        if (!m_IsOpen)
            return;

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(720, 520), ImGuiCond_Appearing);

        if (ImGui::BeginPopupModal("New Project from Template", &m_IsOpen,
                                    ImGuiWindowFlags_NoResize))
        {
            const auto& templates = TemplateManager::GetTemplates();

            ImGui::Text("Select a template:");
            ImGui::Separator();

            // Template grid --------------------------------------------------
            float panelWidth = ImGui::GetContentRegionAvail().x;
            int columns = 3;
            float cellWidth = panelWidth / static_cast<float>(columns);

            ImGui::BeginChild("TemplateGrid", ImVec2(0, 300), true);
            ImGui::Columns(columns, nullptr, false);

            for (int i = 0; i < static_cast<int>(templates.size()); i++)
            {
                const auto& tmpl = templates[i];
                ImGui::PushID(i);

                bool isSelected = (m_SelectedTemplate == i);

                // Card-style selectable
                ImVec2 cardSize(cellWidth - 16.0f, 80.0f);
                if (ImGui::Selectable("##card", isSelected, 0, cardSize))
                    m_SelectedTemplate = i;

                // Overlay text on the selectable
                ImVec2 rectMin = ImGui::GetItemRectMin();
                ImGui::SetCursorScreenPos(ImVec2(rectMin.x + 8.0f, rectMin.y + 8.0f));
                ImGui::Text("%s", tmpl.Name.c_str());
                ImGui::SetCursorScreenPos(ImVec2(rectMin.x + 8.0f, rectMin.y + 28.0f));
                ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "[%s]",
                                   tmpl.Category.c_str());
                if (!tmpl.Description.empty()) {
                    ImGui::SetCursorScreenPos(ImVec2(rectMin.x + 8.0f, rectMin.y + 48.0f));
                    ImGui::TextWrapped("%s", tmpl.Description.c_str());
                }

                ImGui::PopID();
                ImGui::NextColumn();
            }

            ImGui::Columns(1);
            ImGui::EndChild();

            // Project settings ------------------------------------------------
            ImGui::Spacing();
            ImGui::InputText("Project Name", m_ProjectName, sizeof(m_ProjectName));
            ImGui::InputText("Target Directory", m_ProjectPath, sizeof(m_ProjectPath));

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Buttons ---------------------------------------------------------
            bool canCreate = (m_SelectedTemplate >= 0 &&
                              m_SelectedTemplate < static_cast<int>(templates.size()) &&
                              strlen(m_ProjectName) > 0 &&
                              strlen(m_ProjectPath) > 0);

            if (!canCreate)
                ImGui::BeginDisabled();

            if (ImGui::Button("Create", ImVec2(120, 0)))
            {
                const auto& tmpl = templates[m_SelectedTemplate];
                fs::path targetDir = fs::path(m_ProjectPath) / m_ProjectName;

                if (TemplateManager::CreateProjectFromTemplate(tmpl, targetDir.string()))
                {
                    GE_CORE_INFO("NewProjectDialog: project created at '{}'",
                                 targetDir.string());

                    // Create .geproject file
                    ProjectFile proj;
                    proj.Name = m_ProjectName;
                    proj.DefaultScene = tmpl.SceneFile.empty() ? "" : tmpl.SceneFile;
                    proj.AssetsRoot = "Assets";
                    ProjectFile::Save(targetDir.string(), proj);

                    if (m_Callback) {
                        // Build the path to the scene file inside the new project
                        std::string scenePath;
                        if (!tmpl.SceneFile.empty())
                            scenePath = (targetDir / tmpl.SceneFile).string();
                        else
                            scenePath = targetDir.string();

                        m_Callback(scenePath);
                    }

                    m_IsOpen = false;
                    ImGui::CloseCurrentPopup();
                }
            }

            if (!canCreate)
                ImGui::EndDisabled();

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                m_IsOpen = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
}
