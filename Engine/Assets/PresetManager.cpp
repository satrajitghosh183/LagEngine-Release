#include "PresetManager.hpp"
#include "../Core/Logger.hpp"

#include <filesystem>
#include <fstream>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

namespace GameEngine {

    std::vector<ScenePreset> PresetManager::s_Presets;
    std::string PresetManager::s_PresetsDir;

    void PresetManager::Init(const std::string& presetsDir)
    {
        s_PresetsDir = presetsDir;
        ScanPresets();
        GE_CORE_INFO("PresetManager: initialized with {} preset(s) from '{}'",
                      s_Presets.size(), s_PresetsDir);
    }

    const std::vector<ScenePreset>& PresetManager::GetPresets()
    {
        return s_Presets;
    }

    void PresetManager::Refresh()
    {
        s_Presets.clear();
        ScanPresets();
        GE_CORE_INFO("PresetManager: refreshed, found {} preset(s)", s_Presets.size());
    }

    void PresetManager::ScanPresets()
    {
        fs::path root(s_PresetsDir);
        if (!fs::exists(root) || !fs::is_directory(root)) {
            GE_CORE_WARN("PresetManager: presets directory '{}' not found", s_PresetsDir);
            return;
        }

        for (const auto& entry : fs::directory_iterator(root)) {
            if (!entry.is_regular_file())
                continue;

            if (entry.path().extension() != ".scene")
                continue;

            ScenePreset preset;
            preset.FilePath = entry.path().string();

            std::string stem = entry.path().stem().string();

            // Try to read name and description from the scene JSON
            bool metadataFound = false;
            try {
                std::ifstream file(entry.path());
                if (file.is_open()) {
                    nlohmann::json j;
                    file >> j;

                    if (j.contains("name")) {
                        preset.Name = j["name"].get<std::string>();
                        metadataFound = true;
                    }
                    if (j.contains("description")) {
                        preset.Description = j["description"].get<std::string>();
                    }
                    if (j.contains("category")) {
                        preset.Category = j["category"].get<std::string>();
                    }
                }
            }
            catch (const nlohmann::json::exception&) {
                // File is not valid JSON or lacks expected fields; that's fine
            }

            // Fall back to filename-derived values
            if (preset.Name.empty())
                preset.Name = stem;

            if (preset.Category.empty())
                preset.Category = DeriveCategory(stem);

            s_Presets.push_back(std::move(preset));
        }

        // Sort presets alphabetically by category then name
        std::sort(s_Presets.begin(), s_Presets.end(),
                  [](const ScenePreset& a, const ScenePreset& b) {
                      if (a.Category != b.Category)
                          return a.Category < b.Category;
                      return a.Name < b.Name;
                  });
    }

    std::string PresetManager::DeriveCategory(const std::string& filename)
    {
        // Convert to lowercase for matching
        std::string lower = filename;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        // Check common category keywords in the filename
        if (lower.find("physics") != std::string::npos ||
            lower.find("rigidbody") != std::string::npos ||
            lower.find("collision") != std::string::npos)
            return "Physics";

        if (lower.find("particle") != std::string::npos ||
            lower.find("gpu_particle") != std::string::npos)
            return "Particles";

        if (lower.find("fluid") != std::string::npos ||
            lower.find("water") != std::string::npos ||
            lower.find("sph") != std::string::npos)
            return "Fluid";

        if (lower.find("robot") != std::string::npos ||
            lower.find("arm") != std::string::npos ||
            lower.find("joint") != std::string::npos)
            return "Robotics";

        if (lower.find("cloth") != std::string::npos ||
            lower.find("soft") != std::string::npos)
            return "Physics";

        if (lower.find("2d") != std::string::npos ||
            lower.find("sprite") != std::string::npos)
            return "2D";

        if (lower.find("pbr") != std::string::npos ||
            lower.find("render") != std::string::npos ||
            lower.find("shadow") != std::string::npos ||
            lower.find("ibl") != std::string::npos ||
            lower.find("lighting") != std::string::npos ||
            lower.find("graphics") != std::string::npos)
            return "Graphics";

        return "General";
    }
}
