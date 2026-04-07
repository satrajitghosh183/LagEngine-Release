#include "RuntimePaths.hpp"
#include "../Platform/FileSystem.hpp"
#include "Logger.hpp"
#include <filesystem>

namespace GameEngine {

    std::string RuntimePaths::GetExecutableDirectory() {
        return FileSystem::GetExecutableDirectory();
    }

    std::string RuntimePaths::GetShadersDirectory() {
        try {
            std::string exeDir = FileSystem::GetExecutableDirectory();
            if (exeDir.empty()) {
                return "Assets/Shaders";
            }
            std::filesystem::path exe(exeDir);
            std::filesystem::path installShaders = (exe.parent_path() / ".." / "share" / "GameEngine" / "Shaders").lexically_normal();
            if (FileSystem::Exists(installShaders.string())) {
                return installShaders.string();
            }
            std::filesystem::path buildShaders = (exe.parent_path() / ".." / ".." / "Assets" / "Shaders").lexically_normal();
            if (FileSystem::Exists(buildShaders.string())) {
                return buildShaders.string();
            }
            std::filesystem::path cwdShaders = std::filesystem::current_path() / "Assets" / "Shaders";
            if (FileSystem::Exists(cwdShaders.string())) {
                return cwdShaders.string();
            }
            return (std::filesystem::current_path() / "Assets" / "Shaders").string();
        } catch (const std::exception& e) {
            GE_CORE_WARN("GetShadersDirectory failed: {0}", e.what());
            return "Assets/Shaders";
        }
    }

    std::string RuntimePaths::GetContentDirectory() {
        try {
            std::string exeDir = FileSystem::GetExecutableDirectory();
            if (exeDir.empty()) {
                return "Content";
            }
            std::filesystem::path exe(exeDir);
            std::filesystem::path installContent = (exe.parent_path() / ".." / "share" / "GameEngine" / "Content").lexically_normal();
            if (FileSystem::Exists(installContent.string())) {
                return installContent.string();
            }
            std::filesystem::path buildContent = (exe.parent_path() / ".." / ".." / "Content").lexically_normal();
            if (FileSystem::Exists(buildContent.string())) {
                return buildContent.string();
            }
            std::filesystem::path cwdContent = std::filesystem::current_path() / "Content";
            if (FileSystem::Exists(cwdContent.string())) {
                return cwdContent.string();
            }
            return (std::filesystem::current_path() / "Content").string();
        } catch (const std::exception& e) {
            GE_CORE_WARN("GetContentDirectory failed: {0}", e.what());
            return "Content";
        }
    }

    std::string RuntimePaths::Resolve(const std::string& relativePath) {
        if (relativePath.empty()) return relativePath;
        try {
            std::filesystem::path p(relativePath);
            if (p.is_absolute() && FileSystem::Exists(relativePath)) {
                return relativePath;
            }
            std::string shadersDir = GetShadersDirectory();
            std::filesystem::path shaderFull = std::filesystem::path(shadersDir) / p.filename();
            if (FileSystem::Exists(shaderFull.string())) {
                return shaderFull.string();
            }
            shaderFull = std::filesystem::path(shadersDir) / p;
            if (FileSystem::Exists(shaderFull.string())) {
                return shaderFull.string();
            }
            std::filesystem::path cwdFull = std::filesystem::current_path() / p;
            if (FileSystem::Exists(cwdFull.string())) {
                return cwdFull.string();
            }
        } catch (const std::exception& e) {
            GE_CORE_WARN("RuntimePaths::Resolve failed for '{0}': {1}", relativePath, e.what());
        }
        return relativePath;
    }

    std::string RuntimePaths::ResolveShader(const std::string& shaderRelativePath) {
        if (shaderRelativePath.empty()) return shaderRelativePath;
        try {
            std::filesystem::path p(shaderRelativePath);
            if (p.is_absolute() && FileSystem::Exists(shaderRelativePath)) {
                return shaderRelativePath;
            }
            std::string shadersDir = GetShadersDirectory();
            std::filesystem::path full = std::filesystem::path(shadersDir) / p.filename();
            if (FileSystem::Exists(full.string())) {
                return full.string();
            }
            full = std::filesystem::path(shadersDir) / p;
            if (FileSystem::Exists(full.string())) {
                return full.string();
            }
            std::filesystem::path cwdFull = std::filesystem::current_path() / p;
            if (FileSystem::Exists(cwdFull.string())) {
                return cwdFull.string();
            }
        } catch (const std::exception& e) {
            GE_CORE_WARN("RuntimePaths::ResolveShader failed for '{0}': {1}", shaderRelativePath, e.what());
        }
        return shaderRelativePath;
    }
}
