#pragma once

#include "../../Engine/Assets/TemplateManager.hpp"
#include "../../Engine/Core/Base.hpp"
#include "../../Engine/Graphics/Texture2D.hpp"
#include <imgui.h>
#include <vulkan/vulkan.h>
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
        // Procedurally generate a 128x128 Vulkan texture thumbnail for a template
        void GenerateThumbnail(const std::string& name, const std::string& category);

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

        // One Vulkan texture per template (index matches TemplateManager::GetTemplates())
        struct ThumbnailEntry {
            Ref<Texture2D> Texture;
            VkDescriptorSet ImGuiDescriptor = VK_NULL_HANDLE;
        };
        std::vector<ThumbnailEntry> m_Thumbnails;
        bool m_ThumbnailsInitialized = false;

        static constexpr int kThumbSize = 128; // texture resolution (square)
    };
}
