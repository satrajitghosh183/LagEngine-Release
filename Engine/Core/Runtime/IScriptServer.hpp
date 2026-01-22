#pragma once

#include "../Base.hpp"
#include <string>
#include <functional>
#include <vector>

namespace GameEngine {

    // Forward declarations
    class Entity;

    /**
     * @brief Script server interface
     * 
     * Provides Lua script execution without exposing implementation details
     */
    class IScriptServer {
    public:
        using ErrorCallback = std::function<void(const std::string& error, const std::string& file, int line)>;

        virtual ~IScriptServer() = default;

        /**
         * @brief Initialize script server
         */
        virtual void Initialize() = 0;

        /**
         * @brief Shutdown script server
         */
        virtual void Shutdown() = 0;

        /**
         * @brief Load script from file
         */
        virtual bool LoadScript(const std::string& filepath, Entity entity) = 0;

        /**
         * @brief Execute script string
         */
        virtual bool Execute(const std::string& code) = 0;

        /**
         * @brief Call function in script
         */
        virtual bool CallFunction(const std::string& functionName, const std::vector<std::string>& args = {}) = 0;

        /**
         * @brief Register error callback
         */
        virtual void RegisterErrorCallback(ErrorCallback callback) = 0;

        /**
         * @brief Reload script for entity
         */
        virtual bool ReloadScript(Entity entity) = 0;

        /**
         * @brief Call script lifecycle function
         */
        virtual void CallLifecycleFunction(Entity entity, const std::string& functionName, float deltaTime = 0.0f) = 0;
    };

}

