#include "LuaScriptServer.hpp"
#include "../Core/Logger.hpp"

namespace GameEngine {

    LuaScriptServer::LuaScriptServer() {
    }

    void LuaScriptServer::Initialize() {
        ScriptEngine::Init();
        GE_CORE_INFO("LuaScriptServer initialized");
    }

    void LuaScriptServer::Shutdown() {
        m_EntityScripts.clear();
        ScriptEngine::Shutdown();
        GE_CORE_INFO("LuaScriptServer shutdown");
    }

    bool LuaScriptServer::LoadScript(const std::string& filepath, Entity entity) {
        if (!entity.IsValid()) {
            if (m_ErrorCallback) {
                m_ErrorCallback("Invalid entity", filepath, 0);
            }
            return false;
        }

        bool success = ScriptEngine::LoadScript(filepath);
        if (success) {
            ScriptEngine::RegisterEntity(entity);
            m_EntityScripts[entity.GetUUID()] = filepath;
        } else {
            if (m_ErrorCallback) {
                m_ErrorCallback("Failed to load script", filepath, 0);
            }
        }
        return success;
    }

    bool LuaScriptServer::Execute(const std::string& code) {
        return ScriptEngine::ExecuteString(code);
    }

    bool LuaScriptServer::CallFunction(const std::string& functionName, const std::vector<std::string>& args) {
        // TODO: Implement argument passing
        return ScriptEngine::CallFunction(functionName);
    }

    void LuaScriptServer::RegisterErrorCallback(ErrorCallback callback) {
        m_ErrorCallback = callback;
    }

    bool LuaScriptServer::ReloadScript(Entity entity) {
        auto it = m_EntityScripts.find(entity.GetUUID());
        if (it == m_EntityScripts.end()) {
            return false;
        }
        return LoadScript(it->second, entity);
    }

    void LuaScriptServer::CallLifecycleFunction(Entity entity, const std::string& functionName, float deltaTime) {
        // TODO: Implement lifecycle function calling with deltaTime
        // This will require extending ScriptEngine to support parameter passing
        ScriptEngine::CallFunction(functionName);
    }

}

