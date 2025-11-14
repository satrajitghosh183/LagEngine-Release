#pragma once

#include "../Core/Base.hpp"
#include "Scene.hpp"
#include "SceneSerializer.hpp"
#include <string>
#include <unordered_map>

namespace GameEngine {

    /**
     * @brief Scene manager - Multi-scene management
     * 
     * Features:
     * - Multiple scene management
     * - Scene loading/unloading
     * - Scene transitions
     * - Active scene tracking
     * - Scene persistence
     * 
     * Usage:
     *   SceneManager manager;
     *   
     *   auto mainScene = CreateRef<Scene>("MainScene");
     *   manager.AddScene("Main", mainScene);
     *   manager.SetActiveScene("Main");
     *   
     *   manager.LoadScene("Assets/Scenes/Level1.scene", "Level1");
     *   manager.TransitionTo("Level1");
     */
    class SceneManager {
    public:
        SceneManager();
        ~SceneManager();
        
        /**
         * @brief Add scene to manager
         */
        void AddScene(const std::string& name, const Ref<Scene>& scene);
        
        /**
         * @brief Remove scene from manager
         */
        void RemoveScene(const std::string& name);
        
        /**
         * @brief Load scene from file
         */
        Ref<Scene> LoadScene(const std::string& filepath, const std::string& name = "");
        
        /**
         * @brief Save scene to file
         */
        bool SaveScene(const std::string& name, const std::string& filepath);
        
        /**
         * @brief Set active scene
         */
        void SetActiveScene(const std::string& name);
        
        /**
         * @brief Get active scene
         */
        Ref<Scene> GetActiveScene() const { return m_ActiveScene; }
        
        /**
         * @brief Get scene by name
         */
        Ref<Scene> GetScene(const std::string& name);
        
        /**
         * @brief Check if scene exists
         */
        bool HasScene(const std::string& name) const;
        
        /**
         * @brief Transition to another scene
         */
        void TransitionTo(const std::string& name, bool unloadCurrent = false);
        
        /**
         * @brief Get all scene names
         */
        std::vector<std::string> GetSceneNames() const;
        
    private:
        std::unordered_map<std::string, Ref<Scene>> m_Scenes;
        Ref<Scene> m_ActiveScene;
        std::string m_ActiveSceneName;
    };
}