#include "Prefab.hpp"
#include "Scene.hpp"
#include "Components/TransformComponent.hpp"
#include "../Core/Logger.hpp"
#include "../Platform/FileSystem.hpp"
#include <fstream>

namespace GameEngine {

    // ==================== Prefab ====================

    Prefab::Prefab(const std::string& name)
        : m_Name(name) {
        m_ID = UUID();
    }

    Ref<Prefab> Prefab::CreateFromEntity(Entity entity, bool includeChildren) {
        if (!entity.IsValid()) {
            GE_CORE_ERROR("Prefab::CreateFromEntity: Invalid entity");
            return nullptr;
        }
        
        auto prefab = CreateRef<Prefab>(entity.GetName());
        prefab->SerializeEntity(entity, prefab->m_Data, includeChildren);
        
        GE_CORE_INFO("Created prefab '{}' from entity", prefab->m_Name);
        return prefab;
    }

    Entity Prefab::Instantiate(
        Scene* scene,
        const glm::vec3& position,
        const glm::quat& rotation
    ) const {
        if (!scene || m_Data.empty()) {
            GE_CORE_ERROR("Prefab::Instantiate: Invalid scene or empty prefab data");
            return Entity();
        }
        
        Entity root = DeserializeEntity(scene, m_Data, Entity());
        
        // Apply position and rotation offsets
        if (root.IsValid() && root.HasComponent<TransformComponent>()) {
            auto& transform = root.GetComponent<TransformComponent>();
            transform.Position = position;
            transform.Rotation = rotation;
        }
        
        // Add prefab instance component to track the connection
        if (root.IsValid()) {
            auto& instance = root.AddComponent<PrefabInstanceComponent>(m_ID, m_FilePath);
            instance.SetLinked(true);
        }
        
        GE_CORE_INFO("Instantiated prefab '{}' at ({}, {}, {})", 
            m_Name, position.x, position.y, position.z);
        
        return root;
    }

    bool Prefab::Save(const std::string& filepath) const {
        nlohmann::json root;
        root["version"] = 1;
        root["id"] = static_cast<uint64_t>(m_ID);
        root["name"] = m_Name;
        root["description"] = m_Description;
        root["tags"] = m_Tags;
        root["entity"] = m_Data;
        
        std::ofstream file(filepath);
        if (!file.is_open()) {
            GE_CORE_ERROR("Prefab::Save: Failed to open file '{}'", filepath);
            return false;
        }
        
        file << root.dump(2);
        file.close();
        
        GE_CORE_INFO("Saved prefab '{}' to '{}'", m_Name, filepath);
        return true;
    }

    Ref<Prefab> Prefab::Load(const std::string& filepath) {
        if (!FileSystem::Exists(filepath)) {
            GE_CORE_ERROR("Prefab::Load: File not found '{}'", filepath);
            return nullptr;
        }
        
        std::string content = FileSystem::ReadFileAsString(filepath);
        
        try {
            nlohmann::json root = nlohmann::json::parse(content);
            
            auto prefab = CreateRef<Prefab>();
            prefab->m_FilePath = filepath;
            
            if (root.contains("id")) {
                prefab->m_ID = UUID(root["id"].get<uint64_t>());
            }
            if (root.contains("name")) {
                prefab->m_Name = root["name"].get<std::string>();
            }
            if (root.contains("description")) {
                prefab->m_Description = root["description"].get<std::string>();
            }
            if (root.contains("tags")) {
                prefab->m_Tags = root["tags"].get<std::vector<std::string>>();
            }
            if (root.contains("entity")) {
                prefab->m_Data = root["entity"];
            }
            
            GE_CORE_INFO("Loaded prefab '{}' from '{}'", prefab->m_Name, filepath);
            return prefab;
            
        } catch (const std::exception& e) {
            GE_CORE_ERROR("Prefab::Load: JSON parse error - {}", e.what());
            return nullptr;
        }
    }

    void Prefab::AddTag(const std::string& tag) {
        if (!HasTag(tag)) {
            m_Tags.push_back(tag);
        }
    }

    void Prefab::RemoveTag(const std::string& tag) {
        m_Tags.erase(
            std::remove(m_Tags.begin(), m_Tags.end(), tag),
            m_Tags.end()
        );
    }

    bool Prefab::HasTag(const std::string& tag) const {
        return std::find(m_Tags.begin(), m_Tags.end(), tag) != m_Tags.end();
    }

    void Prefab::SerializeEntity(Entity entity, nlohmann::json& data, bool includeChildren) {
        // Basic entity info
        data["name"] = entity.GetName();
        data["tag"] = entity.GetTag();
        data["active"] = entity.IsActive();
        
        // Serialize all components
        nlohmann::json components = nlohmann::json::array();
        
        // Get registry from entity's scene
        // This would iterate through all component types and serialize them
        // Implementation depends on your component reflection system
        
        // For now, serialize known component types
        if (entity.HasComponent<TransformComponent>()) {
            auto& transform = entity.GetComponent<TransformComponent>();
            nlohmann::json comp;
            comp["type"] = "TransformComponent";
            comp["position"] = { transform.Position.x, transform.Position.y, transform.Position.z };
            comp["rotation"] = { transform.Rotation.x, transform.Rotation.y, transform.Rotation.z, transform.Rotation.w };
            comp["scale"] = { transform.Scale.x, transform.Scale.y, transform.Scale.z };
            components.push_back(comp);
        }
        
        // Add more component serialization as needed...
        
        data["components"] = components;
        
        // Serialize children
        if (includeChildren) {
            nlohmann::json children = nlohmann::json::array();
            // Get children from hierarchy (implementation depends on your hierarchy system)
            data["children"] = children;
        }
    }

    Entity Prefab::DeserializeEntity(Scene* scene, const nlohmann::json& data, Entity parent) const {
        std::string name = data.value("name", "Entity");
        Entity entity = scene->CreateEntity(name);
        
        if (data.contains("tag")) {
            entity.SetTag(data["tag"].get<std::string>());
        }
        
        if (data.contains("active")) {
            entity.SetActive(data["active"].get<bool>());
        }
        
        // Deserialize components
        if (data.contains("components")) {
            for (const auto& comp : data["components"]) {
                std::string type = comp.value("type", "");
                
                if (type == "TransformComponent") {
                    auto& transform = entity.GetComponent<TransformComponent>();
                    
                    if (comp.contains("position")) {
                        auto pos = comp["position"];
                        transform.Position = glm::vec3(pos[0], pos[1], pos[2]);
                    }
                    if (comp.contains("rotation")) {
                        auto rot = comp["rotation"];
                        transform.Rotation = glm::quat(rot[3], rot[0], rot[1], rot[2]);
                    }
                    if (comp.contains("scale")) {
                        auto scale = comp["scale"];
                        transform.Scale = glm::vec3(scale[0], scale[1], scale[2]);
                    }
                }
                // Add more component deserialization as needed...
            }
        }
        
        // Deserialize children
        if (data.contains("children")) {
            for (const auto& childData : data["children"]) {
                DeserializeEntity(scene, childData, entity);
            }
        }
        
        return entity;
    }

    // ==================== PrefabInstanceComponent ====================

    PrefabInstanceComponent::PrefabInstanceComponent(UUID prefabID, const std::string& prefabPath)
        : m_PrefabID(prefabID)
        , m_PrefabPath(prefabPath) {
    }

    nlohmann::json PrefabInstanceComponent::Serialize() const {
        nlohmann::json data;
        data["type"] = GetTypeName();
        data["prefabID"] = static_cast<uint64_t>(m_PrefabID);
        data["prefabPath"] = m_PrefabPath;
        data["isLinked"] = m_IsLinked;
        
        // Serialize overrides
        nlohmann::json overrides = nlohmann::json::object();
        for (const auto& [component, properties] : m_Overrides) {
            overrides[component] = properties;
        }
        data["overrides"] = overrides;
        
        return data;
    }

    void PrefabInstanceComponent::Deserialize(const nlohmann::json& data) {
        if (data.contains("prefabID")) {
            m_PrefabID = UUID(data["prefabID"].get<uint64_t>());
        }
        if (data.contains("prefabPath")) {
            m_PrefabPath = data["prefabPath"].get<std::string>();
        }
        if (data.contains("isLinked")) {
            m_IsLinked = data["isLinked"].get<bool>();
        }
        if (data.contains("overrides")) {
            for (auto& [component, properties] : data["overrides"].items()) {
                for (auto& [prop, value] : properties.items()) {
                    m_Overrides[component][prop] = value;
                }
            }
        }
    }

    void PrefabInstanceComponent::SetOverride(
        const std::string& componentType,
        const std::string& property,
        const nlohmann::json& value
    ) {
        m_Overrides[componentType][property] = value;
    }

    bool PrefabInstanceComponent::HasOverride(
        const std::string& componentType,
        const std::string& property
    ) const {
        auto compIt = m_Overrides.find(componentType);
        if (compIt == m_Overrides.end()) return false;
        return compIt->second.find(property) != compIt->second.end();
    }

    nlohmann::json PrefabInstanceComponent::GetOverride(
        const std::string& componentType,
        const std::string& property
    ) const {
        auto compIt = m_Overrides.find(componentType);
        if (compIt == m_Overrides.end()) return nlohmann::json();
        
        auto propIt = compIt->second.find(property);
        if (propIt == compIt->second.end()) return nlohmann::json();
        
        return propIt->second;
    }

    void PrefabInstanceComponent::ClearOverride(
        const std::string& componentType,
        const std::string& property
    ) {
        auto compIt = m_Overrides.find(componentType);
        if (compIt != m_Overrides.end()) {
            compIt->second.erase(property);
            if (compIt->second.empty()) {
                m_Overrides.erase(compIt);
            }
        }
    }

    void PrefabInstanceComponent::ClearAllOverrides() {
        m_Overrides.clear();
    }

    void PrefabInstanceComponent::ApplyPrefabChanges(const Ref<Prefab>& prefab) {
        // Would apply prefab values to entity, skipping overridden properties
        // Implementation depends on component reflection system
    }

    void PrefabInstanceComponent::RevertToSource(const Ref<Prefab>& prefab) {
        ClearAllOverrides();
        ApplyPrefabChanges(prefab);
    }

    // ==================== PrefabLibrary ====================

    std::unordered_map<UUID, Ref<Prefab>> PrefabLibrary::s_Prefabs;
    std::unordered_map<std::string, UUID> PrefabLibrary::s_PathToID;
    std::string PrefabLibrary::s_PrefabDirectory;

    void PrefabLibrary::Initialize(const std::string& prefabDirectory) {
        s_PrefabDirectory = prefabDirectory;
        Refresh();
    }

    void PrefabLibrary::Refresh() {
        s_Prefabs.clear();
        s_PathToID.clear();
        
        if (s_PrefabDirectory.empty() || !FileSystem::Exists(s_PrefabDirectory)) {
            GE_CORE_WARN("PrefabLibrary: Directory not found '{}'", s_PrefabDirectory);
            return;
        }
        
        // Scan directory for .prefab files
        for (const auto& entry : std::filesystem::recursive_directory_iterator(s_PrefabDirectory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".prefab") {
                auto prefab = Prefab::Load(entry.path().string());
                if (prefab) {
                    RegisterPrefab(prefab);
                }
            }
        }
        
        GE_CORE_INFO("PrefabLibrary: Loaded {} prefabs from '{}'", 
            s_Prefabs.size(), s_PrefabDirectory);
    }

    Ref<Prefab> PrefabLibrary::GetPrefab(UUID id) {
        auto it = s_Prefabs.find(id);
        return (it != s_Prefabs.end()) ? it->second : nullptr;
    }

    Ref<Prefab> PrefabLibrary::GetPrefab(const std::string& path) {
        auto it = s_PathToID.find(path);
        if (it != s_PathToID.end()) {
            return GetPrefab(it->second);
        }
        return nullptr;
    }

    const std::unordered_map<UUID, Ref<Prefab>>& PrefabLibrary::GetAllPrefabs() {
        return s_Prefabs;
    }

    void PrefabLibrary::RegisterPrefab(const Ref<Prefab>& prefab) {
        if (!prefab) return;
        
        s_Prefabs[prefab->GetID()] = prefab;
        
        if (!prefab->GetFilePath().empty()) {
            s_PathToID[prefab->GetFilePath()] = prefab->GetID();
        }
    }

    void PrefabLibrary::UnregisterPrefab(UUID id) {
        auto it = s_Prefabs.find(id);
        if (it != s_Prefabs.end()) {
            // Remove path mapping
            if (!it->second->GetFilePath().empty()) {
                s_PathToID.erase(it->second->GetFilePath());
            }
            s_Prefabs.erase(it);
        }
    }

}
