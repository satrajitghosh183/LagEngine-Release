#pragma once

#include "Base.hpp"
#include "Time.hpp"
#include "Logger.hpp"
#include "RuntimeConfig.hpp"
#include "../Platform/Window.hpp"
#include "../Graphics/Renderer3D.hpp"
#include "../Scene/SceneManager.hpp"
#include <memory>

namespace GameEngine {

    // Forward declaration
    class FrameScheduler;

    /**
     * @brief Core application lifecycle manager
     * 
     * Responsibilities:
     * - Initialize all subsystems in correct order
     * - Main game loop (fixed timestep for physics, variable for rendering)
     * - Event dispatching to layers
     * - Graceful shutdown
     * 
     * Usage:
     *   class MyApp : public Application {
     *       MyApp() { ... }
     *   };
     *   int main() {
     *       MyApp* app = new MyApp();
     *       app->Run();
     *       delete app;
     *   }
     */
    class Application {
    public:
        Application(const RuntimeConfig& config = RuntimeConfig());
        Application(const std::string& name); // Deprecated: use RuntimeConfig constructor
        virtual ~Application();
        
        /**
         * @brief Main application loop
         * Runs until window is closed
         */
        void Run();
        
        /**
         * @brief Request application shutdown
         */
        void Close();
        
        /**
         * @brief Get application singleton instance
         */
        static Application& Get() { return *s_Instance; }
        
        /**
         * @brief Get window instance
         */
        Window& GetWindow() { return *m_Window; }
        
        /**
         * @brief Get scene manager
         */
        SceneManager& GetSceneManager() { return *m_SceneManager; }
        
    protected:
        /**
         * @brief Override for custom initialization
         */
        virtual void OnInit() {}
        
        /**
         * @brief Override for custom update logic
         */
        virtual void OnUpdate(float deltaTime) {}
        
        /**
         * @brief Override for custom render logic
         */
        virtual void OnRender() {}
        
        /**
         * @brief Override for custom shutdown
         */
        virtual void OnShutdown() {}
        
    private:
        void Init();
        void Shutdown();
        void ProcessEvents();
        void FixedUpdate(float fixedDeltaTime);
        void Update(float deltaTime);
        void Render();
        
    private:
        static Application* s_Instance;
        
        RuntimeConfig m_Config;
        Scope<Window> m_Window;
        Scope<SceneManager> m_SceneManager;
        Scope<FrameScheduler> m_FrameScheduler;
        
        bool m_Running;
        bool m_Minimized;
        float m_FixedTimestepAccumulator;
        
        std::string m_Name;
    };
    
    /**
     * @brief To be defined in client application
     * Returns a new instance of the application
     */
    Application* CreateApplication();
}