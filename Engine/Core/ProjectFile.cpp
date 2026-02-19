#include "ProjectFile.hpp"
#include "../Platform/FileSystem.hpp"
#include "Logger.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>

namespace GameEngine {

    bool ProjectFile::Save(const std::string& projectDir, const ProjectFile& proj) {
        try {
            nlohmann::json j;
            j["name"] = proj.Name;
            j["defaultScene"] = proj.DefaultScene;
            j["assetsRoot"] = proj.AssetsRoot;
            std::string path = (std::filesystem::path(projectDir) / ".geproject").string();
            std::ofstream f(path);
            if (!f.is_open()) return false;
            f << j.dump(2);
            return true;
        } catch (const std::exception& e) {
            GE_CORE_ERROR("Failed to save .geproject: {0}", e.what());
            return false;
        }
    }

    bool ProjectFile::Load(const std::string& projectPath, ProjectFile& out) {
        try {
            std::ifstream f(projectPath);
            if (!f.is_open()) return false;
            nlohmann::json j;
            f >> j;
            if (j.contains("name")) out.Name = j["name"].get<std::string>();
            if (j.contains("defaultScene")) out.DefaultScene = j["defaultScene"].get<std::string>();
            if (j.contains("assetsRoot")) out.AssetsRoot = j["assetsRoot"].get<std::string>();
            return true;
        } catch (const std::exception& e) {
            GE_CORE_ERROR("Failed to load .geproject: {0}", e.what());
            return false;
        }
    }
}
