#include "AISceneGenerator.hpp"
#include <nlohmann/json.hpp>
#include <sstream>
#include <thread>

namespace GameEngine {

    AISceneGenerator::AISceneGenerator() {
        OllamaConfig config;
        config.Model = m_Model;
        config.Temperature = 0.3f; // Lower temperature for more structured output
        config.MaxTokens = 4096;
        m_Ollama.SetConfig(config);
    }

    void AISceneGenerator::setModel(const std::string& model) {
        m_Model = model;
        OllamaConfig config = m_Ollama.GetConfig();
        config.Model = model;
        m_Ollama.SetConfig(config);
    }

    bool AISceneGenerator::isAvailable() {
        return m_Ollama.IsServerRunning();
    }

    std::string AISceneGenerator::buildScenePrompt(const SceneGenerationRequest& request) {
        std::ostringstream prompt;
        prompt << "You are an AI assistant for the LAG Engine, a 3D/2D game engine for education.\n";
        prompt << "Generate a scene as JSON based on this description: \"" << request.Description << "\"\n\n";
        prompt << "Scene type: " << request.SceneType << "\n";
        prompt << "Difficulty: " << request.Difficulty << "\n";
        prompt << "Include physics: " << (request.IncludePhysics ? "yes" : "no") << "\n";
        prompt << "Include scripting: " << (request.IncludeScripting ? "yes" : "no") << "\n";
        prompt << "Max entities: " << request.MaxEntities << "\n\n";

        prompt << "Respond ONLY with valid JSON in this exact format:\n";
        prompt << R"({
  "sceneName": "Scene Name",
  "explanation": "Brief explanation of what was created and why, for teaching purposes",
  "entities": [
    {
      "name": "EntityName",
      "meshType": "Cube|Sphere|Plane|Cylinder|Cone|Capsule",
      "position": [x, y, z],
      "rotation": [rx, ry, rz],
      "scale": [sx, sy, sz],
      "color": [r, g, b, a],
      "physicsType": "Static|Dynamic|Kinematic|",
      "mass": 1.0,
      "metallic": 0.0,
      "roughness": 0.5,
      "lightType": "Directional|Point|Spot|",
      "lightIntensity": 1.0,
      "script": "-- optional Lua script\n"
    }
  ]
})";
        return prompt.str();
    }

    SceneGenerationResult AISceneGenerator::parseSceneResponse(const std::string& response) {
        SceneGenerationResult result;

        // Find JSON in the response
        size_t jsonStart = response.find('{');
        size_t jsonEnd = response.rfind('}');
        if (jsonStart == std::string::npos || jsonEnd == std::string::npos) {
            result.ErrorMessage = "AI response did not contain valid JSON";
            return result;
        }

        std::string jsonStr = response.substr(jsonStart, jsonEnd - jsonStart + 1);

        try {
            nlohmann::json j = nlohmann::json::parse(jsonStr);

            result.SceneName = j.value("sceneName", "Generated Scene");
            result.Explanation = j.value("explanation", "");

            if (j.contains("entities") && j["entities"].is_array()) {
                for (auto& je : j["entities"]) {
                    GeneratedEntity entity;
                    entity.Name = je.value("name", "Entity");
                    entity.MeshType = je.value("meshType", "Cube");

                    if (je.contains("position") && je["position"].is_array()) {
                        for (int i = 0; i < 3 && i < (int)je["position"].size(); i++)
                            entity.Position[i] = je["position"][i].get<float>();
                    }
                    if (je.contains("rotation") && je["rotation"].is_array()) {
                        for (int i = 0; i < 3 && i < (int)je["rotation"].size(); i++)
                            entity.Rotation[i] = je["rotation"][i].get<float>();
                    }
                    if (je.contains("scale") && je["scale"].is_array()) {
                        for (int i = 0; i < 3 && i < (int)je["scale"].size(); i++)
                            entity.Scale[i] = je["scale"][i].get<float>();
                    }
                    if (je.contains("color") && je["color"].is_array()) {
                        for (int i = 0; i < 4 && i < (int)je["color"].size(); i++)
                            entity.Color[i] = je["color"][i].get<float>();
                    }

                    entity.PhysicsType = je.value("physicsType", "");
                    entity.Mass = je.value("mass", 1.0f);
                    entity.Metallic = je.value("metallic", 0.0f);
                    entity.Roughness = je.value("roughness", 0.5f);
                    entity.LightType = je.value("lightType", "");
                    entity.LightIntensity = je.value("lightIntensity", 1.0f);
                    entity.ScriptCode = je.value("script", "");

                    result.Entities.push_back(entity);
                }
            }

            result.Success = !result.Entities.empty();
            if (!result.Success) {
                result.ErrorMessage = "No entities were generated";
            }
        } catch (const nlohmann::json::exception& e) {
            result.ErrorMessage = "Failed to parse AI response: " + std::string(e.what());
        }

        return result;
    }

    SceneGenerationResult AISceneGenerator::generateScene(const SceneGenerationRequest& request) {
        if (!isAvailable()) {
            SceneGenerationResult result;
            result.ErrorMessage = "Ollama server is not running. Start it with: ollama serve";
            return result;
        }

        std::string prompt = buildScenePrompt(request);
        std::string response = m_Ollama.Generate(prompt);

        if (response.empty()) {
            SceneGenerationResult result;
            result.ErrorMessage = "Empty response from AI. " + m_Ollama.GetLastError();
            return result;
        }

        return parseSceneResponse(response);
    }

    void AISceneGenerator::generateSceneAsync(const SceneGenerationRequest& request,
                                                std::function<void(const SceneGenerationResult&)> callback) {
        std::thread([this, request, callback]() {
            auto result = generateScene(request);
            if (callback) callback(result);
        }).detach();
    }

    SceneGenerationResult AISceneGenerator::generatePhysicsDemo(const std::string& concept) {
        SceneGenerationRequest request;
        request.Description = "Create an interactive physics demonstration showing the concept of: " + concept +
            ". Include objects that students can observe and interact with. Add explanatory labels.";
        request.IncludePhysics = true;
        request.IncludeScripting = true;
        request.Difficulty = "beginner";
        return generateScene(request);
    }

    SceneGenerationResult AISceneGenerator::generateMathVisualization(const std::string& concept) {
        SceneGenerationRequest request;
        request.Description = "Create a 3D visualization of the mathematical concept: " + concept +
            ". Use colored objects to represent different parts of the equation or concept. "
            "Make it interactive where possible.";
        request.IncludePhysics = false;
        request.IncludeScripting = true;
        request.Difficulty = "intermediate";
        return generateScene(request);
    }

    std::string AISceneGenerator::explainScene(const std::string& sceneJson) {
        std::string prompt = "You are an educational AI assistant for the LAG game engine.\n"
            "Explain this scene in simple terms suitable for a student or teacher. "
            "Describe what each entity does and how they interact.\n\n"
            "Scene JSON:\n" + sceneJson;

        return m_Ollama.Generate(prompt);
    }

    std::string AISceneGenerator::suggestImprovements(const std::string& sceneJson) {
        std::string prompt = "You are an educational AI assistant for the LAG game engine.\n"
            "Look at this scene and suggest improvements to make it more educational, "
            "interactive, or visually appealing. Be specific and practical.\n\n"
            "Scene JSON:\n" + sceneJson;

        return m_Ollama.Generate(prompt);
    }

} // namespace GameEngine
