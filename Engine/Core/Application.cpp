#include "Application.hpp"
#include "Time.hpp"
#include "Events/WindowEvents.hpp"
#include "ServerRegistry.hpp"
#include "TelemetryServer.hpp"
#include "JobSystem.hpp"
#include "FrameScheduler.hpp"
#include "../Graphics/RenderCommand.hpp"
#include "../Graphics/OpenGLRenderServer.hpp"
#include "../Physics/PhysicsServer.hpp"
#include "../Scripting/LuaScriptServer.hpp"
#include "../Audio/AudioServer.hpp"

namespace GameEngine {

    Application* Application::s_Instance = nullptr;

    Application::Application(const RuntimeConfig& config)
        : m_Config(config)
        , m_Running(true)
        , m_Minimized(false)
        , m_FixedTimestepAccumulator(0.0f)
        , m_Name(config.title) {
        
        GE_CORE_ASSERT(!s_Instance, "Application already exists!");
        s_Instance = this;
        
        Init();
    }

    Application::Application(const std::string& name)
        : m_Config(1280, 720, name)
        , m_Running(true)
        , m_Minimized(false)
        , m_FixedTimestepAccumulator(0.0f)
        , m_Name(name) {
        
        GE_CORE_ASSERT(!s_Instance, "Application already exists!");
        s_Instance = this;
        
        Init();
    }

    Application::~Application() {
        Shutdown();
    }

    void Application::Init() {
        // Initialize logging first
        Logger::Init("logs/" + m_Name + ".log", LogLevel::Trace);
        GE_CORE_INFO("=================================");
        GE_CORE_INFO("  {0} Starting...", m_Name);
        GE_CORE_INFO("=================================");
        
        // Initialize time system
        Time::Init();
        
        // Create window
        WindowProps props;
        props.Title = m_Config.title;
        props.Width = m_Config.width;
        props.Height = m_Config.height;
        props.VSync = m_Config.vsync;
        
        if (!m_Config.headless) {
            m_Window = Window::Create(props);
            GE_CORE_INFO("Window created: {0}x{1}", props.Width, props.Height);
        } else {
            GE_CORE_INFO("Running in headless mode");
        }
        
        // Set window callbacks (only if not headless)
        if (m_Window) {
            m_Window->SetEventCallback([this](Event& e) {
                if (e.GetEventType() == EventType::WindowClose) {
                    m_Running = false;
                }
                else if (e.GetEventType() == EventType::WindowResize) {
                    WindowResizeEvent& resizeEvent = static_cast<WindowResizeEvent&>(e);
                    if (resizeEvent.GetWidth() == 0 || resizeEvent.GetHeight() == 0) {
                        m_Minimized = true;
                        return;
                    }
                    m_Minimized = false;
                    RenderCommand::SetViewport(0, 0, resizeEvent.GetWidth(), resizeEvent.GetHeight());
                }
            });
        }
        
        // Initialize renderer (only if not headless)
        if (!m_Config.headless) {
            RenderCommand::Init();
            Renderer3D::Init();
            GE_CORE_INFO("Renderer initialized");
        }
        
        // Register servers
        if (!m_Config.headless) {
            auto renderServer = CreateRef<OpenGLRenderServer>();
            renderServer->Initialize();
            ServerRegistry::RegisterRenderServer(renderServer);
        }

        auto physicsServer = CreateRef<PhysicsServer>();
        physicsServer->Initialize();
        ServerRegistry::RegisterPhysicsServer(physicsServer);

        auto audioServer = CreateRef<AudioServer>();
        audioServer->Initialize();
        ServerRegistry::RegisterAudioServer(audioServer);

        auto scriptServer = CreateRef<LuaScriptServer>();
        scriptServer->Initialize();
        ServerRegistry::RegisterScriptServer(scriptServer);

        auto telemetryServer = CreateRef<TelemetryServer>();
        telemetryServer->Initialize();
        ServerRegistry::RegisterTelemetryServer(telemetryServer);

        // Initialize job system
        if (m_Config.enableDebug) {
            JobSystem::Initialize(1); // Single-threaded for debugging
        } else {
            JobSystem::Initialize(); // Use all available cores
        }

        // Create frame scheduler
        m_FrameScheduler = CreateScope<FrameScheduler>();
        if (m_Config.enableDebug) {
            m_FrameScheduler->SetUseJobs(false); // Disable jobs for debugging
        }
        
        // Create scene manager
        m_SceneManager = CreateScope<SceneManager>();
        GE_CORE_INFO("Scene manager created");
        
        // Client initialization
        OnInit();
        
        GE_CORE_INFO("Application initialized successfully");
    }

    void Application::Run() {
        GE_CORE_INFO("Starting main loop...");
        
        while (m_Running) {
            Time::Update();
            float deltaTime = Time::GetDeltaTime();
            float unscaledDeltaTime = Time::GetUnscaledDeltaTime();
            
            // Fixed timestep physics updates
            m_FixedTimestepAccumulator += unscaledDeltaTime;
            float fixedTimestep = Time::GetFixedTimestep();
            
            int iterations = 0;
            const int maxIterations = 5; // Prevent spiral of death
            
            while (m_FixedTimestepAccumulator >= fixedTimestep && iterations < maxIterations) {
                // Use frame scheduler for fixed update
                if (m_FrameScheduler) {
                    m_FrameScheduler->ScheduleFrame(deltaTime, fixedTimestep, m_SceneManager.get());
                    m_FrameScheduler->WaitForFrame();
                } else {
                    FixedUpdate(fixedTimestep);
                }
                m_FixedTimestepAccumulator -= fixedTimestep;
                iterations++;
            }
            
            // Cap accumulator to prevent catch-up
            if (m_FixedTimestepAccumulator > fixedTimestep * maxIterations) {
                m_FixedTimestepAccumulator = 0.0f;
            }
            
            // Variable timestep updates
            if (!m_Minimized && !m_Config.headless) {
                if (m_FrameScheduler && m_FrameScheduler->GetUseJobs()) {
                    // Schedule frame with jobs
                    m_FrameScheduler->ScheduleFrame(deltaTime, fixedTimestep, m_SceneManager.get());
                    m_FrameScheduler->WaitForFrame();
                    Render();
                } else {
                    // Single-threaded mode
                    Update(deltaTime);
                    Render();
                }
            } else if (m_Config.headless) {
                if (m_FrameScheduler && m_FrameScheduler->GetUseJobs()) {
                    m_FrameScheduler->ScheduleFrame(deltaTime, fixedTimestep, m_SceneManager.get());
                    m_FrameScheduler->WaitForFrame();
                } else {
                    Update(deltaTime);
                }
            }
            
            // Process window events (only if not headless)
            if (!m_Config.headless) {
                ProcessEvents();
            }
        }
        
        GE_CORE_INFO("Main loop ended");
    }

    void Application::Close() {
        m_Running = false;
    }

    void Application::ProcessEvents() {
        if (m_Window) {
            m_Window->OnUpdate();
        }
    }

    void Application::FixedUpdate(float fixedDeltaTime) {
        // Update physics with fixed timestep
        if (m_SceneManager && m_SceneManager->GetActiveScene()) {
            m_SceneManager->GetActiveScene()->FixedUpdate(fixedDeltaTime);
        }
    }

    void Application::Update(float deltaTime) {
        // Update scene
        if (m_SceneManager && m_SceneManager->GetActiveScene()) {
            m_SceneManager->GetActiveScene()->Update(deltaTime);
        }
        
        // Client update
        OnUpdate(deltaTime);
    }

    void Application::Render() {
        if (m_Config.headless || !m_Window) {
            return;
        }
        
        // Clear screen
        RenderCommand::SetClearColor({0.1f, 0.1f, 0.15f, 1.0f});
        RenderCommand::Clear();
        
        // Render scene
        if (m_SceneManager && m_SceneManager->GetActiveScene()) {
            m_SceneManager->GetActiveScene()->Render();
        }
        
        // Client render
        OnRender();
        
        // Swap buffers
        m_Window->SwapBuffers();
    }

    void Application::Shutdown() {
        GE_CORE_INFO("Shutting down application...");
        
        // Client shutdown
        OnShutdown();
        
        // Shutdown subsystems in reverse order
        m_FrameScheduler.reset();
        m_SceneManager.reset();
        
        // Shutdown job system
        JobSystem::Shutdown();
        
        // Shutdown servers
        ServerRegistry::Clear();
        
        if (!m_Config.headless) {
            Renderer3D::Shutdown();
        }
        m_Window.reset();
        
        Logger::Shutdown();
    }
}