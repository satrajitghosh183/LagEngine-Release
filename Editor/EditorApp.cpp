#include "EditorApp.hpp"
#include "../Engine/Core/Logger.hpp"
#include "../Engine/Core/EntryPoint.hpp"
#include "UI/UIRenderer.hpp"
#include "Panels/SceneHierarchyPanel.hpp"
// Include other panel headers as they are created

namespace GameEngine {

    EditorApp::EditorApp()
        : Application(RuntimeConfig(1920, 1080, "GameEngine Editor")) {
    }

    EditorApp::~EditorApp() {
    }

    void EditorApp::OnInit() {
        // Initialize UI renderer
        UIRenderer::Init();

        // Create editor context
        m_EditorContext = CreateScope<EditorContext>();
        m_CommandHistory = CreateScope<CommandHistory>();

        // Create panels (stubs for now)
        // m_SceneHierarchyPanel = CreateScope<SceneHierarchyPanel>();
        // ... other panels

        GE_CORE_INFO("Editor initialized");
    }

    void EditorApp::OnUpdate(float deltaTime) {
        // Editor-specific update logic
    }

    void EditorApp::OnRender() {
        UIRenderer::BeginFrame();

        SetupDocking();
        RenderMenuBar();
        RenderPanels();

        UIRenderer::EndFrame();
    }

    void EditorApp::OnShutdown() {
        UIRenderer::Shutdown();
        GE_CORE_INFO("Editor shutdown");
    }

    void EditorApp::SetupDocking() {
        UIRenderer::BeginDockspace();
        // Docking setup happens here
    }

    void EditorApp::RenderMenuBar() {
        // Menu bar rendering
    }

    void EditorApp::RenderPanels() {
        // Render all panels
        // if (m_SceneHierarchyPanel) {
        //     m_SceneHierarchyPanel->OnImGuiRender();
        // }
    }

}

// Entry point
GameEngine::Application* GameEngine::CreateApplication() {
    return new GameEngine::EditorApp();
}
