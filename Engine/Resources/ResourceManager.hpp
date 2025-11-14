#pragma once

#include "../Core/Base.hpp"
#include "../Core/UUID.hpp"
#include "../Graphics/Texture2D.hpp"
#include "../Graphics/Shader.hpp"
#include "../Graphics/Mesh3D.hpp"
#include "../Audio/AudioClip.hpp"
#include <unordered_map>
#include <string>
#include <typeindex>

namespace GameEngine {

    /**
     * @brief Resource type enumeration
     */
    enum class ResourceType {
        Texture,
        Shader,
        Mesh,
        Audio,
        Material,
        Scene
    };

    /**
     * @brief Base resource class
     */
    class Resource {
    public:
        virtual ~Resource() = default;
        
        UUID GetUUID() const { return m_UUID; }
        const std::string& GetPath() const { return m_Path; }
        ResourceType GetType() const { return m_Type; }
        
        bool IsLoaded() const { return m_Loaded; }
        void SetLoaded(bool loaded) { m_Loaded = loaded; }
        
    protected:
        UUID m_UUID;
        std::string m_Path;
        ResourceType m_Type;
        bool m_Loaded = false;
    };

    /**
     * @brief Resource manager - asset loading and caching
     * 
     * Features:
     * - Reference counting
     * - Hot-reloading support
     * - Async loading (future)
     * - Resource dependencies
     */
    class ResourceManager {
    public:
        /**
         * @brief Load texture
         */
        static Ref<Texture2D> LoadTexture(const std::string& path);
        
        /**
         * @brief Load shader
         */
        static Ref<Shader> LoadShader(const std::string& name, 
                                      const std::string& vertexPath, 
                                      const std::string& fragmentPath);
        
        /**
         * @brief Load mesh
         */
        static Ref<Mesh3D> LoadMesh(const std::string& path);
        
        /**
         * @brief Load audio clip
         */
        static Ref<AudioClip> LoadAudio(const std::string& path);
        
        /**
         * @brief Check if resource is loaded
         */
        static bool IsLoaded(const std::string& path);
        
        /**
         * @brief Unload resource
         */
        static void Unload(const std::string& path);
        
        /**
         * @brief Unload all unused resources
         */
        static void UnloadUnused();
        
        /**
         * @brief Unload all resources
         */
        static void UnloadAll();
        
        /**
         * @brief Hot-reload resource
         */
        static void Reload(const std::string& path);
        
        /**
         * @brief Get resource statistics
         */
        struct Stats {
            size_t TotalResources = 0;
            size_t LoadedTextures = 0;
            size_t LoadedShaders = 0;
            size_t LoadedMeshes = 0;
            size_t LoadedAudio = 0;
            size_t MemoryUsage = 0;  // Approximate in bytes
        };
        
        static Stats GetStats();
        
    private:
        template<typename T>
        static Ref<T> GetResource(const std::string& path);
        
        template<typename T>
        static void CacheResource(const std::string& path, const Ref<T>& resource);
        
        static std::unordered_map<std::string, Ref<Texture2D>> s_Textures;
        static std::unordered_map<std::string, Ref<Shader>> s_Shaders;
        static std::unordered_map<std::string, Ref<Mesh3D>> s_Meshes;
        static std::unordered_map<std::string, Ref<AudioClip>> s_AudioClips;
    };
}