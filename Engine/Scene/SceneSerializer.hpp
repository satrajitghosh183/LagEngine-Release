#pragma once

#include "../Core/Base.hpp"
#include "Scene.hpp"
#include <nlohmann/json.hpp>
#include <string>

namespace GameEngine {

    /**
     * @brief Scene serializer - Save/Load scenes to/from JSON
     * 
     * Features:
     * - Complete scene state serialization
     * - Entity hierarchy preservation
     * - Component serialization
     * - Asset reference handling
     * - Human-readable JSON format
     * - Versioning support
     * 
     * Usage:
     *   SceneSerializer serializer(scene);
     *   serializer.Serialize("Assets/Scenes/MainScene.scene");
     *   
     *   auto loadedScene = CreateRef<Scene>();
     *   SceneSerializer loader(loadedScene);
     *   loader.Deserialize("Assets/Scenes/MainScene.scene");
     */
    class SceneSerializer {
    public:
        SceneSerializer(const Ref<Scene>& scene);
        
        /**
         * @brief Serialize scene to file
         */
        bool Serialize(const std::string& filepath);
        
        /**
         * @brief Deserialize scene from file
         */
        bool Deserialize(const std::string& filepath);
        
        /**
         * @brief Serialize to JSON string
         */
        std::string SerializeToString();
        
        /**
         * @brief Deserialize from JSON string
         */
        bool DeserializeFromString(const std::string& jsonString);
        
    private:
        nlohmann::json SerializeEntity(Entity entity);
        Entity DeserializeEntity(const nlohmann::json& entityJson);
        
        nlohmann::json SerializeComponent(Component* component);
        void DeserializeComponent(Entity entity, const nlohmann::json& componentJson);
        
    private:
        Ref<Scene> m_Scene;
        
        // Version for compatibility
        static constexpr int SCENE_VERSION_MAJOR = 1;
        static constexpr int SCENE_VERSION_MINOR = 0;
        static constexpr int SCENE_VERSION_PATCH = 0;
        static constexpr const char* SCENE_VERSION_STRING = "1.0.0";

        /**
         * @brief Migration functions
         */
        bool MigrateScene(nlohmann::json& data, int fromVersion, int toVersion);
        bool MigrateV1ToV2(nlohmann::json& data);
    };
}