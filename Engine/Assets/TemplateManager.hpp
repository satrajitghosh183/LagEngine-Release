#pragma once

#include <string>
#include <vector>
#include "../Core/Base.hpp"
#include <nlohmann/json.hpp>

namespace GameEngine {

    /**
     * @brief Describes a project template stored on disk
     */
    struct ProjectTemplate {
        std::string Name;
        std::string Description;
        std::string Category;      // "Physics", "Graphics", "2D", "Robotics", etc.
        std::string DirectoryPath; // Path to template directory
        std::string SceneFile;     // Relative path to scene file within template
        std::string ThumbnailPath; // Optional thumbnail
    };

    /**
     * @brief Scans Content/Templates/ for project templates
     *
     * Each template is a subdirectory containing a template.json manifest file
     * that describes the template name, category, scene file, and other metadata.
     *
     * Usage:
     *   TemplateManager::Init("Content/Templates");
     *   for (auto& t : TemplateManager::GetTemplates()) { ... }
     *   TemplateManager::CreateProjectFromTemplate(t, "MyProject");
     */
    class TemplateManager {
    public:
        /**
         * @brief Initialize and scan the templates directory
         * @param templatesDir Root directory containing template subdirectories
         */
        static void Init(const std::string& templatesDir = "Content/Templates");

        /**
         * @brief Get all discovered templates
         */
        static const std::vector<ProjectTemplate>& GetTemplates();

        /**
         * @brief Copy a template directory to a target location
         * @param tmpl The template to instantiate
         * @param targetDir Destination directory for the new project
         * @return true if the copy succeeded
         */
        static bool CreateProjectFromTemplate(const ProjectTemplate& tmpl, const std::string& targetDir);

        /**
         * @brief Re-scan the templates directory
         */
        static void Refresh();

    private:
        static void ScanTemplates();
        static std::vector<ProjectTemplate> s_Templates;
        static std::string s_TemplatesDir;
    };
}
