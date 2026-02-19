#pragma once

#include "Base.hpp"
#include <string>

namespace GameEngine {

    /**
     * @brief Resolve asset and shader paths relative to the executable.
     * Supports both in-tree builds (e.g. build/bin) and installed layout
     * (e.g. bin/ + share/GameEngine/Shaders).
     */
    class RuntimePaths {
    public:
        /** @brief Get the directory containing the running executable */
        static std::string GetExecutableDirectory();

        /**
         * @brief Get the shaders root directory (trailing slash not guaranteed).
         * Tries: exe/../share/GameEngine/Shaders, exe/../../Assets/Shaders, then CWD-relative Assets/Shaders.
         */
        static std::string GetShadersDirectory();

        /**
         * @brief Get the content root directory.
         * Tries: exe/../share/GameEngine/Content, exe/../../Content, then CWD-relative Content.
         */
        static std::string GetContentDirectory();

        /**
         * @brief Resolve a path (e.g. "basic.vert" or "Shaders/basic.vert") to an absolute path.
         * Tries shaders directory first, then content, then path as-is.
         */
        static std::string Resolve(const std::string& relativePath);

        /** @brief Resolve a path under the shaders directory (e.g. "basic.vert" -> full path) */
        static std::string ResolveShader(const std::string& shaderRelativePath);
    };
}
