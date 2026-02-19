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
    class RenderPath; // Forward declaration
}

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
         * @return Reference to application, or throws if not initialized
         */
        static Application& Get();
        
        /**
         * @brief Get application singleton pointer (may be null)
         */
        static Application* GetPtr() { return s_Instance; }
        
        /**
         * @brief Get window instance (valid only when not headless)
         */
        Window* GetWindowPtr() { return m_Window.get(); }
        Window& GetWindow();
        
        /**
         * @brief Get scene manager
         */
        SceneManager& GetSceneManager() { return *m_SceneManager; }

        /**
         * @brief Get runtime config
         */
        const RuntimeConfig& GetConfig() const { return m_Config; }
        
        /**
         * @brief Activate a render path (Wicked Engine style)
         * @param path Render path to activate
         * @param fadeSeconds Fade transition time (0 = instant)
         */
        void ActivatePath(Ref<RenderPath> path, float fadeSeconds = 0.0f);
        
        /**
         * @brief Get current active render path
         */
        Ref<RenderPath> GetActiveRenderPath() const { return m_ActiveRenderPath; }
        
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
        Ref<RenderPath> m_ActiveRenderPath;
        
        bool m_Running;
        bool m_Minimized;
        bool m_Initialized;
        float m_FixedTimestepAccumulator;

        std::string m_Name;
    };
    
    /**
     * @brief To be defined in client application
     * Returns a new instance of the application
     */
    Application* CreateApplication();
}