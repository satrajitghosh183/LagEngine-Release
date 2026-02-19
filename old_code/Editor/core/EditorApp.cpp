#include "EditorApp.hpp"
#include "EditorTheme.hpp"
#include "SceneRenderer.hpp"
#include "../panels/EditorPanel.hpp"

#include <iostream>
#include <chrono>

namespace editor {

EditorApp& EditorApp::getInstance() {
    static EditorApp instance;
    return instance;
}

EditorApp::~EditorApp() {
    shutdown();
}

bool EditorApp::initialize(int width, int height, const std::string& title) {
    m_windowWidth = width;
    m_windowHeight = height;
    m_windowTitle = title;
    
    if (!initializeGLFW()) {
        std::cerr << "[Editor] Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    if (!initializeImGui()) {
        std::cerr << "[Editor] Failed to initialize ImGui" << std::endl;
        return false;
    }
    
    // Initialize theme
    m_theme = std::make_unique<EditorTheme>();
    m_theme->applyDarkTheme();
    
    // Initialize scene renderer
    m_sceneRenderer = std::make_unique<SceneRenderer>();
    if (!m_sceneRenderer->initialize()) {
        std::cerr << "[Editor] Failed to initialize SceneRenderer" << std::endl;
        return false;
    }
    
    m_isRunning = true;
    std::cout << "[Editor] Initialized successfully" << std::endl;
    return true;
}

bool EditorApp::initializeGLFW() {
    if (!glfwInit()) {
        std::cerr << "[Editor] Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    // OpenGL 3.3 Core Profile (more compatible)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4); // MSAA
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    
    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, m_windowTitle.c_str(), nullptr, nullptr);
    if (!m_window) {
        std::cerr << "[Editor] Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // VSync
    
    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "[Editor] Failed to initialize GLAD" << std::endl;
        return false;
    }
    
    // Update window size
    glfwGetWindowSize(m_window, &m_windowWidth, &m_windowHeight);
    
    // Window callbacks
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int width, int height) {
        auto* app = static_cast<EditorApp*>(glfwGetWindowUserPointer(window));
        app->m_windowWidth = width;
        app->m_windowHeight = height;
        glViewport(0, 0, width, height);
    });
    
    std::cout << "[Editor] OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "[Editor] GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    
    return true;
}

bool EditorApp::initializeImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    // Font configuration
    io.FontGlobalScale = 1.0f;
    io.Fonts->AddFontDefault();
    
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    
    return true;
}

void EditorApp::run() {
    while (m_isRunning && !glfwWindowShouldClose(m_window)) {
        // Calculate delta time
        float currentTime = static_cast<float>(glfwGetTime());
        m_deltaTime = currentTime - m_lastFrameTime;
        m_lastFrameTime = currentTime;
        m_time = currentTime;
        
        // Poll events
        glfwPollEvents();
        processInput();
        
        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // Render menu bar
        renderMenuBar();
        
        // Render toolbar
        renderToolbar();
        
        // Render all panels
        renderPanels();
        
        // Render status bar
        renderStatusBar();
        
        // Demo windows for debugging
        if (m_showDemoWindow) {
            ImGui::ShowDemoWindow(&m_showDemoWindow);
        }
        if (m_showMetricsWindow) {
            ImGui::ShowMetricsWindow(&m_showMetricsWindow);
        }
        
        // Update panels
        for (auto& panel : m_panels) {
            panel->update(m_deltaTime);
        }
        
        // Rendering
        ImGui::Render();
        int displayW, displayH;
        glfwGetFramebufferSize(m_window, &displayW, &displayH);
        glViewport(0, 0, displayW, displayH);
        glClearColor(0.06f, 0.06f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(m_window);
    }
}

void EditorApp::setupDockspace() {
    // Docking requires ImGui docking branch - simplified version without docking
}

void EditorApp::renderMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Scene", "Ctrl+N")) {}
            if (ImGui::MenuItem("Open Scene...", "Ctrl+O")) {}
            ImGui::Separator();
            if (ImGui::MenuItem("Save", "Ctrl+S")) {}
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {}
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                m_isRunning = false;
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) {}
            if (ImGui::MenuItem("Redo", "Ctrl+Y")) {}
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "Ctrl+X")) {}
            if (ImGui::MenuItem("Copy", "Ctrl+C")) {}
            if (ImGui::MenuItem("Paste", "Ctrl+V")) {}
            ImGui::Separator();
            if (ImGui::MenuItem("Preferences...")) {}
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            for (auto& panel : m_panels) {
                bool visible = panel->isVisible();
                if (ImGui::MenuItem(panel->getName().c_str(), nullptr, &visible)) {
                    panel->setVisible(visible);
                }
            }
            ImGui::Separator();
            ImGui::MenuItem("Show Demo Window", nullptr, &m_showDemoWindow);
            ImGui::MenuItem("Show Metrics", nullptr, &m_showMetricsWindow);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Create")) {
            if (m_sceneRenderer) {
                if (ImGui::MenuItem("Cube")) {
                    int idx = m_sceneRenderer->addObject("Cube", "Cube");
                    SceneObject* obj = m_sceneRenderer->getObject(idx);
                    if (obj) {
                        obj->transform.position.y = 0.5f;
                        obj->color = glm::vec3(0.8f, 0.4f, 0.3f);
                    }
                }
                if (ImGui::MenuItem("Sphere")) {
                    int idx = m_sceneRenderer->addObject("Sphere", "Sphere");
                    SceneObject* obj = m_sceneRenderer->getObject(idx);
                    if (obj) {
                        obj->transform.position.y = 0.5f;
                        obj->color = glm::vec3(0.3f, 0.7f, 0.4f);
                    }
                }
                if (ImGui::MenuItem("Plane")) {
                    int idx = m_sceneRenderer->addObject("Plane", "Plane");
                    SceneObject* obj = m_sceneRenderer->getObject(idx);
                    if (obj) {
                        obj->transform.scale = glm::vec3(5.0f, 1.0f, 5.0f);
                        obj->color = glm::vec3(0.5f, 0.5f, 0.6f);
                    }
                }
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Physics")) {
            if (ImGui::MenuItem("Reset Simulation")) {}
            if (ImGui::MenuItem("Step Forward", "F6")) { step(); }
            ImGui::Separator();
            if (ImGui::MenuItem("Physics Settings...")) {}
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Documentation")) {}
            if (ImGui::MenuItem("About VerletX Engine")) {}
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
}

void EditorApp::renderToolbar() {
    ImGuiWindowFlags toolbar_flags = ImGuiWindowFlags_NoScrollbar | 
                                     ImGuiWindowFlags_NoSavedSettings |
                                     ImGuiWindowFlags_NoNav;
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
    
    float toolbarHeight = 40.0f;
    ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetFrameHeight()));
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(m_windowWidth), toolbarHeight));
    
    if (ImGui::Begin("##Toolbar", nullptr, toolbar_flags | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        // Play/Pause/Stop buttons
        ImGui::PushStyleColor(ImGuiCol_Button, m_isPlaying && !m_isPaused ? ImVec4(0.2f, 0.5f, 0.2f, 1.0f) : ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
        if (ImGui::Button(m_isPlaying && !m_isPaused ? "Pause##Play" : "Play##Play", ImVec2(50, 28))) {
            if (!m_isPlaying) play();
            else if (m_isPaused) pause(); // Resume
        }
        ImGui::PopStyleColor();
        
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, m_isPaused ? ImVec4(0.5f, 0.5f, 0.2f, 1.0f) : ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
        if (ImGui::Button("Pause##Pause", ImVec2(50, 28))) {
            if (m_isPlaying) pause();
        }
        ImGui::PopStyleColor();
        
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
        if (ImGui::Button("Stop##Stop", ImVec2(50, 28))) {
            stop();
        }
        ImGui::PopStyleColor();
        
        ImGui::SameLine();
        if (ImGui::Button("Step##Step", ImVec2(50, 28))) {
            step();
        }
        
        ImGui::SameLine();
        ImGui::Separator();
        
        // Transform mode buttons
        ImGui::SameLine();
        static int transformMode = 0;
        ImGui::PushStyleColor(ImGuiCol_Button, transformMode == 0 ? ImVec4(0.3f, 0.3f, 0.5f, 1.0f) : ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
        if (ImGui::Button("T##Translate", ImVec2(28, 28))) transformMode = 0;
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Translate (W)");
        
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, transformMode == 1 ? ImVec4(0.3f, 0.3f, 0.5f, 1.0f) : ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
        if (ImGui::Button("R##Rotate", ImVec2(28, 28))) transformMode = 1;
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Rotate (E)");
        
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, transformMode == 2 ? ImVec4(0.3f, 0.3f, 0.5f, 1.0f) : ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
        if (ImGui::Button("S##Scale", ImVec2(28, 28))) transformMode = 2;
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Scale (R)");
        
        ImGui::SameLine();
        ImGui::Separator();
        
        // Gizmo space toggle
        ImGui::SameLine();
        static bool localSpace = true;
        if (ImGui::Button(localSpace ? "Local" : "World", ImVec2(50, 28))) {
            localSpace = !localSpace;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle Local/World Space");
        
        // FPS counter on the right
        ImGui::SameLine(ImGui::GetWindowWidth() - 150);
        ImGui::Text("FPS: %.1f (%.2f ms)", 1.0f / m_deltaTime, m_deltaTime * 1000.0f);
    }
    ImGui::End();
    
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

void EditorApp::renderStatusBar() {
    ImGuiWindowFlags statusbar_flags = ImGuiWindowFlags_NoScrollbar | 
                                       ImGuiWindowFlags_NoSavedSettings |
                                       ImGuiWindowFlags_NoNav |
                                       ImGuiWindowFlags_NoTitleBar |
                                       ImGuiWindowFlags_NoResize |
                                       ImGuiWindowFlags_NoMove;
    
    float statusbarHeight = 24.0f;
    ImGui::SetNextWindowPos(ImVec2(0, static_cast<float>(m_windowHeight) - statusbarHeight));
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(m_windowWidth), statusbarHeight));
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 4));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.12f, 1.0f));
    
    if (ImGui::Begin("##StatusBar", nullptr, statusbar_flags)) {
        if (m_sceneRenderer) {
            SceneObject* selected = m_sceneRenderer->getSelectedObject();
            if (selected) {
                ImGui::Text("Selected: %s (%s)", selected->name.c_str(), selected->type.c_str());
            } else {
                ImGui::Text("Ready - %d objects in scene", static_cast<int>(m_sceneRenderer->getObjects().size()));
            }
        } else {
            ImGui::Text("Ready");
        }
        
        ImGui::SameLine(ImGui::GetWindowWidth() - 300);
        ImGui::Text("VerletX Engine Editor v1.0");
    }
    ImGui::End();
    
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void EditorApp::renderPanels() {
    for (auto& panel : m_panels) {
        if (panel->isVisible()) {
            panel->render();
        }
    }
}

void EditorApp::processInput() {
    handleShortcuts();
}

void EditorApp::handleShortcuts() {
    ImGuiIO& io = ImGui::GetIO();
    
    // Only handle shortcuts when not typing in text input
    if (!io.WantTextInput) {
        bool ctrl = io.KeyCtrl;
        bool shift = io.KeyShift;
        
        // File shortcuts
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_N)) {
            // New scene
        }
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_O)) {
            // Open scene
        }
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
            // Save
        }
        
        // Edit shortcuts
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Z)) {
            if (shift) {
                // Redo
            } else {
                // Undo
            }
        }
        
        // Play shortcuts
        if (ImGui::IsKeyPressed(ImGuiKey_F5)) {
            if (m_isPlaying) stop();
            else play();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_F6)) {
            step();
        }
        
        // Delete selected object
        if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
            if (m_sceneRenderer) {
                int selectedIdx = m_sceneRenderer->getSelectedIndex();
                if (selectedIdx >= 0) {
                    m_sceneRenderer->removeObject(selectedIdx);
                }
            }
        }
    }
}

void EditorApp::shutdown() {
    // Clean up scene renderer first
    if (m_sceneRenderer) {
        m_sceneRenderer->shutdown();
        m_sceneRenderer.reset();
    }
    
    if (m_window) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        
        glfwDestroyWindow(m_window);
        glfwTerminate();
        m_window = nullptr;
    }
    
    std::cout << "[Editor] Shutdown complete" << std::endl;
}

void EditorApp::registerPanel(std::shared_ptr<EditorPanel> panel) {
    m_panels.push_back(panel);
}

void EditorApp::unregisterPanel(const std::string& name) {
    m_panels.erase(
        std::remove_if(m_panels.begin(), m_panels.end(),
            [&name](const std::shared_ptr<EditorPanel>& panel) {
                return panel->getName() == name;
            }),
        m_panels.end());
}

EditorPanel* EditorApp::getPanel(const std::string& name) {
    for (auto& panel : m_panels) {
        if (panel->getName() == name) {
            return panel.get();
        }
    }
    return nullptr;
}

void EditorApp::setSelectedObject(void* object, const std::string& type) {
    m_selectedObject = object;
    m_selectedType = type;
    if (onSelectionChanged) {
        onSelectionChanged(object, type);
    }
}

void EditorApp::clearSelection() {
    m_selectedObject = nullptr;
    m_selectedType.clear();
    if (onSelectionChanged) {
        onSelectionChanged(nullptr, "");
    }
}

void EditorApp::play() {
    m_isPlaying = true;
    m_isPaused = false;
    if (onPlay) onPlay();
    std::cout << "[Editor] Simulation started" << std::endl;
}

void EditorApp::pause() {
    if (m_isPlaying) {
        m_isPaused = !m_isPaused;
        if (onPause) onPause();
        std::cout << "[Editor] Simulation " << (m_isPaused ? "paused" : "resumed") << std::endl;
    }
}

void EditorApp::stop() {
    m_isPlaying = false;
    m_isPaused = false;
    if (onStop) onStop();
    std::cout << "[Editor] Simulation stopped" << std::endl;
}

void EditorApp::step() {
    if (!m_isPlaying) {
        m_isPlaying = true;
        m_isPaused = true;
    }
    // Single step logic would trigger scene update here
    std::cout << "[Editor] Single step" << std::endl;
}

} // namespace editor
