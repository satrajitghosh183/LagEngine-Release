#pragma once

#include "../Core/Base.hpp"
#include "Entity.hpp"
#include "Components/Component.hpp"
#include <nlohmann/json.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <filesystem>

namespace GameEngine {

    // Forward declaration
    class Scene;

    /**
     * @brief Prefab - reusable entity template
     * 
     * A prefab stores an entity's complete state including:
     * - All components and their values
     * - Child entities (hierarchy)
     * - Component settings
     * 
     * Prefabs can be:
     * - Created from existing entities
     * - Instantiated into scenes
     * - Saved/loaded from files
     * - Linked (instances update when prefab changes)
     */
    class Prefab {
    public:
        Prefab() = default;
        Prefab(const std::string& name);
        ~Prefab() = default;
        
        /**
         * @brief Create prefab from existing entity
         * @param entity Source entity to create prefab from
         * @param includeChildren Include child entities in prefab
         */
        static Ref<Prefab> CreateFromEntity(Entity entity, bool includeChildren = true);
        
        /**
         * @brief Instantiate prefab into scene
         * @param scene Target scene
         * @param position Optional spawn position
         * @param rotation Optional spawn rotation
         * @return Root entity of instantiated prefab
         */
        Entity Instantiate(
            Scene* scene,
            const glm::vec3& position = glm::vec3(0),
            const glm::quat& rotation = glm::quat(1, 0, 0, 0)
        ) const;
        
        /**
         * @brief Save prefab to file
         */
        bool Save(const std::string& filepath) const;
        
        /**
         * @brief Load prefab from file
         */
        static Ref<Prefab> Load(const std::string& filepath);
        
        /**
         * @brief Get/Set name
         */
        const std::string& GetName() const { return m_Name; }
        void SetName(const std::string& name) { m_Name = name; }
        
        /**
         * @brief Get/Set description
         */
        const std::string& GetDescription() const { return m_Description; }
        void SetDescription(const std::string& description) { m_Description = description; }
        
        /**
         * @brief Get/Set tags for organization
         */
        const std::vector<std::string>& GetTags() const { return m_Tags; }
        void SetTags(const std::vector<std::string>& tags) { m_Tags = tags; }
        void AddTag(const std::string& tag);
        void RemoveTag(const std::string& tag);
        bool HasTag(const std::string& tag) const;
        
        /**
         * @brief Get file path (if loaded from file)
         */
        const std::string& GetFilePath() const { return m_FilePath; }
        
        /**
         * @brief Get unique ID
         */
        UUID GetID() const { return m_ID; }
        
        /**
         * @brief Get serialized data
         */
        const nlohmann::json& GetData() const { return m_Data; }
        
    private:
        void SerializeEntity(Entity entity, nlohmann::json& data, bool includeChildren);
        Entity DeserializeEntity(Scene* scene, const nlohmann::json& data, Entity parent) const;
        
    private:
        UUID m_ID;
        std::string m_Name = "Prefab";
        std::string m_Description;
        std::vector<std::string> m_Tags;
        std::string m_FilePath;
        
        nlohmann::json m_Data;  // Serialized entity data
    };

    /**
     * @brief Prefab instance component
     * 
     * Attached to entities that were instantiated from a prefab.
     * Enables tracking and updating when the source prefab changes.
     */
    class PrefabInstanceComponent : public Component {
    public:
        PrefabInstanceComponent() = default;
        PrefabInstanceComponent(UUID prefabID, const std::string& prefabPath);
        
        COMPONENT_TYPE(PrefabInstanceComponent)
        
        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& data) override;
        
        /**
         * @brief Get source prefab ID
         */
        UUID GetPrefabID() const { return m_PrefabID; }
        
        /**
         * @brief Get source prefab path
         */
        const std::string& GetPrefabPath() const { return m_PrefabPath; }
        
        /**
         * @brief Check if linked to prefab (updates when prefab changes)
         */
        bool IsLinked() const { return m_IsLinked; }
        void SetLinked(bool linked) { m_IsLinked = linked; }
        
        /**
         * @brief Store overrides (properties that differ from prefab)
         */
        void SetOverride(const std::string& componentType, const std::string& property, const nlohmann::json& value);
        bool HasOverride(const std::string& componentType, const std::string& property) const;
        nlohmann::json GetOverride(const std::string& componentType, const std::string& property) const;
        void ClearOverride(const std::string& componentType, const std::string& property);
        void ClearAllOverrides();
        
        /**
         * @brief Apply prefab updates (only non-overridden values)
         */
        void ApplyPrefabChanges(const Ref<Prefab>& prefab);
        
        /**
         * @brief Revert all changes to match prefab
         */
        void RevertToSource(const Ref<Prefab>& prefab);
        
    private:
        UUID m_PrefabID;
        std::string m_PrefabPath;
        bool m_IsLinked = true;
        
        // Component type -> property name -> overridden value
        std::unordered_map<std::string, std::unordered_map<std::string, nlohmann::json>> m_Overrides;
    };

    /**
     * @brief Manages prefab library
     */
    class PrefabLibrary {
    public:
        /**
         * @brief Initialize library from directory
         */
        static void Initialize(const std::string& prefabDirectory);
        
        /**
         * @brief Scan and reload all prefabs
         */
        static void Refresh();
        
        /**
         * @brief Get prefab by ID
         */
        static Ref<Prefab> GetPrefab(UUID id);
        
        /**
         * @brief Get prefab by path
         */
        static Ref<Prefab> GetPrefab(const std::string& path);
        
        /**
         * @brief Get all loaded prefabs
         */
        static const std::unordered_map<UUID, Ref<Prefab>>& GetAllPrefabs();
        
        /**
         * @brief Register a new prefab
         */
        static void RegisterPrefab(const Ref<Prefab>& prefab);
        
        /**
         * @brief Unregister prefab
         */
        static void UnregisterPrefab(UUID id);
        
    private:
        static std::unordered_map<UUID, Ref<Prefab>> s_Prefabs;
        static std::unordered_map<std::string, UUID> s_PathToID;
        static std::string s_PrefabDirectory;
    };

}
