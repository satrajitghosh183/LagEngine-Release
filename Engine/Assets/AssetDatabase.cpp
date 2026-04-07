#include "AssetDatabase.hpp"
#include "../Core/Logger.hpp"
#include <nlohmann/json.hpp>
#include <fstream>

namespace GameEngine {

    std::unordered_map<GUID, AssetMeta> AssetDatabase::s_Assets;
    std::unordered_map<std::string, GUID> AssetDatabase::s_PathIndex;
    std::string AssetDatabase::s_RootDir;

    void AssetDatabase::Init(const std::string& rootDir) {
        s_RootDir = rootDir;
        s_Assets.clear();
        s_PathIndex.clear();

        if (!std::filesystem::exists(rootDir)) {
            GE_CORE_WARN("AssetDatabase: root directory '{}' does not exist", rootDir);
            return;
        }

        ScanDirectory(rootDir);
        GE_CORE_INFO("AssetDatabase initialized: {} assets registered", s_Assets.size());
    }

    GUID AssetDatabase::Register(const std::string& path) {
        // Check if already registered
        auto existing = GetGUID(path);
        if (existing.has_value()) {
            return existing.value();
        }

        // Create new entry
        AssetMeta meta;
        meta.Id = GUID::Generate();
        meta.Path = path;

        auto ext = std::filesystem::path(path).extension().string();
        meta.Type = ClassifyExtension(ext);

        if (std::filesystem::exists(path)) {
            auto ftime = std::filesystem::last_write_time(path);
            meta.LastModifiedTime = static_cast<uint64_t>(
                ftime.time_since_epoch().count());
        }

        s_Assets[meta.Id] = meta;
        s_PathIndex[meta.Path] = meta.Id;

        // Write .meta sidecar
        WriteMeta(meta);

        GE_CORE_DEBUG("AssetDatabase: registered '{}' -> {}", path, meta.Id.ToString());
        return meta.Id;
    }

    std::optional<std::string> AssetDatabase::GetPath(const GUID& id) {
        auto it = s_Assets.find(id);
        if (it != s_Assets.end()) {
            return it->second.Path;
        }
        return std::nullopt;
    }

    std::optional<GUID> AssetDatabase::GetGUID(const std::string& path) {
        auto it = s_PathIndex.find(path);
        if (it != s_PathIndex.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    bool AssetDatabase::Contains(const GUID& id) {
        return s_Assets.find(id) != s_Assets.end();
    }

    bool AssetDatabase::Contains(const std::string& path) {
        return s_PathIndex.find(path) != s_PathIndex.end();
    }

    AssetType AssetDatabase::ClassifyExtension(const std::string& extension) {
        std::string ext = extension;
        // Normalize to lowercase
        for (auto& c : ext) c = static_cast<char>(std::tolower(c));

        if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb" || ext == ".dae")
            return AssetType::Mesh;
        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp" || ext == ".hdr")
            return AssetType::Texture;
        if (ext == ".vert" || ext == ".frag" || ext == ".glsl" || ext == ".shader")
            return AssetType::Shader;
        if (ext == ".scene" || ext == ".json")
            return AssetType::Scene;
        if (ext == ".lua")
            return AssetType::Script;
        if (ext == ".wav" || ext == ".ogg" || ext == ".mp3")
            return AssetType::Audio;
        if (ext == ".mat")
            return AssetType::Material;
        if (ext == ".prefab")
            return AssetType::Prefab;

        return AssetType::Unknown;
    }

    void AssetDatabase::Refresh() {
        if (s_RootDir.empty()) return;
        ScanDirectory(s_RootDir);
    }

    const std::unordered_map<GUID, AssetMeta>& AssetDatabase::GetAll() {
        return s_Assets;
    }

    void AssetDatabase::ScanDirectory(const std::filesystem::path& dir) {
        for (auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
            if (!entry.is_regular_file()) continue;

            auto path = entry.path();

            // Skip .meta files themselves
            if (path.extension() == ".meta") {
                LoadMeta(path);
                continue;
            }

            // If this asset file has a .meta sidecar, it will be loaded from there.
            // If not, register it now.
            auto metaFile = MetaPath(path.string());
            if (!std::filesystem::exists(metaFile)) {
                // Only auto-register known asset types
                auto type = ClassifyExtension(path.extension().string());
                if (type != AssetType::Unknown) {
                    Register(path.string());
                }
            }
        }
    }

    void AssetDatabase::LoadMeta(const std::filesystem::path& metaPath) {
        try {
            std::ifstream file(metaPath);
            if (!file.is_open()) return;

            nlohmann::json j;
            file >> j;

            AssetMeta meta;
            meta.Id = GUID(j.value("guid", ""));
            meta.Path = j.value("path", "");
            meta.Type = static_cast<AssetType>(j.value("type", 0));
            meta.LastModifiedTime = j.value("lastModified", uint64_t(0));

            if (meta.Id.IsValid() && !meta.Path.empty()) {
                s_Assets[meta.Id] = meta;
                s_PathIndex[meta.Path] = meta.Id;
            }
        } catch (const std::exception& e) {
            GE_CORE_ERROR("AssetDatabase: failed to load meta '{}': {}", metaPath.string(), e.what());
        }
    }

    void AssetDatabase::WriteMeta(const AssetMeta& meta) {
        auto metaPath = MetaPath(meta.Path);

        nlohmann::json j;
        j["guid"] = meta.Id.ToString();
        j["path"] = meta.Path;
        j["type"] = static_cast<int>(meta.Type);
        j["lastModified"] = meta.LastModifiedTime;

        try {
            std::ofstream file(metaPath);
            if (file.is_open()) {
                file << j.dump(2);
            }
        } catch (const std::exception& e) {
            GE_CORE_ERROR("AssetDatabase: failed to write meta '{}': {}", metaPath, e.what());
        }
    }

    std::filesystem::path AssetDatabase::MetaPath(const std::string& assetPath) {
        return std::filesystem::path(assetPath).string() + ".meta";
    }
}
