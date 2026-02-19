#include "PresetBrowserPanel.hpp"
#include "../../Engine/Core/Logger.hpp"

namespace GameEngine {

    static const char* s_Categories[] = {
        "All", "Physics", "Graphics", "Particles", "Robotics", "2D", "Fluid", "General"
    };
    static constexpr int s_CategoryCount = sizeof(s_Categories) / sizeof(s_Categories[0]);

    void PresetBrowserPanel::OnImGuiRender()
    {
        if (!ImGui::Begin("Preset Browser"))
        {
            ImGui::End();
            return;
        }

        const auto& presets = PresetManager::GetPresets();

        // Category filter -----------------------------------------------------
        ImGui::Text("Category:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(180.0f);
        ImGui::Combo("##CategoryFilter", &m_SelectedCategory,
                      s_Categories, s_CategoryCount);

        ImGui::SameLine();
        if (ImGui::Button("Refresh"))
        {
            PresetManager::Refresh();
            m_SelectedPreset = -1;
        }

        ImGui::Separator();

        // Preset list ---------------------------------------------------------
        ImGui::BeginChild("PresetList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 4),
                          true);

        int visibleIndex = 0;
        for (int i = 0; i < static_cast<int>(presets.size()); i++)
        {
            const auto& preset = presets[i];

            // Apply category filter
            if (m_SelectedCategory != 0) {
                const char* filterCat = s_Categories[m_SelectedCategory];
                if (preset.Category != filterCat)
                    continue;
            }

            ImGui::PushID(i);

            bool isSelected = (m_SelectedPreset == i);

            // Selectable row
            if (ImGui::Selectable("##preset", isSelected,
                                  ImGuiSelectableFlags_AllowDoubleClick,
                                  ImVec2(0, 48)))
            {
                m_SelectedPreset = i;

                // Double-click to load
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    m_ShowConfirmation = true;
            }

            // Overlay information
            ImVec2 rectMin = ImGui::GetItemRectMin();
            ImGui::SetCursorScreenPos(ImVec2(rectMin.x + 8.0f, rectMin.y + 4.0f));
            ImGui::Text("%s", preset.Name.c_str());

            ImGui::SameLine(0.0f, 12.0f);
            ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "[%s]",
                               preset.Category.c_str());

            if (!preset.Description.empty()) {
                ImGui::SetCursorScreenPos(ImVec2(rectMin.x + 8.0f, rectMin.y + 24.0f));
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s",
                                   preset.Description.c_str());
            }

            ImGui::PopID();
            visibleIndex++;
        }

        if (visibleIndex == 0)
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No presets found.");

        ImGui::EndChild();

        // Load button ---------------------------------------------------------
        bool canLoad = (m_SelectedPreset >= 0 &&
                        m_SelectedPreset < static_cast<int>(presets.size()));

        if (!canLoad)
            ImGui::BeginDisabled();

        if (ImGui::Button("Load Preset", ImVec2(140, 0)))
            m_ShowConfirmation = true;

        if (!canLoad)
            ImGui::EndDisabled();

        // Confirmation dialog -------------------------------------------------
        if (m_ShowConfirmation && canLoad)
        {
            ImGui::OpenPopup("Load Preset?");
            m_ShowConfirmation = false;  // Only open once
        }

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Load Preset?", nullptr,
                                    ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Loading a preset will replace the current scene.");
            ImGui::Text("Any unsaved changes will be lost.");
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("Load", ImVec2(120, 0)))
            {
                const auto& preset = presets[m_SelectedPreset];
                GE_CORE_INFO("PresetBrowserPanel: loading preset '{}'", preset.Name);

                if (m_Callback)
                    m_Callback(preset.FilePath);

                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::End();
    }
}
