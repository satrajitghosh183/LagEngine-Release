#pragma once

#include "Base.hpp"
#include <string>

namespace GameEngine {

    /**
     * @brief .geproject file format for project metadata
     */
    struct ProjectFile {
        std::string Name;
        std::string DefaultScene;  // Relative to project root
        std::string AssetsRoot = "Assets";

        /** @brief Save to .geproject JSON file */
        static bool Save(const std::string& projectDir, const ProjectFile& proj);

        /** @brief Load from .geproject JSON file; projectDir is the folder containing .geproject */
        static bool Load(const std::string& projectPath, ProjectFile& out);
    };
}
