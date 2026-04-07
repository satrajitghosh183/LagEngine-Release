#pragma once

#include "../../Engine/Core/Base.hpp"
#include <imgui.h>
#include <string>
#include <vector>
#include <functional>

namespace GameEngine {

    struct DemoEntry {
        std::string Name;
        std::string Description;
        std::string Category;     // "Physics", "Rendering", "Robotics", "2D", "Education"
        std::string IconLabel;    // Short label shown in the card (e.g. emoji or abbreviation)
        ImVec4 AccentColor;       // Card accent color
        std::function<void()> OnLoad;
    };

    class WelcomePanel {
    public:
        void SetVisible(bool v) { m_Visible = v; }
        bool IsVisible() const { return m_Visible; }
        void SetOnNewScene(std::function<void()> cb)    { m_OnNewScene    = std::move(cb); }
        void SetOnNewProject(std::function<void()> cb)  { m_OnNewProject  = std::move(cb); }
        void SetOnOpenProject(std::function<void()> cb) { m_OnOpenProject = std::move(cb); }
        void SetOnOpenScene(std::function<void()> cb)   { m_OnOpenScene   = std::move(cb); }
        void SetRecentProjects(const std::vector<std::string>& paths) { m_RecentProjects = paths; }

        void AddDemo(const DemoEntry& demo) { m_Demos.push_back(demo); }
        void ClearDemos() { m_Demos.clear(); }

        // Deferred action: returns and clears any pending callback from demo click
        std::function<void()> TakePendingAction() {
            auto action = std::move(m_PendingAction);
            m_PendingAction = nullptr;
            return action;
        }

        void OnImGuiRender();

    private:
        void RenderHeader();
        void RenderQuickStart();
        void RenderDemoGrid();
        void RenderRecentProjects();
        void RenderDemoCard(const DemoEntry& demo, float cardWidth, float cardHeight);

        bool m_Visible = true;
        std::string m_SearchFilter;
        char m_SearchBuf[128] = "";
        int m_SelectedCategory = 0; // 0 = All

        std::function<void()> m_OnNewScene;
        std::function<void()> m_OnNewProject;
        std::function<void()> m_OnOpenProject;
        std::function<void()> m_OnOpenScene;
        std::vector<std::string> m_RecentProjects;
        std::vector<DemoEntry> m_Demos;
        std::function<void()> m_PendingAction;
    };
}
