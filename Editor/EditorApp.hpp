#pragma once

#include "../Engine/Core/Application.hpp"
#include "../Engine/Core/RuntimeConfig.hpp"
#include "Core/EditorContext.hpp"
#include "Core/Command.hpp"
#include <vector>
#include <memory>

namespace GameEngine {

    // Forward declarations for panels
    class SceneHierarchyPanel;

    /**
     * @brief Editor application
     * 
     * Composes all editor panels and manages editor state
     */
    class EditorApp : public Application {
    public:
        EditorApp();
        ~EditorApp() override;

    protected:
        void OnInit() override;
        void OnUpdate(float deltaTime) override;
        void OnRender() override;
        void OnShutdown() override;

    private:
        void SetupDocking();
        void RenderMenuBar();
        void RenderPanels();

        Scope<EditorContext> m_EditorContext;
        Scope<CommandHistory> m_CommandHistory;

        // Panels (only include ones that exist)
        Scope<SceneHierarchyPanel> m_SceneHierarchyPanel;
    };

}
