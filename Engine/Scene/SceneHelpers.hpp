#pragma once

#include "Scene.hpp"
#include "SceneManager.hpp"
#include "../Assets/AssetImporter.hpp"
#include "../Core/Base.hpp"
#include <string>

namespace GameEngine {

    /**
     * @brief Scene helper functions for easy scene access and model loading
     * 
     * Provides Wicked Engine-style convenience functions:
     * - GetScene() - get current global scene
     * - LoadModel() - load model into scene
     * - Scene merging utilities
     */
    namespace SceneHelpers {

        /**
         * @brief Get the current active scene (global scene)
         */
        Ref<Scene> GetScene();

        /**
         * @brief Load a model/scene file into the global scene
         * @param filepath Path to model or scene file
         * @return Root entity of loaded model, or invalid entity if failed
         */
        Entity LoadModel(const std::string& filepath);

        /**
         * @brief Load a model/scene file into a specific scene
         * @param scene Target scene
         * @param filepath Path to model or scene file
         * @return Root entity of loaded model, or invalid entity if failed
         */
        Entity LoadModel(Ref<Scene> scene, const std::string& filepath);

        /**
         * @brief Load model with options
         */
        Entity LoadModel(const std::string& filepath, const MeshImportOptions& options);
        Entity LoadModel(Ref<Scene> scene, const std::string& filepath, const MeshImportOptions& options);

    }

}
