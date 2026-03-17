#pragma once

#include "../../Engine/Core/Base.hpp"
#include <imgui.h>
#include <string>
#include <vector>
#include <functional>

namespace GameEngine {

    /**
     * @brief Welcome/splash screen shown when starting with an empty scene
     */
    class WelcomePanel {
    public:
        void SetVisible(bool v) { m_Visible = v; }
        bool IsVisible() const { return m_Visible; }
        void SetOnNewScene(std::function<void()> cb)    { m_OnNewScene    = std::move(cb); }
        void SetOnNewProject(std::function<void()> cb)  { m_OnNewProject  = std::move(cb); }
        void SetOnOpenProject(std::function<void()> cb) { m_OnOpenProject = std::move(cb); }
        void SetOnOpenScene(std::function<void()> cb)   { m_OnOpenScene   = std::move(cb); }
        void SetRecentProjects(const std::vector<std::string>& paths) { m_RecentProjects = paths; }
        void OnImGuiRender();

    private:
        bool m_Visible = true;
        std::function<void()> m_OnNewScene;
        std::function<void()> m_OnNewProject;
        std::function<void()> m_OnOpenProject;
        std::function<void()> m_OnOpenScene;
        std::vector<std::string> m_RecentProjects;
    };
}
