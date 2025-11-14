#include "ScriptEngine.hpp"
#include "../Core/Logger.hpp"
#include "../Platform/FileSystem.hpp"
#include "../Scene/Components/TransformComponent.hpp"

namespace GameEngine {

    lua_State* ScriptEngine::s_LuaState = nullptr;

    void ScriptEngine::Init() {
        s_LuaState = luaL_newstate();
        luaL_openlibs(s_LuaState);
        
        RegisterCoreFunctions();
        RegisterComponents();
        RegisterMathTypes();
        
        GE_CORE_INFO("ScriptEngine initialized");
    }

    void ScriptEngine::Shutdown() {
        if (s_LuaState) {
            lua_close(s_LuaState);
            s_LuaState = nullptr;
        }
        
        GE_CORE_INFO("ScriptEngine shutdown");
    }

    bool ScriptEngine::LoadScript(const std::string& filepath) {
        if (!FileSystem::Exists(filepath)) {
            GE_CORE_ERROR("Script file not found: {0}", filepath);
            return false;
        }
        
        std::string code = FileSystem::ReadFileAsString(filepath);
        return ExecuteString(code);
    }

    bool ScriptEngine::ExecuteString(const std::string& code) {
        int result = luaL_dostring(s_LuaState, code.c_str());
        
        if (result != LUA_OK) {
            const char* error = lua_tostring(s_LuaState, -1);
            GE_CORE_ERROR("Lua error: {0}", error);
            lua_pop(s_LuaState, 1);
            return false;
        }
        
        return true;
    }

    bool ScriptEngine::CallFunction(const std::string& functionName) {
        lua_getglobal(s_LuaState, functionName.c_str());
        
        if (!lua_isfunction(s_LuaState, -1)) {
            GE_CORE_ERROR("Lua function not found: {0}", functionName);
            lua_pop(s_LuaState, 1);
            return false;
        }
        
        int result = lua_pcall(s_LuaState, 0, 0, 0);
        
        if (result != LUA_OK) {
            const char* error = lua_tostring(s_LuaState, -1);
            GE_CORE_ERROR("Lua error calling {0}: {1}", functionName, error);
            lua_pop(s_LuaState, 1);
            return false;
        }
        
        return true;
    }

    void ScriptEngine::RegisterEntity(Entity entity) {
        // Push entity userdata to Lua
        // TODO: Implement full entity binding
    }

    void ScriptEngine::RegisterCoreFunctions() {
        // Register logging
        lua_register(s_LuaState, "Log", [](lua_State* L) -> int {
            const char* msg = luaL_checkstring(L, 1);
            GE_INFO("{0}", msg);
            return 0;
        });
        
        lua_register(s_LuaState, "LogWarning", [](lua_State* L) -> int {
            const char* msg = luaL_checkstring(L, 1);
            GE_WARN("{0}", msg);
            return 0;
        });
        
        lua_register(s_LuaState, "LogError", [](lua_State* L) -> int {
            const char* msg = luaL_checkstring(L, 1);
            GE_ERROR("{0}", msg);
            return 0;
        });
    }

    void ScriptEngine::RegisterComponents() {
        // TODO: Register component types and functions
        // Example: Transform component access
    }

    void ScriptEngine::RegisterMathTypes() {
        // TODO: Register glm::vec3, glm::quat, etc.
    }
}