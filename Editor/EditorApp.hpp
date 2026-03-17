#pragma once

#include "../Engine/Core/Application.hpp"
#include "../Engine/Core/RuntimeConfig.hpp"
#include "../Engine/Graphics/Mesh3D.hpp"
#include "../Engine/Graphics/Shader.hpp"
#include "../Engine/Graphics/Material.hpp"
#include "../Engine/Platform/FileWatcher.hpp"
#include "../Engine/Platform/Input.hpp"
#include "Core/EditorContext.hpp"
#include "Core/Command.hpp"
#include "Core/Hotkeys.hpp"
#include "Panels/SceneHierarchyPanel.hpp"
#include "Panels/ViewportPanel.hpp"
#include "Panels/ToolbarPanel.hpp"
#include "Panels/ConsolePanel.hpp"
#include "Panels/AssetBrowserPanel.hpp"
#include "Panels/ScriptingConsolePanel.hpp"
#include "Panels/ComponentsWindow.hpp"
#include "Panels/ProfilerWindow.hpp"
#include "Panels/GraphicsSettingsPanel.hpp"
#include "Panels/ThemeEditorWindow.hpp"
#include "Panels/ShaderAssistantPanel.hpp"
#include "Panels/NewProjectDialog.hpp"
#include "Panels/PresetBrowserPanel.hpp"
#include "Panels/WelcomePanel.hpp"
#include "Panels/AnimationPanel.hpp"
#include "Panels/BuildPanel.hpp"
#include "Panels/CodeEditorPanel.hpp"
#include "Panels/AIAssistantPanel.hpp"
#include "Panels/MaterialEditorPanel.hpp"
#include "Panels/ObjectSpawnerPanel.hpp"
#include "Panels/ProjectSettingsPanel.hpp"
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <vector>
#include <memory>

namespace GameEngine {

    // EditorState is defined in Core/EditorContext.hpp

    /**
     * @brief Editor scene data structure for tab management
     */
    struct EditorScene {
        uint64_t m_ID = 0;  // Unique scene tab ID
        std::string m_Path;  // File path (empty for untitled)
        Ref<Scene> m_Scene;  // The scene instance
        bool m_HasUnsavedChanges = false;  // Dirty flag
        
        // Editor camera state per scene (stored separately from scene entities)
        glm::vec3 m_CameraPosition = glm::vec3(0.0f, 0.0f, 10.0f);
        glm::vec3 m_CameraFocalPoint = glm::vec3(0.0f);
        float m_CameraDistance = 10.0f;
        float m_CameraPitch = 30.0f;
        float m_CameraYaw = 45.0f;
        
        // Undo/redo history (per scene)
        // Note: Using CommandHistory for now, but could be extended with Archive-based serialization
    };

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
        void ResetDockLayout();
        void BuildDefaultDockLayout(ImGuiID dockspace_id);
        void RenderMenuBar();
        void RenderPanels();
        void NewScene();
        void OpenScene();
        void SaveScene();
        void SaveSceneAs();
        void OpenProject();
        
        // Play mode controls
        void PlayScene();
        void PauseScene();
        void ResumeScene();
        void StopScene();
        void StepFrame();
        
        // Scene state save/restore for play mode
        void SaveSceneState();
        void RestoreSceneState();

        // Scene tab management
        void RenderSceneTabs();
        bool HasValidCurrentScene() const { return m_CurrentSceneIndex >= 0 && m_CurrentSceneIndex < static_cast<int>(m_Scenes.size()); }
        EditorScene& GetCurrentEditorScene();
        const EditorScene& GetCurrentEditorScene() const;
        EditorScene* TryGetCurrentEditorScene();
        const EditorScene* TryGetCurrentEditorScene() const;
        Ref<Scene> GetCurrentScene();
        Ref<Scene> GetCurrentScene() const;
        EditorScene* FindEditorSceneByID(uint64_t id);
        void SetCurrentScene(int index);
        bool CheckUnsavedChanges(int index);
        void OpenSceneInNewTab(const std::string& path);
        void CloseScene(int index);

        // Default scene setup
        void PopulateDefaultScene();
        Entity CreateDefaultMeshEntity(const std::string& name, const Ref<Mesh3D>& mesh);
        void PostLoadFixupMaterials(const Ref<Scene>& scene);

        // One-click demo scenes
        void LoadPhysicsDemo();
        void LoadLightingDemo();
        void LoadClothDemo();
        void LoadSPHWaterDemo();
        void LoadRobotArmDemo();
        void LoadGPUParticleDemo();
        void LoadSoftBodyDemo();
        void LoadJointsDemo();
        void LoadCharacterDemo();
        void LoadCannonShootingDemo();  // 2D Cannon shooting cloth demo
        void LoadVerletClothDemo();     // Verlet-integrated cloth + cannon

        // Asset drop from browser to viewport
        void OnAssetDroppedInViewport(const std::string& assetPath);

        Scope<EditorContext> m_EditorContext;
        Scope<CommandHistory> m_CommandHistory;
        Scope<HotkeyManager> m_HotkeyManager;

        // Panels
        Scope<SceneHierarchyPanel> m_SceneHierarchyPanel;
        Scope<ViewportPanel> m_ViewportPanel;
        Scope<ToolbarPanel> m_ToolbarPanel;
        Scope<ConsolePanel> m_ConsolePanel;
        Scope<AssetBrowserPanel> m_AssetBrowserPanel;
        Scope<ScriptingConsolePanel> m_ScriptingConsolePanel;
        Scope<ComponentsWindow> m_ComponentsWindow;
        Scope<ProfilerWindow> m_ProfilerWindow;
        Scope<GraphicsSettingsPanel> m_GraphicsSettingsPanel;
        Scope<ThemeEditorWindow> m_ThemeEditorWindow;
        Scope<ShaderAssistantPanel> m_ShaderAssistantPanel;
        Scope<NewProjectDialog> m_NewProjectDialog;
        Scope<PresetBrowserPanel> m_PresetBrowserPanel;
        Scope<WelcomePanel> m_WelcomePanel;
        Scope<AnimationPanel> m_AnimationPanel;
        
        // New integrated panels
        Scope<BuildPanel> m_BuildPanel;
        Scope<CodeEditorPanel> m_CodeEditorPanel;
        Scope<AIAssistantPanel> m_AIAssistantPanel;
        Scope<MaterialEditorPanel> m_MaterialEditorPanel;
        Scope<ObjectSpawnerPanel> m_ObjectSpawnerPanel;
        Scope<ProjectSettingsPanel> m_ProjectSettingsPanel;

        // Multiple scene tabs
        std::vector<std::unique_ptr<EditorScene>> m_Scenes;
        int m_CurrentSceneIndex = -1;
        uint64_t m_NextSceneID = 1;
        
        // Default rendering resources
        Ref<Shader> m_DefaultShader;
        Ref<Material> m_DefaultMaterial;

        // Shader hot-reload
        Scope<FileWatcher> m_ShaderWatcher;

        // Play mode state
        // Play mode state is in EditorContext (no duplicate m_EditorState)
        bool m_StepRequested = false;

        // Auto-save (every 5 minutes)
        float m_AutoSaveTimer = 0.0f;
        static constexpr float AUTO_SAVE_INTERVAL = 300.0f;

        // Error recovery
        std::string m_LastError;
        
    public:
        // Public accessors for panels
        EditorState GetEditorState() const { return m_EditorContext ? m_EditorContext->GetState() : EditorState::Edit; }
        bool IsPlaying() const { return m_EditorContext && m_EditorContext->IsPlaying(); }
        bool IsPaused() const { return m_EditorContext && m_EditorContext->IsPaused(); }
        bool IsEditing() const { return m_EditorContext && m_EditorContext->IsEditing(); }
        
        // Scene accessors (for panels)
        int GetCurrentSceneIndex() const { return m_CurrentSceneIndex; }
        size_t GetSceneCount() const { return m_Scenes.size(); }
    };

}
