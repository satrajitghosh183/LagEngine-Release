#pragma once

#include "../Core/Base.hpp"
#include "../Resources/GUID.hpp"
#include <string>
#include <unordered_map>
#include <filesystem>
#include <optional>

namespace GameEngine {

    /**
     * @brief Asset type classification
     */
    enum class AssetType {
        Unknown,
        Folder,
        Mesh,
        Model,
        Material,
        Texture,
        Shader,
        Scene,
        Script,
        Audio,
        Prefab
    };

    /**
     * @brief Metadata for a registered asset
     */
    struct AssetMeta {
        GUID Id;
        std::string Path;               // Relative path from project root
        AssetType Type = AssetType::Unknown;
        uint64_t LastModifiedTime = 0;
    };

    /**
     * @brief Asset database with persistent UUID-to-path mapping
     *
     * Assigns a stable GUID to every asset file and persists it
     * in a .meta sidecar JSON file alongside the asset. On startup,
     * the database scans asset directories to rebuild the in-memory
     * index from existing .meta files.
     *
     * Usage:
     *   AssetDatabase::Init("Assets");
     *   GUID id = AssetDatabase::Register("Assets/Meshes/cube.obj");
     *   auto path = AssetDatabase::GetPath(id);
     */
    class AssetDatabase {
    public:
        /**
         * @brief Initialize and scan asset directories for .meta files
         */
        static void Init(const std::string& rootDir = "Assets");

        /**
         * @brief Register an asset file; creates .meta sidecar if absent
         * @return The GUID for this asset
         */
        static GUID Register(const std::string& path);

        /**
         * @brief Look up path by GUID
         */
        static std::optional<std::string> GetPath(const GUID& id);

        /**
         * @brief Look up GUID by path
         */
        static std::optional<GUID> GetGUID(const std::string& path);

        /**
         * @brief Check if asset exists in database
         */
        static bool Contains(const GUID& id);
        static bool Contains(const std::string& path);

        /**
         * @brief Get asset type from file extension
         */
        static AssetType ClassifyExtension(const std::string& extension);

        /**
         * @brief Re-scan the asset root for new or changed files
         */
        static void Refresh();

        /**
         * @brief Get all registered assets
         */
        static const std::unordered_map<GUID, AssetMeta>& GetAll();

    private:
        static void ScanDirectory(const std::filesystem::path& dir);
        static void LoadMeta(const std::filesystem::path& metaPath);
        static void WriteMeta(const AssetMeta& meta);
        static std::filesystem::path MetaPath(const std::string& assetPath);

        static std::unordered_map<GUID, AssetMeta> s_Assets;
        static std::unordered_map<std::string, GUID> s_PathIndex;
        static std::string s_RootDir;
    };
}
