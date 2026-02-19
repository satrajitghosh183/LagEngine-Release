#include "SceneSerializer.hpp"
#include "../Core/Logger.hpp"
#include "Components/TransformComponent.hpp"
#include "Components/MeshRendererComponent.hpp"
#include "Components/RigidBodyComponent.hpp"
#include "Components/CameraComponent.hpp"
#include "Components/ColliderComponent.hpp"
#include "Components/LightComponent.hpp"
#include "Components/ClothComponent.hpp"
#include "Components/JointComponent.hpp"
#include "Components/FluidEmitterComponent.hpp"
#include "Components/ScriptComponent.hpp"
#include "Components/GPUParticleComponent.hpp"
#include "Components/RobotArmComponent.hpp"
#include "Components/SoftBodyComponent.hpp"
#include "Components/SpriteComponent.hpp"
#include "Components/SpriteAnimatorComponent.hpp"
#include "Prefab.hpp"
#include <fstream>
#include <sstream>

namespace GameEngine {

    SceneSerializer::SceneSerializer(const Ref<Scene>& scene)
        : m_Scene(scene) {
    }

    bool SceneSerializer::Serialize(const std::string& filepath) {
        std::string jsonString = SerializeToString();
        
        std::ofstream file(filepath);
        if (!file.is_open()) {
            GE_CORE_ERROR("Failed to open file for writing: {0}", filepath);
            return false;
        }
        
        file << jsonString;
        file.close();
        
        GE_CORE_INFO("Scene '{0}' serialized to: {1}", m_Scene->GetName(), filepath);
        return true;
    }

    bool SceneSerializer::Deserialize(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            GE_CORE_ERROR("Failed to open file for reading: {0}", filepath);
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        
        return DeserializeFromString(buffer.str());
    }

    std::string SceneSerializer::SerializeToString() {
        nlohmann::json sceneJson;
        
        // Scene metadata with version
        sceneJson["version"] = SCENE_VERSION_STRING;
        sceneJson["versionMajor"] = SCENE_VERSION_MAJOR;
        sceneJson["versionMinor"] = SCENE_VERSION_MINOR;
        sceneJson["versionPatch"] = SCENE_VERSION_PATCH;
        sceneJson["name"] = m_Scene->GetName();
        sceneJson["uuid"] = m_Scene->GetUUID().ToString();
        
        // Physics settings
        sceneJson["physics"]["gravity"] = {
            m_Scene->GetPhysicsWorld()->GetGravity().x,
            m_Scene->GetPhysicsWorld()->GetGravity().y,
            m_Scene->GetPhysicsWorld()->GetGravity().z
        };
        
        // Entities
        nlohmann::json entitiesJson = nlohmann::json::array();
        
        for (auto& entity : m_Scene->GetAllEntities()) {
            // Only serialize root entities (hierarchy will be preserved)
            if (!entity.HasParent()) {
                entitiesJson.push_back(SerializeEntity(entity));
            }
        }
        
        sceneJson["entities"] = entitiesJson;
        
        // Pretty print with 2-space indentation
        return sceneJson.dump(2);
    }

    bool SceneSerializer::DeserializeFromString(const std::string& jsonString) {
        try {
            nlohmann::json sceneJson = nlohmann::json::parse(jsonString);
            
            // Check version
            if (!sceneJson.contains("version")) {
                GE_CORE_ERROR("Scene file missing version information");
                return false;
            }
            
            // Version checking moved to migration system
            
            // Scene metadata
            if (sceneJson.contains("name")) {
                m_Scene->SetName(sceneJson["name"]);
            }
            
            // Physics settings
            if (sceneJson.contains("physics") && sceneJson["physics"].contains("gravity")) {
                auto gravity = sceneJson["physics"]["gravity"];
                m_Scene->GetPhysicsWorld()->SetGravity(
                    glm::vec3(gravity[0], gravity[1], gravity[2])
                );
            }
            
            // Deserialize entities
            if (sceneJson.contains("entities")) {
                for (const auto& entityJson : sceneJson["entities"]) {
                    DeserializeEntity(entityJson);
                }
            }
            
            GE_CORE_INFO("Scene '{0}' deserialized successfully", m_Scene->GetName());
            return true;
            
        } catch (const nlohmann::json::exception& e) {
            GE_CORE_ERROR("Failed to parse scene JSON: {0}", e.what());
            return false;
        }
    }

    bool SceneSerializer::MigrateScene(nlohmann::json& data, int fromVersion, int toVersion) {
        int currentVersion = fromVersion;
        
        while (currentVersion < toVersion) {
            int nextVersion = currentVersion + 1;
            
            if (currentVersion == 10000 && nextVersion == 20000) {
                // Migrate from 1.0.0 to 2.0.0
                if (!MigrateV1ToV2(data)) {
                    return false;
                }
                currentVersion = nextVersion;
            } else {
                GE_CORE_ERROR("No migration path from version {0} to {1}", currentVersion, nextVersion);
                return false;
            }
        }
        
        // Update version fields
        data["version"] = SCENE_VERSION_STRING;
        data["versionMajor"] = SCENE_VERSION_MAJOR;
        data["versionMinor"] = SCENE_VERSION_MINOR;
        data["versionPatch"] = SCENE_VERSION_PATCH;
        
        return true;
    }

    bool SceneSerializer::MigrateV1ToV2(nlohmann::json& data) {
        // Example migration: add new fields, restructure data, etc.
        // For now, this is a placeholder
        GE_CORE_INFO("Migrating scene from version 1.0.0 to 2.0.0");
        
        // Add any new required fields or restructure as needed
        if (data.contains("scene")) {
            auto& sceneData = data["scene"];
            // Example: add default values for new fields
            if (!sceneData.contains("ambientLight")) {
                sceneData["ambientLight"] = {0.2f, 0.2f, 0.2f};
            }
        }
        
        return true;
    }

    nlohmann::json SceneSerializer::SerializeEntity(Entity entity) {
        nlohmann::json entityJson;
        
        entityJson["uuid"] = entity.GetUUID().ToString();
        entityJson["name"] = entity.GetName();
        entityJson["tag"] = entity.GetTag();
        entityJson["active"] = entity.IsActive();
        
        // Serialize components
        nlohmann::json componentsJson = nlohmann::json::array();
        
        for (auto* component : entity.GetAllComponents()) {
            if (component) {
                componentsJson.push_back(SerializeComponent(component));
            }
        }
        
        entityJson["components"] = componentsJson;
        
        // Serialize children (recursive)
        if (!entity.GetChildren().empty()) {
            nlohmann::json childrenJson = nlohmann::json::array();
            
            for (const auto& child : entity.GetChildren()) {
                childrenJson.push_back(SerializeEntity(child));
            }
            
            entityJson["children"] = childrenJson;
        }
        
        return entityJson;
    }

    Entity SceneSerializer::DeserializeEntity(const nlohmann::json& entityJson) {
        UUID uuid(0);
        
        // Parse UUID
        if (entityJson.contains("uuid")) {
            std::string uuidStr = entityJson["uuid"];
            uuid = UUID(std::stoull(uuidStr, nullptr, 16));
        }
        
        // Create entity
        std::string name = entityJson.value("name", "Entity");
        Entity entity = m_Scene->CreateEntityWithUUID(uuid, name);
        
        // Set properties
        if (entityJson.contains("tag")) {
            entity.SetTag(entityJson["tag"]);
        }
        
        if (entityJson.contains("active")) {
            entity.SetActive(entityJson["active"]);
        }
        
        // Deserialize components
        if (entityJson.contains("components")) {
            for (const auto& componentJson : entityJson["components"]) {
                DeserializeComponent(entity, componentJson);
            }
        }
        
        // Deserialize children (recursive)
        if (entityJson.contains("children")) {
            for (const auto& childJson : entityJson["children"]) {
                Entity child = DeserializeEntity(childJson);
                child.SetParent(entity);
            }
        }
        
        return entity;
    }

    nlohmann::json SceneSerializer::SerializeComponent(Component* component) {
        return component->Serialize();
    }

    void SceneSerializer::DeserializeComponent(Entity entity, const nlohmann::json& componentJson) {
        if (!componentJson.contains("type")) {
            GE_CORE_WARN("Component JSON missing type field");
            return;
        }
        
        std::string type = componentJson["type"];
        
        // Handle each component type
        if (type == "TransformComponent") {
            // Transform is added by default, just deserialize
            if (entity.HasComponent<TransformComponent>()) {
                entity.GetComponent<TransformComponent>().Deserialize(componentJson);
            }
        }
        else if (type == "MeshRendererComponent") {
            auto& comp = entity.AddComponent<MeshRendererComponent>();
            comp.Deserialize(componentJson);
        }
        else if (type == "RigidBodyComponent") {
            auto& comp = entity.AddComponent<RigidBodyComponent>();
            comp.Deserialize(componentJson);
        }
        else if (type == "CameraComponent") {
            auto& comp = entity.AddComponent<CameraComponent>();
            comp.Deserialize(componentJson);
        }
        else if (type == "ColliderComponent") {
            auto& comp = entity.AddComponent<ColliderComponent>();
            comp.Deserialize(componentJson);
        }
        else if (type == "LightComponent") {
            auto& comp = entity.AddComponent<LightComponent>();
            comp.Deserialize(componentJson);
        }
        else if (type == "ClothComponent") {
            auto& comp = entity.AddComponent<ClothComponent>();
            comp.Deserialize(componentJson);
        }
        else if (type == "JointComponent") {
            auto& comp = entity.AddComponent<JointComponent>();
            comp.Deserialize(componentJson);
        }
        else if (type == "FluidEmitterComponent") {
            auto& comp = entity.AddComponent<FluidEmitterComponent>();
            comp.Deserialize(componentJson);
        }
        else if (type == "ScriptComponent") {
            auto& comp = entity.AddComponent<ScriptComponent>();
            comp.Deserialize(componentJson);
        }
        else if (type == "PrefabInstanceComponent") {
            auto& comp = entity.AddComponent<PrefabInstanceComponent>();
            comp.Deserialize(componentJson);
        }
        else if (type == "GPUParticleComponent") {
            auto& comp = entity.AddComponent<GPUParticleComponent>();
            comp.Deserialize(componentJson);
        }
        else if (type == "RobotArmComponent") {
            auto& comp = entity.AddComponent<RobotArmComponent>();
            comp.Deserialize(componentJson);
        }
        else if (type == "SoftBodyComponent") {
            auto& comp = entity.AddComponent<SoftBodyComponent>();
            comp.Deserialize(componentJson);
        }
        else if (type == "SpriteComponent") {
            auto& comp = entity.AddComponent<SpriteComponent>();
            comp.Deserialize(componentJson);
        }
        else if (type == "SpriteAnimatorComponent") {
            auto& comp = entity.AddComponent<SpriteAnimatorComponent>();
            comp.Deserialize(componentJson);
        }
        else {
            GE_CORE_WARN("Unknown component type: {0}", type);
        }
    }

}
