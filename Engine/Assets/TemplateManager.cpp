#include "TemplateManager.hpp"
#include "../Core/Logger.hpp"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace GameEngine {

    std::vector<ProjectTemplate> TemplateManager::s_Templates;
    std::string TemplateManager::s_TemplatesDir;

    void TemplateManager::Init(const std::string& templatesDir)
    {
        s_TemplatesDir = templatesDir;
        ScanTemplates();
        GE_CORE_INFO("TemplateManager: initialized with {} template(s) from '{}'",
                      s_Templates.size(), s_TemplatesDir);
    }

    const std::vector<ProjectTemplate>& TemplateManager::GetTemplates()
    {
        return s_Templates;
    }

    bool TemplateManager::CreateProjectFromTemplate(const ProjectTemplate& tmpl,
                                                     const std::string& targetDir)
    {
        try {
            fs::path src(tmpl.DirectoryPath);
            fs::path dst(targetDir);

            if (!fs::exists(src)) {
                GE_CORE_ERROR("TemplateManager: source directory '{}' does not exist",
                              tmpl.DirectoryPath);
                return false;
            }

            if (fs::exists(dst)) {
                GE_CORE_WARN("TemplateManager: target directory '{}' already exists, "
                             "files may be overwritten", targetDir);
            }

            fs::copy(src, dst,
                     fs::copy_options::recursive |
                     fs::copy_options::overwrite_existing);

            GE_CORE_INFO("TemplateManager: created project '{}' from template '{}'",
                         targetDir, tmpl.Name);
            return true;
        }
        catch (const fs::filesystem_error& e) {
            GE_CORE_ERROR("TemplateManager: failed to create project - {}", e.what());
            return false;
        }
    }

    void TemplateManager::Refresh()
    {
        s_Templates.clear();
        ScanTemplates();
        GE_CORE_INFO("TemplateManager: refreshed, found {} template(s)", s_Templates.size());
    }

    void TemplateManager::ScanTemplates()
    {
        fs::path root(s_TemplatesDir);
        if (!fs::exists(root) || !fs::is_directory(root)) {
            GE_CORE_WARN("TemplateManager: templates directory '{}' not found", s_TemplatesDir);
            return;
        }

        for (const auto& entry : fs::directory_iterator(root)) {
            if (!entry.is_directory())
                continue;

            fs::path manifestPath = entry.path() / "template.json";
            if (!fs::exists(manifestPath))
                continue;

            try {
                std::ifstream file(manifestPath);
                if (!file.is_open()) {
                    GE_CORE_WARN("TemplateManager: could not open '{}'",
                                 manifestPath.string());
                    continue;
                }

                nlohmann::json j;
                file >> j;

                ProjectTemplate tmpl;
                tmpl.Name          = j.value("name", entry.path().filename().string());
                tmpl.Description   = j.value("description", "");
                tmpl.Category      = j.value("category", "General");
                tmpl.DirectoryPath = entry.path().string();
                tmpl.SceneFile     = j.value("sceneFile", "");
                tmpl.ThumbnailPath = j.value("thumbnail", "");

                // Resolve thumbnail to absolute path if relative
                if (!tmpl.ThumbnailPath.empty() &&
                    !fs::path(tmpl.ThumbnailPath).is_absolute()) {
                    tmpl.ThumbnailPath = (entry.path() / tmpl.ThumbnailPath).string();
                }

                s_Templates.push_back(std::move(tmpl));
            }
            catch (const nlohmann::json::exception& e) {
                GE_CORE_WARN("TemplateManager: invalid template.json in '{}' - {}",
                             entry.path().string(), e.what());
            }
        }
    }
}
