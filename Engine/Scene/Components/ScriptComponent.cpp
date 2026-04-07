#include "ScriptComponent.hpp"
#include "../Scene.hpp"
#include "../../Core/Logger.hpp"
#include "../../Core/ServerRegistry.hpp"
#include "../../Scripting/ScriptEngine.hpp"
#include <fstream>
#include <regex>

namespace GameEngine {

    ScriptComponent::ScriptComponent(const std::string& scriptPath)
        : m_ScriptPath(scriptPath) {
    }

    nlohmann::json ScriptComponent::Serialize() const {
        nlohmann::json data;
        data["scriptPath"] = m_ScriptPath;
        
        // Serialize exposed variables
        nlohmann::json varsArray = nlohmann::json::array();
        for (const auto& var : m_Variables) {
            nlohmann::json varData;
            varData["name"] = var.name;
            varData["type"] = static_cast<int>(var.type);
            
            switch (var.type) {
                case ScriptVariable::Type::Float:
                    varData["value"] = var.floatValue;
                    break;
                case ScriptVariable::Type::Int:
                    varData["value"] = var.intValue;
                    break;
                case ScriptVariable::Type::Bool:
                    varData["value"] = var.boolValue;
                    break;
                case ScriptVariable::Type::String:
                    varData["value"] = var.stringValue;
                    break;
                case ScriptVariable::Type::Vec3:
                    varData["value"] = {var.vec3Value[0], var.vec3Value[1], var.vec3Value[2]};
                    break;
            }
            
            varsArray.push_back(varData);
        }
        data["variables"] = varsArray;
        
        return data;
    }

    void ScriptComponent::Deserialize(const nlohmann::json& data) {
        if (data.contains("scriptPath")) {
            m_ScriptPath = data["scriptPath"].get<std::string>();
        }
        
        if (data.contains("variables")) {
            m_Variables.clear();
            for (const auto& varData : data["variables"]) {
                ScriptVariable var;
                var.name = varData["name"].get<std::string>();
                var.type = static_cast<ScriptVariable::Type>(varData["type"].get<int>());
                
                switch (var.type) {
                    case ScriptVariable::Type::Float:
                        var.floatValue = varData["value"].get<float>();
                        break;
                    case ScriptVariable::Type::Int:
                        var.intValue = varData["value"].get<int>();
                        break;
                    case ScriptVariable::Type::Bool:
                        var.boolValue = varData["value"].get<bool>();
                        break;
                    case ScriptVariable::Type::String:
                        var.stringValue = varData["value"].get<std::string>();
                        break;
                    case ScriptVariable::Type::Vec3:
                        var.vec3Value[0] = varData["value"][0].get<float>();
                        var.vec3Value[1] = varData["value"][1].get<float>();
                        var.vec3Value[2] = varData["value"][2].get<float>();
                        break;
                }
                
                m_Variables.push_back(var);
            }
        }
    }

    void ScriptComponent::OnCreate() {
        if (!m_ScriptPath.empty()) {
            LoadScript();
        }
    }

    void ScriptComponent::OnUpdate(float deltaTime) {
        if (!m_IsLoaded || !Enabled) return;
        
        // Call OnStart once if not yet started
        if (!m_HasStarted) {
            CallFunction("OnStart");
            m_HasStarted = true;
        }
        
        // Call OnUpdate every frame
        // TODO: Pass deltaTime to Lua function
        CallFunction("OnUpdate");
    }

    void ScriptComponent::OnDestroy() {
        if (m_IsLoaded) {
            CallFunction("OnDestroy");
        }
    }

    void ScriptComponent::SetScriptPath(const std::string& path) {
        if (m_ScriptPath != path) {
            m_ScriptPath = path;
            m_IsLoaded = false;
            m_HasStarted = false;
            m_Variables.clear();
        }
    }

    bool ScriptComponent::LoadScript() {
        if (m_ScriptPath.empty()) {
            m_LastError = "No script path specified";
            return false;
        }
        
        // Get script server
        if (!ServerRegistry::HasScriptServer()) {
            m_LastError = "Script server not available";
            GE_CORE_ERROR("ScriptComponent: Script server not available");
            return false;
        }
        auto& scriptServer = ServerRegistry::GetScriptServer();
        
        // Register error callback
        scriptServer.RegisterErrorCallback([this](const std::string& error, const std::string& file, int line) {
            OnScriptError(error, file, line);
        });
        
        Entity owner = GetOwnerEntity();
        if (!owner.IsValid()) {
            m_LastError = "No owner entity";
            return false;
        }
        
        m_IsLoaded = scriptServer.LoadScript(m_ScriptPath, owner);
        
        if (m_IsLoaded) {
            ParseExposedVariables();
            PushVariablesToLua();
            GE_CORE_INFO("Script loaded: {0}", m_ScriptPath);
        } else {
            GE_CORE_ERROR("Failed to load script: {0}", m_ScriptPath);
        }
        
        return m_IsLoaded;
    }

    bool ScriptComponent::ReloadScript() {
        m_IsLoaded = false;
        m_HasStarted = false;
        return LoadScript();
    }

    void ScriptComponent::SetVariableFloat(const std::string& name, float value) {
        for (auto& var : m_Variables) {
            if (var.name == name && var.type == ScriptVariable::Type::Float) {
                var.floatValue = value;
                // TODO: Push to Lua
                return;
            }
        }
    }

    void ScriptComponent::SetVariableInt(const std::string& name, int value) {
        for (auto& var : m_Variables) {
            if (var.name == name && var.type == ScriptVariable::Type::Int) {
                var.intValue = value;
                // TODO: Push to Lua
                return;
            }
        }
    }

    void ScriptComponent::SetVariableBool(const std::string& name, bool value) {
        for (auto& var : m_Variables) {
            if (var.name == name && var.type == ScriptVariable::Type::Bool) {
                var.boolValue = value;
                // TODO: Push to Lua
                return;
            }
        }
    }

    void ScriptComponent::SetVariableString(const std::string& name, const std::string& value) {
        for (auto& var : m_Variables) {
            if (var.name == name && var.type == ScriptVariable::Type::String) {
                var.stringValue = value;
                // TODO: Push to Lua
                return;
            }
        }
    }

    void ScriptComponent::SetVariableVec3(const std::string& name, float x, float y, float z) {
        for (auto& var : m_Variables) {
            if (var.name == name && var.type == ScriptVariable::Type::Vec3) {
                var.vec3Value[0] = x;
                var.vec3Value[1] = y;
                var.vec3Value[2] = z;
                // TODO: Push to Lua
                return;
            }
        }
    }

    void ScriptComponent::CallFunction(const std::string& functionName) {
        Entity owner = GetOwnerEntity();
        if (!m_IsLoaded || !owner.IsValid()) return;

        if (!ServerRegistry::HasScriptServer()) {
            return;
        }

        auto& scriptServer = ServerRegistry::GetScriptServer();
        scriptServer.CallLifecycleFunction(owner, functionName);
    }

    void ScriptComponent::ParseExposedVariables() {
        // Parse script file for exposed variables
        // Look for comments like: --@expose float Speed = 10.0
        
        std::ifstream file(m_ScriptPath);
        if (!file.is_open()) return;
        
        std::string line;
        std::regex exposeRegex(R"(--\s*@expose\s+(\w+)\s+(\w+)\s*=?\s*(.*))");
        
        while (std::getline(file, line)) {
            std::smatch match;
            if (std::regex_search(line, match, exposeRegex)) {
                ScriptVariable var;
                std::string typeStr = match[1].str();
                var.name = match[2].str();
                std::string defaultValue = match[3].str();
                
                // Determine type
                if (typeStr == "float" || typeStr == "number") {
                    var.type = ScriptVariable::Type::Float;
                    if (!defaultValue.empty()) {
                        try {
                            var.floatValue = std::stof(defaultValue);
                        } catch (...) {}
                    }
                } else if (typeStr == "int" || typeStr == "integer") {
                    var.type = ScriptVariable::Type::Int;
                    if (!defaultValue.empty()) {
                        try {
                            var.intValue = std::stoi(defaultValue);
                        } catch (...) {}
                    }
                } else if (typeStr == "bool" || typeStr == "boolean") {
                    var.type = ScriptVariable::Type::Bool;
                    var.boolValue = (defaultValue == "true");
                } else if (typeStr == "string") {
                    var.type = ScriptVariable::Type::String;
                    var.stringValue = defaultValue;
                } else if (typeStr == "vec3" || typeStr == "Vector3") {
                    var.type = ScriptVariable::Type::Vec3;
                    // TODO: Parse vec3 default value
                }
                
                m_Variables.push_back(var);
            }
        }
    }

    void ScriptComponent::PushVariablesToLua() {
        // TODO: Push all variable values to Lua global table
        // This requires ScriptEngine support for setting globals
    }

    void ScriptComponent::OnScriptError(const std::string& error, const std::string& file, int line) {
        m_LastError = error;
        GE_CORE_ERROR("Script error in {0}:{1}: {2}", file, line, error);
    }

}
