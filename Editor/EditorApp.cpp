#include "EditorApp.hpp"
#include "../Engine/Core/Logger.hpp"
#include "../Engine/Core/EntryPoint.hpp"
#include "../Engine/Scene/SceneSerializer.hpp"
#include "../Engine/Scene/Components/TransformComponent.hpp"
#include "../Engine/Scene/Components/MeshRendererComponent.hpp"
#include "../Engine/Scene/Components/LightComponent.hpp"
#include "../Engine/Scene/Components/CameraComponent.hpp"
#include "../Engine/Scene/Components/RigidBodyComponent.hpp"
#include "../Engine/Scene/Components/ColliderComponent.hpp"
#include "../Engine/Scene/Components/ClothComponent.hpp"
#include "../Engine/Scene/Components/FluidEmitterComponent.hpp"
#include "../Engine/Scene/Components/GPUParticleComponent.hpp"
#include "../Engine/Scene/Components/SoftBodyComponent.hpp"
#include "../Engine/Scene/Components/JointComponent.hpp"
#include "../Engine/Scene/Components/RobotArmComponent.hpp"
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
#include "Panels/AnimationPanel.hpp"
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

        // Debug drawing now requires a Vulkan render pass; the editor wires
        // it up later from the active render path. Skip the no-op default
        // init here to avoid the legacy GL-style call.

        // Create editor context
        m_EditorContext = CreateScope<EditorContext>();
        // NOTE: the definitive state-change callback is registered below after
        // all panels are constructed. This placeholder is intentionally removed.
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
                if (auto* es = TryGetCurrentEditorScene()) {
                    es->m_HasUnsavedChanges = true;
                }
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
        m_WelcomePanel->SetOnNewScene([this]() { m_WelcomePanel->SetVisible(false); NewScene(); });
        m_WelcomePanel->SetOnNewProject([this]() { if (m_NewProjectDialog) m_NewProjectDialog->Open(); });
        m_WelcomePanel->SetOnOpenProject([this]() { OpenProject(); });
        m_WelcomePanel->SetOnOpenScene([this]() { OpenScene(); });
        RegisterDemos();

        m_AnimationPanel = CreateScope<AnimationPanel>();
        m_AnimationPanel->SetContext(currentScene.m_Scene);

        // Initialize new integrated panels
        m_BuildPanel = CreateScope<BuildPanel>();
        m_BuildPanel->SetProjectPath(std::filesystem::current_path().string());

        m_CodeEditorPanel = CreateScope<CodeEditorPanel>();

        m_AIAssistantPanel = CreateScope<AIAssistantPanel>();

        m_MaterialEditorPanel = CreateScope<MaterialEditorPanel>();
        m_MaterialEditorPanel->SetApplyToSelectedCallback([this](Ref<Material> material) {
            if (!material) {
                GE_CORE_WARN("No material to apply");
                return;
            }
            Entity selected = m_SceneHierarchyPanel ? m_SceneHierarchyPanel->GetSelectedEntity() : Entity{};
            if (!selected) {
                GE_CORE_WARN("No entity selected to apply material to");
                return;
            }
            if (selected.HasComponent<MeshRendererComponent>()) {
                auto& meshRenderer = selected.GetComponent<MeshRendererComponent>();
                meshRenderer.SetMaterial(material);
                GE_CORE_INFO("Applied material '{}' to entity '{}'", 
                            material->GetName(), selected.GetTag());
                if (auto* es = TryGetCurrentEditorScene()) {
                    es->m_HasUnsavedChanges = true;
                }
            } else {
                GE_CORE_WARN("Selected entity does not have a MeshRendererComponent");
            }
        });

        m_ObjectSpawnerPanel = CreateScope<ObjectSpawnerPanel>();
        m_ObjectSpawnerPanel->SetScene(currentScene.m_Scene.get());
        m_ObjectSpawnerPanel->SetOnEntitySpawnedCallback([this](Entity e) {
            if (m_SceneHierarchyPanel) m_SceneHierarchyPanel->SetSelectedEntity(e);
            if (auto* es = TryGetCurrentEditorScene()) {
                es->m_HasUnsavedChanges = true;
            }
        });

        m_ProjectSettingsPanel = CreateScope<ProjectSettingsPanel>();

        // Initialize template and preset managers
        TemplateManager::Init("Content/Templates");
        PresetManager::Init("Content/Presets");

        // Set up state change callback to update scene references after stop/restore
        m_EditorContext->SetStateChangeCallback([this](EditorState oldState, EditorState newState) {
            if (newState == EditorState::Edit && oldState != EditorState::Edit) {
                // EditorContext::RestoreSceneState() replaces m_ActiveScene with the
                // restored copy. Pull that back into m_Scenes so all panels see it.
                Ref<Scene> restored = m_EditorContext->GetActiveScene();
                if (restored && m_CurrentSceneIndex >= 0 && m_CurrentSceneIndex < static_cast<int>(m_Scenes.size())) {
                    m_Scenes[m_CurrentSceneIndex]->m_Scene = restored;
                }
                auto currentScene = GetCurrentScene();
                if (currentScene && m_SceneHierarchyPanel) {
                    m_SceneHierarchyPanel->SetContext(currentScene);
                    m_SceneHierarchyPanel->SetSelectedEntity({});
                }
                if (currentScene && m_ViewportPanel) {
                    m_ViewportPanel->SetContext(currentScene);
                }
                if (currentScene && m_AnimationPanel) {
                    m_AnimationPanel->SetContext(currentScene);
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

        m_DefaultMaterial = CreateRef<Material>();
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
        // Vulkan materials carry pipeline references — there's no Shader
        // object to fix up post-load. The renderer assigns the default
        // pipeline at draw time when a material has none set.
        (void)scene;
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

                Entity entity = currentScene->CreateEntity(entityName);
                // TransformComponent already added by CreateEntity
                entity.AddComponent<MeshRendererComponent>(mesh, material);

                if (m_SceneHierarchyPanel) {
                    m_SceneHierarchyPanel->SetSelectedEntity(entity);
                }

                if (auto* es = TryGetCurrentEditorScene()) {
                    es->m_HasUnsavedChanges = true;
                }

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
        // Execute deferred actions from previous frame (e.g. demo loads triggered by UI clicks)
        if (m_WelcomePanel) {
            auto pendingAction = m_WelcomePanel->TakePendingAction();
            if (pendingAction) {
                m_DeferredActions.push_back(std::move(pendingAction));
            }
        }
        if (!m_DeferredActions.empty()) {
            auto actions = std::move(m_DeferredActions);
            m_DeferredActions.clear();
            for (auto& action : actions) {
                action();
            }
        }

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

        // Tell the viewport panel whether we are in play/pause so it can switch cameras
        if (m_ViewportPanel) {
            m_ViewportPanel->SetPlayMode(editorState == EditorState::Play
                                      || editorState == EditorState::Pause);
        }
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
        if (m_ViewportPanel && HasValidCurrentScene()) {
            // Before update: push hierarchy selection to viewport
            if (m_SceneHierarchyPanel) {
                m_ViewportPanel->SetSelectedEntity(m_SceneHierarchyPanel->GetSelectedEntity());
            }

            m_ViewportPanel->OnUpdate(deltaTime);

            // Save camera state to current scene
            if (auto* editorScene = TryGetCurrentEditorScene()) {
                auto& cam = m_ViewportPanel->GetEditorCamera();
                editorScene->m_CameraFocalPoint = cam.GetFocalPoint();
                editorScene->m_CameraDistance = cam.GetDistance();
                editorScene->m_CameraPitch = cam.GetPitch();
                editorScene->m_CameraYaw = cam.GetYaw();
            }

            // After update: if viewport picked a different entity, push back to hierarchy
            if (m_SceneHierarchyPanel) {
                Entity viewportSel = m_ViewportPanel->GetSelectedEntity();
                Entity hierarchySel = m_SceneHierarchyPanel->GetSelectedEntity();
                if (viewportSel != hierarchySel) {
                    m_SceneHierarchyPanel->SetSelectedEntity(viewportSel);
                }
            }
        }

            // Update components window and animation panel with current selection
        if (m_SceneHierarchyPanel) {
            Entity selected = m_SceneHierarchyPanel->GetSelectedEntity();
            if (m_ComponentsWindow) m_ComponentsWindow->SetSelectedEntity(selected);
            if (m_AnimationPanel)   m_AnimationPanel->SetSelectedEntity(selected);
        }

        // Auto-save every 5 minutes (only when editing, scene has path and has unsaved changes)
        m_AutoSaveTimer += deltaTime;
        if (m_AutoSaveTimer >= AUTO_SAVE_INTERVAL) {
            m_AutoSaveTimer = 0.0f;
            Ref<Scene> scene = GetCurrentScene();
            auto* es = TryGetCurrentEditorScene();
            if (scene && es && m_EditorContext && m_EditorContext->IsEditing()) {
                if (es->m_HasUnsavedChanges && !es->m_Path.empty()) {
                    std::string autosavePath = std::filesystem::path(es->m_Path).parent_path().string() + "/.autosave.scene";
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
        m_AnimationPanel.reset();
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

        // ── Unity-style layout ───────────────────────────────────────────────
        //
        //  ┌──────────────────────────────────────────────────────────────────┐
        //  │  Menu bar  (inside DockSpace window)                             │
        //  ├───────────────────────────┬──────────────────────────────────────┤
        //  │  Scene Tabs               │  ▶ ‖ ■  Toolbar (play/pause/stop)   │
        //  ├───────────┬───────────────┴──────────────────────┬───────────────┤
        //  │           │                                       │               │
        //  │ Hierarchy │            Viewport                   │  Components   │
        //  │           │                                       │  (Inspector)  │
        //  │           ├─────────────────────┬─────────────────┤  tabbed with  │
        //  │           │  Console / Script    │  Asset Browser  │  Graphics     │
        //  │           │  (tabbed)            │  / Profiler     │  Theme Ed.    │
        //  └───────────┴─────────────────────┴─────────────────┴───────────────┘

        // 1. Top bar: Scene Tabs (left 55%) | Toolbar play controls (right 45%)
        ImGuiID dock_topbar, dock_body;
        ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Up, 0.045f, &dock_topbar, &dock_body);

        ImGuiID dock_scene_tabs, dock_toolbar;
        ImGui::DockBuilderSplitNode(dock_topbar, ImGuiDir_Left, 0.55f, &dock_scene_tabs, &dock_toolbar);

        // 2. Left column: Scene Hierarchy (17%)
        ImGuiID dock_hierarchy, dock_center_right;
        ImGui::DockBuilderSplitNode(dock_body, ImGuiDir_Left, 0.17f, &dock_hierarchy, &dock_center_right);

        // 3. Right column: Inspector / Components (25% of remaining width)
        ImGuiID dock_inspector, dock_center;
        ImGui::DockBuilderSplitNode(dock_center_right, ImGuiDir_Right, 0.25f, &dock_inspector, &dock_center);

        // 4. Center: Viewport (top 76%) | bottom panels (24%)
        ImGuiID dock_viewport, dock_bottom;
        ImGui::DockBuilderSplitNode(dock_center, ImGuiDir_Down, 0.24f, &dock_bottom, &dock_viewport);

        // 5. Bottom strip: Console area (left 50%) | Asset/utility area (right 50%)
        ImGuiID dock_console, dock_assets;
        ImGui::DockBuilderSplitNode(dock_bottom, ImGuiDir_Left, 0.50f, &dock_console, &dock_assets);

        // 6. Hide the tab bar on the two top-bar nodes so they look like a
        //    clean toolbar strip rather than tabbed panels.
        if (ImGuiDockNode* n = ImGui::DockBuilderGetNode(dock_scene_tabs))
            n->LocalFlags |= ImGuiDockNodeFlags_AutoHideTabBar;
        if (ImGuiDockNode* n = ImGui::DockBuilderGetNode(dock_toolbar))
            n->LocalFlags |= ImGuiDockNodeFlags_AutoHideTabBar;

        // 7. Assign windows to nodes
        ImGui::DockBuilderDockWindow("Scene Tabs",        dock_scene_tabs);
        ImGui::DockBuilderDockWindow("##toolbar",         dock_toolbar);

        ImGui::DockBuilderDockWindow("Scene Hierarchy",   dock_hierarchy);

        // Inspector column — Components is the primary tab
        ImGui::DockBuilderDockWindow("Components",        dock_inspector);
        ImGui::DockBuilderDockWindow("Graphics Settings", dock_inspector);
        ImGui::DockBuilderDockWindow("Theme Editor",      dock_inspector);

        ImGui::DockBuilderDockWindow("Viewport",          dock_viewport);

        // Console area (tabbed) — Animation shares this strip
        ImGui::DockBuilderDockWindow("Console",           dock_console);
        ImGui::DockBuilderDockWindow("Scripting Console", dock_console);
        ImGui::DockBuilderDockWindow("Animation",         dock_console);

        // Asset / utility area (tabbed)
        ImGui::DockBuilderDockWindow("Asset Browser",     dock_assets);
        ImGui::DockBuilderDockWindow("Profiler",          dock_assets);

        // New panels - docked into existing regions
        ImGui::DockBuilderDockWindow("Object Spawner",    dock_hierarchy);   // Left with Hierarchy
        ImGui::DockBuilderDockWindow("Code Editor",       dock_viewport);    // Center with Viewport
        ImGui::DockBuilderDockWindow("Build",             dock_console);     // Bottom-left with Console
        ImGui::DockBuilderDockWindow("Material Editor",   dock_inspector);   // Right with Components
        ImGui::DockBuilderDockWindow("AI Assistant",      dock_inspector);   // Right with Components
        ImGui::DockBuilderDockWindow("Project Settings",  dock_inspector);   // Right with Components

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
                    if (auto* es = TryGetCurrentEditorScene()) {
                        es->m_HasUnsavedChanges = true;
                    }
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
            
            if (ImGui::BeginMenu("Demos")) {
                ImGui::TextDisabled("Load a pre-built demo scene");
                ImGui::Separator();
                ImGui::TextDisabled("-- Physics --");
                if (ImGui::MenuItem("Physics Simulation"))    m_DeferredActions.push_back([this]() { LoadPhysicsDemo(); });
                if (ImGui::MenuItem("Cloth Simulation"))      m_DeferredActions.push_back([this]() { LoadClothDemo(); });
                if (ImGui::MenuItem("SPH Water"))             m_DeferredActions.push_back([this]() { LoadSPHWaterDemo(); });
                if (ImGui::MenuItem("Soft Body (XPBD)"))      m_DeferredActions.push_back([this]() { LoadSoftBodyDemo(); });
                if (ImGui::MenuItem("Joints & Constraints"))  m_DeferredActions.push_back([this]() { LoadJointsDemo(); });
                if (ImGui::MenuItem("Character Demo"))        m_DeferredActions.push_back([this]() { LoadCharacterDemo(); });
                if (ImGui::MenuItem("Cannon Shooting"))       m_DeferredActions.push_back([this]() { LoadCannonShootingDemo(); });
                if (ImGui::MenuItem("Spring-Block Simulator")) m_DeferredActions.push_back([this]() { LoadSpringBlockDemo(); });
                if (ImGui::MenuItem("Particle System"))       m_DeferredActions.push_back([this]() { LoadParticleSystemDemo(); });
                ImGui::Separator();
                ImGui::TextDisabled("-- Visual --");
                if (ImGui::MenuItem("Lighting Showcase"))     m_DeferredActions.push_back([this]() { LoadLightingDemo(); });
                if (ImGui::MenuItem("GPU Particles"))         m_DeferredActions.push_back([this]() { LoadGPUParticleDemo(); });
                if (ImGui::MenuItem("Post-Processing"))       m_DeferredActions.push_back([this]() { LoadPostProcessingDemo(); });
                if (ImGui::MenuItem("Procedural Terrain"))    m_DeferredActions.push_back([this]() { LoadTerrainDemo(); });
                ImGui::Separator();
                ImGui::TextDisabled("-- 2D --");
                if (ImGui::MenuItem("2D Physics Playground")) m_DeferredActions.push_back([this]() { Load2DPhysicsDemo(); });
                ImGui::Separator();
                ImGui::TextDisabled("-- Robotics --");
                if (ImGui::MenuItem("Robot Arm (IK)"))        m_DeferredActions.push_back([this]() { LoadRobotArmDemo(); });
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
                if (ImGui::MenuItem("Shader Assistant", nullptr, m_ShaderAssistantPanel && m_ShaderAssistantPanel->IsOpen())) {
                    if (m_ShaderAssistantPanel) m_ShaderAssistantPanel->Toggle();
                }
                ImGui::Separator();
                ImGui::TextDisabled("-- Tools --");
                if (ImGui::MenuItem("Build", nullptr, m_BuildPanel && m_BuildPanel->IsOpen())) {
                    if (m_BuildPanel) m_BuildPanel->Toggle();
                }
                if (ImGui::MenuItem("Code Editor", nullptr, m_CodeEditorPanel && m_CodeEditorPanel->IsOpen())) {
                    if (m_CodeEditorPanel) m_CodeEditorPanel->Toggle();
                }
                if (ImGui::MenuItem("AI Assistant", nullptr, m_AIAssistantPanel && m_AIAssistantPanel->IsOpen())) {
                    if (m_AIAssistantPanel) m_AIAssistantPanel->Toggle();
                }
                ImGui::Separator();
                ImGui::TextDisabled("-- Content --");
                if (ImGui::MenuItem("Material Editor", nullptr, m_MaterialEditorPanel && m_MaterialEditorPanel->IsOpen())) {
                    if (m_MaterialEditorPanel) m_MaterialEditorPanel->Toggle();
                }
                if (ImGui::MenuItem("Object Spawner", nullptr, m_ObjectSpawnerPanel && m_ObjectSpawnerPanel->IsOpen())) {
                    if (m_ObjectSpawnerPanel) m_ObjectSpawnerPanel->Toggle();
                }
                if (ImGui::MenuItem("Project Settings", nullptr, m_ProjectSettingsPanel && m_ProjectSettingsPanel->IsOpen())) {
                    if (m_ProjectSettingsPanel) m_ProjectSettingsPanel->Toggle();
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

        if (m_AnimationPanel) {
            m_AnimationPanel->OnImGuiRender();
        }

        // Render new integrated panels
        if (m_BuildPanel) {
            m_BuildPanel->OnImGuiRender();
        }
        if (m_CodeEditorPanel) {
            m_CodeEditorPanel->OnImGuiRender();
        }
        if (m_AIAssistantPanel) {
            m_AIAssistantPanel->OnImGuiRender();
        }
        if (m_MaterialEditorPanel) {
            m_MaterialEditorPanel->OnImGuiRender();
        }
        if (m_ObjectSpawnerPanel) {
            m_ObjectSpawnerPanel->OnImGuiRender();
        }
        if (m_ProjectSettingsPanel) {
            m_ProjectSettingsPanel->OnImGuiRender();
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

    EditorScene* EditorApp::TryGetCurrentEditorScene() {
        if (m_CurrentSceneIndex < 0 || m_CurrentSceneIndex >= static_cast<int>(m_Scenes.size())) {
            return nullptr;
        }
        return m_Scenes[m_CurrentSceneIndex].get();
    }

    const EditorScene* EditorApp::TryGetCurrentEditorScene() const {
        if (m_CurrentSceneIndex < 0 || m_CurrentSceneIndex >= static_cast<int>(m_Scenes.size())) {
            return nullptr;
        }
        return m_Scenes[m_CurrentSceneIndex].get();
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
        if (m_ViewportPanel) {
            if (auto* oldScene = TryGetCurrentEditorScene()) {
                auto& cam = m_ViewportPanel->GetEditorCamera();
                oldScene->m_CameraFocalPoint = cam.GetFocalPoint();
                oldScene->m_CameraDistance = cam.GetDistance();
                oldScene->m_CameraPitch = cam.GetPitch();
                oldScene->m_CameraYaw = cam.GetYaw();
            }
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
        if (m_AnimationPanel) {
            m_AnimationPanel->SetContext(editorScene.m_Scene);
            m_AnimationPanel->SetSelectedEntity({});
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
        // If currently playing or paused, stop cleanly before switching scenes.
        if (m_EditorContext && !m_EditorContext->IsEditing()) {
            m_EditorContext->Stop();
        }

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
        // Stop play/pause before loading a scene into a new tab.
        if (m_EditorContext && !m_EditorContext->IsEditing()) {
            m_EditorContext->Stop();
        }

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
                m_CurrentSceneIndex = -1; // Mark as invalid before erasing
                m_Scenes.erase(m_Scenes.begin());
                NewScene();
            }
        } else {
            m_Scenes.erase(m_Scenes.begin() + index);
            if (index < m_CurrentSceneIndex) {
                m_CurrentSceneIndex--;
            }
        }
        
        // Sanity check: ensure index is valid after operation
        if (!m_Scenes.empty() && m_CurrentSceneIndex >= static_cast<int>(m_Scenes.size())) {
            GE_CORE_WARN("CloseScene: Correcting invalid scene index {} to {}", m_CurrentSceneIndex, m_Scenes.size() - 1);
            m_CurrentSceneIndex = static_cast<int>(m_Scenes.size()) - 1;
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

    // ==================== Demo Scenes ====================

    void EditorApp::LoadPhysicsDemo() {
        if (m_EditorContext && !m_EditorContext->IsEditing()) m_EditorContext->Stop();

        auto editorScene = std::make_unique<EditorScene>();
        editorScene->m_ID = m_NextSceneID++;
        editorScene->m_Path = "";
        editorScene->m_Scene = CreateRef<Scene>("Physics Demo");
        editorScene->m_HasUnsavedChanges = true;
        m_Scenes.push_back(std::move(editorScene));
        SetCurrentScene(static_cast<int>(m_Scenes.size() - 1));

        auto scene = GetCurrentScene();
        if (!scene) return;
        if (!m_DefaultShader || !m_DefaultMaterial) return;

        // === Checkerboard ground (like old_code/arm-simulation) ===
        // Creates a 50x50 grid of alternating light/dark squares
        float squareSize = 0.4f;
        int gridSize = 25;
        for (int i = 0; i < gridSize; i++) {
            for (int j = 0; j < gridSize; j++) {
                bool isLight = (i + j) % 2 == 0;
                glm::vec3 color = isLight ? glm::vec3(0.8f, 0.8f, 0.8f) : glm::vec3(0.3f, 0.3f, 0.3f);
                
                auto tileMesh = MeshGenerator3D::CreatePlane(squareSize, squareSize);
                std::string name = "Tile_" + std::to_string(i) + "_" + std::to_string(j);
                Entity tile = scene->CreateEntity(name);
                auto tMat = CreateRef<Material>(*m_DefaultMaterial);
                tMat->SetAlbedo(color);
                tMat->SetRoughness(0.6f);
                tMat->SetMetallic(0.1f);
                tile.AddComponent<MeshRendererComponent>(tileMesh, tMat);
                auto& tf = tile.GetComponent<TransformComponent>();
                tf.Position = glm::vec3(
                    (i - gridSize/2) * squareSize,
                    -0.5f,
                    (j - gridSize/2) * squareSize
                );
            }
        }
        
        // Ground collision plane
        Entity groundCollider = scene->CreateEntity("Ground Collider");
        groundCollider.GetComponent<TransformComponent>().Position = glm::vec3(0.0f, -0.5f, 0.0f);
        auto& gRb = groundCollider.AddComponent<RigidBodyComponent>();
        gRb.Type = RigidBodyComponent::BodyType::Static;
        groundCollider.AddComponent<ColliderComponent>(CreateRef<Physics::BoxShape>(glm::vec3(5.0f, 0.1f, 5.0f)));

        // Stack of colourful cubes (3x3 tower)
        glm::vec3 cubeColors[] = {
            {0.9f,0.2f,0.2f},{0.2f,0.7f,0.9f},{0.9f,0.8f,0.2f},
            {0.8f,0.3f,0.9f},{0.2f,0.9f,0.4f},{0.9f,0.5f,0.1f},
            {0.5f,0.9f,0.9f},{0.9f,0.3f,0.5f},{0.3f,0.5f,0.9f}
        };
        for (int y = 0; y < 3; y++) {
            for (int x = 0; x < 3; x++) {
                auto cubeMesh = MeshGenerator3D::CreateCube(0.9f);
                std::string name = "Cube_" + std::to_string(y * 3 + x);
                Entity cube = scene->CreateEntity(name);
                auto cMat = CreateRef<Material>(*m_DefaultMaterial);
                cMat->SetAlbedo(cubeColors[y * 3 + x]);
                cMat->SetRoughness(0.4f);
                cube.AddComponent<MeshRendererComponent>(cubeMesh, cMat);
                auto& tf = cube.GetComponent<TransformComponent>();
                tf.Position = glm::vec3((x - 1) * 1.1f, y * 1.0f + 0.5f, 0.0f);
                auto& cRb = cube.AddComponent<RigidBodyComponent>();
                cRb.Type = RigidBodyComponent::BodyType::Dynamic;
                cRb.Mass = 1.0f;
                cube.AddComponent<ColliderComponent>(CreateRef<Physics::BoxShape>(glm::vec3(0.45f)));
            }
        }

        // Green transparent sphere (like arm-simulation's sphere)
        auto sphereMesh = MeshGenerator3D::CreateSphere(0.4f, 16, 16);
        Entity sphere = scene->CreateEntity("Green Sphere");
        auto sMat = CreateRef<Material>(*m_DefaultMaterial);
        sMat->SetAlbedo(glm::vec3(0.0f, 0.5f, 0.0f));  // Green
        sMat->SetRoughness(0.3f);
        sMat->SetMetallic(0.1f);
        sphere.AddComponent<MeshRendererComponent>(sphereMesh, sMat);
        auto& sTf = sphere.GetComponent<TransformComponent>();
        sTf.Position = glm::vec3(0.0f, 0.4f, 0.0f);
        auto& sRb = sphere.AddComponent<RigidBodyComponent>();
        sRb.Type = RigidBodyComponent::BodyType::Dynamic;
        sRb.Mass = 0.5f;
        sphere.AddComponent<ColliderComponent>(CreateRef<Physics::SphereShape>(0.4f));

        // Ramp
        auto rampMesh = MeshGenerator3D::CreatePlane(4.0f, 6.0f);
        Entity ramp = scene->CreateEntity("Ramp");
        auto rMat = CreateRef<Material>(*m_DefaultMaterial);
        rMat->SetAlbedo(glm::vec3(0.7f, 0.6f, 0.5f));
        ramp.AddComponent<MeshRendererComponent>(rampMesh, rMat);
        auto& rTf = ramp.GetComponent<TransformComponent>();
        rTf.Position = glm::vec3(4.0f, 1.0f, 0.0f);
        rTf.Rotation = glm::quat(glm::radians(glm::vec3(0.0f, 0.0f, -25.0f)));

        // Moving light (like arm-simulation's rotating light)
        Entity light = scene->CreateEntity("Moving Light");
        auto& lTf = light.GetComponent<TransformComponent>();
        lTf.Position = glm::vec3(6.0f, 3.5f, 6.0f);  // Start position
        auto lightMesh = MeshGenerator3D::CreateCube(0.2f);
        auto lightMat = CreateRef<Material>(*m_DefaultMaterial);
        lightMat->SetAlbedo(glm::vec3(1.0f, 1.0f, 1.0f));  // White cube marker
        light.AddComponent<MeshRendererComponent>(lightMesh, lightMat);
        auto& lc = light.AddComponent<LightComponent>(LightType::Point);
        lc.Intensity = 3.0f;
        lc.Color = glm::vec3(1.0f, 1.0f, 1.0f);
        lc.Range = 15.0f;
        lc.SyncToLight();

        // Camera (like arm-simulation's initial view)
        Entity cam = scene->CreateEntity("Camera");
        auto& camTf = cam.GetComponent<TransformComponent>();
        camTf.Position = glm::vec3(6.0f, 4.0f, 6.0f);
        camTf.Rotation = glm::quat(glm::radians(glm::vec3(-25.0f, 45.0f, 0.0f)));
        auto& cc = cam.AddComponent<CameraComponent>();
        cc.IsMainCamera = true;
        cc.FOV = 60.0f;

        if (m_SceneHierarchyPanel) m_SceneHierarchyPanel->SetSelectedEntity({});
        GE_CORE_INFO("Loaded Physics Demo");
    }

    void EditorApp::LoadLightingDemo() {
        if (m_EditorContext && !m_EditorContext->IsEditing()) m_EditorContext->Stop();

        auto editorScene = std::make_unique<EditorScene>();
        editorScene->m_ID = m_NextSceneID++;
        editorScene->m_Path = "";
        editorScene->m_Scene = CreateRef<Scene>("Lighting Demo");
        editorScene->m_HasUnsavedChanges = true;
        m_Scenes.push_back(std::move(editorScene));
        SetCurrentScene(static_cast<int>(m_Scenes.size() - 1));

        auto scene = GetCurrentScene();
        if (!scene || !m_DefaultShader || !m_DefaultMaterial) return;

        // Large ground
        auto groundMesh = MeshGenerator3D::CreatePlane(20.0f, 20.0f);
        Entity ground = scene->CreateEntity("Floor");
        auto gMat = CreateRef<Material>(*m_DefaultMaterial);
        gMat->SetAlbedo(glm::vec3(0.15f));
        gMat->SetRoughness(0.1f);
        gMat->SetMetallic(0.8f);
        ground.AddComponent<MeshRendererComponent>(groundMesh, gMat);
        ground.GetComponent<TransformComponent>().Position = glm::vec3(0, -0.5f, 0);

        // Central sphere
        Entity sphere = scene->CreateEntity("Chrome Sphere");
        auto sMat = CreateRef<Material>(*m_DefaultMaterial);
        sMat->SetAlbedo(glm::vec3(0.9f));
        sMat->SetMetallic(1.0f);
        sMat->SetRoughness(0.05f);
        sphere.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreateSphere(1.0f, 48, 24), sMat);
        sphere.GetComponent<TransformComponent>().Position = glm::vec3(0, 0.5f, 0);

        // Surrounding cubes on pedestals
        struct CubeInfo { glm::vec3 pos; glm::vec3 color; float metal; float rough; };
        CubeInfo cubes[] = {
            {{ 3.5f, 0.4f,  0.0f}, {1.0f,0.2f,0.2f}, 0.0f, 0.7f},
            {{-3.5f, 0.4f,  0.0f}, {0.2f,0.5f,1.0f}, 0.0f, 0.5f},
            {{ 0.0f, 0.4f,  3.5f}, {0.2f,1.0f,0.4f}, 0.8f, 0.2f},
            {{ 0.0f, 0.4f, -3.5f}, {1.0f,0.8f,0.1f}, 0.1f, 0.9f},
        };
        for (auto& ci : cubes) {
            Entity e = scene->CreateEntity("Pedestal Object");
            auto mat = CreateRef<Material>(*m_DefaultMaterial);
            mat->SetAlbedo(ci.color);
            mat->SetMetallic(ci.metal);
            mat->SetRoughness(ci.rough);
            e.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreateCube(0.7f), mat);
            e.GetComponent<TransformComponent>().Position = ci.pos;
        }

        // Coloured point lights circling the scene
        struct LightInfo { glm::vec3 pos; glm::vec3 color; float intensity; };
        LightInfo lights[] = {
            {{ 4.0f, 2.5f,  0.0f}, {1.0f,0.2f,0.2f}, 4.0f},
            {{-4.0f, 2.5f,  0.0f}, {0.2f,0.5f,1.0f}, 4.0f},
            {{ 0.0f, 2.5f,  4.0f}, {0.2f,1.0f,0.3f}, 4.0f},
            {{ 0.0f, 2.5f, -4.0f}, {1.0f,0.8f,0.1f}, 4.0f},
        };
        for (auto& li : lights) {
            Entity e = scene->CreateEntity("Point Light");
            auto& tf = e.GetComponent<TransformComponent>();
            tf.Position = li.pos;
            auto& lc = e.AddComponent<LightComponent>(LightType::Point);
            lc.Color = li.color;
            lc.Intensity = li.intensity;
            lc.Range = 10.0f;
            lc.SyncToLight();
        }

        // Overhead directional fill light (soft)
        Entity sun = scene->CreateEntity("Fill Light");
        sun.GetComponent<TransformComponent>().Rotation =
            glm::quat(glm::radians(glm::vec3(-60.0f, 20.0f, 0.0f)));
        auto& slc = sun.AddComponent<LightComponent>(LightType::Directional);
        slc.Intensity = 0.5f;
        slc.Color = glm::vec3(0.8f, 0.85f, 1.0f);
        slc.SyncToLight();

        // Camera for demo
        Entity cam = scene->CreateEntity("Demo Camera");
        cam.GetComponent<TransformComponent>().Position = glm::vec3(6.0f, 4.0f, 6.0f);
        cam.GetComponent<TransformComponent>().Rotation =
            glm::quat(glm::radians(glm::vec3(-25.0f, 45.0f, 0.0f)));
        auto& cc = cam.AddComponent<CameraComponent>();
        cc.IsMainCamera = true;
        cc.FOV = 50.0f;

        if (m_SceneHierarchyPanel) m_SceneHierarchyPanel->SetSelectedEntity({});
        GE_CORE_INFO("Loaded Lighting Demo");
    }

    // ==================== Cloth Simulation ====================

    void EditorApp::LoadClothDemo() {
        if (m_EditorContext && !m_EditorContext->IsEditing()) m_EditorContext->Stop();

        auto editorScene = std::make_unique<EditorScene>();
        editorScene->m_ID = m_NextSceneID++;
        editorScene->m_Scene = CreateRef<Scene>("Cloth Simulation");
        editorScene->m_HasUnsavedChanges = true;
        m_Scenes.push_back(std::move(editorScene));
        SetCurrentScene(static_cast<int>(m_Scenes.size() - 1));

        auto scene = GetCurrentScene();
        if (!scene || !m_DefaultMaterial) return;

        // Ground
        Entity ground = scene->CreateEntity("Ground");
        auto gMat = CreateRef<Material>(*m_DefaultMaterial);
        gMat->SetAlbedo(glm::vec3(0.30f, 0.45f, 0.25f));
        gMat->SetRoughness(0.9f);
        ground.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreatePlane(12.0f, 12.0f), gMat);
        ground.GetComponent<TransformComponent>().Position = glm::vec3(0, -2.0f, 0);

        // Flagpole
        Entity pole = scene->CreateEntity("Flagpole");
        auto poleMat = CreateRef<Material>(*m_DefaultMaterial);
        poleMat->SetAlbedo(glm::vec3(0.55f, 0.35f, 0.15f));
        poleMat->SetRoughness(0.8f);
        pole.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreateCylinder(0.06f, 5.0f, 12), poleMat);
        pole.GetComponent<TransformComponent>().Position = glm::vec3(-1.5f, 0.5f, 0.0f);

        // Cloth flag — pinned on left edge, wind blowing right
        // resolutionX=20, resolutionY=14 → CreatePlane subdivisions=(resX-1, resY-1)=(19,13)
        // so vertex count = 20*14 = 280 == particle count
        Entity flag = scene->CreateEntity("Cloth Flag");
        flag.GetComponent<TransformComponent>().Position = glm::vec3(-1.4f, 1.8f, 0.0f);
        auto flagMesh = MeshGenerator3D::CreatePlane(3.0f, 2.0f, 19, 13);
        auto flagMat  = CreateRef<Material>(*m_DefaultMaterial);
        flagMat->SetAlbedo(glm::vec3(0.85f, 0.20f, 0.15f));
        flagMat->SetRoughness(0.8f);
        flag.AddComponent<MeshRendererComponent>(flagMesh, flagMat);
        auto& flagCloth = flag.AddComponent<ClothComponent>();
        flagCloth.Initialize(3.0f, 2.0f, 20, 14);
        flagCloth.SetMesh(flagMesh);   // ClothComponent drives this mesh's vertices
        flagCloth.PinTopLeft     = true;
        flagCloth.PinTopRight    = false;
        flagCloth.PinBottomLeft  = true;
        flagCloth.PinBottomRight = false;
        flagCloth.WindEnabled    = true;
        flagCloth.WindDirection  = glm::vec3(1.0f, 0.0f, 0.0f);
        flagCloth.WindStrength   = 2.5f;
        flagCloth.Stiffness      = 45.0f;
        flagCloth.Damping        = 0.97f;
        flagCloth.Mass           = 0.5f;

        // Free cloth — no pins, falls and drapes
        // resolutionX=15, resolutionY=15 → subdivisions=(14,14), vertices=15*15=225
        Entity cloth2 = scene->CreateEntity("Free Cloth");
        cloth2.GetComponent<TransformComponent>().Position = glm::vec3(3.0f, 2.5f, 0.0f);
        auto cloth2Mesh = MeshGenerator3D::CreatePlane(2.0f, 2.0f, 14, 14);
        auto cloth2Mat  = CreateRef<Material>(*m_DefaultMaterial);
        cloth2Mat->SetAlbedo(glm::vec3(0.25f, 0.50f, 0.85f));
        cloth2Mat->SetRoughness(0.85f);
        cloth2.AddComponent<MeshRendererComponent>(cloth2Mesh, cloth2Mat);
        auto& c2 = cloth2.AddComponent<ClothComponent>();
        c2.Initialize(2.0f, 2.0f, 15, 15);
        c2.SetMesh(cloth2Mesh);        // ClothComponent drives this mesh's vertices
        c2.PinTopLeft  = false;
        c2.PinTopRight = false;
        c2.WindEnabled = false;
        c2.Stiffness   = 60.0f;
        c2.Mass        = 1.0f;

        // 3 colored sphere obstacles (matching 02-spring-cloth-simulation/flag.cpp)
        // Sphere 1: pos (0,-3,2), radius 2.0, RED (1,0,0)
        Entity sphere1 = scene->CreateEntity("Sphere Red");
        auto s1Mat = CreateRef<Material>(*m_DefaultMaterial);
        s1Mat->SetAlbedo(glm::vec3(1.0f, 0.0f, 0.0f));
        s1Mat->SetMetallic(0.3f);
        s1Mat->SetRoughness(0.4f);
        sphere1.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreateSphere(1.0f, 24, 12), s1Mat);
        sphere1.GetComponent<TransformComponent>().Position = glm::vec3(0.0f, -1.0f, 1.5f);
        auto& s1Rb = sphere1.AddComponent<RigidBodyComponent>();
        s1Rb.Type = RigidBodyComponent::BodyType::Static;
        sphere1.AddComponent<ColliderComponent>(CreateRef<Physics::SphereShape>(1.0f));

        // Sphere 2: pos (1.5,-3.5,0.7), radius 1.5, YELLOW (1,1,0)
        Entity sphere2 = scene->CreateEntity("Sphere Yellow");
        auto s2Mat = CreateRef<Material>(*m_DefaultMaterial);
        s2Mat->SetAlbedo(glm::vec3(1.0f, 1.0f, 0.0f));
        s2Mat->SetMetallic(0.3f);
        s2Mat->SetRoughness(0.4f);
        sphere2.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreateSphere(0.75f, 24, 12), s2Mat);
        sphere2.GetComponent<TransformComponent>().Position = glm::vec3(1.5f, -1.5f, 0.7f);
        auto& s2Rb = sphere2.AddComponent<RigidBodyComponent>();
        s2Rb.Type = RigidBodyComponent::BodyType::Static;
        sphere2.AddComponent<ColliderComponent>(CreateRef<Physics::SphereShape>(0.75f));

        // Sphere 3: pos (3,-2,-1.5), radius 0.8, GREEN (0,1,0)
        Entity sphere3 = scene->CreateEntity("Sphere Green");
        auto s3Mat = CreateRef<Material>(*m_DefaultMaterial);
        s3Mat->SetAlbedo(glm::vec3(0.0f, 1.0f, 0.0f));
        s3Mat->SetMetallic(0.3f);
        s3Mat->SetRoughness(0.4f);
        sphere3.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreateSphere(0.5f, 24, 12), s3Mat);
        sphere3.GetComponent<TransformComponent>().Position = glm::vec3(3.0f, -0.5f, -1.0f);
        auto& s3Rb = sphere3.AddComponent<RigidBodyComponent>();
        s3Rb.Type = RigidBodyComponent::BodyType::Static;
        sphere3.AddComponent<ColliderComponent>(CreateRef<Physics::SphereShape>(0.5f));

        // Directional light
        Entity sun = scene->CreateEntity("Sun");
        sun.GetComponent<TransformComponent>().Rotation =
            glm::quat(glm::radians(glm::vec3(-45.0f, 30.0f, 0.0f)));
        auto& lc = sun.AddComponent<LightComponent>(LightType::Directional);
        lc.Intensity = 2.2f;
        lc.Color     = glm::vec3(1.0f, 0.97f, 0.88f);
        lc.SyncToLight();

        if (m_SceneHierarchyPanel) m_SceneHierarchyPanel->SetSelectedEntity({});
        GE_CORE_INFO("Loaded Cloth Simulation Demo");
    }

    // ==================== SPH Water Simulation ====================

    void EditorApp::LoadSPHWaterDemo() {
        if (m_EditorContext && !m_EditorContext->IsEditing()) m_EditorContext->Stop();

        auto editorScene = std::make_unique<EditorScene>();
        editorScene->m_ID = m_NextSceneID++;
        editorScene->m_Scene = CreateRef<Scene>("SPH Water Simulation");
        editorScene->m_HasUnsavedChanges = true;
        m_Scenes.push_back(std::move(editorScene));
        SetCurrentScene(static_cast<int>(m_Scenes.size() - 1));

        auto scene = GetCurrentScene();
        if (!scene || !m_DefaultMaterial) return;

        // Glass-like tank material
        auto wallMat = CreateRef<Material>(*m_DefaultMaterial);
        wallMat->SetAlbedo(glm::vec3(0.75f, 0.85f, 0.95f));
        wallMat->SetMetallic(0.1f);
        wallMat->SetRoughness(0.05f);

        // Helper to create a static axis-aligned box (visual + collider)
        auto makeBox = [&](const std::string& name, glm::vec3 pos, glm::vec3 scale) {
            Entity e = scene->CreateEntity(name);
            auto& tf = e.GetComponent<TransformComponent>();
            tf.Position = pos;
            tf.Scale    = scale;
            e.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreateCube(1.0f), wallMat);
            auto& rb = e.AddComponent<RigidBodyComponent>();
            rb.Type = RigidBodyComponent::BodyType::Static;
            e.AddComponent<ColliderComponent>(CreateRef<Physics::BoxShape>(scale * 0.5f));
        };

        // 4 × 4 × 4 open-top tank
        makeBox("Tank Floor",   { 0.0f, -2.1f,  0.0f}, {4.2f, 0.2f, 4.2f});
        makeBox("Tank Wall +X", { 2.1f,  0.0f,  0.0f}, {0.2f, 4.0f, 4.2f});
        makeBox("Tank Wall -X", {-2.1f,  0.0f,  0.0f}, {0.2f, 4.0f, 4.2f});
        makeBox("Tank Wall +Z", { 0.0f,  0.0f,  2.1f}, {4.2f, 4.0f, 0.2f});
        makeBox("Tank Wall -Z", { 0.0f,  0.0f, -2.1f}, {4.2f, 4.0f, 0.2f});

        // -----------------------------------------------------------------------
        // SPH Water — block initialization matching old_code/src/sph_water.cpp
        //
        //   Old code spawns: nx=36, ny=25, nz=18 (≈16k particles)
        //   We use a scaled-down version that fits in the 4×4×4 tank:
        //     nx=16, ny=12, nz=12  ≈ 2304 particles, spacing=0.14 m
        //
        //   BoundsMin/Max match the tank interior walls.
        //   After adding to scene, InitWaterBlock() is called in OnCreate
        //   (or we call it manually via the editor's Play → OnCreate path).
        // -----------------------------------------------------------------------
        Entity fluid = scene->CreateEntity("Water (SPH)");
        fluid.GetComponent<TransformComponent>().Position = glm::vec3(0.0f, 0.0f, 0.0f);
        auto& fe = fluid.AddComponent<FluidEmitterComponent>();
        // SPH parameters matching old_code/src/sph_water.cpp exactly
        fe.SmoothingLength      = 0.045f;
        fe.RestDensity          = 1000.0f;
        fe.PressureCoefficient  = 1.6f;
        fe.ViscosityCoefficient = 0.15f;
        fe.BounceCoefficient    = 0.4f;
        fe.ParticleSize         = 0.06f;   // visual render radius (bigger for visibility)
        fe.BoundsMin            = glm::vec3(-1.9f, -1.9f, -1.9f);
        fe.BoundsMax            = glm::vec3( 1.9f,  1.9f,  1.9f);
        fe.Emitting             = false;   // block init — no continuous emission
        fe.MaxParticles         = 5000;
        // Block init: nx=16, ny=12, nz=12 at spacing=0.06 (= h*1.33 — kernels overlap properly)
        // → 2304 particles filling 0.96×0.72×0.72m in the lower quarter of the 3.8m tank
        fe.InitBlockNx      = 16;
        fe.InitBlockNy      = 12;
        fe.InitBlockNz      = 12;
        fe.InitBlockSpacing = 0.06f;  // OnCreate() auto-calls InitWaterBlock() with these params

        // Sky directional light
        Entity sky = scene->CreateEntity("Sky Light");
        sky.GetComponent<TransformComponent>().Rotation =
            glm::quat(glm::radians(glm::vec3(-60.0f, 15.0f, 0.0f)));
        auto& slc = sky.AddComponent<LightComponent>(LightType::Directional);
        slc.Intensity = 1.8f;
        slc.Color     = glm::vec3(0.85f, 0.92f, 1.0f);
        slc.SyncToLight();

        // Blue underwater glow
        Entity waterLight = scene->CreateEntity("Water Glow");
        waterLight.GetComponent<TransformComponent>().Position = glm::vec3(0.0f, -1.5f, 0.0f);
        auto& wlc = waterLight.AddComponent<LightComponent>(LightType::Point);
        wlc.Color     = glm::vec3(0.15f, 0.55f, 1.0f);
        wlc.Intensity = 3.5f;
        wlc.Range     = 8.0f;
        wlc.SyncToLight();

        // Camera
        Entity cam = scene->CreateEntity("Tank Camera");
        auto& camTf = cam.GetComponent<TransformComponent>();
        camTf.Position = glm::vec3(4.5f, 3.0f, 4.5f);  // closer so particles are visible
        camTf.Rotation = glm::quat(glm::radians(glm::vec3(-28.0f, 45.0f, 0.0f)));
        auto& cc = cam.AddComponent<CameraComponent>();
        cc.IsMainCamera = true;
        cc.FOV          = 60.0f;

        if (m_SceneHierarchyPanel) m_SceneHierarchyPanel->SetSelectedEntity({});
        GE_CORE_INFO("Loaded SPH Water Simulation Demo");
    }

    // ==================== Robot Arm IK ====================

    void EditorApp::LoadRobotArmDemo() {
        if (m_EditorContext && !m_EditorContext->IsEditing()) m_EditorContext->Stop();

        auto editorScene = std::make_unique<EditorScene>();
        editorScene->m_ID = m_NextSceneID++;
        editorScene->m_Scene = CreateRef<Scene>("Robot Arm (IK)");
        editorScene->m_HasUnsavedChanges = true;
        m_Scenes.push_back(std::move(editorScene));
        SetCurrentScene(static_cast<int>(m_Scenes.size() - 1));

        auto scene = GetCurrentScene();
        if (!scene || !m_DefaultMaterial) return;

        // === Checkerboard floor (like old_code/arm-simulation) ===
        float squareSize = 0.2f;
        int gridSize = 50;
        for (int i = 0; i < gridSize; i++) {
            for (int j = 0; j < gridSize; j++) {
                bool isLight = (i + j) % 2 == 0;
                glm::vec3 color = isLight ? glm::vec3(0.8f, 0.8f, 0.8f) : glm::vec3(0.3f, 0.3f, 0.3f);
                
                auto tileMesh = MeshGenerator3D::CreatePlane(squareSize, squareSize);
                std::string name = "Tile_" + std::to_string(i) + "_" + std::to_string(j);
                Entity tile = scene->CreateEntity(name);
                auto tMat = CreateRef<Material>(*m_DefaultMaterial);
                tMat->SetAlbedo(color);
                tMat->SetRoughness(0.6f);
                tMat->SetMetallic(0.1f);
                tile.AddComponent<MeshRendererComponent>(tileMesh, tMat);
                auto& tf = tile.GetComponent<TransformComponent>();
                tf.Position = glm::vec3(
                    (i - gridSize/2) * squareSize,
                    0.0f,
                    (j - gridSize/2) * squareSize
                );
            }
        }

        // Pedestal base (like arm-simulation's base)
        Entity base = scene->CreateEntity("Arm Base");
        auto bMat = CreateRef<Material>(*m_DefaultMaterial);
        bMat->SetAlbedo(glm::vec3(0.3f, 0.3f, 0.35f));
        bMat->SetMetallic(0.6f);
        bMat->SetRoughness(0.4f);
        base.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreateCylinder(0.4f, 0.3f, 16), bMat);
        base.GetComponent<TransformComponent>().Position = glm::vec3(0, 0.15f, 0);

        // Robot arm with DH-style parameters (like arm-simulation)
        Entity arm = scene->CreateEntity("Robot Arm");
        arm.GetComponent<TransformComponent>().Position = glm::vec3(0, 0.3f, 0);
        auto& rac = arm.AddComponent<RobotArmComponent>();
        rac.IKEnabled       = true;
        rac.TargetPosition  = glm::vec3(1.5f, 1.0f, 0.5f);
        // PID gains like arm-simulation's default (P=0.05, I=0, D=0)
        rac.Kp              = 0.05f;
        rac.Ki              = 0.0f;
        rac.Kd              = 0.02f;
        rac.IKMaxIterations = 200;

        // IK target marker — bright red sphere (like arm-simulation's IK sphere)
        Entity target = scene->CreateEntity("IK Target");
        auto tMat = CreateRef<Material>(*m_DefaultMaterial);
        tMat->SetAlbedo(glm::vec3(1.0f, 0.2f, 0.2f));  // Red like arm-sim
        tMat->SetMetallic(0.0f);
        tMat->SetRoughness(0.3f);
        target.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreateSphere(0.05f, 12, 8), tMat);
        target.GetComponent<TransformComponent>().Position = rac.TargetPosition;

        // Green transparent sphere (like arm-simulation's sphere)
        Entity greenSphere = scene->CreateEntity("Reference Sphere");
        auto gsMat = CreateRef<Material>(*m_DefaultMaterial);
        gsMat->SetAlbedo(glm::vec3(0.0f, 0.5f, 0.0f));
        gsMat->SetMetallic(0.1f);
        gsMat->SetRoughness(0.5f);
        greenSphere.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreateSphere(0.2f, 16, 12), gsMat);
        greenSphere.GetComponent<TransformComponent>().Position = glm::vec3(0.0f, 0.2f, 0.0f);

        // Moving light (like arm-simulation's rotating light at 6*sin(t), 3.5, 6*cos(t))
        Entity light = scene->CreateEntity("Moving Light");
        auto& lTf = light.GetComponent<TransformComponent>();
        lTf.Position = glm::vec3(6.0f, 3.5f, 6.0f);
        auto lightMesh = MeshGenerator3D::CreateCube(0.2f);
        auto lightMat = CreateRef<Material>(*m_DefaultMaterial);
        lightMat->SetAlbedo(glm::vec3(1.0f, 1.0f, 1.0f));  // White light cube
        light.AddComponent<MeshRendererComponent>(lightMesh, lightMat);
        auto& lc = light.AddComponent<LightComponent>(LightType::Point);
        lc.Color     = glm::vec3(1.0f, 1.0f, 1.0f);
        lc.Intensity = 3.0f;
        lc.Range     = 15.0f;
        lc.SyncToLight();

        // Camera positioned like arm-simulation's gluLookAt
        Entity cam = scene->CreateEntity("Arm Camera");
        auto& camTf = cam.GetComponent<TransformComponent>();
        camTf.Position = glm::vec3(4.0f, 2.0f, 4.0f);
        camTf.Rotation = glm::quat(glm::radians(glm::vec3(-25.0f, 45.0f, 0.0f)));
        auto& cc = cam.AddComponent<CameraComponent>();
        cc.IsMainCamera = true;
        cc.FOV          = 50.0f;

        if (m_SceneHierarchyPanel) m_SceneHierarchyPanel->SetSelectedEntity(arm);
        GE_CORE_INFO("Loaded Robot Arm IK Demo");
    }

    // ==================== GPU Particle System ====================

    void EditorApp::LoadGPUParticleDemo() {
        if (m_EditorContext && !m_EditorContext->IsEditing()) m_EditorContext->Stop();

        auto editorScene = std::make_unique<EditorScene>();
        editorScene->m_ID = m_NextSceneID++;
        editorScene->m_Scene = CreateRef<Scene>("GPU Particle System");
        editorScene->m_HasUnsavedChanges = true;
        m_Scenes.push_back(std::move(editorScene));
        SetCurrentScene(static_cast<int>(m_Scenes.size() - 1));

        auto scene = GetCurrentScene();
        if (!scene || !m_DefaultMaterial) return;

        // === Purple Pyramid Platforms (like old_code/particle-sim) ===
        // Creates 5 floors forming a stepped pyramid
        // Positions: y=-15 size=25, y=-12.5 size=20, y=-10 size=15, y=-7.5 size=10, y=-5 size=5
        float pyramidLevels[][2] = {
            {-15.0f, 25.0f}, {-12.5f, 20.0f}, {-10.0f, 15.0f}, {-7.5f, 10.0f}, {-5.0f, 5.0f}
        };
        int numLevels = 5;
        for (int i = 0; i < numLevels; i++) {
            float yPos = pyramidLevels[i][0];
            float size = pyramidLevels[i][1];
            
            Entity platform = scene->CreateEntity("Platform_" + std::to_string(i));
            auto pMat = CreateRef<Material>(*m_DefaultMaterial);
            // Purple color (186, 126, 207) with varying alpha-like brightness
            float brightness = 1.0f - (float(i) / float(numLevels)) * 0.6f;
            pMat->SetAlbedo(glm::vec3(0.73f * brightness, 0.49f * brightness, 0.81f * brightness));
            pMat->SetRoughness(0.6f);
            pMat->SetMetallic(0.2f);
            
            auto mesh = MeshGenerator3D::CreateCube(1.0f);
            platform.AddComponent<MeshRendererComponent>(mesh, pMat);
            auto& tf = platform.GetComponent<TransformComponent>();
            tf.Position = glm::vec3(0.0f, yPos, 0.0f);
            tf.Scale = glm::vec3(size, 0.2f, size);
        }

        // Central particle emitter (cannon at y=15)
        Entity fountain = scene->CreateEntity("Particle Cannon");
        fountain.GetComponent<TransformComponent>().Position = glm::vec3(0.0f, 15.0f, 0.0f);
        auto& gpu1 = fountain.AddComponent<GPUParticleComponent>();
        gpu1.MaxParticles = 50000;
        gpu1.Gravity      = glm::vec3(0.0f, -1.0f, 0.0f);  // Matches old gravity=0.1 scaled
        gpu1.Restitution  = 0.5f;   // Friction-based bounce
        gpu1.Damping      = 0.8f;   // 1 - friction(0.2)
        gpu1.EmitHeight   = 15.0f;  // Cannon position y
        gpu1.FloorY       = -15.0f; // Lowest platform
        gpu1.ParticleSize = 0.25f;  // scaleFactor from old code
        gpu1.EmitRate     = 500;    // Continuous fire rate

        // Corner light (like old code's lightSource at 150,150,150)
        Entity light = scene->CreateEntity("Corner Light");
        light.GetComponent<TransformComponent>().Position = glm::vec3(15.0f, 15.0f, 15.0f);
        auto& lc = light.AddComponent<LightComponent>(LightType::Point);
        lc.Color     = glm::vec3(1.0f, 1.0f, 1.0f);
        lc.Intensity = 3.0f;
        lc.Range     = 50.0f;
        lc.SyncToLight();

        // Ambient fill light
        Entity fillLight = scene->CreateEntity("Fill Light");
        fillLight.GetComponent<TransformComponent>().Position = glm::vec3(-10.0f, 10.0f, -10.0f);
        fillLight.GetComponent<TransformComponent>().Rotation = glm::quat(glm::radians(glm::vec3(-45.0f, 45.0f, 0.0f)));
        auto& flc = fillLight.AddComponent<LightComponent>(LightType::Directional);
        flc.Color     = glm::vec3(0.4f, 0.4f, 0.5f);
        flc.Intensity = 0.5f;
        flc.SyncToLight();

        // Camera positioned like old code's gluLookAt
        Entity cam = scene->CreateEntity("Particle Camera");
        auto& camTf = cam.GetComponent<TransformComponent>();
        camTf.Position = glm::vec3(50.0f, 25.0f, 50.0f);  // zoom=50, xRotate=25
        camTf.Rotation = glm::quat(glm::radians(glm::vec3(-25.0f, -135.0f, 0.0f)));
        auto& cc = cam.AddComponent<CameraComponent>();
        cc.IsMainCamera = true;
        cc.FOV          = 74.0f;  // From old code's gluPerspective

        if (m_SceneHierarchyPanel) m_SceneHierarchyPanel->SetSelectedEntity({});
        GE_CORE_INFO("Loaded GPU Particle System Demo");
    }

    // ==================== Soft Body (XPBD) ====================

    void EditorApp::LoadSoftBodyDemo() {
        if (m_EditorContext && !m_EditorContext->IsEditing()) m_EditorContext->Stop();

        auto editorScene = std::make_unique<EditorScene>();
        editorScene->m_ID = m_NextSceneID++;
        editorScene->m_Scene = CreateRef<Scene>("Soft Body (XPBD)");
        editorScene->m_HasUnsavedChanges = true;
        m_Scenes.push_back(std::move(editorScene));
        SetCurrentScene(static_cast<int>(m_Scenes.size() - 1));

        auto scene = GetCurrentScene();
        if (!scene || !m_DefaultMaterial) return;

        // === Gray ground plane (like old_code/feather) ===
        Entity ground = scene->CreateEntity("Ground");
        auto gMat = CreateRef<Material>(*m_DefaultMaterial);
        gMat->SetAlbedo(glm::vec3(0.5f, 0.5f, 0.5f));  // Gray like feather demo
        gMat->SetRoughness(0.4f);
        gMat->SetMetallic(0.5f);
        ground.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreatePlane(40.0f, 40.0f), gMat);
        ground.GetComponent<TransformComponent>().Position = glm::vec3(0, 0.0f, 0);
        auto& gRb = ground.AddComponent<RigidBodyComponent>();
        gRb.Type = RigidBodyComponent::BodyType::Static;
        ground.AddComponent<ColliderComponent>(CreateRef<Physics::BoxShape>(glm::vec3(20.0f, 0.1f, 20.0f)));

        // === Primary cloth (like feather's cloth with 32x32 resolution) ===
        // White/bright cloth like carpet texture
        Entity cloth = scene->CreateEntity("Cloth");
        cloth.GetComponent<TransformComponent>().Position = glm::vec3(0.0f, 7.0f, 0.0f);
        auto clothMesh = MeshGenerator3D::CreatePlane(10.0f, 10.0f, 31, 31);  // 32x32 vertices
        auto clothMat  = CreateRef<Material>(*m_DefaultMaterial);
        clothMat->SetAlbedo(glm::vec3(2.0f, 2.0f, 2.0f));  // Bright white like feather
        clothMat->SetRoughness(0.8f);
        clothMat->SetMetallic(0.0f);
        cloth.AddComponent<MeshRendererComponent>(clothMesh, clothMat);
        auto& sb = cloth.AddComponent<SoftBodyComponent>();
        sb.GridResX                  = 32;
        sb.GridResY                  = 32;
        sb.Width                     = 10.0f;
        sb.Height                    = 10.0f;
        sb.Mass                      = 1.0f;
        sb.Damping                   = 0.9f;   // Like feather's damping
        sb.SubSteps                  = 4;      // 4 iterations like feather
        sb.EnableDistanceConstraints = true;
        sb.EnableBendConstraints     = true;
        sb.EnableVolumeConstraints   = false;  // Cloth doesn't need volume
        sb.EnableCollisionConstraints= true;
        sb.DistanceCompliance        = 0.1f;   // stretchCompliance
        sb.BendCompliance            = 0.01f;  // bendCompliance
        sb.SetMesh(clothMesh);

        // === Orange cube (like feather's cube) ===
        Entity cube = scene->CreateEntity("Orange Cube");
        cube.GetComponent<TransformComponent>().Position = glm::vec3(5.0f, 4.0f, -8.0f);
        auto cubeMesh = MeshGenerator3D::CreateCube(1.0f);
        auto cubeMat = CreateRef<Material>(*m_DefaultMaterial);
        cubeMat->SetAlbedo(glm::vec3(1.0f, 0.6f, 0.0f));  // Orange like feather
        cubeMat->SetMetallic(0.2f);
        cubeMat->SetRoughness(0.8f);
        cube.AddComponent<MeshRendererComponent>(cubeMesh, cubeMat);
        auto& cubeRb = cube.AddComponent<RigidBodyComponent>();
        cubeRb.Type = RigidBodyComponent::BodyType::Dynamic;
        cubeRb.Mass = 1.0f;
        cube.AddComponent<ColliderComponent>(CreateRef<Physics::BoxShape>(glm::vec3(0.5f)));

        // === Soft sphere (like feather's soft earth sphere) ===
        Entity sphere = scene->CreateEntity("Soft Sphere");
        sphere.GetComponent<TransformComponent>().Position = glm::vec3(0.0f, 9.0f, 0.0f);
        sphere.GetComponent<TransformComponent>().Scale = glm::vec3(1.5f);
        auto sphereMesh = MeshGenerator3D::CreateSphere(1.0f, 16, 16);
        auto sphereMat = CreateRef<Material>(*m_DefaultMaterial);
        sphereMat->SetAlbedo(glm::vec3(0.3f, 0.6f, 0.9f));  // Blue-ish
        sphereMat->SetMetallic(0.2f);
        sphereMat->SetRoughness(0.6f);
        sphere.AddComponent<MeshRendererComponent>(sphereMesh, sphereMat);
        auto& sphereSb = sphere.AddComponent<SoftBodyComponent>();
        sphereSb.GridResX = 16;
        sphereSb.GridResY = 16;
        sphereSb.Width = 2.0f;
        sphereSb.Height = 2.0f;
        sphereSb.Mass = 1.0f;
        sphereSb.Damping = 0.95f;
        sphereSb.SubSteps = 4;
        sphereSb.EnableDistanceConstraints = true;
        sphereSb.EnableVolumeConstraints = true;
        sphereSb.DistanceCompliance = 0.0001f;
        sphereSb.SetMesh(sphereMesh);

        // === Blue bunny-like soft object ===
        Entity bunny = scene->CreateEntity("Soft Object");
        bunny.GetComponent<TransformComponent>().Position = glm::vec3(10.0f, 3.0f, -8.0f);
        auto bunnyMesh = MeshGenerator3D::CreatePlane(2.0f, 2.0f, 15, 15);
        auto bunnyMat = CreateRef<Material>(*m_DefaultMaterial);
        bunnyMat->SetAlbedo(glm::vec3(0.4f, 0.4f, 1.0f));  // Blue like feather's bunny
        bunnyMat->SetMetallic(0.1f);
        bunnyMat->SetRoughness(0.8f);
        bunny.AddComponent<MeshRendererComponent>(bunnyMesh, bunnyMat);
        auto& bunnySb = bunny.AddComponent<SoftBodyComponent>();
        bunnySb.GridResX = 16;
        bunnySb.GridResY = 16;
        bunnySb.Width = 2.0f;
        bunnySb.Height = 2.0f;
        bunnySb.Mass = 1.0f;
        bunnySb.Damping = 0.95f;
        bunnySb.SubSteps = 4;
        bunnySb.EnableDistanceConstraints = true;
        bunnySb.SetMesh(bunnyMesh);

        // === Directional light (rotating sun like feather) ===
        Entity sun = scene->CreateEntity("Sun");
        auto& sunTf = sun.GetComponent<TransformComponent>();
        sunTf.Position = glm::vec3(-1.0f, 1.0f, 0.0f);  // Direction like feather
        sunTf.Rotation = glm::quat(glm::radians(glm::vec3(-45.0f, 45.0f, 0.0f)));
        auto& lc = sun.AddComponent<LightComponent>(LightType::Directional);
        lc.Intensity = 2.0f;
        lc.Color     = glm::vec3(1.0f, 1.0f, 1.0f);
        lc.SyncToLight();

        // === Orbit camera (like feather demo) ===
        Entity cam = scene->CreateEntity("Demo Camera");
        auto& camTf = cam.GetComponent<TransformComponent>();
        // Position like feather's OrbitCamera: radius=20, theta=-pi/4, phi=pi/3
        camTf.Position = glm::vec3(14.0f, 12.0f, 14.0f);
        camTf.Rotation = glm::quat(glm::radians(glm::vec3(-35.0f, 45.0f, 0.0f)));
        auto& cc = cam.AddComponent<CameraComponent>();
        cc.IsMainCamera = true;
        cc.FOV          = 45.0f;

        if (m_SceneHierarchyPanel) m_SceneHierarchyPanel->SetSelectedEntity({});
        GE_CORE_INFO("Loaded Soft Body (XPBD) Demo");
    }

    // ==================== Joints & Constraints ====================

    void EditorApp::LoadJointsDemo() {
        if (m_EditorContext && !m_EditorContext->IsEditing()) m_EditorContext->Stop();

        auto editorScene = std::make_unique<EditorScene>();
        editorScene->m_ID = m_NextSceneID++;
        editorScene->m_Scene = CreateRef<Scene>("Joints & Constraints");
        editorScene->m_HasUnsavedChanges = true;
        m_Scenes.push_back(std::move(editorScene));
        SetCurrentScene(static_cast<int>(m_Scenes.size() - 1));

        auto scene = GetCurrentScene();
        if (!scene || !m_DefaultMaterial) return;

        // Ground
        Entity ground = scene->CreateEntity("Ground");
        auto gMat = CreateRef<Material>(*m_DefaultMaterial);
        gMat->SetAlbedo(glm::vec3(0.25f, 0.25f, 0.28f));
        gMat->SetRoughness(0.7f);
        ground.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreatePlane(14.0f, 14.0f), gMat);
        ground.GetComponent<TransformComponent>().Position = glm::vec3(0, -4.5f, 0);

        // ---- Pendulum chain (5 links) ----
        auto metalMat = CreateRef<Material>(*m_DefaultMaterial);
        metalMat->SetAlbedo(glm::vec3(0.5f, 0.5f, 0.55f));
        metalMat->SetMetallic(0.9f);
        metalMat->SetRoughness(0.2f);

        // Ceiling beam (visual)
        Entity beam = scene->CreateEntity("Ceiling Beam");
        beam.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreateCube(1.0f), metalMat);
        auto& beamTf = beam.GetComponent<TransformComponent>();
        beamTf.Position = glm::vec3(-2.0f, 4.5f, 0.0f);
        beamTf.Scale    = glm::vec3(4.0f, 0.15f, 0.15f);
        auto& beamRb = beam.AddComponent<RigidBodyComponent>();
        beamRb.Type = RigidBodyComponent::BodyType::Static;

        // Static anchor the chain hangs from
        Entity anchor = scene->CreateEntity("Chain Anchor");
        anchor.GetComponent<TransformComponent>().Position = glm::vec3(-2.0f, 4.0f, 0.0f);
        auto& ancRb = anchor.AddComponent<RigidBodyComponent>();
        ancRb.Type = RigidBodyComponent::BodyType::Static;

        glm::vec3 linkColors[] = {
            {0.9f, 0.2f, 0.2f}, {0.9f, 0.6f, 0.1f}, {0.2f, 0.8f, 0.3f},
            {0.2f, 0.5f, 0.9f}, {0.7f, 0.2f, 0.9f}
        };
        Entity prev = anchor;
        for (int i = 0; i < 5; i++) {
            Entity link = scene->CreateEntity("Link_" + std::to_string(i));
            auto lMat = CreateRef<Material>(*m_DefaultMaterial);
            lMat->SetAlbedo(linkColors[i]);
            lMat->SetMetallic(0.6f);
            lMat->SetRoughness(0.3f);
            link.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreateSphere(0.22f, 14, 8), lMat);
            auto& tf = link.GetComponent<TransformComponent>();
            tf.Position = glm::vec3(-2.0f, 4.0f - (i + 1) * 0.9f, 0.0f);
            auto& rb = link.AddComponent<RigidBodyComponent>();
            rb.Mass           = 0.5f;
            rb.LinearDamping  = 0.05f;
            rb.AngularDamping = 0.1f;
            rb.UseGravity     = true;
            auto& jc = link.AddComponent<JointComponent>();
            jc.Type              = JointComponent::JointType::Distance;
            jc.ConnectedEntityID = static_cast<uint64_t>(prev.GetUUID());
            jc.Distance          = 0.9f;
            jc.AnchorA           = glm::vec3(0.0f,  0.22f, 0.0f);
            jc.AnchorB           = glm::vec3(0.0f, -0.22f, 0.0f);
            jc.IsSpring          = false;
            prev = link;
        }

        // ---- Hinge pendulum (single heavy bob with angular limits) ----
        Entity hingeAnchor = scene->CreateEntity("Hinge Anchor");
        hingeAnchor.GetComponent<TransformComponent>().Position = glm::vec3(2.5f, 4.0f, 0.0f);
        auto& haRb = hingeAnchor.AddComponent<RigidBodyComponent>();
        haRb.Type = RigidBodyComponent::BodyType::Static;

        Entity hingeBob = scene->CreateEntity("Hinge Bob");
        auto hbMat = CreateRef<Material>(*m_DefaultMaterial);
        hbMat->SetAlbedo(glm::vec3(1.0f, 0.8f, 0.1f));
        hbMat->SetMetallic(0.9f);
        hbMat->SetRoughness(0.1f);
        hingeBob.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreateSphere(0.4f, 16, 10), hbMat);
        hingeBob.GetComponent<TransformComponent>().Position = glm::vec3(2.5f, 1.0f, 0.0f);
        auto& hbRb = hingeBob.AddComponent<RigidBodyComponent>();
        hbRb.Mass          = 2.0f;
        hbRb.LinearDamping = 0.05f;
        auto& hjc = hingeBob.AddComponent<JointComponent>();
        hjc.Type              = JointComponent::JointType::Hinge;
        hjc.ConnectedEntityID = static_cast<uint64_t>(hingeAnchor.GetUUID());
        hjc.Axis              = glm::vec3(0.0f, 0.0f, 1.0f);
        hjc.Distance          = 3.0f;
        hjc.UseAngularLimits  = true;
        hjc.LowerAngularLimit = glm::radians(-80.0f);
        hjc.UpperAngularLimit = glm::radians( 80.0f);

        // ---- Spring pendulum ----
        Entity springAnchor = scene->CreateEntity("Spring Anchor");
        springAnchor.GetComponent<TransformComponent>().Position = glm::vec3(-5.5f, 4.0f, 0.0f);
        auto& saRb = springAnchor.AddComponent<RigidBodyComponent>();
        saRb.Type = RigidBodyComponent::BodyType::Static;

        Entity springBob = scene->CreateEntity("Spring Bob");
        auto sbMat = CreateRef<Material>(*m_DefaultMaterial);
        sbMat->SetAlbedo(glm::vec3(0.1f, 0.9f, 0.6f));
        sbMat->SetMetallic(0.5f);
        sbMat->SetRoughness(0.3f);
        springBob.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreateSphere(0.35f, 14, 8), sbMat);
        springBob.GetComponent<TransformComponent>().Position = glm::vec3(-5.5f, 1.2f, 0.0f);
        auto& sbRb = springBob.AddComponent<RigidBodyComponent>();
        sbRb.Mass          = 1.2f;
        sbRb.LinearDamping = 0.1f;
        auto& sjc = springBob.AddComponent<JointComponent>();
        sjc.Type              = JointComponent::JointType::Distance;
        sjc.ConnectedEntityID = static_cast<uint64_t>(springAnchor.GetUUID());
        sjc.Distance          = 2.8f;
        sjc.IsSpring          = true;
        sjc.SpringStiffness   = 60.0f;
        sjc.SpringDamping     = 4.0f;

        // Directional light
        Entity sun = scene->CreateEntity("Sun");
        sun.GetComponent<TransformComponent>().Rotation =
            glm::quat(glm::radians(glm::vec3(-50.0f, 30.0f, 0.0f)));
        auto& lc = sun.AddComponent<LightComponent>(LightType::Directional);
        lc.Intensity = 2.0f;
        lc.Color     = glm::vec3(1.0f, 0.97f, 0.9f);
        lc.SyncToLight();

        // Camera
        Entity cam = scene->CreateEntity("Demo Camera");
        auto& camTf = cam.GetComponent<TransformComponent>();
        camTf.Position = glm::vec3(0.0f, 3.5f, 10.0f);
        camTf.Rotation = glm::quat(glm::radians(glm::vec3(-10.0f, 0.0f, 0.0f)));
        auto& cc = cam.AddComponent<CameraComponent>();
        cc.IsMainCamera = true;
        cc.FOV          = 55.0f;

        if (m_SceneHierarchyPanel) m_SceneHierarchyPanel->SetSelectedEntity({});
        GE_CORE_INFO("Loaded Joints & Constraints Demo");
    }

    // ==================== Character / Platformer Level ====================

    void EditorApp::LoadCharacterDemo() {
        if (m_EditorContext && !m_EditorContext->IsEditing()) m_EditorContext->Stop();

        auto editorScene = std::make_unique<EditorScene>();
        editorScene->m_ID = m_NextSceneID++;
        editorScene->m_Scene = CreateRef<Scene>("Character Demo");
        editorScene->m_HasUnsavedChanges = true;
        m_Scenes.push_back(std::move(editorScene));
        SetCurrentScene(static_cast<int>(m_Scenes.size() - 1));

        auto scene = GetCurrentScene();
        if (!scene || !m_DefaultMaterial) return;

        // Ground
        Entity ground = scene->CreateEntity("Ground");
        auto gMat = CreateRef<Material>(*m_DefaultMaterial);
        gMat->SetAlbedo(glm::vec3(0.4f, 0.55f, 0.35f));
        gMat->SetRoughness(0.85f);
        ground.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreatePlane(20.0f, 20.0f), gMat);
        ground.GetComponent<TransformComponent>().Position = glm::vec3(0, -0.05f, 0);
        auto& gRb = ground.AddComponent<RigidBodyComponent>();
        gRb.Type = RigidBodyComponent::BodyType::Static;
        ground.AddComponent<ColliderComponent>(
            CreateRef<Physics::BoxShape>(glm::vec3(10.0f, 0.05f, 10.0f)));

        // Staircase — 6 ascending steps
        auto stepMat = CreateRef<Material>(*m_DefaultMaterial);
        stepMat->SetAlbedo(glm::vec3(0.65f, 0.55f, 0.45f));
        stepMat->SetRoughness(0.75f);
        for (int i = 0; i < 6; i++) {
            float stepH = (i + 1) * 0.3f;
            Entity step = scene->CreateEntity("Step_" + std::to_string(i));
            step.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreateCube(1.0f), stepMat);
            auto& tf = step.GetComponent<TransformComponent>();
            tf.Position = glm::vec3(1.0f + i * 1.2f, stepH * 0.5f, 0.0f);
            tf.Scale    = glm::vec3(1.2f, stepH, 2.0f);
            auto& rb = step.AddComponent<RigidBodyComponent>();
            rb.Type = RigidBodyComponent::BodyType::Static;
            step.AddComponent<ColliderComponent>(
                CreateRef<Physics::BoxShape>(glm::vec3(0.6f, stepH * 0.5f, 1.0f)));
        }

        // Floating kinematic platforms
        auto platMat = CreateRef<Material>(*m_DefaultMaterial);
        platMat->SetAlbedo(glm::vec3(0.3f, 0.5f, 0.8f));
        platMat->SetMetallic(0.4f);
        platMat->SetRoughness(0.5f);

        Entity platform1 = scene->CreateEntity("Moving Platform A");
        platform1.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreateCube(1.0f), platMat);
        auto& p1Tf = platform1.GetComponent<TransformComponent>();
        p1Tf.Position = glm::vec3(10.0f, 1.5f, -2.0f);
        p1Tf.Scale    = glm::vec3(3.0f, 0.3f, 3.0f);
        auto& p1Rb = platform1.AddComponent<RigidBodyComponent>();
        p1Rb.Type = RigidBodyComponent::BodyType::Kinematic;
        platform1.AddComponent<ColliderComponent>(
            CreateRef<Physics::BoxShape>(glm::vec3(1.5f, 0.15f, 1.5f)));

        Entity platform2 = scene->CreateEntity("Moving Platform B");
        platform2.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreateCube(1.0f), platMat);
        auto& p2Tf = platform2.GetComponent<TransformComponent>();
        p2Tf.Position = glm::vec3(14.0f, 3.0f, 2.0f);
        p2Tf.Scale    = glm::vec3(3.0f, 0.3f, 3.0f);
        auto& p2Rb = platform2.AddComponent<RigidBodyComponent>();
        p2Rb.Type = RigidBodyComponent::BodyType::Kinematic;
        platform2.AddComponent<ColliderComponent>(
            CreateRef<Physics::BoxShape>(glm::vec3(1.5f, 0.15f, 1.5f)));

        // Goal marker (bright cyan sphere)
        Entity goal = scene->CreateEntity("Goal");
        auto goalMat = CreateRef<Material>(*m_DefaultMaterial);
        goalMat->SetAlbedo(glm::vec3(0.1f, 1.0f, 0.8f));
        goalMat->SetMetallic(0.0f);
        goalMat->SetRoughness(0.2f);
        goal.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreateSphere(0.4f, 16, 10), goalMat);
        goal.GetComponent<TransformComponent>().Position = glm::vec3(14.0f, 4.0f, 2.0f);

        // Goal glow light
        Entity goalLight = scene->CreateEntity("Goal Light");
        goalLight.GetComponent<TransformComponent>().Position = glm::vec3(14.0f, 5.0f, 2.0f);
        auto& glc = goalLight.AddComponent<LightComponent>(LightType::Point);
        glc.Color     = glm::vec3(0.1f, 1.0f, 0.8f);
        glc.Intensity = 3.0f;
        glc.Range     = 5.0f;
        glc.SyncToLight();

        // Player sphere (dynamic rigid body)
        Entity player = scene->CreateEntity("Player");
        auto playerMat = CreateRef<Material>(*m_DefaultMaterial);
        playerMat->SetAlbedo(glm::vec3(0.9f, 0.3f, 0.2f));
        playerMat->SetMetallic(0.1f);
        playerMat->SetRoughness(0.6f);
        player.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreateSphere(0.4f, 20, 12), playerMat);
        player.GetComponent<TransformComponent>().Position = glm::vec3(-1.0f, 0.8f, 0.0f);
        auto& pRb = player.AddComponent<RigidBodyComponent>();
        pRb.Type          = RigidBodyComponent::BodyType::Dynamic;
        pRb.Mass          = 1.0f;
        pRb.LinearDamping = 0.3f;
        pRb.UseGravity    = true;
        player.AddComponent<ColliderComponent>(CreateRef<Physics::SphereShape>(0.4f));

        // Sun
        Entity sun = scene->CreateEntity("Sun");
        sun.GetComponent<TransformComponent>().Rotation =
            glm::quat(glm::radians(glm::vec3(-50.0f, 35.0f, 0.0f)));
        auto& lc = sun.AddComponent<LightComponent>(LightType::Directional);
        lc.Intensity = 2.5f;
        lc.Color     = glm::vec3(1.0f, 0.97f, 0.88f);
        lc.SyncToLight();

        // Cool blue fill
        Entity fill = scene->CreateEntity("Fill Light");
        fill.GetComponent<TransformComponent>().Position = glm::vec3(-5.0f, 5.0f, -5.0f);
        auto& flc = fill.AddComponent<LightComponent>(LightType::Point);
        flc.Color     = glm::vec3(0.4f, 0.6f, 1.0f);
        flc.Intensity = 1.5f;
        flc.Range     = 20.0f;
        flc.SyncToLight();

        // Camera overlooking the level
        Entity cam = scene->CreateEntity("Level Camera");
        auto& camTf = cam.GetComponent<TransformComponent>();
        camTf.Position = glm::vec3(6.0f, 8.0f, 14.0f);
        camTf.Rotation = glm::quat(glm::radians(glm::vec3(-25.0f, 0.0f, 0.0f)));
        auto& cc = cam.AddComponent<CameraComponent>();
        cc.IsMainCamera = true;
        cc.FOV          = 60.0f;

        if (m_SceneHierarchyPanel) m_SceneHierarchyPanel->SetSelectedEntity(player);
        GE_CORE_INFO("Loaded Character Demo");
    }

    // ==================== Verlet Cloth Demo ====================
    // Based on old_code/src/main2D.cpp - Verlet Integration Physics
    // Demonstrates: Verlet integration (pos = pos + (pos - oldPos) + acc*dt²),
    //               Distance constraints, Cloth simulation, Projectile physics
    
    void EditorApp::LoadVerletClothDemo() {
        if (m_EditorContext && !m_EditorContext->IsEditing()) m_EditorContext->Stop();

        auto editorScene = std::make_unique<EditorScene>();
        editorScene->m_ID = m_NextSceneID++;
        editorScene->m_Scene = CreateRef<Scene>("Verlet Cloth");
        editorScene->m_HasUnsavedChanges = true;
        m_Scenes.push_back(std::move(editorScene));
        SetCurrentScene(static_cast<int>(m_Scenes.size() - 1));

        auto scene = GetCurrentScene();
        if (!scene || !m_DefaultMaterial) return;

        // === Dark background ground (original bg: 0.1, 0.1, 0.15) ===
        Entity ground = scene->CreateEntity("Ground");
        auto gMat = CreateRef<Material>(*m_DefaultMaterial);
        gMat->SetAlbedo(glm::vec3(0.1f, 0.1f, 0.15f));
        gMat->SetRoughness(0.8f);
        gMat->SetMetallic(0.1f);
        ground.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreatePlane(30.0f, 20.0f), gMat);
        ground.GetComponent<TransformComponent>().Position = glm::vec3(0, 0, 0);
        auto& gRb = ground.AddComponent<RigidBodyComponent>();
        gRb.Type = RigidBodyComponent::BodyType::Static;
        ground.AddComponent<ColliderComponent>(CreateRef<Physics::BoxShape>(glm::vec3(15.0f, 0.1f, 10.0f)));

        // === Cloth hanging from two anchor points (original: 20x15 grid at (0, 2.5, 0)) ===
        // Two anchor poles to show where cloth is pinned
        auto poleMat = CreateRef<Material>(*m_DefaultMaterial);
        poleMat->SetAlbedo(glm::vec3(0.4f, 0.4f, 0.45f));
        poleMat->SetMetallic(0.7f);
        poleMat->SetRoughness(0.3f);

        Entity poleL = scene->CreateEntity("Anchor Left");
        poleL.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreateCylinder(0.05f, 3.0f, 8), poleMat);
        poleL.GetComponent<TransformComponent>().Position = glm::vec3(-2.0f, 4.0f, 0.0f);
        auto& plRb = poleL.AddComponent<RigidBodyComponent>();
        plRb.Type = RigidBodyComponent::BodyType::Static;

        Entity poleR = scene->CreateEntity("Anchor Right");
        poleR.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreateCylinder(0.05f, 3.0f, 8), poleMat);
        poleR.GetComponent<TransformComponent>().Position = glm::vec3(2.0f, 4.0f, 0.0f);
        auto& prRb = poleR.AddComponent<RigidBodyComponent>();
        prRb.Type = RigidBodyComponent::BodyType::Static;

        // Cloth mesh (20x15 particle grid, spacing 0.2, at origin)
        Entity cloth = scene->CreateEntity("Verlet Cloth");
        cloth.GetComponent<TransformComponent>().Position = glm::vec3(0.0f, 2.5f, 0.0f);
        auto clothMesh = MeshGenerator3D::CreatePlane(4.0f, 3.0f, 19, 14);
        auto clothMat = CreateRef<Material>(*m_DefaultMaterial);
        clothMat->SetAlbedo(glm::vec3(0.9f, 0.9f, 0.92f));
        clothMat->SetRoughness(0.8f);
        clothMat->SetMetallic(0.0f);
        cloth.AddComponent<MeshRendererComponent>(clothMesh, clothMat);
        auto& clothSb = cloth.AddComponent<SoftBodyComponent>();
        clothSb.GridResX = 20;
        clothSb.GridResY = 15;
        clothSb.Width = 4.0f;
        clothSb.Height = 3.0f;
        clothSb.Mass = 1.0f;
        clothSb.Damping = 0.99f;
        clothSb.SubSteps = 5;
        clothSb.EnableDistanceConstraints = true;
        clothSb.EnableBendConstraints = true;
        clothSb.DistanceCompliance = 0.0f;
        clothSb.BendCompliance = 0.001f;
        clothSb.SetMesh(clothMesh);

        // === Ball A (original: pos (-1, 4, 0), radius 0.3, mass 1.0) ===
        Entity ballA = scene->CreateEntity("Ball A");
        auto ballAMat = CreateRef<Material>(*m_DefaultMaterial);
        ballAMat->SetAlbedo(glm::vec3(0.8f, 0.3f, 0.3f));
        ballAMat->SetRoughness(0.3f);
        ballAMat->SetMetallic(0.2f);
        ballA.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreateSphere(0.3f, 16, 10), ballAMat);
        ballA.GetComponent<TransformComponent>().Position = glm::vec3(-1.0f, 4.0f, 0.0f);
        auto& ballARb = ballA.AddComponent<RigidBodyComponent>();
        ballARb.Type = RigidBodyComponent::BodyType::Dynamic;
        ballARb.Mass = 1.0f;
        ballARb.Restitution = 0.5f;
        ballA.AddComponent<ColliderComponent>(CreateRef<Physics::SphereShape>(0.3f));

        // === Ball B (original: pos (1, 4, 0), radius 0.4, mass 2.0) ===
        Entity ballB = scene->CreateEntity("Ball B");
        auto ballBMat = CreateRef<Material>(*m_DefaultMaterial);
        ballBMat->SetAlbedo(glm::vec3(0.3f, 0.3f, 0.8f));
        ballBMat->SetRoughness(0.3f);
        ballBMat->SetMetallic(0.2f);
        ballB.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreateSphere(0.4f, 16, 10), ballBMat);
        ballB.GetComponent<TransformComponent>().Position = glm::vec3(1.0f, 4.0f, 0.0f);
        auto& ballBRb = ballB.AddComponent<RigidBodyComponent>();
        ballBRb.Type = RigidBodyComponent::BodyType::Dynamic;
        ballBRb.Mass = 2.0f;
        ballBRb.Restitution = 0.5f;
        ballB.AddComponent<ColliderComponent>(CreateRef<Physics::SphereShape>(0.4f));

        // === Directional light ===
        Entity sun = scene->CreateEntity("Sun");
        sun.GetComponent<TransformComponent>().Rotation =
            glm::quat(glm::radians(glm::vec3(-45.0f, 30.0f, 0.0f)));
        auto& lc = sun.AddComponent<LightComponent>(LightType::Directional);
        lc.Intensity = 2.0f;
        lc.Color = glm::vec3(1.0f, 1.0f, 1.0f);
        lc.SyncToLight();

        // === Camera looking at the cloth from front ===
        Entity cam = scene->CreateEntity("Camera");
        auto& camTf = cam.GetComponent<TransformComponent>();
        camTf.Position = glm::vec3(0.0f, 3.0f, 8.0f);
        camTf.Rotation = glm::quat(glm::radians(glm::vec3(-10.0f, 0.0f, 0.0f)));
        auto& cc = cam.AddComponent<CameraComponent>();
        cc.IsMainCamera = true;
        cc.FOV = 55.0f;

        if (m_SceneHierarchyPanel) m_SceneHierarchyPanel->SetSelectedEntity(cloth);
        GE_CORE_INFO("Loaded Verlet Cloth Demo (based on Live-working-demos/01-verlet-physics)");
    }

    void EditorApp::LoadCannonShootingDemo() {
        // Same as Verlet Cloth Demo (cannon + cloth from old_code/main2D.cpp)
        LoadVerletClothDemo();
    }

    // =========================================================================
    // Spring-Block Simulator (faithful to Live-working-demos/11-spring-block-sim)
    // Blocks connected by springs with PBR materials, shadows, and bloom
    // =========================================================================
    void EditorApp::LoadSpringBlockDemo() {
        if (m_EditorContext && !m_EditorContext->IsEditing()) m_EditorContext->Stop();

        auto editorScene = std::make_unique<EditorScene>();
        editorScene->m_ID = m_NextSceneID++;
        editorScene->m_Scene = CreateRef<Scene>("Spring-Block Simulator");
        editorScene->m_HasUnsavedChanges = true;
        m_Scenes.push_back(std::move(editorScene));
        SetCurrentScene(static_cast<int>(m_Scenes.size() - 1));

        auto scene = GetCurrentScene();
        if (!scene || !m_DefaultMaterial) return;

        // Ground plane at Y=0 (implicit collision surface)
        Entity ground = scene->CreateEntity("Ground");
        auto gMat = CreateRef<Material>(*m_DefaultMaterial);
        gMat->SetAlbedo(glm::vec3(0.3f, 0.3f, 0.32f));
        gMat->SetRoughness(0.4f);
        gMat->SetMetallic(0.5f);
        ground.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreatePlane(40.0f, 40.0f), gMat);
        ground.GetComponent<TransformComponent>().Position = glm::vec3(0, 0, 0);
        auto& gRb = ground.AddComponent<RigidBodyComponent>();
        gRb.Type = RigidBodyComponent::BodyType::Static;
        ground.AddComponent<ColliderComponent>(CreateRef<Physics::BoxShape>(glm::vec3(20.0f, 0.1f, 20.0f)));

        // --- Block 0: Ceiling Anchor (fixed) ---
        // Original: pos (0,6,0), size 0.4x0.15x0.4, grey, metallic 0.5, roughness 0.6
        Entity anchor = scene->CreateEntity("Ceiling Anchor");
        auto anchorMat = CreateRef<Material>(*m_DefaultMaterial);
        anchorMat->SetAlbedo(glm::vec3(0.3f, 0.3f, 0.3f));
        anchorMat->SetMetallic(0.5f);
        anchorMat->SetRoughness(0.6f);
        anchor.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreateCube(1.0f), anchorMat);
        auto& anchorTf = anchor.GetComponent<TransformComponent>();
        anchorTf.Position = glm::vec3(0.0f, 6.0f, 0.0f);
        anchorTf.Scale = glm::vec3(0.4f, 0.15f, 0.4f);
        auto& anchorRb = anchor.AddComponent<RigidBodyComponent>();
        anchorRb.Type = RigidBodyComponent::BodyType::Static;

        // --- Block 1: Mass A (blue, dynamic) ---
        // Original: pos (0,3,0), size 0.45, blue (0.12,0.45,0.80), metallic 0.95, roughness 0.15, mass 2.0
        Entity massA = scene->CreateEntity("Mass A");
        auto massAMat = CreateRef<Material>(*m_DefaultMaterial);
        massAMat->SetAlbedo(glm::vec3(0.12f, 0.45f, 0.80f));
        massAMat->SetMetallic(0.95f);
        massAMat->SetRoughness(0.15f);
        massA.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreateCube(0.45f), massAMat);
        massA.GetComponent<TransformComponent>().Position = glm::vec3(0.0f, 3.0f, 0.0f);
        auto& massARb = massA.AddComponent<RigidBodyComponent>();
        massARb.Type = RigidBodyComponent::BodyType::Dynamic;
        massARb.Mass = 2.0f;
        massARb.Restitution = 0.4f;
        massA.AddComponent<ColliderComponent>(CreateRef<Physics::BoxShape>(glm::vec3(0.225f)));

        // --- Block 2: Mass B (red/orange, dynamic) ---
        // Original: pos (0,0.5,0), size 0.35, red (0.85,0.25,0.15), metallic 0.9, roughness 0.2, mass 1.5
        Entity massB = scene->CreateEntity("Mass B");
        auto massBMat = CreateRef<Material>(*m_DefaultMaterial);
        massBMat->SetAlbedo(glm::vec3(0.85f, 0.25f, 0.15f));
        massBMat->SetMetallic(0.9f);
        massBMat->SetRoughness(0.2f);
        massB.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreateCube(0.35f), massBMat);
        massB.GetComponent<TransformComponent>().Position = glm::vec3(0.0f, 0.5f, 0.0f);
        auto& massBRb = massB.AddComponent<RigidBodyComponent>();
        massBRb.Type = RigidBodyComponent::BodyType::Dynamic;
        massBRb.Mass = 1.5f;
        massBRb.Restitution = 0.4f;
        massB.AddComponent<ColliderComponent>(CreateRef<Physics::BoxShape>(glm::vec3(0.175f)));

        // --- Block 3: Floor Block (green, dynamic) ---
        // Original: pos (3,0.5,0), size 0.5, green (0.6,0.9,0.3), metallic 0.8, roughness 0.3, mass 3.0
        Entity floorBlock = scene->CreateEntity("Floor Block");
        auto floorMat = CreateRef<Material>(*m_DefaultMaterial);
        floorMat->SetAlbedo(glm::vec3(0.6f, 0.9f, 0.3f));
        floorMat->SetMetallic(0.8f);
        floorMat->SetRoughness(0.3f);
        floorBlock.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreateCube(0.5f), floorMat);
        floorBlock.GetComponent<TransformComponent>().Position = glm::vec3(3.0f, 0.5f, 0.0f);
        auto& floorRb = floorBlock.AddComponent<RigidBodyComponent>();
        floorRb.Type = RigidBodyComponent::BodyType::Dynamic;
        floorRb.Mass = 3.0f;
        floorRb.Restitution = 0.4f;
        floorBlock.AddComponent<ColliderComponent>(CreateRef<Physics::BoxShape>(glm::vec3(0.25f)));

        // --- Spring visuals (silver cylinders connecting blocks) ---
        // Spring 0: Anchor → Mass A (rest length 2.5)
        auto springMat = CreateRef<Material>(*m_DefaultMaterial);
        springMat->SetAlbedo(glm::vec3(0.65f, 0.65f, 0.68f));
        springMat->SetMetallic(0.95f);
        springMat->SetRoughness(0.15f);

        Entity spring0 = scene->CreateEntity("Spring 0 (Anchor-A)");
        spring0.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreateCylinder(0.035f, 2.5f, 8), springMat);
        spring0.GetComponent<TransformComponent>().Position = glm::vec3(0.0f, 4.5f, 0.0f);

        // Spring 1: Mass A → Mass B (rest length 2.0)
        Entity spring1 = scene->CreateEntity("Spring 1 (A-B)");
        spring1.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreateCylinder(0.035f, 2.0f, 8), springMat);
        spring1.GetComponent<TransformComponent>().Position = glm::vec3(0.0f, 1.75f, 0.0f);

        // Spring 2: Fixed anchor (3,5,0) → Floor Block (rest length 3.5)
        Entity springAnchor2 = scene->CreateEntity("Spring Anchor 2");
        auto sa2Mat = CreateRef<Material>(*m_DefaultMaterial);
        sa2Mat->SetAlbedo(glm::vec3(0.3f, 0.3f, 0.3f));
        sa2Mat->SetMetallic(0.5f);
        sa2Mat->SetRoughness(0.6f);
        springAnchor2.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreateCube(1.0f), sa2Mat);
        auto& sa2Tf = springAnchor2.GetComponent<TransformComponent>();
        sa2Tf.Position = glm::vec3(3.0f, 5.0f, 0.0f);
        sa2Tf.Scale = glm::vec3(0.2f, 0.1f, 0.2f);
        auto& sa2Rb = springAnchor2.AddComponent<RigidBodyComponent>();
        sa2Rb.Type = RigidBodyComponent::BodyType::Static;

        Entity spring2 = scene->CreateEntity("Spring 2 (World-Floor)");
        spring2.AddComponent<MeshRendererComponent>(MeshGenerator3D::CreateCylinder(0.035f, 3.5f, 8), springMat);
        spring2.GetComponent<TransformComponent>().Position = glm::vec3(3.0f, 2.75f, 0.0f);

        // --- Lighting: warm directional (matching original) ---
        // Original: direction (-1,-2,-1.5), color (6.0, 5.8, 5.5) HDR
        Entity sun = scene->CreateEntity("Key Light");
        sun.GetComponent<TransformComponent>().Rotation =
            glm::quat(glm::radians(glm::vec3(-55.0f, -30.0f, 0.0f)));
        auto& sunLC = sun.AddComponent<LightComponent>(LightType::Directional);
        sunLC.Intensity = 3.0f;
        sunLC.Color = glm::vec3(1.0f, 0.97f, 0.92f);
        sunLC.SyncToLight();

        // --- Camera: orbit at yaw 35, pitch 20, distance 14, target (0.5, 2, 0) ---
        Entity cam = scene->CreateEntity("Camera");
        auto& camTf = cam.GetComponent<TransformComponent>();
        // Approximate orbit camera position from yaw=35, pitch=20, dist=14, target=(0.5,2,0)
        float yaw = glm::radians(35.0f), pitch = glm::radians(20.0f), dist = 14.0f;
        camTf.Position = glm::vec3(
            0.5f + dist * std::cos(pitch) * std::sin(yaw),
            2.0f + dist * std::sin(pitch),
            0.0f + dist * std::cos(pitch) * std::cos(yaw));
        camTf.Rotation = glm::quat(glm::radians(glm::vec3(-20.0f, 35.0f, 0.0f)));
        auto& cc = cam.AddComponent<CameraComponent>();
        cc.IsMainCamera = true;
        cc.FOV = 55.0f;

        if (m_SceneHierarchyPanel) m_SceneHierarchyPanel->SetSelectedEntity({});
        GE_CORE_INFO("Loaded Spring-Block Simulator (based on Live-working-demos/11-spring-block-sim)");
    }

    // =========================================================================
    // Particle System (faithful to Live-working-demos/04-particle-system/Source.cpp)
    // Cannon at top fires particles down through pyramid-stacked floors
    // Colors: cyan→yellow→magenta as particles bounce/settle
    // =========================================================================
    void EditorApp::LoadParticleSystemDemo() {
        if (m_EditorContext && !m_EditorContext->IsEditing()) m_EditorContext->Stop();

        auto editorScene = std::make_unique<EditorScene>();
        editorScene->m_ID = m_NextSceneID++;
        editorScene->m_Scene = CreateRef<Scene>("Particle System");
        editorScene->m_HasUnsavedChanges = true;
        m_Scenes.push_back(std::move(editorScene));
        SetCurrentScene(static_cast<int>(m_Scenes.size() - 1));

        auto scene = GetCurrentScene();
        if (!scene || !m_DefaultMaterial) return;

        // Original: 5 stacked floors in pyramid arrangement
        // Floors at heights 25, 20, 15, 10, 5 with positions (-15 to -5) X, decreasing size
        // Color: purple (186, 126, 207) / (0.73, 0.49, 0.81)
        auto floorMat = CreateRef<Material>(*m_DefaultMaterial);
        floorMat->SetAlbedo(glm::vec3(0.73f, 0.49f, 0.81f));
        floorMat->SetRoughness(0.5f);
        floorMat->SetMetallic(0.1f);

        float floorHeights[] = {5.0f, 9.0f, 13.0f, 17.0f, 21.0f};
        float floorWidths[]  = {10.0f, 8.0f, 6.0f, 4.0f, 2.5f};
        float floorXOff[]    = {0.0f, -1.0f, -2.0f, -3.0f, -3.5f};

        for (int i = 0; i < 5; i++) {
            Entity floor = scene->CreateEntity("Floor_" + std::to_string(i));
            auto fMat = CreateRef<Material>(*floorMat);
            // Slight alpha variation (original uses transparency, we vary roughness instead)
            fMat->SetRoughness(0.4f + i * 0.08f);
            floor.AddComponent<MeshRendererComponent>(
                MeshGenerator3D::CreateCube(1.0f), fMat);
            auto& fTf = floor.GetComponent<TransformComponent>();
            fTf.Position = glm::vec3(floorXOff[i], floorHeights[i], 0.0f);
            fTf.Scale = glm::vec3(floorWidths[i], 0.3f, floorWidths[i]);
            auto& fRb = floor.AddComponent<RigidBodyComponent>();
            fRb.Type = RigidBodyComponent::BodyType::Static;
            floor.AddComponent<ColliderComponent>(
                CreateRef<Physics::BoxShape>(glm::vec3(floorWidths[i] * 0.5f, 0.15f, floorWidths[i] * 0.5f)));
        }

        // Particle gun / emitter at (0, 25, 0) — original firePosition
        Entity emitter = scene->CreateEntity("Particle Gun");
        auto eMat = CreateRef<Material>(*m_DefaultMaterial);
        eMat->SetAlbedo(glm::vec3(0.6f, 0.6f, 0.65f));
        eMat->SetMetallic(0.8f);
        eMat->SetRoughness(0.2f);
        emitter.AddComponent<MeshRendererComponent>(
            MeshGenerator3D::CreateCone(0.5f, 1.2f, 12), eMat);
        auto& emTf = emitter.GetComponent<TransformComponent>();
        emTf.Position = glm::vec3(0.0f, 25.0f, 0.0f);
        emTf.Rotation = glm::quat(glm::radians(glm::vec3(180.0f, 0, 0)));

        // Particles — 3 color phases from original Source.cpp:
        // Cyan (0, 162, 211)  = fresh/unbounced
        // Yellow (250, 224, 20) = bounced once
        // Magenta (224, 8, 133) = stationary/dying
        glm::vec3 pColors[] = {
            {0.0f, 0.635f, 0.827f},   // Cyan
            {0.98f, 0.878f, 0.078f},  // Yellow
            {0.878f, 0.031f, 0.522f}  // Magenta
        };

        // Create particles scattered at various heights (simulating mid-fall)
        for (int i = 0; i < 40; i++) {
            Entity p = scene->CreateEntity("Particle_" + std::to_string(i));
            auto pMat = CreateRef<Material>(*m_DefaultMaterial);
            int colorIdx = (i < 15) ? 0 : (i < 30) ? 1 : 2;
            pMat->SetAlbedo(pColors[colorIdx]);
            pMat->SetRoughness(0.3f);
            pMat->SetMetallic(0.1f);
            float r = 0.25f;  // Original scale factor 0.25
            p.AddComponent<MeshRendererComponent>(
                MeshGenerator3D::CreateSphere(r, 10, 8), pMat);
            auto& pTf = p.GetComponent<TransformComponent>();
            // Spread particles in a column with random XZ
            float angle = (float)i / 40.0f * 6.283f;
            float spread = 0.5f + (i % 4) * 0.3f;
            float height = 24.0f - (i % 15) * 1.5f;
            pTf.Position = glm::vec3(
                std::cos(angle) * spread,
                height,
                std::sin(angle) * spread);
            auto& pRb = p.AddComponent<RigidBodyComponent>();
            pRb.Type = RigidBodyComponent::BodyType::Dynamic;
            pRb.Mass = 0.1f;
            pRb.Restitution = 0.5f;
            pRb.LinearDamping = 0.05f;
            p.AddComponent<ColliderComponent>(CreateRef<Physics::SphereShape>(r));
        }

        // Light source at (150, 150, 150) → far white light (original)
        Entity sun = scene->CreateEntity("Light");
        sun.GetComponent<TransformComponent>().Rotation =
            glm::quat(glm::radians(glm::vec3(-45.0f, 45.0f, 0.0f)));
        auto& slc = sun.AddComponent<LightComponent>(LightType::Directional);
        slc.Intensity = 2.0f;
        slc.Color = glm::vec3(1.0f, 1.0f, 1.0f);
        slc.SyncToLight();

        // Camera
        Entity cam = scene->CreateEntity("Camera");
        cam.GetComponent<TransformComponent>().Position = glm::vec3(15.0f, 15.0f, 15.0f);
        cam.GetComponent<TransformComponent>().Rotation =
            glm::quat(glm::radians(glm::vec3(-25.0f, 45.0f, 0.0f)));
        auto& cc = cam.AddComponent<CameraComponent>();
        cc.IsMainCamera = true;
        cc.FOV = 60.0f;

        if (m_SceneHierarchyPanel) m_SceneHierarchyPanel->SetSelectedEntity({});
        GE_CORE_INFO("Loaded Particle System (based on Live-working-demos/04-particle-system)");
    }

    // =========================================================================
    // 2D Physics Playground
    // =========================================================================
    void EditorApp::Load2DPhysicsDemo() {
        if (m_EditorContext && !m_EditorContext->IsEditing()) m_EditorContext->Stop();

        auto editorScene = std::make_unique<EditorScene>();
        editorScene->m_ID = m_NextSceneID++;
        editorScene->m_Scene = CreateRef<Scene>("2D Physics Playground");
        editorScene->m_HasUnsavedChanges = true;
        m_Scenes.push_back(std::move(editorScene));
        SetCurrentScene(static_cast<int>(m_Scenes.size() - 1));

        auto scene = GetCurrentScene();
        if (!scene || !m_DefaultMaterial) return;

        // Ground (wide, side-view style)
        Entity ground = scene->CreateEntity("Ground");
        auto gMat = CreateRef<Material>(*m_DefaultMaterial);
        gMat->SetAlbedo(glm::vec3(0.3f, 0.6f, 0.3f));
        gMat->SetRoughness(0.8f);
        ground.AddComponent<MeshRendererComponent>(
            MeshGenerator3D::CreatePlane(30.0f, 2.0f), gMat);
        ground.GetComponent<TransformComponent>().Position = glm::vec3(0, -0.5f, 0);
        auto& gRb = ground.AddComponent<RigidBodyComponent>();
        gRb.Type = RigidBodyComponent::BodyType::Static;
        ground.AddComponent<ColliderComponent>(
            CreateRef<Physics::BoxShape>(glm::vec3(15.0f, 0.1f, 1.0f)));

        // Left wall
        Entity wallL = scene->CreateEntity("Left Wall");
        auto wlMat = CreateRef<Material>(*m_DefaultMaterial);
        wlMat->SetAlbedo(glm::vec3(0.4f, 0.4f, 0.45f));
        wallL.AddComponent<MeshRendererComponent>(
            MeshGenerator3D::CreateCube(1.0f), wlMat);
        auto& wlTf = wallL.GetComponent<TransformComponent>();
        wlTf.Position = glm::vec3(-10.0f, 5.0f, 0);
        wlTf.Scale = glm::vec3(0.5f, 10.0f, 2.0f);
        auto& wlRb = wallL.AddComponent<RigidBodyComponent>();
        wlRb.Type = RigidBodyComponent::BodyType::Static;
        wallL.AddComponent<ColliderComponent>(
            CreateRef<Physics::BoxShape>(glm::vec3(0.25f, 5.0f, 1.0f)));

        // Right wall
        Entity wallR = scene->CreateEntity("Right Wall");
        auto wrMat = CreateRef<Material>(*m_DefaultMaterial);
        wrMat->SetAlbedo(glm::vec3(0.4f, 0.4f, 0.45f));
        wallR.AddComponent<MeshRendererComponent>(
            MeshGenerator3D::CreateCube(1.0f), wrMat);
        auto& wrTf = wallR.GetComponent<TransformComponent>();
        wrTf.Position = glm::vec3(10.0f, 5.0f, 0);
        wrTf.Scale = glm::vec3(0.5f, 10.0f, 2.0f);
        auto& wrRb = wallR.AddComponent<RigidBodyComponent>();
        wrRb.Type = RigidBodyComponent::BodyType::Static;
        wallR.AddComponent<ColliderComponent>(
            CreateRef<Physics::BoxShape>(glm::vec3(0.25f, 5.0f, 1.0f)));

        // Ramp
        Entity ramp = scene->CreateEntity("Ramp");
        auto rMat = CreateRef<Material>(*m_DefaultMaterial);
        rMat->SetAlbedo(glm::vec3(0.6f, 0.5f, 0.3f));
        ramp.AddComponent<MeshRendererComponent>(
            MeshGenerator3D::CreatePlane(8.0f, 2.0f), rMat);
        auto& rTf = ramp.GetComponent<TransformComponent>();
        rTf.Position = glm::vec3(-4.0f, 2.0f, 0);
        rTf.Rotation = glm::quat(glm::radians(glm::vec3(0, 0, -20.0f)));
        auto& rRb = ramp.AddComponent<RigidBodyComponent>();
        rRb.Type = RigidBodyComponent::BodyType::Static;

        // Bouncing balls
        glm::vec3 ballColors[] = {
            {0.9f, 0.2f, 0.2f}, {0.2f, 0.5f, 0.9f}, {0.9f, 0.8f, 0.1f},
            {0.2f, 0.9f, 0.4f}, {0.8f, 0.3f, 0.9f}, {0.9f, 0.5f, 0.2f}
        };
        for (int i = 0; i < 6; i++) {
            Entity ball = scene->CreateEntity("Ball_" + std::to_string(i));
            auto bMat = CreateRef<Material>(*m_DefaultMaterial);
            bMat->SetAlbedo(ballColors[i]);
            bMat->SetRoughness(0.3f);
            bMat->SetMetallic(0.1f);
            float r = 0.3f + (i % 3) * 0.15f;
            ball.AddComponent<MeshRendererComponent>(
                MeshGenerator3D::CreateSphere(r, 16, 12), bMat);
            ball.GetComponent<TransformComponent>().Position =
                glm::vec3(-6.0f + i * 2.0f, 6.0f + i * 0.5f, 0);
            auto& bRb = ball.AddComponent<RigidBodyComponent>();
            bRb.Type = RigidBodyComponent::BodyType::Dynamic;
            bRb.Mass = 0.5f + i * 0.2f;
            bRb.Restitution = 0.7f;
            ball.AddComponent<ColliderComponent>(
                CreateRef<Physics::SphereShape>(r));
        }

        // Stacked boxes
        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < (4 - row); col++) {
                Entity box = scene->CreateEntity("Box_" + std::to_string(row) + "_" + std::to_string(col));
                auto bxMat = CreateRef<Material>(*m_DefaultMaterial);
                float t = (float)(row * 4 + col) / 16.0f;
                bxMat->SetAlbedo(glm::vec3(0.3f + t * 0.5f, 0.5f, 0.8f - t * 0.3f));
                bxMat->SetRoughness(0.5f);
                box.AddComponent<MeshRendererComponent>(
                    MeshGenerator3D::CreateCube(0.8f), bxMat);
                box.GetComponent<TransformComponent>().Position =
                    glm::vec3(5.0f + col * 0.85f - row * 0.425f, 0.4f + row * 0.85f, 0);
                auto& bxRb = box.AddComponent<RigidBodyComponent>();
                bxRb.Type = RigidBodyComponent::BodyType::Dynamic;
                bxRb.Mass = 1.0f;
                box.AddComponent<ColliderComponent>(
                    CreateRef<Physics::BoxShape>(glm::vec3(0.4f)));
            }
        }

        // Seesaw
        Entity pivot = scene->CreateEntity("Pivot");
        auto pvMat = CreateRef<Material>(*m_DefaultMaterial);
        pvMat->SetAlbedo(glm::vec3(0.7f, 0.7f, 0.2f));
        pivot.AddComponent<MeshRendererComponent>(
            MeshGenerator3D::CreateCone(0.5f, 0.8f, 16), pvMat);
        pivot.GetComponent<TransformComponent>().Position = glm::vec3(0, 0.4f, 0);

        Entity plank = scene->CreateEntity("Seesaw Plank");
        auto pkMat = CreateRef<Material>(*m_DefaultMaterial);
        pkMat->SetAlbedo(glm::vec3(0.6f, 0.4f, 0.2f));
        plank.AddComponent<MeshRendererComponent>(
            MeshGenerator3D::CreateCube(1.0f), pkMat);
        auto& pkTf = plank.GetComponent<TransformComponent>();
        pkTf.Position = glm::vec3(0, 0.9f, 0);
        pkTf.Scale = glm::vec3(6.0f, 0.15f, 1.5f);
        auto& pkRb = plank.AddComponent<RigidBodyComponent>();
        pkRb.Type = RigidBodyComponent::BodyType::Dynamic;
        pkRb.Mass = 2.0f;
        plank.AddComponent<ColliderComponent>(
            CreateRef<Physics::BoxShape>(glm::vec3(3.0f, 0.075f, 0.75f)));

        // Light
        Entity sun = scene->CreateEntity("Sun");
        sun.GetComponent<TransformComponent>().Rotation =
            glm::quat(glm::radians(glm::vec3(-45.0f, 20.0f, 0.0f)));
        auto& slc = sun.AddComponent<LightComponent>(LightType::Directional);
        slc.Intensity = 2.0f;
        slc.Color = glm::vec3(1.0f, 1.0f, 1.0f);
        slc.SyncToLight();

        // Side-view camera (like 2D)
        Entity cam = scene->CreateEntity("Camera");
        cam.GetComponent<TransformComponent>().Position = glm::vec3(0, 5, 20);
        cam.GetComponent<TransformComponent>().Rotation =
            glm::quat(glm::radians(glm::vec3(-10.0f, 0, 0)));
        auto& cc = cam.AddComponent<CameraComponent>();
        cc.IsMainCamera = true;
        cc.FOV = 50.0f;

        if (m_SceneHierarchyPanel) m_SceneHierarchyPanel->SetSelectedEntity({});
        GE_CORE_INFO("Loaded 2D Physics Playground");
    }

    // =========================================================================
    // Procedural Terrain Demo
    // =========================================================================
    void EditorApp::LoadTerrainDemo() {
        if (m_EditorContext && !m_EditorContext->IsEditing()) m_EditorContext->Stop();

        auto editorScene = std::make_unique<EditorScene>();
        editorScene->m_ID = m_NextSceneID++;
        editorScene->m_Scene = CreateRef<Scene>("Procedural Terrain");
        editorScene->m_HasUnsavedChanges = true;
        m_Scenes.push_back(std::move(editorScene));
        SetCurrentScene(static_cast<int>(m_Scenes.size() - 1));

        auto scene = GetCurrentScene();
        if (!scene || !m_DefaultMaterial) return;

        // Terrain as a large subdivided plane with vertex displacement
        // The actual Terrain class handles GPU mesh, but here we show the
        // concept using a high-res plane with varied heights
        int gridRes = 20;
        float terrainSize = 40.0f;
        float cellSize = terrainSize / gridRes;

        for (int z = 0; z < gridRes; z++) {
            for (int x = 0; x < gridRes; x++) {
                float nx = (float)x / gridRes;
                float nz = (float)z / gridRes;
                // Simple height function (sin-based hills)
                float h = 2.0f * std::sin(nx * 3.14f * 2.0f) * std::cos(nz * 3.14f * 2.0f)
                        + 0.5f * std::sin(nx * 6.28f + 1.0f) * std::sin(nz * 6.28f + 2.0f);

                Entity tile = scene->CreateEntity("Terrain_" + std::to_string(z * gridRes + x));
                auto tMat = CreateRef<Material>(*m_DefaultMaterial);
                // Color by height: low=green, mid=brown, high=white
                float t = (h + 3.0f) / 6.0f; // normalize to 0-1
                glm::vec3 color;
                if (t < 0.3f) color = glm::vec3(0.2f, 0.5f, 0.15f); // grass
                else if (t < 0.6f) color = glm::vec3(0.5f, 0.4f, 0.25f); // dirt
                else if (t < 0.8f) color = glm::vec3(0.5f, 0.5f, 0.5f); // rock
                else color = glm::vec3(0.9f, 0.92f, 0.95f); // snow

                tMat->SetAlbedo(color);
                tMat->SetRoughness(0.8f);
                tMat->SetMetallic(0.05f);
                tile.AddComponent<MeshRendererComponent>(
                    MeshGenerator3D::CreatePlane(cellSize, cellSize), tMat);
                tile.GetComponent<TransformComponent>().Position =
                    glm::vec3((x - gridRes / 2) * cellSize, h, (z - gridRes / 2) * cellSize);
            }
        }

        // Trees (simple cone + cylinder)
        for (int i = 0; i < 15; i++) {
            float tx = (float)(i * 7 % gridRes - gridRes / 2) * cellSize * 0.8f;
            float tz = (float)(i * 11 % gridRes - gridRes / 2) * cellSize * 0.8f;
            float nx = (tx / terrainSize + 0.5f);
            float nz = (tz / terrainSize + 0.5f);
            float th = 2.0f * std::sin(nx * 3.14f * 2.0f) * std::cos(nz * 3.14f * 2.0f);

            // Trunk
            Entity trunk = scene->CreateEntity("TreeTrunk_" + std::to_string(i));
            auto trunkMat = CreateRef<Material>(*m_DefaultMaterial);
            trunkMat->SetAlbedo(glm::vec3(0.4f, 0.25f, 0.1f));
            trunkMat->SetRoughness(0.9f);
            trunk.AddComponent<MeshRendererComponent>(
                MeshGenerator3D::CreateCylinder(0.15f, 1.5f, 6), trunkMat);
            trunk.GetComponent<TransformComponent>().Position = glm::vec3(tx, th + 0.75f, tz);

            // Canopy
            Entity canopy = scene->CreateEntity("TreeCanopy_" + std::to_string(i));
            auto canopyMat = CreateRef<Material>(*m_DefaultMaterial);
            float green = 0.3f + (i % 4) * 0.1f;
            canopyMat->SetAlbedo(glm::vec3(0.1f, green, 0.05f));
            canopyMat->SetRoughness(0.8f);
            canopy.AddComponent<MeshRendererComponent>(
                MeshGenerator3D::CreateCone(0.8f, 2.0f, 8), canopyMat);
            canopy.GetComponent<TransformComponent>().Position = glm::vec3(tx, th + 2.5f, tz);
        }

        // Sun
        Entity sun = scene->CreateEntity("Sun");
        sun.GetComponent<TransformComponent>().Rotation =
            glm::quat(glm::radians(glm::vec3(-40.0f, 45.0f, 0.0f)));
        auto& slc = sun.AddComponent<LightComponent>(LightType::Directional);
        slc.Intensity = 2.5f;
        slc.Color = glm::vec3(1.0f, 0.95f, 0.85f);
        slc.SyncToLight();

        // Camera
        Entity cam = scene->CreateEntity("Camera");
        cam.GetComponent<TransformComponent>().Position = glm::vec3(15.0f, 12.0f, 20.0f);
        cam.GetComponent<TransformComponent>().Rotation =
            glm::quat(glm::radians(glm::vec3(-20.0f, 35.0f, 0.0f)));
        auto& cc = cam.AddComponent<CameraComponent>();
        cc.IsMainCamera = true;
        cc.FOV = 60.0f;

        if (m_SceneHierarchyPanel) m_SceneHierarchyPanel->SetSelectedEntity({});
        GE_CORE_INFO("Loaded Procedural Terrain Demo");
    }

    // =========================================================================
    // Post-Processing Showcase
    // =========================================================================
    void EditorApp::LoadPostProcessingDemo() {
        if (m_EditorContext && !m_EditorContext->IsEditing()) m_EditorContext->Stop();

        auto editorScene = std::make_unique<EditorScene>();
        editorScene->m_ID = m_NextSceneID++;
        editorScene->m_Scene = CreateRef<Scene>("Post-Processing Showcase");
        editorScene->m_HasUnsavedChanges = true;
        m_Scenes.push_back(std::move(editorScene));
        SetCurrentScene(static_cast<int>(m_Scenes.size() - 1));

        auto scene = GetCurrentScene();
        if (!scene || !m_DefaultMaterial) return;

        // Reflective ground
        Entity ground = scene->CreateEntity("Ground");
        auto gMat = CreateRef<Material>(*m_DefaultMaterial);
        gMat->SetAlbedo(glm::vec3(0.15f, 0.15f, 0.18f));
        gMat->SetRoughness(0.1f);
        gMat->SetMetallic(0.9f);
        ground.AddComponent<MeshRendererComponent>(
            MeshGenerator3D::CreatePlane(30.0f, 30.0f), gMat);
        ground.GetComponent<TransformComponent>().Position = glm::vec3(0, 0, 0);

        // Emissive spheres (to show bloom)
        glm::vec3 emissiveColors[] = {
            {3.0f, 0.3f, 0.1f},  // Hot red
            {0.1f, 0.5f, 3.0f},  // Electric blue
            {0.1f, 3.0f, 0.3f},  // Neon green
            {3.0f, 2.0f, 0.1f},  // Bright yellow
            {2.0f, 0.1f, 3.0f},  // Purple
        };
        for (int i = 0; i < 5; i++) {
            Entity sphere = scene->CreateEntity("Glow_" + std::to_string(i));
            auto sMat = CreateRef<Material>(*m_DefaultMaterial);
            sMat->SetAlbedo(emissiveColors[i]);
            sMat->SetRoughness(0.1f);
            sMat->SetMetallic(0.0f);
            sphere.AddComponent<MeshRendererComponent>(
                MeshGenerator3D::CreateSphere(0.5f, 24, 16), sMat);
            float angle = (float)i / 5.0f * 6.283f;
            sphere.GetComponent<TransformComponent>().Position =
                glm::vec3(std::cos(angle) * 4.0f, 1.5f, std::sin(angle) * 4.0f);

            // Point light at each sphere
            auto& lc = sphere.AddComponent<LightComponent>(LightType::Point);
            lc.Intensity = 5.0f;
            lc.Color = glm::normalize(emissiveColors[i]);
            lc.Range = 8.0f;
            lc.SyncToLight();
        }

        // Chrome sphere in center (reflections)
        Entity chrome = scene->CreateEntity("Chrome Sphere");
        auto cMat = CreateRef<Material>(*m_DefaultMaterial);
        cMat->SetAlbedo(glm::vec3(0.95f, 0.95f, 0.97f));
        cMat->SetRoughness(0.02f);
        cMat->SetMetallic(1.0f);
        chrome.AddComponent<MeshRendererComponent>(
            MeshGenerator3D::CreateSphere(1.2f, 32, 24), cMat);
        chrome.GetComponent<TransformComponent>().Position = glm::vec3(0, 1.5f, 0);

        // Rough spheres ring (roughness gradient)
        for (int i = 0; i < 8; i++) {
            Entity rs = scene->CreateEntity("Roughness_" + std::to_string(i));
            auto rMat = CreateRef<Material>(*m_DefaultMaterial);
            rMat->SetAlbedo(glm::vec3(0.8f, 0.2f, 0.2f));
            rMat->SetRoughness((float)i / 7.0f);
            rMat->SetMetallic(0.9f);
            rs.AddComponent<MeshRendererComponent>(
                MeshGenerator3D::CreateSphere(0.35f, 16, 12), rMat);
            float angle = (float)i / 8.0f * 6.283f;
            rs.GetComponent<TransformComponent>().Position =
                glm::vec3(std::cos(angle) * 7.0f, 0.5f, std::sin(angle) * 7.0f);
        }

        // Directional light (warm sunset)
        Entity sun = scene->CreateEntity("Sun");
        sun.GetComponent<TransformComponent>().Rotation =
            glm::quat(glm::radians(glm::vec3(-30.0f, 60.0f, 0.0f)));
        auto& slc = sun.AddComponent<LightComponent>(LightType::Directional);
        slc.Intensity = 1.5f;
        slc.Color = glm::vec3(1.0f, 0.85f, 0.7f);
        slc.SyncToLight();

        // Camera
        Entity cam = scene->CreateEntity("Camera");
        cam.GetComponent<TransformComponent>().Position = glm::vec3(10.0f, 6.0f, 10.0f);
        cam.GetComponent<TransformComponent>().Rotation =
            glm::quat(glm::radians(glm::vec3(-20.0f, 40.0f, 0.0f)));
        auto& cc = cam.AddComponent<CameraComponent>();
        cc.IsMainCamera = true;
        cc.FOV = 55.0f;

        if (m_SceneHierarchyPanel) m_SceneHierarchyPanel->SetSelectedEntity({});
        GE_CORE_INFO("Loaded Post-Processing Showcase");
    }

    // =========================================================================
    // RegisterDemos — populates the WelcomePanel with all available demos
    // =========================================================================
    void EditorApp::RegisterDemos() {
        if (!m_WelcomePanel) return;

        m_WelcomePanel->ClearDemos();

        // Physics demos
        m_WelcomePanel->AddDemo({
            "Physics Simulation", "Rigid body cubes, spheres, ramps with collision and stacking",
            "Physics", "P", ImVec4(0.8f, 0.3f, 0.2f, 0.7f),
            [this]() { LoadPhysicsDemo(); }
        });
        m_WelcomePanel->AddDemo({
            "Cloth Simulation", "XPBD cloth with wind, pinning, and sphere obstacles",
            "Physics", "C", ImVec4(0.3f, 0.6f, 0.9f, 0.7f),
            [this]() { LoadClothDemo(); }
        });
        m_WelcomePanel->AddDemo({
            "SPH Water", "Smoothed Particle Hydrodynamics fluid simulation",
            "Physics", "W", ImVec4(0.2f, 0.5f, 0.9f, 0.7f),
            [this]() { LoadSPHWaterDemo(); }
        });
        m_WelcomePanel->AddDemo({
            "Soft Body (XPBD)", "Deformable soft body with XPBD constraint solver",
            "Physics", "S", ImVec4(0.6f, 0.3f, 0.8f, 0.7f),
            [this]() { LoadSoftBodyDemo(); }
        });
        m_WelcomePanel->AddDemo({
            "Joints & Constraints", "Hinge, slider, and fixed joints connecting rigid bodies",
            "Physics", "J", ImVec4(0.7f, 0.5f, 0.2f, 0.7f),
            [this]() { LoadJointsDemo(); }
        });
        m_WelcomePanel->AddDemo({
            "Spring-Block Sim", "PBR spring-mass chain with multiple blocks and collision",
            "Physics", "B", ImVec4(0.5f, 0.5f, 0.55f, 0.7f),
            [this]() { LoadSpringBlockDemo(); }
        });
        m_WelcomePanel->AddDemo({
            "Particle System", "3D particle fountain with multi-floor collision",
            "Physics", "F", ImVec4(0.9f, 0.6f, 0.1f, 0.7f),
            [this]() { LoadParticleSystemDemo(); }
        });
        m_WelcomePanel->AddDemo({
            "Cannon Shooting", "Verlet cloth with cannon projectiles",
            "Physics", "X", ImVec4(0.9f, 0.2f, 0.2f, 0.7f),
            [this]() { LoadCannonShootingDemo(); }
        });
        m_WelcomePanel->AddDemo({
            "Character Demo", "Character controller with movement and collision",
            "Physics", "K", ImVec4(0.3f, 0.7f, 0.4f, 0.7f),
            [this]() { LoadCharacterDemo(); }
        });

        // 2D demos
        m_WelcomePanel->AddDemo({
            "2D Physics Playground", "Bouncing balls, stacked boxes, ramps, and seesaws",
            "2D", "2", ImVec4(0.2f, 0.7f, 0.8f, 0.7f),
            [this]() { Load2DPhysicsDemo(); }
        });

        // Rendering demos
        m_WelcomePanel->AddDemo({
            "Lighting Showcase", "Multiple light types with PBR material variations",
            "Rendering", "L", ImVec4(0.9f, 0.8f, 0.2f, 0.7f),
            [this]() { LoadLightingDemo(); }
        });
        m_WelcomePanel->AddDemo({
            "GPU Particles", "GPU-instanced particle systems with effects",
            "Rendering", "G", ImVec4(0.8f, 0.4f, 0.9f, 0.7f),
            [this]() { LoadGPUParticleDemo(); }
        });
        m_WelcomePanel->AddDemo({
            "Post-Processing", "Bloom, tone mapping, PBR spheres with roughness gradients",
            "Rendering", "E", ImVec4(0.4f, 0.2f, 0.7f, 0.7f),
            [this]() { LoadPostProcessingDemo(); }
        });
        m_WelcomePanel->AddDemo({
            "Procedural Terrain", "Height-mapped terrain with trees and biome coloring",
            "Rendering", "T", ImVec4(0.3f, 0.6f, 0.2f, 0.7f),
            [this]() { LoadTerrainDemo(); }
        });

        // Robotics
        m_WelcomePanel->AddDemo({
            "Robot Arm (IK)", "Forward/inverse kinematics robot arm with DH parameters",
            "Robotics", "R", ImVec4(0.5f, 0.7f, 0.9f, 0.7f),
            [this]() { LoadRobotArmDemo(); }
        });
    }

} // namespace GameEngine

// Entry point
GameEngine::Application* GameEngine::CreateApplication() {
    return new GameEngine::EditorApp();
}
