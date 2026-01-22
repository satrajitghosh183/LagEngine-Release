#pragma once

#include "../Core/Runtime/IScriptServer.hpp"
#include "ScriptEngine.hpp"
#include "../Scene/Entity.hpp"
#include <unordered_map>
#include <string>

namespace GameEngine {

    /**
     * @brief Lua script server implementation
     * 
     * Wraps ScriptEngine behind IScriptServer interface
     */
    class LuaScriptServer : public IScriptServer {
    public:
        LuaScriptServer();
        ~LuaScriptServer() override = default;

        void Initialize() override;
        void Shutdown() override;

        bool LoadScript(const std::string& filepath, Entity entity) override;
        bool Execute(const std::string& code) override;
        bool CallFunction(const std::string& functionName, const std::vector<std::string>& args = {}) override;

        void RegisterErrorCallback(ErrorCallback callback) override;

        bool ReloadScript(Entity entity) override;
        void CallLifecycleFunction(Entity entity, const std::string& functionName, float deltaTime = 0.0f) override;

    private:
        ErrorCallback m_ErrorCallback;
        std::unordered_map<UUID, std::string> m_EntityScripts; // Entity UUID -> script filepath
    };

}

