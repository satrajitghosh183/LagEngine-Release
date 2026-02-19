#include "EditorApp.hpp"
#include "../Engine/Core/Logger.hpp"
#include "../Engine/Core/EntryPoint.hpp"
#include "../Engine/Scene/SceneSerializer.hpp"
#include "../Engine/Scene/Components/TransformComponent.hpp"
#include "../Engine/Scene/Components/MeshRendererComponent.hpp"
#include "../Engine/Scene/Components/LightComponent.hpp"
#include "../Engine/Scene/Components/CameraComponent.hpp"
#include "../Engine/Graphics/MeshGenerator3D.hpp"
#include "../Engine/Graphics/Shader.hpp"
#include "../Engine/Graphics/Material.hpp"
#include "../Engine/Platform/FileDialog.hpp"
#include "../Engine/Platform/FileSystem.hpp"
#include "../Engine/Assets/AssetDatabase.hpp"
#include "../Engine/Assets/AssetImporter.hpp"
#include "../Engine/Graphics/IBL.hpp"
#include "../Engine/Core/ProjectFile.hpp"
#include "UI/UIRenderer.hpp"
#include "Panels/SceneHierarchyPanel.hpp"
#include "Panels/ViewportPanel.hpp"
#include "Panels/ToolbarPanel.hpp"
#include "Panels/ConsolePanel.hpp"
#include "Panels/AssetBrowserPanel.hpp"
#include "Panels/ProfilerWindow.hpp"
#include "Panels/GraphicsSettingsPanel.hpp"
#include "Panels/ThemeEditorWindow.hpp"
#include "../Engine/Assets/TemplateManager.hpp"
#include "../Engine/Assets/PresetManager.hpp"
#include "../Engine/Graphics/RenderPath.hpp"
#include "../Engine/Utilities/Debug/DebugDraw.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <filesystem>
#include <algorithm>
#include <glm/glm.hpp>

namespace GameEngine {

    EditorApp::EditorApp()
        : Application(RuntimeConfig(1920, 1080, "GameEngine Editor")) {
    }

    EditorApp::~EditorApp() {
    }

    void EditorApp::OnInit() {
        // Initialize UI renderer
        UIRenderer::Init();

        // Initialize debug drawing system
        DebugDraw::Init();

        // Create editor context
        m_EditorContext = CreateScope<EditorContext>();
        m_EditorContext->SetStateChangeCallback([this](EditorState oldState, EditorState newState) {
            if ((oldState == EditorState::Play || oldState == EditorState::Pause) && newState == EditorState::Edit) {
                Ref<Scene> restored = m_EditorContext->GetActiveScene();
                if (restored && m_CurrentSceneIndex >= 0 && m_CurrentSceneIndex < static_cast<int>(m_Scenes.size())) {
                    m_Scenes[m_CurrentSceneIndex]->m_Scene = restored;
                    if (m_SceneHierarchyPanel) m_SceneHierarchyPanel->SetContext(restored);
                    if (m_ViewportPanel) m_ViewportPanel->SetContext(restored);
                }
            }
        });
        m_CommandHistory = CreateScope<CommandHistory>();
        m_HotkeyManager = CreateScope<HotkeyManager>();
        m_HotkeyManager->LoadFromConfig("editor_hotkeys.json");

        // Create default scene (first tab)
        auto editorScene = std::make_unique<EditorScene>();
        editorScene->m_ID = m_NextSceneID++;
        editorScene->m_Path = "";
        editorScene->m_Scene = CreateRef<Scene>("Untitled Scene");
        editorScene->m_HasUnsavedChanges = false;
        m_Scenes.push_back(std::move(editorScene));
        m_CurrentSceneIndex = 0;
        
        auto& currentScene = GetCurrentEditorScene();
        m_EditorContext->SetActiveScene(currentScene.m_Scene);

        // Create panels
        m_SceneHierarchyPanel = CreateScope<SceneHierarchyPanel>();
        m_SceneHierarchyPanel->SetContext(currentScene.m_Scene);
        m_SceneHierarchyPanel->SetCommandHistory(m_CommandHistory.get());
        m_SceneHierarchyPanel->SetCreateMeshEntityCallback([this](const std::string& meshType) {
            Ref<Mesh3D> mesh;
            if (meshType == "Cube") mesh = MeshGenerator3D::CreateCube(1.0f);
            else if (meshType == "Sphere") mesh = MeshGenerator3D::CreateSphere(0.5f, 32, 16);
            else if (meshType == "Plane") mesh = MeshGenerator3D::CreatePlane(5.0f, 5.0f);
            if (mesh) {
                Entity entity = CreateDefaultMeshEntity(meshType, mesh);
                if (entity.IsValid() && m_SceneHierarchyPanel) {
                    m_SceneHierarchyPanel->SetSelectedEntity(entity);
                }
                GetCurrentEditorScene().m_HasUnsavedChanges = true;
            }
        });
        
        m_ViewportPanel = CreateScope<ViewportPanel>();
        m_ViewportPanel->SetContext(currentScene.m_Scene);
        m_ViewportPanel->SetAssetDropCallback([this](const std::string& assetPath) {
            OnAssetDroppedInViewport(assetPath);
        });
        
        m_ToolbarPanel = CreateScope<ToolbarPanel>();
        m_ToolbarPanel->SetContext(m_EditorContext.get());
        
        m_ConsolePanel = CreateScope<ConsolePanel>();
        
        m_AssetBrowserPanel = CreateScope<AssetBrowserPanel>();
        m_AssetBrowserPanel->SetOnOpenScene([this](const std::string& path) { OpenSceneInNewTab(path); });
        
        m_ScriptingConsolePanel = CreateScope<ScriptingConsolePanel>();
        
        m_ComponentsWindow = CreateScope<ComponentsWindow>();
        m_ComponentsWindow->SetContext(m_EditorContext.get());
        m_ComponentsWindow->SetCommandHistory(m_CommandHistory.get());
        
        m_ThemeEditorWindow = CreateScope<ThemeEditorWindow>();

        m_ShaderAssistantPanel = CreateScope<ShaderAssistantPanel>();

        m_NewProjectDialog = CreateScope<NewProjectDialog>();
        m_NewProjectDialog->SetCreateCallback([this](const std::string& scenePath) {
            OpenSceneInNewTab(scenePath);
        });

        m_PresetBrowserPanel = CreateScope<PresetBrowserPanel>();
        m_PresetBrowserPanel->SetLoadCallback([this](const std::string& scenePath) {
            OpenSceneInNewTab(scenePath);
        });

        m_WelcomePanel = CreateScope<WelcomePanel>();
        m_WelcomePanel->SetVisible(true);
        m_WelcomePanel->SetOnNewProject([this]() { if (m_NewProjectDialog) m_NewProjectDialog->Open(); });
        m_WelcomePanel->SetOnOpenProject([this]() { OpenProject(); });
        m_WelcomePanel->SetOnOpenScene([this]() { OpenScene(); });

        // Initialize template and preset managers
        TemplateManager::Init("Content/Templates");
        PresetManager::Init("Content/Presets");

        // Set up state change callback to update scene references
        m_EditorContext->SetStateChangeCallback([this](EditorState oldState, EditorState newState) {
            // When stopping, the scene may have been replaced
            if (newState == EditorState::Edit && oldState != EditorState::Edit) {
                auto currentScene = GetCurrentScene();
                if (currentScene && m_SceneHierarchyPanel) {
                    m_SceneHierarchyPanel->SetContext(currentScene);
                    m_SceneHierarchyPanel->SetSelectedEntity({});
                }
                if (currentScene && m_ViewportPanel) {
                    m_ViewportPanel->SetContext(currentScene);
                }
            }
        });

        // Initialize asset database
        AssetDatabase::Init("Assets");

        // Initialize IBL for PBR rendering
        IBL::Init();

        // Load default shader and create default material
        PopulateDefaultScene();

        // Start shader hot-reload watcher
        m_ShaderWatcher = CreateScope<FileWatcher>(500);
        m_ShaderWatcher->WatchDirectory(
            "Assets/Shaders",
            { ".vert", ".frag", ".glsl", ".shader" },
            [this](const std::string& path, FileChangeType type) {
                if (type == FileChangeType::Modified) {
                    GE_CORE_INFO("Shader file changed: {}", path);
                    if (m_DefaultShader) {
                        m_DefaultShader->Reload();
                    }
                }
            },
            true
        );
        m_ShaderWatcher->Start();

        GE_CORE_INFO("Editor initialized");
    }

    void EditorApp::PopulateDefaultScene() {
        // Create default PBR shader from external files
        std::string shaderDir = "Assets/Shaders/";
        std::string vertPath = shaderDir + "default_pbr.vert";
        std::string fragPath = shaderDir + "default_pbr.frag";

        if (std::filesystem::exists(vertPath) && std::filesystem::exists(fragPath)) {
            m_DefaultShader = CreateRef<Shader>(vertPath, fragPath);
        } else {
            // Fallback: PBR shader matching Material::Bind() + Renderer3D uniform interface
            const char* vertSrc = R"(
#version 420 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

uniform mat4 u_ViewProjection;
uniform mat4 u_Model;
uniform mat4 u_Transform;
uniform mat3 u_NormalMatrix;

out vec3 v_Normal;
out vec3 v_WorldPos;
out vec2 v_TexCoord;

void main() {
    vec4 worldPos = u_Model * vec4(a_Position, 1.0);
    v_WorldPos = worldPos.xyz;
    v_Normal = u_NormalMatrix * a_Normal;
    v_TexCoord = a_TexCoord;
    gl_Position = u_ViewProjection * worldPos;
}
            )";
            const char* fragSrc = R"(
#version 420 core
layout(location = 0) out vec4 FragColor;

in vec3 v_Normal;
in vec3 v_WorldPos;
in vec2 v_TexCoord;

// Material uniforms (set by Material::Bind)
struct MaterialProperties {
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;
};
uniform MaterialProperties u_Material;
uniform vec3 u_Color;  // legacy alias for albedo

// Texture presence flags
uniform int u_HasAlbedoMap;
uniform int u_HasNormalMap;
uniform int u_HasMetallicMap;
uniform int u_HasRoughnessMap;
uniform int u_HasAOMap;

// Scene uniforms (set by Renderer3D)
uniform vec3 u_CameraPosition;

// IBL uniforms (set by IBL::ApplyToShader)
uniform samplerCube u_IrradianceMap;
uniform samplerCube u_PrefilterMap;
uniform sampler2D u_BRDFLUT;
uniform int u_HasIBL;

// Lighting
struct AmbientLight { vec3 color; float intensity; };
uniform AmbientLight u_AmbientLight;

#define MAX_LIGHTS 16
struct LightInfo {
    int type;
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
    float range;
    float innerCutoff;
    float outerCutoff;
};
uniform LightInfo u_Lights[MAX_LIGHTS];
uniform int u_LightCount;

const float PI = 3.14159265359;

// PBR functions
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec3 albedo = u_Material.albedo;
    float metallic = u_Material.metallic;
    float roughness = u_Material.roughness;
    float ao = u_Material.ao;

    // Fallback: use u_Color if albedo is black (legacy shaders)
    if (dot(albedo, albedo) < 0.001) albedo = u_Color;

    vec3 N = normalize(v_Normal);
    vec3 V = normalize(u_CameraPosition - v_WorldPos);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // Direct lighting
    vec3 Lo = vec3(0.0);
    for (int i = 0; i < u_LightCount && i < MAX_LIGHTS; i++) {
        vec3 L;
        float attenuation = 1.0;

        if (u_Lights[i].type == 0) {
            // Directional
            L = normalize(-u_Lights[i].direction);
        } else {
            // Point / Spot
            vec3 d = u_Lights[i].position - v_WorldPos;
            L = normalize(d);
            float dist = length(d);
            attenuation = 1.0 / (u_Lights[i].constant + u_Lights[i].linear * dist + u_Lights[i].quadratic * dist * dist);

            // Spot cone falloff
            if (u_Lights[i].type == 2) {
                float theta = dot(L, normalize(-u_Lights[i].direction));
                float epsilon = u_Lights[i].innerCutoff - u_Lights[i].outerCutoff;
                attenuation *= clamp((theta - u_Lights[i].outerCutoff) / max(epsilon, 0.001), 0.0, 1.0);
            }
        }

        vec3 H = normalize(V + L);
        vec3 radiance = u_Lights[i].color * u_Lights[i].intensity * attenuation;

        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;

        vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
        float NdotL = max(dot(N, L), 0.0);

        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // Ambient (IBL or flat)
    vec3 ambient;
    if (u_HasIBL != 0) {
        vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
        vec3 kD = (1.0 - F) * (1.0 - metallic);
        vec3 irradiance = texture(u_IrradianceMap, N).rgb;
        vec3 diffuse = irradiance * albedo;

        vec3 R = reflect(-V, N);
        const float MAX_REFLECTION_LOD = 4.0;
        vec3 prefilteredColor = textureLod(u_PrefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
        vec2 brdf = texture(u_BRDFLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
        vec3 specularIBL = prefilteredColor * (F * brdf.x + brdf.y);

        ambient = (kD * diffuse + specularIBL) * ao;
    } else {
        ambient = u_AmbientLight.color * u_AmbientLight.intensity * albedo * ao;
    }

    vec3 color = ambient + Lo;

    // Tone mapping (Reinhard) + gamma
    color = pow(color / (color + vec3(1.0)), vec3(1.0 / 2.2));
    FragColor = vec4(color, 1.0);
}
            )";
            m_DefaultShader = CreateRef<Shader>("DefaultPBR", vertSrc, fragSrc);
            GE_CORE_WARN("External PBR shader not found, using fallback inline shader");
        }

        m_DefaultMaterial = CreateRef<Material>(m_DefaultShader);
        m_DefaultMaterial->SetName("Default");
        m_DefaultMaterial->SetAlbedo(glm::vec3(0.8f, 0.8f, 0.8f));
        m_DefaultMaterial->SetRoughness(0.5f);
        m_DefaultMaterial->SetMetallic(0.0f);
        m_DefaultMaterial->SetAO(1.0f);

        // Create default directional light (sun-like, from above-front)
        auto currentScene = GetCurrentScene();
        if (!currentScene) return;
        Entity dirLight = currentScene->CreateEntity("Directional Light");
        // TransformComponent already added by CreateEntity
        auto& lightTf = dirLight.GetComponent<TransformComponent>();
        lightTf.Position = glm::vec3(5.0f, 10.0f, 5.0f);
        // Point the light toward the origin (down and forward) for better illumination
        lightTf.Rotation = glm::quat(glm::radians(glm::vec3(-45.0f, 45.0f, 0.0f)));
        auto& lc = dirLight.AddComponent<LightComponent>(LightType::Directional);
        lc.Intensity = 2.0f;  // Brighter for better visibility
        lc.Color = glm::vec3(1.0f, 0.98f, 0.95f);  // Warm white
        lc.SyncToLight();

        // Create a cube so there's something visible
        CreateDefaultMeshEntity("Cube", MeshGenerator3D::CreateCube(1.0f));

        // Create ground plane
        auto groundMesh = MeshGenerator3D::CreatePlane(10.0f, 10.0f);
        groundMesh->SetName("Plane");
        Entity ground = CreateDefaultMeshEntity("Ground", groundMesh);
        auto& groundTf = ground.GetComponent<TransformComponent>();
        groundTf.Position = glm::vec3(0.0f, -0.5f, 0.0f);
    }

    void EditorApp::PostLoadFixupMaterials(const Ref<Scene>& scene) {
        auto entities = scene->GetEntitiesWith<MeshRendererComponent>();
        for (auto& entity : entities) {
            auto& meshRenderer = entity.GetComponent<MeshRendererComponent>();
            auto material = meshRenderer.GetMaterial();
            if (material && !material->GetShader() && m_DefaultShader) {
                material->SetShader(m_DefaultShader);
            }
        }
    }

    Entity EditorApp::CreateDefaultMeshEntity(const std::string& name, const Ref<Mesh3D>& mesh) {
        auto currentScene = GetCurrentScene();
        if (!currentScene) return {};
        
        // Set mesh name for serialization (primitive type identification)
        if (mesh && mesh->GetName().empty()) {
            mesh->SetName(name);
        }
        Entity entity = currentScene->CreateEntity(name);
        // TransformComponent already added by CreateEntity - no need to add again
        // Each entity gets its own material instance so editing one doesn't affect others
        auto entityMaterial = CreateRef<Material>(*m_DefaultMaterial);
        entity.AddComponent<MeshRendererComponent>(mesh, entityMaterial);
        return entity;
    }

    void EditorApp::OnAssetDroppedInViewport(const std::string& assetPath) {
        auto currentScene = GetCurrentScene();
        if (!currentScene) return;

        std::filesystem::path path(assetPath);
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        // Model files -> import and create entity
        if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb" || ext == ".dae") {
            MeshImportOptions options;
            options.TextureSearchPath = path.parent_path().string();
            auto result = AssetImporter::ImportMesh(assetPath, options);

            if (result.Success && !result.Meshes.empty()) {
                std::string entityName = path.stem().string();
                auto mesh = result.Meshes[0];
                mesh->SetSourcePath(assetPath);

                auto material = (!result.Materials.empty() && result.Materials[0])
                    ? result.Materials[0] : m_DefaultMaterial;
                if (material && !material->GetShader() && m_DefaultShader) {
                    material->SetShader(m_DefaultShader);
                }

                Entity entity = currentScene->CreateEntity(entityName);
                // TransformComponent already added by CreateEntity
                entity.AddComponent<MeshRendererComponent>(mesh, material);

                if (m_SceneHierarchyPanel) {
                    m_SceneHierarchyPanel->SetSelectedEntity(entity);
                }
                
                GetCurrentEditorScene().m_HasUnsavedChanges = true;

                GE_CORE_INFO("Dropped model '{}' into viewport", entityName);
            } else {
                GE_CORE_ERROR("Failed to import model: {}", result.ErrorMessage);
            }
        }
        // Scene files -> load scene in new tab
        else if (ext == ".scene") {
            OpenSceneInNewTab(assetPath);
        }
    }

    void EditorApp::OnUpdate(float deltaTime) {
        try {
        // Process shader hot-reload callbacks on main thread
        if (m_ShaderWatcher) {
            m_ShaderWatcher->ProcessCallbacks();
        }

        // Keyboard shortcuts via hotkey manager
        if (m_HotkeyManager) {
            if (m_HotkeyManager->CheckInput(EditorActions::UNDO_ACTION)) {
                if (m_CommandHistory) m_CommandHistory->Undo();
            }
            if (m_HotkeyManager->CheckInput(EditorActions::REDO_ACTION)) {
                if (m_CommandHistory) m_CommandHistory->Redo();
            }
            if (m_HotkeyManager->CheckInput(EditorActions::NEW_SCENE)) {
                NewScene();
            }
            if (m_HotkeyManager->CheckInput(EditorActions::OPEN_SCENE)) {
                OpenScene();
            }
            if (m_HotkeyManager->CheckInput(EditorActions::SAVE_SCENE)) {
                SaveScene();
            }
            if (m_HotkeyManager->CheckInput(EditorActions::SAVE_SCENE_AS)) {
                SaveSceneAs();
            }
        }
        
        // HOME key toggle for scripting console (check directly since it's not in hotkey system yet)
        if (Input::IsKeyJustPressed(KeyCode::Home) && m_ScriptingConsolePanel) {
            m_ScriptingConsolePanel->Toggle();
        }

        // ============ PLAY MODE: Update scene (physics, scripts, etc.) ============
        // Use EditorContext state (which is updated by ToolbarPanel play/pause/stop buttons)
        EditorState editorState = m_EditorContext ? m_EditorContext->GetState() : EditorState::Edit;
        if (editorState == EditorState::Play || editorState == EditorState::Pause) {
            auto currentScene = GetCurrentScene();
            if (currentScene) {
                bool shouldUpdate = (editorState == EditorState::Play) || m_StepRequested;
                
                if (shouldUpdate) {
                    // Regular update (scripts, animation, etc.)
                    currentScene->Update(deltaTime);
                    
                    // Fixed update (physics) - using fixed timestep for stability
                    static float accumulator = 0.0f;
                    const float fixedDeltaTime = 1.0f / 60.0f;  // 60 Hz physics
                    
                    accumulator += deltaTime;
                    while (accumulator >= fixedDeltaTime) {
                        currentScene->FixedUpdate(fixedDeltaTime);
                        accumulator -= fixedDeltaTime;
                    }
                    
                    // Clear step request after one frame
                    if (m_StepRequested) {
                        m_StepRequested = false;
                    }
                }
            }
        }

        // Update viewport panel (handles camera input + picking)
        if (m_ViewportPanel && m_CurrentSceneIndex >= 0) {
            // Before update: push hierarchy selection to viewport
            if (m_SceneHierarchyPanel) {
                m_ViewportPanel->SetSelectedEntity(m_SceneHierarchyPanel->GetSelectedEntity());
            }

            m_ViewportPanel->OnUpdate(deltaTime);

            // Save camera state to current scene
            auto& editorScene = GetCurrentEditorScene();
            auto& cam = m_ViewportPanel->GetEditorCamera();
            editorScene.m_CameraFocalPoint = cam.GetFocalPoint();
            editorScene.m_CameraDistance = cam.GetDistance();
            editorScene.m_CameraPitch = cam.GetPitch();
            editorScene.m_CameraYaw = cam.GetYaw();

            // After update: if viewport picked a different entity, push back to hierarchy
            if (m_SceneHierarchyPanel) {
                Entity viewportSel = m_ViewportPanel->GetSelectedEntity();
                Entity hierarchySel = m_SceneHierarchyPanel->GetSelectedEntity();
                if (viewportSel != hierarchySel) {
                    m_SceneHierarchyPanel->SetSelectedEntity(viewportSel);
                }
            }
        }

            // Update components window with current selection
        if (m_ComponentsWindow && m_SceneHierarchyPanel) {
            Entity selected = m_SceneHierarchyPanel->GetSelectedEntity();
            m_ComponentsWindow->SetSelectedEntity(selected);
        }

        // Auto-save every 5 minutes (only when editing, scene has path and has unsaved changes)
        m_AutoSaveTimer += deltaTime;
        if (m_AutoSaveTimer >= AUTO_SAVE_INTERVAL) {
            m_AutoSaveTimer = 0.0f;
            Ref<Scene> scene = GetCurrentScene();
            if (scene && m_EditorContext->IsEditing()) {
                auto& es = GetCurrentEditorScene();
                if (es.m_HasUnsavedChanges && !es.m_Path.empty()) {
                    std::string autosavePath = std::filesystem::path(es.m_Path).parent_path().string() + "/.autosave.scene";
                    SceneSerializer s(scene);
                    if (s.Serialize(autosavePath)) GE_CORE_DEBUG("Auto-saved to {0}", autosavePath);
                }
            }
        }
        } catch (const std::exception& e) {
            GE_CORE_CRITICAL("Editor update error: {0}", e.what());
            m_LastError = e.what();
        }
    }

    void EditorApp::OnRender() {
        try {
        UIRenderer::BeginFrame();

        SetupDocking();
        RenderMenuBar();
        RenderPanels();

        // Error modal
        if (!m_LastError.empty()) {
            ImGui::OpenPopup("Error");
        }
        if (ImGui::BeginPopupModal("Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "An error occurred:");
            ImGui::TextWrapped("%s", m_LastError.c_str());
            if (ImGui::Button("OK")) {
                m_LastError.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        UIRenderer::EndDockspace();
        UIRenderer::EndFrame();
        } catch (const std::exception& e) {
            GE_CORE_CRITICAL("Editor render error: {0}", e.what());
            m_LastError = e.what();
        }
    }

    void EditorApp::OnShutdown() {
        if (m_HotkeyManager) m_HotkeyManager->SaveToConfig("editor_hotkeys.json");
        if (m_ShaderWatcher) {
            m_ShaderWatcher->Stop();
            m_ShaderWatcher.reset();
        }
        m_ComponentsWindow.reset();
        m_ScriptingConsolePanel.reset();
        m_ProfilerWindow.reset();
        m_GraphicsSettingsPanel.reset();
        m_AssetBrowserPanel.reset();
        m_ConsolePanel.reset();
        m_ToolbarPanel.reset();
        m_ViewportPanel.reset();
        m_SceneHierarchyPanel.reset();
        DebugDraw::Shutdown();
        IBL::Shutdown();
        UIRenderer::Shutdown();
        GE_CORE_INFO("Editor shutdown");
    }

    void EditorApp::SetupDocking() {
        UIRenderer::BeginDockspace();

#ifdef IMGUI_HAS_DOCK
        // Build default dock layout on first run or when layout is stale
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        static bool s_FirstTime = true;
        if (s_FirstTime) {
            s_FirstTime = false;
            // Always rebuild layout to ensure panels are correctly placed
            BuildDefaultDockLayout(dockspace_id);
        }
#endif
    }

    void EditorApp::ResetDockLayout() {
#ifdef IMGUI_HAS_DOCK
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockBuilderRemoveNode(dockspace_id);
        BuildDefaultDockLayout(dockspace_id);
#endif
    }

    void EditorApp::BuildDefaultDockLayout(ImGuiID dockspace_id) {
#ifdef IMGUI_HAS_DOCK
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->WorkSize);

        // Split: left (15%) | center+right (85%)
        ImGuiID dock_left, dock_main;
        ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.15f, &dock_left, &dock_main);

        // Split center+right: center (78%) | right (22%)
        ImGuiID dock_center, dock_right;
        ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, 0.22f, &dock_right, &dock_center);

        // Split center: viewport (top 80%) | bottom (20%)
        ImGuiID dock_viewport, dock_bottom;
        ImGui::DockBuilderSplitNode(dock_center, ImGuiDir_Down, 0.20f, &dock_bottom, &dock_viewport);

        // Split bottom: console (50%) | asset browser (50%)
        ImGuiID dock_console, dock_assets;
        ImGui::DockBuilderSplitNode(dock_bottom, ImGuiDir_Left, 0.50f, &dock_console, &dock_assets);

        // Dock windows into their positions
        ImGui::DockBuilderDockWindow("Scene Hierarchy", dock_left);
        ImGui::DockBuilderDockWindow("Components", dock_right);
        ImGui::DockBuilderDockWindow("Viewport", dock_viewport);
        ImGui::DockBuilderDockWindow("Console", dock_console);
        ImGui::DockBuilderDockWindow("Asset Browser", dock_assets);
        ImGui::DockBuilderDockWindow("Scripting Console", dock_console);
        ImGui::DockBuilderDockWindow("Profiler", dock_bottom);
        ImGui::DockBuilderDockWindow("Graphics Settings", dock_right);
        ImGui::DockBuilderDockWindow("Theme Editor", dock_right);

        ImGui::DockBuilderFinish(dockspace_id);
#endif
    }

    void EditorApp::RenderSceneTabs() {
        static int s_PendingCloseIndex = -1;
        
        // Close confirmation dialog
        if (s_PendingCloseIndex >= 0) {
            ImGui::OpenPopup("Close Scene?##ConfirmClose");
            if (ImGui::BeginPopupModal("Close Scene?##ConfirmClose", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                auto& scene = *m_Scenes[s_PendingCloseIndex];
                std::string sceneName = scene.m_Path.empty() ? "Untitled Scene" : std::filesystem::path(scene.m_Path).filename().string();
                
                ImGui::Text("Save changes to '%s'?", sceneName.c_str());
                ImGui::Separator();
                
                if (ImGui::Button("Save", ImVec2(120, 0))) {
                    if (s_PendingCloseIndex == m_CurrentSceneIndex) {
                        SaveScene();
                    }
                    CloseScene(s_PendingCloseIndex);
                    s_PendingCloseIndex = -1;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Don't Save", ImVec2(120, 0))) {
                    CloseScene(s_PendingCloseIndex);
                    s_PendingCloseIndex = -1;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                    s_PendingCloseIndex = -1;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
        
        if (m_Scenes.empty()) {
            // Show + button even when no scenes
            ImGui::Begin("Scene Tabs", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
            if (ImGui::Button("+", ImVec2(30, 0))) {
                NewScene();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("New Scene");
            }
            ImGui::End();
            return;
        }
        
        ImGui::Begin("Scene Tabs", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
        
        // Tab styling colors
        ImVec4 selectedColor = ImVec4(0.2f, 0.4f, 0.8f, 0.4f);
        ImVec4 hoverColor = ImVec4(0.3f, 0.3f, 0.3f, 0.3f);
        ImVec4 unsavedColor = ImVec4(1.0f, 0.8f, 0.0f, 0.3f);
        
        for (size_t i = 0; i < m_Scenes.size(); ++i) {
            auto& scene = m_Scenes[i];
            std::string tabName = scene->m_Path.empty() ? "Untitled Scene" : std::filesystem::path(scene->m_Path).filename().string();
            if (scene->m_HasUnsavedChanges) {
                tabName += "*";
            }
            
            bool isSelected = (static_cast<int>(i) == m_CurrentSceneIndex);
            ImGui::PushID(static_cast<int>(i));
            
            // Tab button with improved styling
            ImGui::PushStyleColor(ImGuiCol_Button, isSelected ? selectedColor : ImVec4(0.2f, 0.2f, 0.2f, 0.2f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, selectedColor);
            
            if (ImGui::Button(tabName.c_str(), ImVec2(0, 0))) {
                SetCurrentScene(static_cast<int>(i));
            }
            
            // Tooltip with full path
            if (ImGui::IsItemHovered()) {
                std::string tooltip = scene->m_Path.empty() ? "Untitled Scene (unsaved)" : scene->m_Path;
                if (scene->m_HasUnsavedChanges) {
                    tooltip += " (unsaved changes)";
                }
                ImGui::SetTooltip("%s", tooltip.c_str());
            }
            
            // Visual indicator for unsaved changes
            if (scene->m_HasUnsavedChanges && !isSelected) {
                ImGui::GetWindowDrawList()->AddRectFilled(
                    ImGui::GetItemRectMin(),
                    ImGui::GetItemRectMax(),
                    ImGui::ColorConvertFloat4ToU32(unsavedColor),
                    0.0f,
                    ImDrawFlags_RoundCornersTop
                );
            }
            
            ImGui::PopStyleColor(3);
            
            // Close button with better styling
            ImGui::SameLine(0, 2);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.1f, 0.1f, 0.8f));
            
            if (ImGui::SmallButton("x##CloseTab")) {
                if (scene->m_HasUnsavedChanges) {
                    s_PendingCloseIndex = static_cast<int>(i);
                } else {
                    CloseScene(static_cast<int>(i));
                    ImGui::PopID();
                    ImGui::PopStyleColor(3);
                    break; // Exit loop since we modified the vector
                }
            }
            
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Close Scene");
            }
            
            ImGui::PopStyleColor(3);
            ImGui::PopID();
            
            if (i < m_Scenes.size() - 1) {
                ImGui::SameLine();
            }
        }
        
        // Add + button to create new scene
        ImGui::SameLine();
        if (ImGui::Button("+", ImVec2(30, 0))) {
            NewScene();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("New Scene");
        }
        
        ImGui::End();
    }

    void EditorApp::RenderMenuBar() {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                std::string newSceneStr = "New Scene";
                if (m_HotkeyManager) {
                    newSceneStr += " (" + m_HotkeyManager->GetInputString(EditorActions::NEW_SCENE) + ")";
                }
                if (ImGui::MenuItem(newSceneStr.c_str())) {
                    NewScene();
                }
                
                std::string openSceneStr = "Open Scene...";
                if (m_HotkeyManager) {
                    openSceneStr += " (" + m_HotkeyManager->GetInputString(EditorActions::OPEN_SCENE) + ")";
                }
                if (ImGui::MenuItem(openSceneStr.c_str())) {
                    OpenScene();
                }
                
                std::string saveSceneStr = "Save Scene";
                if (m_HotkeyManager) {
                    saveSceneStr += " (" + m_HotkeyManager->GetInputString(EditorActions::SAVE_SCENE) + ")";
                }
                if (ImGui::MenuItem(saveSceneStr.c_str())) {
                    SaveScene();
                }
                
                std::string saveSceneAsStr = "Save Scene As...";
                if (m_HotkeyManager) {
                    saveSceneAsStr += " (" + m_HotkeyManager->GetInputString(EditorActions::SAVE_SCENE_AS) + ")";
                }
                if (ImGui::MenuItem(saveSceneAsStr.c_str())) {
                    SaveSceneAs();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("New from Template...")) {
                    if (m_NewProjectDialog) {
                        m_NewProjectDialog->Open();
                    }
                }
                if (ImGui::MenuItem("Open Project...")) {
                    OpenProject();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit")) {
                    Close();
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
                    if (m_CommandHistory) {
                        m_CommandHistory->Undo();
                    }
                }
                if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
                    if (m_CommandHistory) {
                        m_CommandHistory->Redo();
                    }
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Entity")) {
                auto currentScene = GetCurrentScene();
                auto selectEntity = [this](Entity e) {
                    if (m_SceneHierarchyPanel) m_SceneHierarchyPanel->SetSelectedEntity(e);
                    GetCurrentEditorScene().m_HasUnsavedChanges = true;
                };
                if (ImGui::MenuItem("Create Empty")) {
                    if (currentScene) {
                        // TransformComponent already added by CreateEntity
                        Entity e = currentScene->CreateEntity("New Entity");
                        selectEntity(e);
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Create Camera")) {
                    if (currentScene) {
                        Entity camera = currentScene->CreateEntity("Camera");
                        // TransformComponent already added by CreateEntity
                        camera.AddComponent<CameraComponent>();
                        selectEntity(camera);
                    }
                }
                if (ImGui::MenuItem("Create Directional Light")) {
                    if (currentScene) {
                        Entity light = currentScene->CreateEntity("Directional Light");
                        // TransformComponent already added by CreateEntity
                        auto& tf = light.GetComponent<TransformComponent>();
                        tf.Position = glm::vec3(0.0f, 5.0f, 0.0f);
                        auto& lc = light.AddComponent<LightComponent>(LightType::Directional);
                        lc.Intensity = 1.0f;
                        lc.Color = glm::vec3(1.0f);
                        lc.SyncToLight();
                        selectEntity(light);
                    }
                }
                if (ImGui::MenuItem("Create Point Light")) {
                    if (currentScene) {
                        Entity light = currentScene->CreateEntity("Point Light");
                        // TransformComponent already added by CreateEntity
                        auto& tf = light.GetComponent<TransformComponent>();
                        tf.Position = glm::vec3(0.0f, 3.0f, 0.0f);
                        auto& lc = light.AddComponent<LightComponent>(LightType::Point);
                        lc.Intensity = 1.0f;
                        lc.Range = 10.0f;
                        lc.SyncToLight();
                        selectEntity(light);
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Create Cube")) {
                    if (currentScene) {
                        Entity e = CreateDefaultMeshEntity("Cube", MeshGenerator3D::CreateCube(1.0f));
                        selectEntity(e);
                    }
                }
                if (ImGui::MenuItem("Create Sphere")) {
                    if (currentScene) {
                        Entity e = CreateDefaultMeshEntity("Sphere", MeshGenerator3D::CreateSphere(0.5f, 32, 16));
                        selectEntity(e);
                    }
                }
                if (ImGui::MenuItem("Create Plane")) {
                    if (currentScene) {
                        Entity e = CreateDefaultMeshEntity("Plane", MeshGenerator3D::CreatePlane(5.0f, 5.0f));
                        selectEntity(e);
                    }
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("View")) {
                if (ImGui::MenuItem("Reset Camera")) {
                    if (m_ViewportPanel) {
                        m_ViewportPanel->GetEditorCamera().Reset();
                    }
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Window")) {
                if (ImGui::MenuItem("Reset Layout")) {
                    ResetDockLayout();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Profiler", nullptr, m_ProfilerWindow && m_ProfilerWindow->IsOpen())) {
                    if (m_ProfilerWindow) {
                        m_ProfilerWindow->Toggle();
                    }
                }
                if (ImGui::MenuItem("Graphics Settings", nullptr, m_GraphicsSettingsPanel && m_GraphicsSettingsPanel->IsOpen())) {
                    if (m_GraphicsSettingsPanel) {
                        m_GraphicsSettingsPanel->Toggle();
                    }
                }
                if (ImGui::MenuItem("Theme Editor", nullptr, m_ThemeEditorWindow && m_ThemeEditorWindow->IsOpen())) {
                    if (m_ThemeEditorWindow) {
                        m_ThemeEditorWindow->Toggle();
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Shader Assistant")) {
                    // Panel is always rendered, just use ImGui::Begin visibility
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("About")) {
                    // Show about dialog
                }
                ImGui::EndMenu();
            }
            
            ImGui::EndMenuBar();
        }
    }

    void EditorApp::RenderPanels() {
        // Render scene tabs
        RenderSceneTabs();
        
        // Render toolbar
        if (m_ToolbarPanel) {
            m_ToolbarPanel->OnImGuiRender();
        }
        
        // Render viewport
        if (m_ViewportPanel) {
            m_ViewportPanel->OnImGuiRender();
        }
        
        // Render hierarchy
        if (m_SceneHierarchyPanel) {
            m_SceneHierarchyPanel->OnImGuiRender();
        }
        
        // Render components window (replaces old inspector)
        if (m_ComponentsWindow) {
            m_ComponentsWindow->OnImGuiRender();
        }
        
        // Render console
        if (m_ConsolePanel) {
            m_ConsolePanel->OnImGuiRender();
        }
        
        // Render asset browser
        if (m_AssetBrowserPanel) {
            m_AssetBrowserPanel->OnImGuiRender();
        }
        
        // Render scripting console (can be toggled with HOME key)
        if (m_ScriptingConsolePanel) {
            m_ScriptingConsolePanel->OnImGuiRender();
        }
        
        // Render profiler
        if (m_ProfilerWindow) {
            m_ProfilerWindow->OnImGuiRender();
        }
        
        // Render graphics settings
        if (m_GraphicsSettingsPanel) {
            m_GraphicsSettingsPanel->OnImGuiRender();
        }

        // Render shader assistant
        if (m_ShaderAssistantPanel) {
            m_ShaderAssistantPanel->OnImGuiRender();
        }

        // Render preset browser
        if (m_PresetBrowserPanel) {
            m_PresetBrowserPanel->OnImGuiRender();
        }

        // Render new project dialog (modal)
        if (m_NewProjectDialog) {
            m_NewProjectDialog->OnImGuiRender();
        }
        if (m_WelcomePanel) {
            m_WelcomePanel->OnImGuiRender();
        }
    }

    EditorScene& EditorApp::GetCurrentEditorScene() {
        if (m_CurrentSceneIndex < 0 || m_CurrentSceneIndex >= static_cast<int>(m_Scenes.size())) {
            throw std::runtime_error("GetCurrentEditorScene: invalid scene index " + std::to_string(m_CurrentSceneIndex));
        }
        return *m_Scenes[m_CurrentSceneIndex].get();
    }

    const EditorScene& EditorApp::GetCurrentEditorScene() const {
        if (m_CurrentSceneIndex < 0 || m_CurrentSceneIndex >= static_cast<int>(m_Scenes.size())) {
            throw std::runtime_error("GetCurrentEditorScene: invalid scene index " + std::to_string(m_CurrentSceneIndex));
        }
        return *m_Scenes[m_CurrentSceneIndex].get();
    }

    Ref<Scene> EditorApp::GetCurrentScene() {
        if (m_CurrentSceneIndex < 0 || m_CurrentSceneIndex >= static_cast<int>(m_Scenes.size())) {
            return nullptr;
        }
        return m_Scenes[m_CurrentSceneIndex]->m_Scene;
    }

    Ref<Scene> EditorApp::GetCurrentScene() const {
        if (m_CurrentSceneIndex < 0 || m_CurrentSceneIndex >= static_cast<int>(m_Scenes.size())) {
            return nullptr;
        }
        return m_Scenes[m_CurrentSceneIndex]->m_Scene;
    }

    EditorScene* EditorApp::FindEditorSceneByID(uint64_t id) {
        for (auto& scene : m_Scenes) {
            if (scene->m_ID == id) {
                return scene.get();
            }
        }
        return nullptr;
    }

    void EditorApp::SetCurrentScene(int index) {
        if (index < 0 || index >= static_cast<int>(m_Scenes.size())) {
            return;
        }
        
        // Save current camera state before switching
        if (m_CurrentSceneIndex >= 0 && m_ViewportPanel) {
            auto& oldScene = GetCurrentEditorScene();
            auto& cam = m_ViewportPanel->GetEditorCamera();
            oldScene.m_CameraFocalPoint = cam.GetFocalPoint();
            oldScene.m_CameraDistance = cam.GetDistance();
            oldScene.m_CameraPitch = cam.GetPitch();
            oldScene.m_CameraYaw = cam.GetYaw();
        }
        
        m_CurrentSceneIndex = index;
        auto& editorScene = GetCurrentEditorScene();
        
        // Update panels with new scene
        if (m_SceneHierarchyPanel) {
            m_SceneHierarchyPanel->SetContext(editorScene.m_Scene);
            m_SceneHierarchyPanel->SetSelectedEntity({});
        }
        if (m_ViewportPanel) {
            m_ViewportPanel->SetContext(editorScene.m_Scene);
            m_ViewportPanel->SetSelectedEntity(Entity{});
            // Restore camera state
            auto& cam = m_ViewportPanel->GetEditorCamera();
            cam.SetFocalPoint(editorScene.m_CameraFocalPoint);
            cam.SetDistance(editorScene.m_CameraDistance);
            cam.SetPitch(editorScene.m_CameraPitch);
            cam.SetYaw(editorScene.m_CameraYaw);
        }
        if (m_EditorContext) {
            m_EditorContext->SetActiveScene(editorScene.m_Scene);
        }
    }

    bool EditorApp::CheckUnsavedChanges(int index) {
        int checkIndex = (index == -1) ? m_CurrentSceneIndex : index;
        if (checkIndex < 0 || checkIndex >= static_cast<int>(m_Scenes.size())) {
            return false;
        }
        
        auto& scene = *m_Scenes[checkIndex];
        return scene.m_HasUnsavedChanges;
    }

    void EditorApp::NewScene() {
        auto editorScene = std::make_unique<EditorScene>();
        editorScene->m_ID = m_NextSceneID++;
        editorScene->m_Path = "";
        editorScene->m_Scene = CreateRef<Scene>("Untitled Scene");
        editorScene->m_HasUnsavedChanges = false;

        m_Scenes.push_back(std::move(editorScene));
        SetCurrentScene(static_cast<int>(m_Scenes.size() - 1));

        // Populate after setting current so GetCurrentScene() returns the new scene
        PopulateDefaultScene();

        GE_CORE_INFO("New scene created");
    }

    void EditorApp::OpenScene() {
        std::vector<FileFilter> filters = {
            FileFilter("Scene Files", "*.scene"),
            FileFilter("JSON Files", "*.json")
        };
        
        auto filepath = FileDialog::OpenFile("Open Scene", filters, "Assets/Scenes");
        
        if (filepath.has_value()) {
            OpenSceneInNewTab(filepath.value());
        }
    }

    void EditorApp::OpenProject() {
        std::vector<FileFilter> filters = {
            FileFilter("GameEngine Project", "*.geproject"),
            FileFilter("All Files", "*.*")
        };
        auto filepath = FileDialog::OpenFile("Open Project", filters, "");
        if (!filepath.has_value() || filepath->empty()) return;
        ProjectFile proj;
        if (!ProjectFile::Load(*filepath, proj)) return;
        std::filesystem::path p(*filepath);
        std::string projectDir = p.parent_path().string();
        std::string assetsRoot = (std::filesystem::path(projectDir) / proj.AssetsRoot).string();
        if (m_AssetBrowserPanel) m_AssetBrowserPanel->SetAssetsDirectory(assetsRoot);
        if (!proj.DefaultScene.empty()) {
            std::string scenePath = (std::filesystem::path(projectDir) / proj.DefaultScene).string();
            if (FileSystem::Exists(scenePath)) OpenSceneInNewTab(scenePath);
        }
    }

    void EditorApp::OpenSceneInNewTab(const std::string& path) {
        Ref<Scene> newScene = CreateRef<Scene>();
        SceneSerializer serializer(newScene);
        
        if (serializer.Deserialize(path)) {
            auto editorScene = std::make_unique<EditorScene>();
            editorScene->m_ID = m_NextSceneID++;
            editorScene->m_Path = path;
            editorScene->m_Scene = newScene;
            editorScene->m_HasUnsavedChanges = false;
            
            // Extract scene name from path
            std::filesystem::path pathObj(path);
            std::string sceneName = pathObj.stem().string();
            newScene->SetName(sceneName);

            // Post-load: assign shaders to deserialized materials
            PostLoadFixupMaterials(newScene);

            m_Scenes.push_back(std::move(editorScene));
            SetCurrentScene(static_cast<int>(m_Scenes.size() - 1));

            GE_CORE_INFO("Scene loaded: {0}", path);
        } else {
            GE_CORE_ERROR("Failed to load scene: {0}", path);
        }
    }

    void EditorApp::SaveScene() {
        auto currentScene = GetCurrentScene();
        if (!currentScene) return;
        
        auto& editorScene = GetCurrentEditorScene();
        
        // If we already have a path, save directly
        if (!editorScene.m_Path.empty()) {
            SceneSerializer serializer(currentScene);
            if (serializer.Serialize(editorScene.m_Path)) {
                editorScene.m_HasUnsavedChanges = false;
                GE_CORE_INFO("Scene saved: {0}", editorScene.m_Path);
            } else {
                GE_CORE_ERROR("Failed to save scene: {0}", editorScene.m_Path);
            }
            return;
        }
        
        // Otherwise, show save dialog
        SaveSceneAs();
    }
    
    void EditorApp::SaveSceneAs() {
        auto currentScene = GetCurrentScene();
        if (!currentScene) return;
        
        std::vector<FileFilter> filters = {
            FileFilter("Scene Files", "*.scene")
        };
        
        std::string defaultName = currentScene->GetName() + ".scene";
        auto filepath = FileDialog::SaveFile("Save Scene", filters, "Assets/Scenes", defaultName);
        
        if (filepath.has_value()) {
            SceneSerializer serializer(currentScene);
            if (serializer.Serialize(filepath.value())) {
                auto& editorScene = GetCurrentEditorScene();
                editorScene.m_Path = filepath.value();
                editorScene.m_HasUnsavedChanges = false;
                
                // Extract scene name from path
                std::filesystem::path pathObj(filepath.value());
                std::string sceneName = pathObj.stem().string();
                currentScene->SetName(sceneName);
                
                GE_CORE_INFO("Scene saved: {0}", filepath.value());
            } else {
                GE_CORE_ERROR("Failed to save scene: {0}", filepath.value());
            }
        }
    }

    void EditorApp::CloseScene(int index) {
        if (index < 0 || index >= static_cast<int>(m_Scenes.size())) {
            return;
        }
        
        // Check for unsaved changes
        if (CheckUnsavedChanges(index)) {
            // TODO: Show save dialog
            // For now, just close without saving
        }
        
        // If closing current scene, switch to another
        if (index == m_CurrentSceneIndex) {
            if (m_Scenes.size() > 1) {
                // Switch to next scene, or previous if closing last
                int newIndex = (index < static_cast<int>(m_Scenes.size()) - 1) ? index : index - 1;
                m_Scenes.erase(m_Scenes.begin() + index);
                SetCurrentScene(newIndex);
            } else {
                // Last scene - create new empty one
                m_Scenes.erase(m_Scenes.begin());
                NewScene();
            }
        } else {
            m_Scenes.erase(m_Scenes.begin() + index);
            if (index < m_CurrentSceneIndex) {
                m_CurrentSceneIndex--;
            }
        }
    }
    
    // ==================== Play Mode (delegated to EditorContext) ====================
    
    void EditorApp::PlayScene() {
        if (m_EditorContext) m_EditorContext->Play();
    }
    
    void EditorApp::PauseScene() {
        if (m_EditorContext) m_EditorContext->Pause();
    }
    
    void EditorApp::ResumeScene() {
        if (m_EditorContext) m_EditorContext->Resume();
    }
    
    void EditorApp::StopScene() {
        if (m_EditorContext) m_EditorContext->Stop();
    }
    
    void EditorApp::StepFrame() {
        if (GetEditorState() != EditorState::Pause) return;
        m_StepRequested = true;
        GE_CORE_DEBUG("Step frame requested");
    }
    
    void EditorApp::SaveSceneState() {
        // No-op: EditorContext owns save/restore for play mode
    }
    
    void EditorApp::RestoreSceneState() {
        // No-op: EditorContext handles restore; callback syncs tab and panels
    }

}

// Entry point
GameEngine::Application* GameEngine::CreateApplication() {
    return new GameEngine::EditorApp();
}
