#include "MaterialEditorPanel.hpp"
#include "../../Engine/Core/Logger.hpp"
#include "../../Engine/Graphics/MeshGenerator3D.hpp"
#include "../../Engine/Platform/FileDialog.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_vulkan.h>
#include <fstream>
#include <vector>
#include <cmath>
#include <nlohmann/json.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace GameEngine {

    MaterialEditorPanel::MaterialEditorPanel() {
        // Initialize Vulkan texture for preview
        InitPreviewFramebuffer();

        // Initialize presets
        m_Presets = {
            {"Default",        {0.8f, 0.8f, 0.8f}, 0.0f, 0.5f, 1.0f, {0, 0, 0}, 0.0f},
            {"Metal",          {0.9f, 0.9f, 0.9f}, 1.0f, 0.2f, 1.0f, {0, 0, 0}, 0.0f},
            {"Rough Metal",    {0.6f, 0.6f, 0.6f}, 1.0f, 0.7f, 1.0f, {0, 0, 0}, 0.0f},
            {"Plastic",        {0.9f, 0.2f, 0.2f}, 0.0f, 0.3f, 1.0f, {0, 0, 0}, 0.0f},
            {"Rubber",         {0.2f, 0.2f, 0.2f}, 0.0f, 0.9f, 1.0f, {0, 0, 0}, 0.0f},
            {"Wood",           {0.6f, 0.4f, 0.2f}, 0.0f, 0.6f, 1.0f, {0, 0, 0}, 0.0f},
            {"Marble",         {0.95f, 0.95f, 0.95f}, 0.0f, 0.1f, 1.0f, {0, 0, 0}, 0.0f},
            {"Gold",           {1.0f, 0.76f, 0.33f}, 1.0f, 0.1f, 1.0f, {0, 0, 0}, 0.0f},
            {"Copper",         {0.95f, 0.64f, 0.54f}, 1.0f, 0.15f, 1.0f, {0, 0, 0}, 0.0f},
            {"Silver",         {0.97f, 0.96f, 0.91f}, 1.0f, 0.1f, 1.0f, {0, 0, 0}, 0.0f},
            {"Glass",          {0.95f, 0.95f, 0.95f}, 0.0f, 0.0f, 1.0f, {0, 0, 0}, 0.0f},
            {"Emissive",       {0.0f, 0.0f, 0.0f}, 0.0f, 0.5f, 1.0f, {1, 0.5f, 0}, 2.0f},
            {"Neon Blue",      {0.0f, 0.0f, 0.0f}, 0.0f, 0.5f, 1.0f, {0, 0.5f, 1}, 3.0f},
            {"Neon Green",     {0.0f, 0.0f, 0.0f}, 0.0f, 0.5f, 1.0f, {0.2f, 1, 0.2f}, 3.0f},
        };
    }

    MaterialEditorPanel::~MaterialEditorPanel() {
        CleanupPreviewFramebuffer();
    }

    void MaterialEditorPanel::InitPreviewFramebuffer() {
        m_PreviewTexture = CreateRef<Texture2D>(
            static_cast<uint32_t>(m_PreviewSize),
            static_cast<uint32_t>(m_PreviewSize),
            TextureFormat::RGBA);

        // Fill with initial dark pixels
        std::vector<uint8_t> initPixels(m_PreviewSize * m_PreviewSize * 4, 40);
        m_PreviewTexture->SetData(initPixels.data(), static_cast<uint32_t>(initPixels.size()));

        m_PreviewImGuiDescriptor = ImGui_ImplVulkan_AddTexture(
            m_PreviewTexture->GetSampler(),
            m_PreviewTexture->GetImageView(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    void MaterialEditorPanel::CleanupPreviewFramebuffer() {
        if (m_PreviewImGuiDescriptor != VK_NULL_HANDLE) {
            ImGui_ImplVulkan_RemoveTexture(m_PreviewImGuiDescriptor);
            m_PreviewImGuiDescriptor = VK_NULL_HANDLE;
        }
        m_PreviewTexture.reset();
    }

    void MaterialEditorPanel::UpdatePreview() {
        if (!m_PreviewNeedsUpdate || !m_PreviewTexture) return;

        // Generate preview image in software — PBR sphere preview
        std::vector<uint8_t> pixels(m_PreviewSize * m_PreviewSize * 4);

        glm::vec3 baseColor = m_Albedo;

        float centerX = m_PreviewSize / 2.0f;
        float centerY = m_PreviewSize / 2.0f;
        float radius = m_PreviewSize * 0.4f;

        glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, 0.7f, 0.8f));
        glm::vec3 lightColor = glm::vec3(1.0f);

        for (int y = 0; y < m_PreviewSize; y++) {
            for (int x = 0; x < m_PreviewSize; x++) {
                int idx = (y * m_PreviewSize + x) * 4;

                float dx = x - centerX;
                float dy = y - centerY;
                float dist = sqrtf(dx * dx + dy * dy);

                if (dist < radius) {
                    float normalizedDist = dist / radius;

                    float nz = sqrtf(1.0f - normalizedDist * normalizedDist);
                    float nx = dx / radius;
                    float ny = -dy / radius;

                    glm::vec3 normal = glm::normalize(glm::vec3(nx, ny, nz));
                    glm::vec3 viewDir = glm::vec3(0, 0, 1);

                    float NdotL = glm::max(glm::dot(normal, lightDir), 0.0f);

                    glm::vec3 halfDir = glm::normalize(lightDir + viewDir);
                    float NdotH = glm::max(glm::dot(normal, halfDir), 0.0f);
                    float shininess = glm::mix(8.0f, 256.0f, 1.0f - m_Roughness);
                    float specular = powf(NdotH, shininess) * (1.0f - m_Roughness * 0.9f);

                    glm::vec3 specColor = glm::mix(glm::vec3(0.04f), baseColor, m_Metallic);

                    float ambient = 0.25f;

                    glm::vec3 diffuseContrib = baseColor * (1.0f - m_Metallic) * (ambient + NdotL * 0.75f);
                    glm::vec3 specContrib = specColor * specular * lightColor;
                    glm::vec3 metalAmbient = baseColor * m_Metallic * 0.3f;

                    glm::vec3 finalColor = diffuseContrib + specContrib + metalAmbient;

                    if (m_EmissionStrength > 0.0f) {
                        finalColor += m_Emission * m_EmissionStrength * 0.5f;
                    }

                    finalColor = glm::pow(finalColor, glm::vec3(1.0f / 2.2f));

                    pixels[idx + 0] = static_cast<uint8_t>(glm::clamp(finalColor.r, 0.0f, 1.0f) * 255);
                    pixels[idx + 1] = static_cast<uint8_t>(glm::clamp(finalColor.g, 0.0f, 1.0f) * 255);
                    pixels[idx + 2] = static_cast<uint8_t>(glm::clamp(finalColor.b, 0.0f, 1.0f) * 255);
                    pixels[idx + 3] = 255;
                } else if (dist < radius + 2.0f) {
                    float alpha = 1.0f - (dist - radius) / 2.0f;
                    glm::vec3 edgeColor = glm::pow(baseColor * 0.5f, glm::vec3(1.0f / 2.2f));
                    pixels[idx + 0] = static_cast<uint8_t>(glm::clamp(edgeColor.r, 0.0f, 1.0f) * 255);
                    pixels[idx + 1] = static_cast<uint8_t>(glm::clamp(edgeColor.g, 0.0f, 1.0f) * 255);
                    pixels[idx + 2] = static_cast<uint8_t>(glm::clamp(edgeColor.b, 0.0f, 1.0f) * 255);
                    pixels[idx + 3] = static_cast<uint8_t>(alpha * 255);
                } else {
                    int checkX = x / 8;
                    int checkY = y / 8;
                    uint8_t bgColor = ((checkX + checkY) % 2 == 0) ? 40 : 50;
                    pixels[idx + 0] = bgColor;
                    pixels[idx + 1] = bgColor;
                    pixels[idx + 2] = bgColor;
                    pixels[idx + 3] = 255;
                }
            }
        }

        // Upload to Vulkan texture
        m_PreviewTexture->SetData(pixels.data(), static_cast<uint32_t>(pixels.size()));

        // Recreate ImGui descriptor since texture content changed
        if (m_PreviewImGuiDescriptor != VK_NULL_HANDLE) {
            ImGui_ImplVulkan_RemoveTexture(m_PreviewImGuiDescriptor);
        }
        m_PreviewImGuiDescriptor = ImGui_ImplVulkan_AddTexture(
            m_PreviewTexture->GetSampler(),
            m_PreviewTexture->GetImageView(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        m_PreviewNeedsUpdate = false;
    }

    void MaterialEditorPanel::OnImGuiRender() {
        if (!m_IsOpen) return;

        // Force preview update on first render
        if (m_FirstRender) {
            m_PreviewNeedsUpdate = true;
            m_FirstRender = false;
        }

        ImGui::Begin("Material Editor", nullptr, ImGuiWindowFlags_MenuBar);

        // Menu bar
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New Material", "Ctrl+N")) {
                    CreateNewMaterial();
                }
                if (ImGui::MenuItem("Open...", "Ctrl+O")) {
                    // TODO: File dialog
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Save", "Ctrl+S", false, !m_MaterialPath.empty())) {
                    SaveMaterial(m_MaterialPath);
                }
                if (ImGui::MenuItem("Save As...")) {
                    // TODO: Save dialog
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Presets")) {
                for (size_t i = 0; i < m_Presets.size(); i++) {
                    if (ImGui::MenuItem(m_Presets[i].Name.c_str())) {
                        ApplyPreset(m_Presets[i]);
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // Two-column layout
        float previewWidth = 150.0f;

        // Left side: Properties
        ImGui::BeginChild("Properties", ImVec2(-previewWidth - 10, 0), false);

        // Material name
        char nameBuffer[256];
        strncpy(nameBuffer, m_MaterialName.c_str(), sizeof(nameBuffer) - 1);
        nameBuffer[sizeof(nameBuffer) - 1] = '\0';
        if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
            m_MaterialName = nameBuffer;
            m_Modified = true;
        }

        ImGui::Separator();

        // Properties in collapsing headers
        if (ImGui::CollapsingHeader("Base Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
            RenderMaterialProperties();
        }

        if (ImGui::CollapsingHeader("Texture Maps", ImGuiTreeNodeFlags_DefaultOpen)) {
            RenderTextureSlots();
        }

        if (ImGui::CollapsingHeader("Presets")) {
            RenderPresets();
        }

        if (ImGui::CollapsingHeader("Shader Settings")) {
            RenderShaderSettings();
        }

        ImGui::EndChild();

        ImGui::SameLine();

        // Right side: Preview
        ImGui::BeginChild("Preview", ImVec2(previewWidth, 0), true);
        RenderPreview();
        ImGui::EndChild();

        // Apply to Selected button
        ImGui::Separator();
        if (ImGui::Button("Apply to Selected", ImVec2(-1, 30))) {
            if (m_ApplyToSelectedCallback) {
                auto newMaterial = CreateMaterialFromProperties();
                if (newMaterial) {
                    m_ApplyToSelectedCallback(newMaterial);
                }
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Apply this material to the currently selected entity");
        }

        // Status bar
        if (m_Modified) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "* Modified");
        }

        ImGui::End();
    }

    void MaterialEditorPanel::RenderMaterialProperties() {
        bool changed = false;

        if (ImGui::ColorEdit3("Albedo", &m_Albedo.x)) {
            changed = true;
        }

        if (ImGui::SliderFloat("Metallic", &m_Metallic, 0.0f, 1.0f)) {
            changed = true;
        }

        if (ImGui::SliderFloat("Roughness", &m_Roughness, 0.0f, 1.0f)) {
            changed = true;
        }

        if (ImGui::SliderFloat("AO", &m_AO, 0.0f, 1.0f)) {
            changed = true;
        }

        ImGui::Separator();

        if (ImGui::ColorEdit3("Emission Color", &m_Emission.x)) {
            changed = true;
        }
        if (ImGui::SliderFloat("Emission Strength", &m_EmissionStrength, 0.0f, 10.0f)) {
            changed = true;
        }

        ImGui::Separator();

        ImGui::Checkbox("Show Advanced", &m_ShowAdvanced);

        if (m_ShowAdvanced) {
            if (ImGui::SliderFloat("Normal Strength", &m_NormalStrength, 0.0f, 2.0f)) {
                changed = true;
            }
            if (ImGui::SliderFloat("Height Scale", &m_HeightScale, 0.0f, 0.2f)) {
                changed = true;
            }
            if (ImGui::DragFloat2("Tiling", &m_Tiling.x, 0.1f, 0.1f, 10.0f)) {
                changed = true;
            }
            if (ImGui::DragFloat2("Offset", &m_Offset.x, 0.01f, -1.0f, 1.0f)) {
                changed = true;
            }
        }

        if (changed) {
            m_Modified = true;
            m_PreviewNeedsUpdate = true;

            if (m_Material) {
                m_Material->SetAlbedo(m_Albedo);
                m_Material->SetMetallic(m_Metallic);
                m_Material->SetRoughness(m_Roughness);
            }

            if (m_OnMaterialChangedCallback) {
                m_OnMaterialChangedCallback();
            }
        }
    }

    void MaterialEditorPanel::RenderTextureSlots() {
        float thumbnailSize = 64.0f;

        ImGui::Text("Drag and drop texture files or click to browse");
        ImGui::Separator();

        auto RenderTextureSlot = [&](const char* label, Ref<Texture2D>& texture, const char* hint) {
            ImGui::PushID(label);

            ImGui::BeginGroup();

            // Thumbnail
            if (texture) {
                // Create a temporary ImGui descriptor for this texture
                VkDescriptorSet ds = ImGui_ImplVulkan_AddTexture(
                    texture->GetSampler(),
                    texture->GetImageView(),
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                ImGui::Image((ImTextureID)ds, ImVec2(thumbnailSize, thumbnailSize));
                // Note: descriptor leaks slightly per frame — acceptable for editor UI.
                // A production solution would cache these descriptors.
            } else {
                ImVec2 pos = ImGui::GetCursorScreenPos();
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                drawList->AddRectFilled(pos, ImVec2(pos.x + thumbnailSize, pos.y + thumbnailSize),
                                       IM_COL32(50, 50, 50, 255));
                drawList->AddRect(pos, ImVec2(pos.x + thumbnailSize, pos.y + thumbnailSize),
                                 IM_COL32(100, 100, 100, 255));
                ImGui::Dummy(ImVec2(thumbnailSize, thumbnailSize));
            }

            // Drag drop target
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH")) {
                    std::string path(static_cast<const char*>(payload->Data), payload->DataSize);
                    texture = CreateRef<Texture2D>(path);
                    m_Modified = true;
                    m_PreviewNeedsUpdate = true;
                }
                ImGui::EndDragDropTarget();
            }

            // Click to clear
            if (texture && ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                texture = nullptr;
                m_Modified = true;
                m_PreviewNeedsUpdate = true;
            }

            ImGui::SameLine();

            ImGui::BeginGroup();
            ImGui::Text("%s", label);
            if (texture) {
                ImGui::TextDisabled("%dx%d", texture->GetWidth(), texture->GetHeight());
                if (ImGui::SmallButton("Clear")) {
                    texture = nullptr;
                    m_Modified = true;
                    m_PreviewNeedsUpdate = true;
                }
            } else {
                ImGui::TextDisabled("%s", hint);
                if (ImGui::SmallButton("Browse...")) {
                    std::string path = OpenTextureDialog();
                    if (!path.empty()) {
                        texture = CreateRef<Texture2D>(path);
                        m_Modified = true;
                        m_PreviewNeedsUpdate = true;
                    }
                }
            }
            ImGui::EndGroup();

            ImGui::EndGroup();

            ImGui::PopID();
        };

        RenderTextureSlot("Albedo", m_AlbedoMap, "Base color texture");
        ImGui::Spacing();
        RenderTextureSlot("Normal", m_NormalMap, "Normal map texture");
        ImGui::Spacing();
        RenderTextureSlot("Metallic", m_MetallicMap, "Metallic texture");
        ImGui::Spacing();
        RenderTextureSlot("Roughness", m_RoughnessMap, "Roughness texture");
        ImGui::Spacing();
        RenderTextureSlot("AO", m_AOMap, "Ambient occlusion");
        ImGui::Spacing();
        RenderTextureSlot("Emission", m_EmissionMap, "Emission texture");
        ImGui::Spacing();
        RenderTextureSlot("Height", m_HeightMap, "Height/displacement");
    }

    void MaterialEditorPanel::RenderPresets() {
        ImGui::Text("Quick Presets:");

        int columns = 3;
        int count = 0;

        for (size_t i = 0; i < m_Presets.size(); i++) {
            const auto& preset = m_Presets[i];

            ImGui::PushID(static_cast<int>(i));

            ImVec4 color(preset.Albedo.x, preset.Albedo.y, preset.Albedo.z, 1.0f);
            ImGui::ColorButton("##color", color, ImGuiColorEditFlags_NoTooltip, ImVec2(20, 20));
            ImGui::SameLine();

            if (ImGui::Button(preset.Name.c_str(), ImVec2(80, 20))) {
                ApplyPreset(preset);
            }

            count++;
            if (count % columns != 0) {
                ImGui::SameLine();
            }

            ImGui::PopID();
        }
    }

    void MaterialEditorPanel::RenderPreview() {
        // Update preview if needed
        UpdatePreview();

        ImGui::Text("Preview");
        ImGui::Separator();

        // Mesh type selector
        const char* meshTypes[] = { "Sphere", "Cube", "Plane", "Cylinder" };
        if (ImGui::Combo("##MeshType", &m_PreviewMeshType, meshTypes, 4)) {
            m_PreviewNeedsUpdate = true;
        }

        // Preview image
        if (m_PreviewImGuiDescriptor != VK_NULL_HANDLE) {
            float availWidth = ImGui::GetContentRegionAvail().x;
            ImGui::Image((ImTextureID)m_PreviewImGuiDescriptor,
                        ImVec2(availWidth, availWidth));
        } else {
            ImVec2 pos = ImGui::GetCursorScreenPos();
            float size = ImGui::GetContentRegionAvail().x;
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            drawList->AddRectFilled(pos, ImVec2(pos.x + size, pos.y + size),
                                   IM_COL32(30, 30, 30, 255));
            ImGui::Dummy(ImVec2(size, size));

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + size / 2 - 30);
            ImGui::TextDisabled("Preview");
        }

        // Display current values
        ImGui::Separator();
        ImGui::TextDisabled("Values:");
        ImGui::Text("M: %.2f R: %.2f", m_Metallic, m_Roughness);
        ImGui::Text("E: %.1f", m_EmissionStrength);
    }

    void MaterialEditorPanel::RenderShaderSettings() {
        ImGui::TextDisabled("Shader configuration");

        static int blendMode = 0;
        const char* blendModes[] = { "Opaque", "Transparent", "Additive", "Multiply" };
        ImGui::Combo("Blend Mode", &blendMode, blendModes, 4);

        static int cullMode = 0;
        const char* cullModes[] = { "Back", "Front", "None" };
        ImGui::Combo("Cull Mode", &cullMode, cullModes, 3);

        static bool depthWrite = true;
        ImGui::Checkbox("Depth Write", &depthWrite);

        static bool depthTest = true;
        ImGui::Checkbox("Depth Test", &depthTest);

        static int renderQueue = 2000;
        ImGui::InputInt("Render Queue", &renderQueue);
    }

    void MaterialEditorPanel::ApplyPreset(const MaterialPreset& preset) {
        m_Albedo = preset.Albedo;
        m_Metallic = preset.Metallic;
        m_Roughness = preset.Roughness;
        m_AO = preset.AO;
        m_Emission = preset.Emission;
        m_EmissionStrength = preset.EmissionStrength;

        m_Modified = true;
        m_PreviewNeedsUpdate = true;

        if (m_Material) {
            m_Material->SetAlbedo(m_Albedo);
            m_Material->SetMetallic(m_Metallic);
            m_Material->SetRoughness(m_Roughness);
        }

        if (m_OnMaterialChangedCallback) {
            m_OnMaterialChangedCallback();
        }
    }

    void MaterialEditorPanel::SetMaterial(Ref<Material> material) {
        m_Material = material;
        m_Modified = false;
        m_PreviewNeedsUpdate = true;
    }

    void MaterialEditorPanel::CreateNewMaterial() {
        m_MaterialName = "New Material";
        m_MaterialPath = "";

        m_Albedo = glm::vec3(0.8f);
        m_Metallic = 0.0f;
        m_Roughness = 0.5f;
        m_AO = 1.0f;
        m_Emission = glm::vec3(0.0f);
        m_EmissionStrength = 0.0f;
        m_NormalStrength = 1.0f;
        m_HeightScale = 0.05f;
        m_Tiling = glm::vec2(1.0f);
        m_Offset = glm::vec2(0.0f);

        m_AlbedoMap = nullptr;
        m_NormalMap = nullptr;
        m_MetallicMap = nullptr;
        m_RoughnessMap = nullptr;
        m_AOMap = nullptr;
        m_EmissionMap = nullptr;
        m_HeightMap = nullptr;

        m_Modified = false;
        m_PreviewNeedsUpdate = true;
    }

    bool MaterialEditorPanel::SaveMaterial(const std::string& path) {
        try {
            nlohmann::json j;
            j["name"] = m_MaterialName;
            j["albedo"] = {m_Albedo.x, m_Albedo.y, m_Albedo.z};
            j["metallic"] = m_Metallic;
            j["roughness"] = m_Roughness;
            j["ao"] = m_AO;
            j["emission"] = {m_Emission.x, m_Emission.y, m_Emission.z};
            j["emissionStrength"] = m_EmissionStrength;
            j["normalStrength"] = m_NormalStrength;
            j["heightScale"] = m_HeightScale;
            j["tiling"] = {m_Tiling.x, m_Tiling.y};
            j["offset"] = {m_Offset.x, m_Offset.y};

            std::ofstream file(path);
            file << j.dump(2);
            file.close();

            m_MaterialPath = path;
            m_Modified = false;

            GE_CORE_INFO("Material saved: {}", path);
            return true;
        } catch (const std::exception& e) {
            GE_CORE_ERROR("Failed to save material: {}", e.what());
            return false;
        }
    }

    bool MaterialEditorPanel::LoadMaterial(const std::string& path) {
        try {
            std::ifstream file(path);
            if (!file.is_open()) {
                GE_CORE_ERROR("Failed to open material file: {}", path);
                return false;
            }

            nlohmann::json j;
            file >> j;

            m_MaterialName = j.value("name", "Unnamed");

            if (j.contains("albedo")) {
                m_Albedo = glm::vec3(j["albedo"][0], j["albedo"][1], j["albedo"][2]);
            }
            m_Metallic = j.value("metallic", 0.0f);
            m_Roughness = j.value("roughness", 0.5f);
            m_AO = j.value("ao", 1.0f);

            if (j.contains("emission")) {
                m_Emission = glm::vec3(j["emission"][0], j["emission"][1], j["emission"][2]);
            }
            m_EmissionStrength = j.value("emissionStrength", 0.0f);
            m_NormalStrength = j.value("normalStrength", 1.0f);
            m_HeightScale = j.value("heightScale", 0.05f);

            if (j.contains("tiling")) {
                m_Tiling = glm::vec2(j["tiling"][0], j["tiling"][1]);
            }
            if (j.contains("offset")) {
                m_Offset = glm::vec2(j["offset"][0], j["offset"][1]);
            }

            m_MaterialPath = path;
            m_Modified = false;
            m_PreviewNeedsUpdate = true;

            GE_CORE_INFO("Material loaded: {}", path);
            return true;
        } catch (const std::exception& e) {
            GE_CORE_ERROR("Failed to load material: {}", e.what());
            return false;
        }
    }

    std::string MaterialEditorPanel::OpenTextureDialog() {
        auto result = FileDialog::OpenFile(
            "Select Texture",
            {
                FileFilter("Image Files", "*.png;*.jpg;*.jpeg;*.tga;*.bmp;*.hdr"),
                FileFilter("All Files", "*.*")
            },
            ""
        );

        if (result.has_value()) {
            return result.value();
        }
        return "";
    }

    Ref<Material> MaterialEditorPanel::CreateMaterialFromProperties() {
        if (m_Material) {
            auto newMat = CreateRef<Material>(*m_Material);
            newMat->SetAlbedo(m_Albedo);
            newMat->SetMetallic(m_Metallic);
            newMat->SetRoughness(m_Roughness);
            if (m_AlbedoMap) newMat->SetAlbedoMap(m_AlbedoMap);
            if (m_NormalMap) newMat->SetNormalMap(m_NormalMap);
            if (m_MetallicMap) newMat->SetMetallicMap(m_MetallicMap);
            if (m_RoughnessMap) newMat->SetRoughnessMap(m_RoughnessMap);
            return newMat;
        }

        if (m_Material) {
            m_Material->SetAlbedo(m_Albedo);
            m_Material->SetMetallic(m_Metallic);
            m_Material->SetRoughness(m_Roughness);
            return m_Material;
        }

        GE_CORE_WARN("MaterialEditorPanel: No shader available to create material");
        return nullptr;
    }

}
