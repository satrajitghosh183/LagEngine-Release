/**
 * VerletX Engine Editor
 * 
 * A modern game engine editor inspired by LumixEngine
 * Features: Scene hierarchy, inspector, viewport, console, asset browser,
 *           physics settings, and performance monitoring.
 * 
 * Built with ImGui + GLFW + OpenGL
 */

#include "core/EditorApp.hpp"
#include "core/EditorTheme.hpp"
#include "core/SceneRenderer.hpp"
#include "panels/SceneHierarchyPanel.hpp"
#include "panels/InspectorPanel.hpp"
#include "panels/ViewportPanel.hpp"
#include "panels/ConsolePanel.hpp"
#include "panels/AssetBrowserPanel.hpp"
#include "panels/PhysicsPanel.hpp"
#include "panels/PerformancePanel.hpp"

#include <iostream>
#include <memory>

int main(int /*argc*/, char* /*argv*/[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "   VerletX Engine Editor v1.0" << std::endl;
    std::cout << "   Powered by ImGui + OpenGL" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Get editor instance
    auto& editor = editor::EditorApp::getInstance();
    
    // Initialize with default window size
    if (!editor.initialize(1920, 1080, "VerletX Engine Editor")) {
        std::cerr << "Failed to initialize editor!" << std::endl;
        return -1;
    }
    
    // Get the scene renderer
    editor::SceneRenderer* sceneRenderer = editor.getSceneRenderer();
    
    // Create and register panels
    auto hierarchyPanel = std::make_shared<editor::SceneHierarchyPanel>();
    auto inspectorPanel = std::make_shared<editor::InspectorPanel>();
    auto viewportPanel = std::make_shared<editor::ViewportPanel>();
    auto consolePanel = std::make_shared<editor::ConsolePanel>();
    auto assetBrowserPanel = std::make_shared<editor::AssetBrowserPanel>();
    auto physicsPanel = std::make_shared<editor::PhysicsPanel>();
    auto performancePanel = std::make_shared<editor::PerformancePanel>();
    
    // Connect panels to scene renderer
    hierarchyPanel->setSceneRenderer(sceneRenderer);
    inspectorPanel->setSceneRenderer(sceneRenderer);
    viewportPanel->setSceneRenderer(sceneRenderer);
    
    // Connect hierarchy selection callback
    hierarchyPanel->onSelectionChanged = [&](int index) {
        // Selection is already handled by the panel, just update status
        if (sceneRenderer && index >= 0) {
            auto* obj = sceneRenderer->getObject(index);
            if (obj) {
                editor.setSelectedObject(obj, obj->type);
            }
        } else {
            editor.clearSelection();
        }
    };
    
    // Register all panels
    editor.registerPanel(hierarchyPanel);
    editor.registerPanel(inspectorPanel);
    editor.registerPanel(viewportPanel);
    editor.registerPanel(consolePanel);
    editor.registerPanel(assetBrowserPanel);
    editor.registerPanel(physicsPanel);
    editor.registerPanel(performancePanel);
    
    // Set up editor callbacks
    editor.onPlay = []() {
        editor::ConsolePanel::info("Simulation started");
    };
    
    editor.onPause = []() {
        editor::ConsolePanel::info("Simulation paused");
    };
    
    editor.onStop = []() {
        editor::ConsolePanel::info("Simulation stopped");
    };
    
    // Log startup message
    editor::ConsolePanel::info("Editor initialized successfully");
    editor::ConsolePanel::info("Welcome to VerletX Engine Editor!");
    editor::ConsolePanel::debug("Scene renderer loaded with default objects");
    editor::ConsolePanel::info("Use 'Create' menu or right-click in Scene Hierarchy to add objects");
    editor::ConsolePanel::info("Right-click + WASD to navigate camera, scroll to zoom");
    
    // Run the editor main loop
    editor.run();
    
    // Cleanup
    editor.shutdown();
    
    std::cout << "Editor closed." << std::endl;
    return 0;
}
