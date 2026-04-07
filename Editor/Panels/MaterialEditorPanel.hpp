#pragma once

#include "../../Engine/Core/Base.hpp"
#include "../../Engine/Graphics/Material.hpp"
#include "../../Engine/Graphics/Texture2D.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace GameEngine {

    struct MaterialPreset {
        std::string Name;
        glm::vec3 Albedo;
        float Metallic;
        float Roughness;
        float AO;
        glm::vec3 Emission;
        float EmissionStrength;
    };

    class MaterialEditorPanel {
    public:
        MaterialEditorPanel();
        ~MaterialEditorPanel();

        void OnImGuiRender();

        // Visibility control
        void Toggle() { m_IsOpen = !m_IsOpen; }
        void SetOpen(bool open) { m_IsOpen = open; }
        bool IsOpen() const { return m_IsOpen; }

        // Material management
        void SetMaterial(Ref<Material> material);
        Ref<Material> GetMaterial() const { return m_Material; }
        
        void CreateNewMaterial();
        bool SaveMaterial(const std::string& path);
        bool LoadMaterial(const std::string& path);

        // Callbacks
        void SetOnMaterialChangedCallback(std::function<void()> callback) {
            m_OnMaterialChangedCallback = callback;
        }
        
        // Apply to selected entity
        using ApplyMaterialCallback = std::function<void(Ref<Material>)>;
        void SetApplyToSelectedCallback(ApplyMaterialCallback callback) {
            m_ApplyToSelectedCallback = callback;
        }
        
        // Create material from current properties
        Ref<Material> CreateMaterialFromProperties();

    private:
        void RenderMaterialProperties();
        void RenderTextureSlots();
        void RenderPresets();
        void RenderPreview();
        void RenderShaderSettings();

        void ApplyPreset(const MaterialPreset& preset);
        void UpdatePreview();
        void InitPreviewFramebuffer();
        void CleanupPreviewFramebuffer();

        // Texture loading helpers
        void LoadTextureSlot(const char* label, Ref<Texture2D>& texture, const std::string& uniformName);
        std::string OpenTextureDialog();

    private:
        Ref<Material> m_Material;
        std::string m_MaterialPath;
        std::string m_MaterialName = "New Material";

        // PBR properties
        glm::vec3 m_Albedo = glm::vec3(0.8f);
        float m_Metallic = 0.0f;
        float m_Roughness = 0.5f;
        float m_AO = 1.0f;
        glm::vec3 m_Emission = glm::vec3(0.0f);
        float m_EmissionStrength = 0.0f;
        float m_NormalStrength = 1.0f;
        float m_HeightScale = 0.05f;
        glm::vec2 m_Tiling = glm::vec2(1.0f);
        glm::vec2 m_Offset = glm::vec2(0.0f);

        // Texture slots
        Ref<Texture2D> m_AlbedoMap;
        Ref<Texture2D> m_NormalMap;
        Ref<Texture2D> m_MetallicMap;
        Ref<Texture2D> m_RoughnessMap;
        Ref<Texture2D> m_AOMap;
        Ref<Texture2D> m_EmissionMap;
        Ref<Texture2D> m_HeightMap;

        // Preview
        uint32_t m_PreviewFramebuffer = 0;
        uint32_t m_PreviewTexture = 0;
        uint32_t m_PreviewDepthBuffer = 0;
        int m_PreviewSize = 128;
        bool m_PreviewNeedsUpdate = true;
        int m_PreviewMeshType = 0;  // 0=Sphere, 1=Cube, 2=Plane, 3=Cylinder

        // Presets
        std::vector<MaterialPreset> m_Presets;
        int m_SelectedPreset = -1;

        // UI state
        bool m_ShowAdvanced = false;
        bool m_Modified = false;

        // Callbacks
        std::function<void()> m_OnMaterialChangedCallback;
        ApplyMaterialCallback m_ApplyToSelectedCallback;

        // Visibility
        bool m_IsOpen = true;
        
        // First render flag
        bool m_FirstRender = true;
    };

}
