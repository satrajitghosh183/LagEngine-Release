#include "AssetRegistry.hpp"
#include "../Core/Logger.hpp"
#include "../Graphics/Mesh3D.hpp"
#include "../Graphics/Texture2D.hpp"
#include "../Graphics/Shader.hpp"

namespace GameEngine {

    std::unordered_map<GUID, AssetRegistry::AssetInfo> AssetRegistry::s_Assets;
    std::unordered_map<std::filesystem::path, GUID> AssetRegistry::s_PathToGUID;
    std::unordered_map<GUID, Ref<Mesh3D>> AssetRegistry::s_MeshCache;
    std::unordered_map<GUID, Ref<Texture2D>> AssetRegistry::s_TextureCache;
    std::unordered_map<GUID, Ref<Shader>> AssetRegistry::s_ShaderCache;

    void AssetRegistry::Initialize() {
        GE_CORE_INFO("AssetRegistry initialized");
    }

    void AssetRegistry::Shutdown() {
        s_Assets.clear();
        s_PathToGUID.clear();
        s_MeshCache.clear();
        s_TextureCache.clear();
        s_ShaderCache.clear();
        GE_CORE_INFO("AssetRegistry shutdown");
    }

    GUID AssetRegistry::RegisterAsset(const std::filesystem::path& path, const std::string& type) {
        auto it = s_PathToGUID.find(path);
        if (it != s_PathToGUID.end()) {
            return it->second;
        }

        GUID guid = GUID::Generate();
        AssetInfo info;
        info.guid = guid;
        info.path = path;
        info.type = type;
        
        if (std::filesystem::exists(path)) {
            info.lastModified = std::filesystem::last_write_time(path);
        }

        s_Assets[guid] = info;
        s_PathToGUID[path] = guid;

        return guid;
    }

    std::filesystem::path AssetRegistry::GetAssetPath(const GUID& guid) {
        auto it = s_Assets.find(guid);
        if (it != s_Assets.end()) {
            return it->second.path;
        }
        return {};
    }

    GUID AssetRegistry::GetGUID(const std::filesystem::path& path) {
        auto it = s_PathToGUID.find(path);
        if (it != s_PathToGUID.end()) {
            return it->second;
        }
        return RegisterAsset(path, "unknown");
    }

    Ref<Mesh3D> AssetRegistry::LoadMesh(const GUID& guid) {
        auto it = s_MeshCache.find(guid);
        if (it != s_MeshCache.end()) {
            return it->second;
        }

        auto path = GetAssetPath(guid);
        return LoadMesh(path);
    }

    Ref<Mesh3D> AssetRegistry::LoadMesh(const std::filesystem::path& path) {
        GUID guid = GetGUID(path);
        auto it = s_MeshCache.find(guid);
        if (it != s_MeshCache.end()) {
            return it->second;
        }

        // TODO: Implement actual mesh loading
        GE_CORE_WARN("Mesh loading not yet implemented for: {0}", path.string());
        return nullptr;
    }

    Ref<Texture2D> AssetRegistry::LoadTexture(const GUID& guid) {
        auto it = s_TextureCache.find(guid);
        if (it != s_TextureCache.end()) {
            return it->second;
        }

        auto path = GetAssetPath(guid);
        return LoadTexture(path);
    }

    Ref<Texture2D> AssetRegistry::LoadTexture(const std::filesystem::path& path) {
        GUID guid = GetGUID(path);
        auto it = s_TextureCache.find(guid);
        if (it != s_TextureCache.end()) {
            return it->second;
        }

        // TODO: Implement actual texture loading
        GE_CORE_WARN("Texture loading not yet implemented for: {0}", path.string());
        return nullptr;
    }

    Ref<Shader> AssetRegistry::LoadShader(const GUID& guid) {
        auto it = s_ShaderCache.find(guid);
        if (it != s_ShaderCache.end()) {
            return it->second;
        }

        auto path = GetAssetPath(guid);
        return LoadShader(path);
    }

    Ref<Shader> AssetRegistry::LoadShader(const std::filesystem::path& path) {
        GUID guid = GetGUID(path);
        auto it = s_ShaderCache.find(guid);
        if (it != s_ShaderCache.end()) {
            return it->second;
        }

        // TODO: Implement actual shader loading
        GE_CORE_WARN("Shader loading not yet implemented for: {0}", path.string());
        return nullptr;
    }

    bool AssetRegistry::NeedsReimport(const GUID& guid) {
        auto it = s_Assets.find(guid);
        if (it == s_Assets.end()) {
            return false;
        }

        const auto& info = it->second;
        if (!std::filesystem::exists(info.path)) {
            return false;
        }

        auto currentModified = std::filesystem::last_write_time(info.path);
        return currentModified > info.lastModified;
    }

}

