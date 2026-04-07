#pragma once

#include "../Core/Base.hpp"
#include "OllamaClient.hpp"
#include <string>
#include <vector>
#include <functional>

namespace GameEngine {

    struct SceneGenerationRequest {
        std::string Description;         // Natural language description
        std::string SceneType = "3D";    // "2D" or "3D"
        std::string Difficulty = "beginner"; // "beginner", "intermediate", "advanced"
        bool IncludePhysics = true;
        bool IncludeScripting = true;
        bool IncludeUI = false;
        int MaxEntities = 20;
    };

    struct GeneratedEntity {
        std::string Name;
        std::string MeshType; // "Cube", "Sphere", "Plane", "Cylinder", etc.
        float Position[3] = {0, 0, 0};
        float Rotation[3] = {0, 0, 0};
        float Scale[3] = {1, 1, 1};
        float Color[4] = {1, 1, 1, 1};

        std::string PhysicsType; // "Static", "Dynamic", "Kinematic", ""
        float Mass = 1.0f;

        std::string ScriptCode; // Lua script content

        // Material
        float Metallic = 0.0f;
        float Roughness = 0.5f;

        // Light
        std::string LightType; // "Directional", "Point", "Spot", ""
        float LightIntensity = 1.0f;
    };

    struct SceneGenerationResult {
        bool Success = false;
        std::string SceneName;
        std::vector<GeneratedEntity> Entities;
        std::string LuaSetupScript;
        std::string ErrorMessage;
        std::string Explanation; // Teaching explanation of what was generated
    };

    class AISceneGenerator {
    public:
        AISceneGenerator();

        void setModel(const std::string& model);
        bool isAvailable();

        // Generate a scene from natural language
        SceneGenerationResult generateScene(const SceneGenerationRequest& request);
        void generateSceneAsync(const SceneGenerationRequest& request,
                                 std::function<void(const SceneGenerationResult&)> callback);

        // Quick templates for teachers
        SceneGenerationResult generatePhysicsDemo(const std::string& concept);
        SceneGenerationResult generateMathVisualization(const std::string& concept);

        // Explain an existing scene
        std::string explainScene(const std::string& sceneJson);

        // Suggest improvements
        std::string suggestImprovements(const std::string& sceneJson);

    private:
        std::string buildScenePrompt(const SceneGenerationRequest& request);
        SceneGenerationResult parseSceneResponse(const std::string& response);

        OllamaClient m_Ollama;
        std::string m_Model = "codellama:13b";
    };

} // namespace GameEngine
