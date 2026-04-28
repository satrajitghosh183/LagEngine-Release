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

    // Configure VulkanBase with ImGui enabled
    vkdemo::AppConfig config;
    config.Title = title;
    config.Width = width;
    config.Height = height;
    config.EnableValidation = true;
    config.EnableImGui = true;
    config.MaxFramesInFlight = 2;

    try {
        m_vkBase = std::make_unique<vkdemo::VulkanBase>();
        m_vkBase->Init(config);
    } catch (const std::exception& e) {
        std::cerr << "[Editor] Failed to initialize Vulkan: " << e.what() << std::endl;
        return false;
    }

    // Track resize
    m_vkBase->OnResize = [this](int w, int h) {
        m_windowWidth = w;
        m_windowHeight = h;
    };

    // Update actual window size (may differ if maximized, etc.)
    m_windowWidth = m_vkBase->GetWidth();
    m_windowHeight = m_vkBase->GetHeight();

    // Initialize theme (ImGui context already created by VulkanBase)
    m_theme = std::make_unique<EditorTheme>();
    m_theme->applyDarkTheme();

    // Initialize scene renderer (no longer needs GL — pure data)
    m_sceneRenderer = std::make_unique<SceneRenderer>();
    if (!m_sceneRenderer->initialize()) {
        std::cerr << "[Editor] Failed to initialize SceneRenderer" << std::endl;
        return false;
    }

    m_isRunning = true;

    // Print Vulkan device info
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(m_vkBase->GetPhysicalDevice(), &props);
    std::cout << "[Editor] Vulkan Device: " << props.deviceName << std::endl;
    std::cout << "[Editor] Vulkan API: "
              << VK_VERSION_MAJOR(props.apiVersion) << "."
              << VK_VERSION_MINOR(props.apiVersion) << "."
              << VK_VERSION_PATCH(props.apiVersion) << std::endl;
    std::cout << "[Editor] Initialized successfully" << std::endl;

    return true;
}

void EditorApp::run() {
    while (m_isRunning) {
        // Calculate delta time
        float currentTime = static_cast<float>(glfwGetTime());
        m_deltaTime = currentTime - m_lastFrameTime;
        m_lastFrameTime = currentTime;
        m_time = currentTime;

        // BeginFrame handles glfwPollEvents, acquires swapchain image,
        // begins command buffer and render pass.
        if (!m_vkBase->BeginFrame()) {
            m_isRunning = false;
            break;
        }

        // Update window size from VulkanBase (handles resize)
        m_windowWidth = m_vkBase->GetWidth();
        m_windowHeight = m_vkBase->GetHeight();

        processInput();

        // Start ImGui frame (Vulkan + GLFW backends)
        m_vkBase->ImGuiNewFrame();

        // Render editor UI
        renderMenuBar();
        renderToolbar();
        renderPanels();
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

        // Record ImGui draw commands into the Vulkan command buffer
        VkCommandBuffer cmd = m_vkBase->GetCurrentCommandBuffer();
        m_vkBase->ImGuiRender(cmd);

        // EndFrame ends the render pass, submits, and presents
        m_vkBase->EndFrame();
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
        ImGui::Text("VerletX Engine Editor v1.0 [Vulkan]");
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

    // VulkanBase::Shutdown handles ImGui shutdown, Vulkan cleanup, and GLFW teardown
    if (m_vkBase) {
        m_vkBase->Shutdown();
        m_vkBase.reset();
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
