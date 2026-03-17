#include "MaterialLibrary.hpp"
#include "../Core/Logger.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <algorithm>

namespace GameEngine {

    MaterialLibrary& MaterialLibrary::Get() {
        static MaterialLibrary instance;
        return instance;
    }

    void MaterialLibrary::ScanDirectory(const std::string& directory) {
        m_RootDirectory = directory;
        
        if (!std::filesystem::exists(directory)) {
            GE_CORE_WARN("MaterialLibrary: Directory does not exist: {}", directory);
            return;
        }

        GE_CORE_INFO("MaterialLibrary: Scanning directory: {}", directory);

        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                if (ext == ".mat" || ext == ".material") {
                    LoadMaterial(entry.path().string());
                }
            }
        }

        GE_CORE_INFO("MaterialLibrary: Found {} materials", m_Materials.size());
    }

    void MaterialLibrary::Refresh() {
        if (!m_RootDirectory.empty()) {
            Clear();
            ScanDirectory(m_RootDirectory);
        }
    }

    void MaterialLibrary::Clear() {
        m_Materials.clear();
        m_NameToIndex.clear();
    }

    Ref<Material> MaterialLibrary::GetMaterial(const std::string& name) {
        auto it = m_NameToIndex.find(name);
        if (it != m_NameToIndex.end()) {
            return m_Materials[it->second].MaterialRef;
        }
        return nullptr;
    }

    Ref<Material> MaterialLibrary::GetMaterialByPath(const std::string& path) {
        for (auto& asset : m_Materials) {
            if (asset.Path == path) {
                return asset.MaterialRef;
            }
        }
        return nullptr;
    }

    bool MaterialLibrary::HasMaterial(const std::string& name) const {
        return m_NameToIndex.find(name) != m_NameToIndex.end();
    }

    Ref<Material> MaterialLibrary::CreateMaterial(const std::string& name, const std::string& category) {
        if (HasMaterial(name)) {
            GE_CORE_WARN("MaterialLibrary: Material already exists: {}", name);
            return GetMaterial(name);
        }

        MaterialAsset asset;
        asset.Name = name;
        asset.Category = category;
        asset.MaterialRef = CreateRef<Material>();
        asset.IsDirty = true;

        m_NameToIndex[name] = m_Materials.size();
        m_Materials.push_back(asset);

        GE_CORE_INFO("MaterialLibrary: Created material: {}", name);
        return asset.MaterialRef;
    }

    bool MaterialLibrary::SaveMaterial(const std::string& name, const std::string& path) {
        MaterialAsset* asset = FindMaterial(name);
        if (!asset) {
            GE_CORE_ERROR("MaterialLibrary: Material not found: {}", name);
            return false;
        }

        try {
            nlohmann::json j;
            j["name"] = asset->Name;
            j["category"] = asset->Category;
            
            // Serialize material properties
            // This depends on your Material class implementation
            // For now, create a basic structure
            j["properties"] = nlohmann::json::object();
            
            std::ofstream file(path);
            if (!file.is_open()) {
                GE_CORE_ERROR("MaterialLibrary: Failed to open file for writing: {}", path);
                return false;
            }

            file << j.dump(2);
            file.close();

            asset->Path = path;
            asset->IsDirty = false;

            GE_CORE_INFO("MaterialLibrary: Saved material: {} to {}", name, path);
            return true;
        } catch (const std::exception& e) {
            GE_CORE_ERROR("MaterialLibrary: Failed to save material: {}", e.what());
            return false;
        }
    }

    bool MaterialLibrary::LoadMaterial(const std::string& path) {
        try {
            std::ifstream file(path);
            if (!file.is_open()) {
                GE_CORE_ERROR("MaterialLibrary: Failed to open file: {}", path);
                return false;
            }

            nlohmann::json j;
            file >> j;

            MaterialAsset asset;
            asset.Name = j.value("name", std::filesystem::path(path).stem().string());
            asset.Path = path;
            asset.Category = j.value("category", "Default");
            asset.MaterialRef = CreateRef<Material>();
            asset.IsDirty = false;

            // Load material properties
            if (j.contains("properties")) {
                // Apply properties to material
                // This depends on your Material class implementation
            }

            // Check for duplicates
            if (HasMaterial(asset.Name)) {
                // Make name unique
                int counter = 1;
                std::string baseName = asset.Name;
                while (HasMaterial(asset.Name)) {
                    asset.Name = baseName + "_" + std::to_string(counter++);
                }
            }

            m_NameToIndex[asset.Name] = m_Materials.size();
            m_Materials.push_back(asset);

            GE_CORE_TRACE("MaterialLibrary: Loaded material: {}", asset.Name);
            return true;
        } catch (const std::exception& e) {
            GE_CORE_ERROR("MaterialLibrary: Failed to load material from {}: {}", path, e.what());
            return false;
        }
    }

    bool MaterialLibrary::DeleteMaterial(const std::string& name) {
        auto it = m_NameToIndex.find(name);
        if (it == m_NameToIndex.end()) {
            return false;
        }

        size_t index = it->second;
        
        // Delete file if exists
        if (!m_Materials[index].Path.empty() && std::filesystem::exists(m_Materials[index].Path)) {
            std::filesystem::remove(m_Materials[index].Path);
        }

        // Remove from vector
        m_Materials.erase(m_Materials.begin() + index);
        
        // Rebuild index map
        m_NameToIndex.clear();
        for (size_t i = 0; i < m_Materials.size(); i++) {
            m_NameToIndex[m_Materials[i].Name] = i;
        }

        GE_CORE_INFO("MaterialLibrary: Deleted material: {}", name);
        return true;
    }

    std::vector<std::string> MaterialLibrary::GetCategories() const {
        std::vector<std::string> categories;
        
        for (const auto& asset : m_Materials) {
            if (std::find(categories.begin(), categories.end(), asset.Category) == categories.end()) {
                categories.push_back(asset.Category);
            }
        }

        std::sort(categories.begin(), categories.end());
        return categories;
    }

    std::vector<MaterialAsset*> MaterialLibrary::GetMaterialsByCategory(const std::string& category) {
        std::vector<MaterialAsset*> result;
        
        for (auto& asset : m_Materials) {
            if (asset.Category == category) {
                result.push_back(&asset);
            }
        }

        return result;
    }

    void MaterialLibrary::GenerateThumbnails() {
        // TODO: Implement thumbnail generation
        // This would render each material to a small framebuffer
        GE_CORE_INFO("MaterialLibrary: Generating thumbnails...");
    }

    uint32_t MaterialLibrary::GetThumbnail(const std::string& name) {
        MaterialAsset* asset = FindMaterial(name);
        if (asset) {
            return asset->ThumbnailID;
        }
        return 0;
    }

    MaterialAsset* MaterialLibrary::FindMaterial(const std::string& name) {
        auto it = m_NameToIndex.find(name);
        if (it != m_NameToIndex.end()) {
            return &m_Materials[it->second];
        }
        return nullptr;
    }

}
