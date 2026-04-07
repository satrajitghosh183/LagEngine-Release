#include "SceneManager.hpp"
#include "../Core/Logger.hpp"

namespace GameEngine {

    SceneManager::SceneManager() {
        GE_CORE_INFO("SceneManager created");
    }

    SceneManager::~SceneManager() {
        // Stop active scene
        if (m_ActiveScene) {
            m_ActiveScene->OnStop();
        }
        
        m_Scenes.clear();
        GE_CORE_INFO("SceneManager destroyed");
    }

    void SceneManager::AddScene(const std::string& name, const Ref<Scene>& scene) {
        if (m_Scenes.find(name) != m_Scenes.end()) {
            GE_CORE_WARN("Scene '{0}' already exists, replacing", name);
        }
        
        m_Scenes[name] = scene;
        GE_CORE_INFO("Scene '{0}' added to manager", name);
    }

    void SceneManager::RemoveScene(const std::string& name) {
        auto it = m_Scenes.find(name);
        if (it != m_Scenes.end()) {
            // Stop scene if it's active
            if (m_ActiveSceneName == name) {
                m_ActiveScene->OnStop();
                m_ActiveScene.reset();
                m_ActiveSceneName.clear();
            }
            
            m_Scenes.erase(it);
            GE_CORE_INFO("Scene '{0}' removed from manager", name);
        } else {
            GE_CORE_WARN("Attempted to remove non-existent scene '{0}'", name);
        }
    }

    Ref<Scene> SceneManager::LoadScene(const std::string& filepath, const std::string& name) {
        std::string sceneName = name;
        
        // Extract name from filepath if not provided
        if (sceneName.empty()) {
            size_t lastSlash = filepath.find_last_of("/\\");
            size_t lastDot = filepath.rfind('.');
            
            if (lastSlash != std::string::npos && lastDot != std::string::npos) {
                sceneName = filepath.substr(lastSlash + 1, lastDot - lastSlash - 1);
            } else {
                sceneName = "LoadedScene";
            }
        }
        
        // Create new scene
        auto scene = CreateRef<Scene>(sceneName);
        
        // Deserialize
        SceneSerializer serializer(scene);
        if (serializer.Deserialize(filepath)) {
            AddScene(sceneName, scene);
            GE_CORE_INFO("Scene loaded successfully: {0} -> {1}", filepath, sceneName);
            return scene;
        } else {
            GE_CORE_ERROR("Failed to load scene from: {0}", filepath);
            return nullptr;
        }
    }

    bool SceneManager::SaveScene(const std::string& name, const std::string& filepath) {
        auto scene = GetScene(name);
        if (!scene) {
            GE_CORE_ERROR("Scene '{0}' not found", name);
            return false;
        }
        
        SceneSerializer serializer(scene);
        return serializer.Serialize(filepath);
    }

    void SceneManager::SetActiveScene(const std::string& name) {
        auto it = m_Scenes.find(name);
        if (it == m_Scenes.end()) {
            GE_CORE_ERROR("Scene '{0}' not found", name);
            return;
        }
        
        // Stop current scene
        if (m_ActiveScene) {
            m_ActiveScene->OnStop();
        }
        
        // Set new active scene
        m_ActiveScene = it->second;
        m_ActiveSceneName = name;
        
        // Start new scene
        m_ActiveScene->OnStart();
        
        GE_CORE_INFO("Active scene set to: {0}", name);
    }

    void SceneManager::SetActiveScene(const Ref<Scene>& scene) {
        if (!scene) {
            GE_CORE_ERROR("Cannot set null scene as active");
            return;
        }
        
        // Stop current scene
        if (m_ActiveScene) {
            m_ActiveScene->OnStop();
        }
        
        // Set new active scene
        m_ActiveScene = scene;
        m_ActiveSceneName = scene->GetName();
        
        // Add to scenes map if not already present
        if (m_Scenes.find(m_ActiveSceneName) == m_Scenes.end()) {
            m_Scenes[m_ActiveSceneName] = scene;
        }
        
        // Start new scene
        m_ActiveScene->OnStart();
        
        GE_CORE_INFO("Active scene set to: {0}", m_ActiveSceneName);
    }

    Ref<Scene> SceneManager::GetScene(const std::string& name) {
        auto it = m_Scenes.find(name);
        if (it != m_Scenes.end()) {
            return it->second;
        }
        return nullptr;
    }

    bool SceneManager::HasScene(const std::string& name) const {
        return m_Scenes.find(name) != m_Scenes.end();
    }

    void SceneManager::TransitionTo(const std::string& name, bool unloadCurrent) {
        if (unloadCurrent && !m_ActiveSceneName.empty()) {
            RemoveScene(m_ActiveSceneName);
        }
        
        SetActiveScene(name);
    }

    std::vector<std::string> SceneManager::GetSceneNames() const {
        std::vector<std::string> names;
        names.reserve(m_Scenes.size());
        
        for (const auto& [name, scene] : m_Scenes) {
            names.push_back(name);
        }
        
        return names;
    }
}