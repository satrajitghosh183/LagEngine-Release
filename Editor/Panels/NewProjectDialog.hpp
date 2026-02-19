#pragma once

#include "../../Engine/Assets/TemplateManager.hpp"
#include <imgui.h>
#include <string>
#include <functional>

namespace GameEngine {

    /**
     * @brief ImGui modal dialog for creating a new project from a template
     *
     * Displays available templates in a 3-column grid with name and category.
     * The user selects a template, enters a project name and target directory,
     * then clicks Create to instantiate it.
     */
    class NewProjectDialog {
    public:
        using CreateCallback = std::function<void(const std::string& scenePath)>;

        /**
         * @brief Open the dialog
         */
        void Open();

        /**
         * @brief Render the dialog (call every frame)
         */
        void OnImGuiRender();

        /**
         * @brief Set the callback invoked after project creation
         */
        void SetCreateCallback(const CreateCallback& callback) { m_Callback = callback; }

    private:
        bool m_IsOpen = false;
        int m_SelectedTemplate = -1;
        char m_ProjectName[256] = "NewProject";
        char m_ProjectPath[512] = "";
        CreateCallback m_Callback;
    };
}
