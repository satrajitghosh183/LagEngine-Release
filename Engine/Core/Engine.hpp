#pragma once

#include "Application.hpp"
#include <string>

namespace GameEngine {

    /**
     * @brief Global engine access point
     * 
     * Provides centralized access to all engine subsystems
     * Acts as a facade for common operations
     */
    class Engine {
    public:
        /**
         * @brief Get application instance
         */
        static Application& GetApp() {
            return Application::Get();
        }
        
        /**
         * @brief Get window
         */
        static Window& GetWindow() {
            return Application::Get().GetWindow();
        }
        
        /**
         * @brief Get scene manager
         */
        static SceneManager& GetSceneManager() {
            return Application::Get().GetSceneManager();
        }
        
        /**
         * @brief Get engine version string
         */
        static std::string GetVersion() {
            return "1.0.0";
        }
        
        /**
         * @brief Request engine shutdown
         */
        static void Shutdown() {
            Application::Get().Close();
        }
    };
}