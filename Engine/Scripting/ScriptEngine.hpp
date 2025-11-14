#pragma once

#include "../Core/Base.hpp"
#include "../Scene/Entity.hpp"
#include "../../External/lua/src/lua.hpp"
#include <string>

namespace GameEngine {

    /**
     * @brief Lua scripting engine
     * 
     * Features:
     * - Entity scripting
     * - Component access
     * - Hot-reloading
     * - Error handling
     */
    class ScriptEngine {
    public:
        /**
         * @brief Initialize script engine
         */
        static void Init();
        
        /**
         * @brief Shutdown script engine
         */
        static void Shutdown();
        
        /**
         * @brief Load and execute script file
         */
        static bool LoadScript(const std::string& filepath);
        
        /**
         * @brief Execute Lua code
         */
        static bool ExecuteString(const std::string& code);
        
        /**
         * @brief Call Lua function
         */
        static bool CallFunction(const std::string& functionName);
        
        /**
         * @brief Register entity with Lua
         */
        static void RegisterEntity(Entity entity);
        
        /**
         * @brief Get Lua state
         */
        static lua_State* GetLuaState() { return s_LuaState; }
        
    private:
        static void RegisterCoreFunctions();
        static void RegisterComponents();
        static void RegisterMathTypes();
        
        static lua_State* s_LuaState;
    };
}