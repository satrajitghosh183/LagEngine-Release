#pragma once

#include "../../Engine/Assets/TemplateManager.hpp"
#include <imgui.h>
#include <string>
#include <functional>
#include <vector>
#include <cstdint>

namespace GameEngine {

    /**
     * @brief ImGui modal dialog for creating a new project from a template
     *
     * Displays available templates in a 3-column grid with procedurally-generated
     * thumbnail images, name, category, and a short description.
     * The user selects a template, enters a project name and target directory,
     * then clicks Create to instantiate it.
     */
    class NewProjectDialog {
    public:
        using CreateCallback = std::function<void(const std::string& scenePath)>;

        ~NewProjectDialog();

        void Open();
        void OnImGuiRender();
        void SetCreateCallback(const CreateCallback& callback) { m_Callback = callback; }

    private:
        // Procedurally generate a 128x128 OpenGL texture thumbnail for a template
        uint32_t GenerateThumbnailTexture(const std::string& name, const std::string& category);

        // Build one texture per loaded template; called once on first render
        void InitThumbnails();

        // Release all generated textures
        void CleanupThumbnails();

        bool m_IsOpen         = false;
        bool m_PendingOpen    = false;
        int  m_SelectedTemplate = -1;
        char m_ProjectName[256] = "NewProject";
        char m_ProjectPath[512] = "";
        CreateCallback m_Callback;

        // One GL texture handle per template (index matches TemplateManager::GetTemplates())
        std::vector<uint32_t> m_ThumbnailTextures;
        bool m_ThumbnailsInitialized = false;

        static constexpr int kThumbSize = 128; // texture resolution (square)
    };
}
