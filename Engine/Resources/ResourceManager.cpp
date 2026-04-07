#include "ResourceManager.hpp"
#include "../Core/Logger.hpp"
#include "../Platform/FileSystem.hpp"

namespace GameEngine {

    std::unordered_map<std::string, Ref<Texture2D>> ResourceManager::s_Textures;
    std::unordered_map<std::string, Ref<Shader>> ResourceManager::s_Shaders;
    std::unordered_map<std::string, Ref<Mesh3D>> ResourceManager::s_Meshes;
    std::unordered_map<std::string, Ref<AudioClip>> ResourceManager::s_AudioClips;

    Ref<Texture2D> ResourceManager::LoadTexture(const std::string& path) {
        // Check if already loaded
        auto it = s_Textures.find(path);
        if (it != s_Textures.end()) {
            GE_CORE_TRACE("Texture already loaded: {0}", path);
            return it->second;
        }
        
        // Load texture
        if (!FileSystem::Exists(path)) {
            GE_CORE_ERROR("Texture file not found: {0}", path);
            return nullptr;
        }
        
        auto texture = CreateRef<Texture2D>(path);
        s_Textures[path] = texture;
        
        GE_CORE_INFO("Texture loaded: {0}", path);
        return texture;
    }

    Ref<Shader> ResourceManager::LoadShader(const std::string& name, 
                                           const std::string& vertexPath, 
                                           const std::string& fragmentPath) {
        // Check if already loaded
        auto it = s_Shaders.find(name);
        if (it != s_Shaders.end()) {
            GE_CORE_TRACE("Shader already loaded: {0}", name);
            return it->second;
        }
        
        // Load shader
        if (!FileSystem::Exists(vertexPath) || !FileSystem::Exists(fragmentPath)) {
            GE_CORE_ERROR("Shader files not found: {0}, {1}", vertexPath, fragmentPath);
            return nullptr;
        }
        
        auto shader = CreateRef<Shader>(vertexPath, fragmentPath);
        s_Shaders[name] = shader;
        
        GE_CORE_INFO("Shader loaded: {0}", name);
        return shader;
    }

    Ref<Mesh3D> ResourceManager::LoadMesh(const std::string& path) {
        // Check if already loaded
        auto it = s_Meshes.find(path);
        if (it != s_Meshes.end()) {
            GE_CORE_TRACE("Mesh already loaded: {0}", path);
            return it->second;
        }
        
        // TODO: Implement mesh loading from file (use Assimp)
        GE_CORE_WARN("Mesh loading from file not yet implemented: {0}", path);
        return nullptr;
    }

    Ref<AudioClip> ResourceManager::LoadAudio(const std::string& path) {
        // Check if already loaded
        auto it = s_AudioClips.find(path);
        if (it != s_AudioClips.end()) {
            GE_CORE_TRACE("Audio clip already loaded: {0}", path);
            return it->second;
        }
        
        // Load audio
        if (!FileSystem::Exists(path)) {
            GE_CORE_ERROR("Audio file not found: {0}", path);
            return nullptr;
        }
        
        auto clip = CreateRef<AudioClip>(path);
        if (clip->IsValid()) {
            s_AudioClips[path] = clip;
            GE_CORE_INFO("Audio clip loaded: {0}", path);
            return clip;
        }
        
        return nullptr;
    }

    bool ResourceManager::IsLoaded(const std::string& path) {
        return s_Textures.find(path) != s_Textures.end() ||
               s_Shaders.find(path) != s_Shaders.end() ||
               s_Meshes.find(path) != s_Meshes.end() ||
               s_AudioClips.find(path) != s_AudioClips.end();
    }

    void ResourceManager::Unload(const std::string& path) {
        s_Textures.erase(path);
        s_Shaders.erase(path);
        s_Meshes.erase(path);
        s_AudioClips.erase(path);
        
        GE_CORE_INFO("Resource unloaded: {0}", path);
    }

    void ResourceManager::UnloadUnused() {
        // Remove resources with only 1 reference (the cache itself)
        auto removeUnused = [](auto& map) {
            for (auto it = map.begin(); it != map.end();) {
                if (it->second.use_count() <= 1) {
                    GE_CORE_TRACE("Unloading unused resource: {0}", it->first);
                    it = map.erase(it);
                } else {
                    ++it;
                }
            }
        };
        
        removeUnused(s_Textures);
        removeUnused(s_Shaders);
        removeUnused(s_Meshes);
        removeUnused(s_AudioClips);
    }

    void ResourceManager::UnloadAll() {
        s_Textures.clear();
        s_Shaders.clear();
        s_Meshes.clear();
        s_AudioClips.clear();
        
        GE_CORE_INFO("All resources unloaded");
    }

    void ResourceManager::Reload(const std::string& path) {
        // Find and reload resource
        auto texIt = s_Textures.find(path);
        if (texIt != s_Textures.end()) {
            texIt->second = CreateRef<Texture2D>(path);
            GE_CORE_INFO("Texture reloaded: {0}", path);
            return;
        }
        
        auto shaderIt = s_Shaders.find(path);
        if (shaderIt != s_Shaders.end()) {
            shaderIt->second->Reload();
            GE_CORE_INFO("Shader reloaded: {0}", path);
            return;
        }
        
        GE_CORE_WARN("Resource not found for reload: {0}", path);
    }

    ResourceManager::Stats ResourceManager::GetStats() {
        Stats stats;
        
        stats.TotalResources = s_Textures.size() + s_Shaders.size() + 
                              s_Meshes.size() + s_AudioClips.size();
        stats.LoadedTextures = s_Textures.size();
        stats.LoadedShaders = s_Shaders.size();
        stats.LoadedMeshes = s_Meshes.size();
        stats.LoadedAudio = s_AudioClips.size();
        
        // Approximate memory usage
        for (const auto& [path, tex] : s_Textures) {
            stats.MemoryUsage += tex->GetWidth() * tex->GetHeight() * 4;  // RGBA
        }
        
        return stats;
    }
}