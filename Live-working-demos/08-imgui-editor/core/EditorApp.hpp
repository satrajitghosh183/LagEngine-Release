#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <string>
#include <memory>
#include <vector>
#include <functional>

namespace editor {

class EditorPanel;
class EditorTheme;
class SceneRenderer;

/**
 * @brief Main Editor Application class - manages the entire editor lifecycle
 * Inspired by LumixEngine's StudioApp architecture
 */
class EditorApp {
public:
    static EditorApp& getInstance();
    
    bool initialize(int width = 1920, int height = 1080, const std::string& title = "VerletX Engine Editor");
    void run();
    void shutdown();
    
    // Window access
    GLFWwindow* getWindow() const { return m_window; }
    int getWindowWidth() const { return m_windowWidth; }
    int getWindowHeight() const { return m_windowHeight; }
    
    // Panel management
    void registerPanel(std::shared_ptr<EditorPanel> panel);
    void unregisterPanel(const std::string& name);
    EditorPanel* getPanel(const std::string& name);
    
    // Scene renderer access
    SceneRenderer* getSceneRenderer() { return m_sceneRenderer.get(); }
    
    // Selection management
    void setSelectedObject(void* object, const std::string& type);
    void* getSelectedObject() const { return m_selectedObject; }
    std::string getSelectedType() const { return m_selectedType; }
    void clearSelection();
    
    // Editor state
    bool isPlaying() const { return m_isPlaying; }
    bool isPaused() const { return m_isPaused; }
    void play();
    void pause();
    void stop();
    void step();
    
    // Time
    float getDeltaTime() const { return m_deltaTime; }
    float getTime() const { return m_time; }
    
    // Events
    std::function<void()> onPlay;
    std::function<void()> onPause;
    std::function<void()> onStop;
    std::function<void(void*, const std::string&)> onSelectionChanged;

private:
    EditorApp() = default;
    ~EditorApp();
    
    EditorApp(const EditorApp&) = delete;
    EditorApp& operator=(const EditorApp&) = delete;
    
    bool initializeGLFW();
    bool initializeImGui();
    void setupDockspace();
    void renderMenuBar();
    void renderToolbar();
    void renderStatusBar();
    void renderPanels();
    void processInput();
    void handleShortcuts();
    
    // Window
    GLFWwindow* m_window = nullptr;
    int m_windowWidth = 1920;
    int m_windowHeight = 1080;
    std::string m_windowTitle = "VerletX Engine Editor";
    
    // Panels
    std::vector<std::shared_ptr<EditorPanel>> m_panels;
    
    // Scene renderer
    std::unique_ptr<SceneRenderer> m_sceneRenderer;
    
    // Selection
    void* m_selectedObject = nullptr;
    std::string m_selectedType;
    
    // Editor state
    bool m_isRunning = false;
    bool m_isPlaying = false;
    bool m_isPaused = false;
    
    // Time
    float m_deltaTime = 0.0f;
    float m_time = 0.0f;
    float m_lastFrameTime = 0.0f;
    
    // Theme
    std::unique_ptr<EditorTheme> m_theme;
    
    // UI state
    bool m_showDemoWindow = false;
    bool m_showMetricsWindow = false;
};

} // namespace editor
