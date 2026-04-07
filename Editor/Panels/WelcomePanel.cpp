#include "WelcomePanel.hpp"
#include "../../Engine/Core/Logger.hpp"
#include <imgui.h>
#include <algorithm>
#include <cstring>

namespace GameEngine {

    static const char* s_Categories[] = {
        "All", "Physics", "Rendering", "Robotics", "2D", "Education"
    };
    static constexpr int s_CategoryCount = sizeof(s_Categories) / sizeof(s_Categories[0]);

    void WelcomePanel::OnImGuiRender() {
        if (!m_Visible) return;

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImVec2 winSize(820, 620);
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(winSize, ImGuiCond_Appearing);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 16));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
        if (ImGui::Begin("Welcome to LAG Engine", &m_Visible, flags)) {
            RenderHeader();
            ImGui::Spacing();

            // Tab bar for sections
            if (ImGui::BeginTabBar("WelcomeTabs")) {
                if (ImGui::BeginTabItem("Get Started")) {
                    RenderQuickStart();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Demo Scenes")) {
                    RenderDemoGrid();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Recent")) {
                    RenderRecentProjects();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::End();
        ImGui::PopStyleVar(2);
    }

    void WelcomePanel::RenderHeader() {
        // Title
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts.Size > 0 ? ImGui::GetIO().Fonts->Fonts[0] : nullptr);
        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "LAG Engine");
        ImGui::PopFont();
        ImGui::SameLine();
        ImGui::TextDisabled("v1.0.0");
        ImGui::TextWrapped("A modular, cross-platform 3D/2D game engine for education and research.");
        ImGui::Separator();
    }

    void WelcomePanel::RenderQuickStart() {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Quick Start");
        ImGui::Spacing();

        float btnWidth = (ImGui::GetContentRegionAvail().x - 12.0f) / 2.0f;
        float btnHeight = 40.0f;

        // Row 1
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 0.9f, 0.9f));
        if (ImGui::Button("New Empty Scene", ImVec2(btnWidth, btnHeight)) && m_OnNewScene)
            m_OnNewScene();
        ImGui::PopStyleColor(2);

        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.3f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.4f, 0.9f));
        if (ImGui::Button("New Project from Template", ImVec2(btnWidth, btnHeight)) && m_OnNewProject)
            m_OnNewProject();
        ImGui::PopStyleColor(2);

        // Row 2
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.4f, 0.8f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.5f, 0.9f, 0.9f));
        if (ImGui::Button("Open Project...", ImVec2(btnWidth, btnHeight)) && m_OnOpenProject)
            m_OnOpenProject();
        ImGui::PopStyleColor(2);

        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.5f, 0.2f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.6f, 0.3f, 0.9f));
        if (ImGui::Button("Open Scene...", ImVec2(btnWidth, btnHeight)) && m_OnOpenScene)
            m_OnOpenScene();
        ImGui::PopStyleColor(2);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Quick demo section - show first 6 demos as quick-access buttons
        if (!m_Demos.empty()) {
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "One-Click Demos");
            ImGui::TextWrapped("Load a pre-built interactive demo scene instantly:");
            ImGui::Spacing();

            int cols = 3;
            float cardW = (ImGui::GetContentRegionAvail().x - (cols - 1) * 8.0f) / cols;

            int shown = 0;
            for (size_t i = 0; i < m_Demos.size() && shown < 6; i++, shown++) {
                if (shown % cols != 0) ImGui::SameLine(0, 8.0f);

                ImGui::PushID(static_cast<int>(i));
                ImGui::PushStyleColor(ImGuiCol_Button, m_Demos[i].AccentColor);
                ImVec4 hovered = m_Demos[i].AccentColor;
                hovered.x = std::min(hovered.x + 0.15f, 1.0f);
                hovered.y = std::min(hovered.y + 0.15f, 1.0f);
                hovered.z = std::min(hovered.z + 0.15f, 1.0f);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hovered);
                ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));

                if (ImGui::Button(m_Demos[i].Name.c_str(), ImVec2(cardW, 36.0f))) {
                    if (m_Demos[i].OnLoad) {
                        m_PendingAction = m_Demos[i].OnLoad;
                        m_Visible = false;
                    }
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s\n\nCategory: %s",
                        m_Demos[i].Description.c_str(),
                        m_Demos[i].Category.c_str());
                }

                ImGui::PopStyleVar();
                ImGui::PopStyleColor(2);
                ImGui::PopID();
            }

            ImGui::Spacing();
            ImGui::TextDisabled("See the 'Demo Scenes' tab for all %zu demos", m_Demos.size());
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextDisabled("Documentation: Getting Started | Editor Guide | Scripting API | Tutorials");
    }

    void WelcomePanel::RenderDemoGrid() {
        ImGui::Spacing();

        // Search bar
        ImGui::SetNextItemWidth(250.0f);
        ImGui::InputTextWithHint("##search", "Search demos...", m_SearchBuf, sizeof(m_SearchBuf));
        std::string searchLower = m_SearchBuf;
        std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

        ImGui::SameLine();

        // Category filter
        ImGui::SetNextItemWidth(130.0f);
        ImGui::Combo("##category", &m_SelectedCategory, s_Categories, s_CategoryCount);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (m_Demos.empty()) {
            ImGui::TextDisabled("No demos registered.");
            return;
        }

        // Filter demos
        std::vector<const DemoEntry*> filtered;
        for (auto& d : m_Demos) {
            // Category filter
            if (m_SelectedCategory > 0) {
                if (d.Category != s_Categories[m_SelectedCategory]) continue;
            }
            // Search filter
            if (!searchLower.empty()) {
                std::string nameLower = d.Name;
                std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
                std::string descLower = d.Description;
                std::transform(descLower.begin(), descLower.end(), descLower.begin(), ::tolower);
                if (nameLower.find(searchLower) == std::string::npos &&
                    descLower.find(searchLower) == std::string::npos)
                    continue;
            }
            filtered.push_back(&d);
        }

        if (filtered.empty()) {
            ImGui::TextDisabled("No demos match your search.");
            return;
        }

        // Render demo cards in a grid
        int cols = 3;
        float cardW = (ImGui::GetContentRegionAvail().x - (cols - 1) * 10.0f) / cols;
        float cardH = 110.0f;

        ImGui::BeginChild("DemoScrollArea", ImVec2(0, 0), false);
        for (size_t i = 0; i < filtered.size(); i++) {
            if (i % cols != 0) ImGui::SameLine(0, 10.0f);
            RenderDemoCard(*filtered[i], cardW, cardH);
        }
        ImGui::EndChild();
    }

    void WelcomePanel::RenderDemoCard(const DemoEntry& demo, float cardWidth, float cardHeight) {
        ImGui::PushID(demo.Name.c_str());

        ImVec2 cursor = ImGui::GetCursorScreenPos();

        // Card background
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 cardMin = cursor;
        ImVec2 cardMax(cursor.x + cardWidth, cursor.y + cardHeight);

        // Background
        draw->AddRectFilled(cardMin, cardMax,
            ImGui::ColorConvertFloat4ToU32(ImVec4(0.15f, 0.15f, 0.18f, 1.0f)), 6.0f);

        // Accent stripe at top
        ImVec2 stripeMax(cardMax.x, cardMin.y + 4.0f);
        draw->AddRectFilled(cardMin, stripeMax,
            ImGui::ColorConvertFloat4ToU32(demo.AccentColor), 6.0f, ImDrawFlags_RoundCornersTop);

        // Invisible button for click detection
        ImGui::InvisibleButton("card", ImVec2(cardWidth, cardHeight));
        bool hovered = ImGui::IsItemHovered();
        bool clicked = ImGui::IsItemClicked();

        if (hovered) {
            draw->AddRect(cardMin, cardMax,
                ImGui::ColorConvertFloat4ToU32(ImVec4(0.5f, 0.7f, 1.0f, 0.8f)), 6.0f, 0, 2.0f);
        }

        if (clicked && demo.OnLoad) {
            // Store callback and defer execution until after ImGui frame completes
            m_PendingAction = demo.OnLoad;
            m_Visible = false;
        }

        // Content
        float pad = 8.0f;
        float textY = cardMin.y + 10.0f;

        // Icon / Label
        draw->AddText(ImVec2(cardMin.x + pad, textY),
            ImGui::ColorConvertFloat4ToU32(demo.AccentColor),
            demo.IconLabel.c_str());

        // Name
        draw->AddText(ImVec2(cardMin.x + pad + 24.0f, textY),
            ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)),
            demo.Name.c_str());

        // Category badge
        std::string badge = "[" + demo.Category + "]";
        ImVec2 badgeSize = ImGui::CalcTextSize(badge.c_str());
        draw->AddText(ImVec2(cardMax.x - badgeSize.x - pad, textY),
            ImGui::ColorConvertFloat4ToU32(ImVec4(0.6f, 0.6f, 0.6f, 1.0f)),
            badge.c_str());

        // Description (wrapped manually)
        float descY = textY + 22.0f;
        float maxDescW = cardWidth - 2 * pad;
        // Simple single-line truncation
        std::string desc = demo.Description;
        ImVec2 descSize = ImGui::CalcTextSize(desc.c_str());
        if (descSize.x > maxDescW * 2) {
            // Truncate with ellipsis
            while (desc.size() > 10 && ImGui::CalcTextSize(desc.c_str()).x > maxDescW * 2) {
                desc.pop_back();
            }
            desc += "...";
        }

        // Draw description text with word wrap
        draw->AddText(nullptr, 0.0f, ImVec2(cardMin.x + pad, descY),
            ImGui::ColorConvertFloat4ToU32(ImVec4(0.7f, 0.7f, 0.7f, 1.0f)),
            desc.c_str(), desc.c_str() + desc.size(), maxDescW);

        // "Launch" hint at bottom
        if (hovered) {
            draw->AddText(ImVec2(cardMin.x + pad, cardMax.y - 20.0f),
                ImGui::ColorConvertFloat4ToU32(ImVec4(0.4f, 0.8f, 0.4f, 1.0f)),
                "Click to launch");
        }

        // Tooltip on hover
        if (hovered) {
            ImGui::SetTooltip("%s\n\n%s\nCategory: %s",
                demo.Name.c_str(), demo.Description.c_str(), demo.Category.c_str());
        }

        ImGui::PopID();
    }

    void WelcomePanel::RenderRecentProjects() {
        ImGui::Spacing();
        if (m_RecentProjects.empty()) {
            ImGui::TextDisabled("No recent projects.");
            ImGui::Spacing();
            ImGui::TextWrapped("Create a new project or open an existing one to get started.");
        } else {
            ImGui::Text("Recent Projects:");
            ImGui::Spacing();
            for (size_t i = 0; i < m_RecentProjects.size(); i++) {
                ImGui::PushID(static_cast<int>(i));
                if (ImGui::Selectable(m_RecentProjects[i].c_str(), false, 0, ImVec2(-1, 0))) {
                    if (m_OnOpenProject) m_OnOpenProject();
                }
                ImGui::PopID();
            }
        }
    }

}
