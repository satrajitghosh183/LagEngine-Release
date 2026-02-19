#pragma once

#include "Component.hpp"
#include "../../Core/Base.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace GameEngine {

    /**
     * @brief Script variable for Inspector exposure
     */
    struct ScriptVariable {
        enum class Type {
            Float,
            Int,
            Bool,
            String,
            Vec3
        };
        
        Type type;
        std::string name;
        
        // Value storage (union-like)
        float floatValue = 0.0f;
        int intValue = 0;
        bool boolValue = false;
        std::string stringValue;
        float vec3Value[3] = {0.0f, 0.0f, 0.0f};
    };

    /**
     * @brief Script component - attaches Lua scripts to entities
     * 
     * Features:
     * - Script file path
     * - Exposed public variables for Inspector
     * - OnStart, OnUpdate, OnDestroy lifecycle callbacks
     * - Hot-reloading support
     * - Error handling
     */
    class ScriptComponent : public Component {
    public:
        ScriptComponent() = default;
        ScriptComponent(const std::string& scriptPath);
        ~ScriptComponent() override = default;
        
        COMPONENT_TYPE(ScriptComponent)
        
        /**
         * @brief Serialization
         */
        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& data) override;
        
        /**
         * @brief Lifecycle callbacks
         */
        void OnCreate() override;
        void OnUpdate(float deltaTime) override;
        void OnDestroy() override;
        
        /**
         * @brief Script path
         */
        void SetScriptPath(const std::string& path);
        const std::string& GetScriptPath() const { return m_ScriptPath; }
        
        /**
         * @brief Load/reload the script
         */
        bool LoadScript();
        bool ReloadScript();
        
        /**
         * @brief Check if script is loaded
         */
        bool IsLoaded() const { return m_IsLoaded; }
        
        /**
         * @brief Get last error message
         */
        const std::string& GetLastError() const { return m_LastError; }
        
        /**
         * @brief Exposed variables for Inspector
         */
        const std::vector<ScriptVariable>& GetVariables() const { return m_Variables; }
        std::vector<ScriptVariable>& GetVariables() { return m_Variables; }
        
        /**
         * @brief Set variable value (pushes to Lua)
         */
        void SetVariableFloat(const std::string& name, float value);
        void SetVariableInt(const std::string& name, int value);
        void SetVariableBool(const std::string& name, bool value);
        void SetVariableString(const std::string& name, const std::string& value);
        void SetVariableVec3(const std::string& name, float x, float y, float z);
        
        /**
         * @brief Call a function in the script
         */
        void CallFunction(const std::string& functionName);
        
    private:
        void ParseExposedVariables();
        void PushVariablesToLua();
        void OnScriptError(const std::string& error, const std::string& file, int line);
        
    private:
        std::string m_ScriptPath;
        bool m_IsLoaded = false;
        bool m_HasStarted = false;
        std::string m_LastError;
        
        // Exposed variables from the script
        std::vector<ScriptVariable> m_Variables;
    };

}
