#pragma once

#include "../Core/Base.hpp"
#include "Material.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>

namespace GameEngine {

    struct MaterialAsset {
        std::string Name;
        std::string Path;
        std::string Category;
        Ref<Material> MaterialRef;
        uint32_t ThumbnailID = 0;
        bool IsDirty = false;
    };

    class MaterialLibrary {
    public:
        static MaterialLibrary& Get();

        // Library management
        void ScanDirectory(const std::string& directory);
        void Refresh();
        void Clear();

        // Material access
        Ref<Material> GetMaterial(const std::string& name);
        Ref<Material> GetMaterialByPath(const std::string& path);
        bool HasMaterial(const std::string& name) const;
        
        // Material creation
        Ref<Material> CreateMaterial(const std::string& name, const std::string& category = "Default");
        bool SaveMaterial(const std::string& name, const std::string& path);
        bool LoadMaterial(const std::string& path);
        bool DeleteMaterial(const std::string& name);

        // Getters
        const std::vector<MaterialAsset>& GetAllMaterials() const { return m_Materials; }
        std::vector<std::string> GetCategories() const;
        std::vector<MaterialAsset*> GetMaterialsByCategory(const std::string& category);

        // Thumbnail generation
        void GenerateThumbnails();
        uint32_t GetThumbnail(const std::string& name);

    private:
        MaterialLibrary() = default;
        ~MaterialLibrary() = default;
        MaterialLibrary(const MaterialLibrary&) = delete;
        MaterialLibrary& operator=(const MaterialLibrary&) = delete;

        MaterialAsset* FindMaterial(const std::string& name);

    private:
        std::vector<MaterialAsset> m_Materials;
        std::unordered_map<std::string, size_t> m_NameToIndex;
        std::string m_RootDirectory;
    };

}
