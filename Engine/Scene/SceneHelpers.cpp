#include "SceneHelpers.hpp"
#include "../Core/Application.hpp"
#include "../Core/Logger.hpp"
#include "SceneSerializer.hpp"
#include "../Scene/Components/TransformComponent.hpp"
#include "../Scene/Components/MeshRendererComponent.hpp"
#include <filesystem>

namespace GameEngine {
namespace SceneHelpers {

    Ref<Scene> GetScene() {
        auto& app = Application::Get();
        auto& sceneManager = app.GetSceneManager();
        auto activeScene = sceneManager.GetActiveScene();
        
        // If no active scene, create a default one
        if (!activeScene) {
            activeScene = CreateRef<Scene>("Global Scene");
            sceneManager.SetActiveScene(activeScene);
        }
        
        return activeScene;
    }

    Entity LoadModel(const std::string& filepath) {
        return LoadModel(GetScene(), filepath);
    }

    Entity LoadModel(Ref<Scene> scene, const std::string& filepath) {
        MeshImportOptions options;
        return LoadModel(scene, filepath, options);
    }

    Entity LoadModel(const std::string& filepath, const MeshImportOptions& options) {
        return LoadModel(GetScene(), filepath, options);
    }

    Entity LoadModel(Ref<Scene> scene, const std::string& filepath, const MeshImportOptions& options) {
        if (!scene) {
            GE_CORE_ERROR("SceneHelpers::LoadModel: Invalid scene");
            return {};
        }

        if (!std::filesystem::exists(filepath)) {
            GE_CORE_ERROR("SceneHelpers::LoadModel: File not found: {}", filepath);
            return {};
        }

        std::filesystem::path path(filepath);
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        // Check if it's a scene file
        if (ext == ".scene" || ext == ".json") {
            // Load as scene
            Ref<Scene> loadedScene = CreateRef<Scene>();
            SceneSerializer serializer(loadedScene);
            
            if (serializer.Deserialize(filepath)) {
                // Merge loaded scene into target scene
                scene->Merge(loadedScene);
                
                // Return first entity from merged scene
                auto entities = scene->GetAllEntities();
                // Find the entity we just merged (last ones added)
                // For now, return the last entity (not perfect but works)
                if (!entities.empty()) {
                    return entities.back();
                }
            } else {
                GE_CORE_ERROR("SceneHelpers::LoadModel: Failed to load scene: {}", filepath);
                return {};
            }
        } else {
            // Load as model using AssetImporter
            auto result = AssetImporter::ImportMesh(filepath, options);
            
            if (!result.Success) {
                GE_CORE_ERROR("SceneHelpers::LoadModel: Failed to import model: {}", result.ErrorMessage);
                return {};
            }

            if (result.Meshes.empty()) {
                GE_CORE_WARN("SceneHelpers::LoadModel: No meshes found in: {}", filepath);
                return {};
            }

            // Create root entity
            std::string entityName = path.stem().string();
            Entity rootEntity = scene->CreateEntity(entityName);
            rootEntity.AddComponent<TransformComponent>();

            // If single mesh, attach directly to root
            if (result.Meshes.size() == 1) {
                auto material = (!result.Materials.empty() && result.Materials[0]) 
                    ? result.Materials[0] 
                    : CreateRef<Material>();
                
                rootEntity.AddComponent<MeshRendererComponent>(result.Meshes[0], material);
            } else {
                // Multiple meshes - create child entities
                for (size_t i = 0; i < result.Meshes.size(); ++i) {
                    Entity meshEntity = scene->CreateEntity(entityName + "_Mesh" + std::to_string(i));
                    meshEntity.AddComponent<TransformComponent>();
                    
                    auto material = (i < result.Materials.size() && result.Materials[i]) 
                        ? result.Materials[i] 
                        : CreateRef<Material>();
                    
                    meshEntity.AddComponent<MeshRendererComponent>(result.Meshes[i], material);
                    
                    // Set parent
                    // TODO: Use hierarchy system if available
                }
            }

            return rootEntity;
        }

        return {};
    }

}

}
