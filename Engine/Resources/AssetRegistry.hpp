#pragma once

#include "../Core/Base.hpp"
#include "GUID.hpp"
#include <string>
#include <unordered_map>
#include <filesystem>
#include <functional>

namespace GameEngine {

    // Forward declarations
    class Mesh3D;
    class Texture2D;
    class Shader;

    /**
     * @brief Asset registry - central asset management
     * 
     * Features:
     * - GUID-based asset references
     * - Central registry with loaders
     * - Caching and reimport detection
     */
    class AssetRegistry {
    public:
        /**
         * @brief Initialize asset registry
         */
        static void Initialize();

        /**
         * @brief Shutdown asset registry
         */
        static void Shutdown();

        /**
         * @brief Register asset with GUID
         */
        static GUID RegisterAsset(const std::filesystem::path& path, const std::string& type);

        /**
         * @brief Get asset path from GUID
         */
        static std::filesystem::path GetAssetPath(const GUID& guid);

        /**
         * @brief Get GUID from asset path
         */
        static GUID GetGUID(const std::filesystem::path& path);

        /**
         * @brief Load mesh
         */
        static Ref<Mesh3D> LoadMesh(const GUID& guid);
        static Ref<Mesh3D> LoadMesh(const std::filesystem::path& path);

        /**
         * @brief Load texture
         */
        static Ref<Texture2D> LoadTexture(const GUID& guid);
        static Ref<Texture2D> LoadTexture(const std::filesystem::path& path);

        /**
         * @brief Load shader
         */
        static Ref<Shader> LoadShader(const GUID& guid);
        static Ref<Shader> LoadShader(const std::filesystem::path& path);

        /**
         * @brief Check if asset needs reimport
         */
        static bool NeedsReimport(const GUID& guid);

    private:
        struct AssetInfo {
            GUID guid;
            std::filesystem::path path;
            std::string type;
            std::filesystem::file_time_type lastModified;
        };

        static std::unordered_map<GUID, AssetInfo> s_Assets;
        static std::unordered_map<std::filesystem::path, GUID> s_PathToGUID;
        static std::unordered_map<GUID, Ref<Mesh3D>> s_MeshCache;
        static std::unordered_map<GUID, Ref<Texture2D>> s_TextureCache;
        static std::unordered_map<GUID, Ref<Shader>> s_ShaderCache;
    };

}

