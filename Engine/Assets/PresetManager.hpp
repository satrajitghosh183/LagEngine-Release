#pragma once

#include <string>
#include <vector>
#include "../Core/Base.hpp"

namespace GameEngine {

    /**
     * @brief Describes a loadable scene preset
     */
    struct ScenePreset {
        std::string Name;
        std::string Description;
        std::string Category;
        std::string FilePath;    // Full path to .scene file
    };

    /**
     * @brief Scans Content/Presets/ for .scene files and provides them as loadable presets
     *
     * Categories are derived from filename prefixes or embedded JSON metadata.
     *
     * Usage:
     *   PresetManager::Init("Content/Presets");
     *   for (auto& p : PresetManager::GetPresets()) { ... }
     */
    class PresetManager {
    public:
        /**
         * @brief Initialize and scan the presets directory
         * @param presetsDir Directory containing .scene preset files
         */
        static void Init(const std::string& presetsDir = "Content/Presets");

        /**
         * @brief Get all discovered presets
         */
        static const std::vector<ScenePreset>& GetPresets();

        /**
         * @brief Re-scan the presets directory
         */
        static void Refresh();

    private:
        static void ScanPresets();
        static std::string DeriveCategory(const std::string& filename);
        static std::vector<ScenePreset> s_Presets;
        static std::string s_PresetsDir;
    };
}
