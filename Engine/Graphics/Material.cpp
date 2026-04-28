#include "Material.hpp"
#include "../Core/Logger.hpp"

namespace GameEngine {

    Material::Material() {}

    // -------------------------------------------------------------------------
    // Custom textures
    // -------------------------------------------------------------------------

    void Material::SetTexture(const std::string& name, const Ref<Texture2D>& texture) {
        // Replace if already present, otherwise append
        for (auto& [n, t] : m_CustomTextures) {
            if (n == name) { t = texture; return; }
        }
        m_CustomTextures.emplace_back(name, texture);
    }

    // -------------------------------------------------------------------------
    // Generic parameter setters
    // -------------------------------------------------------------------------

    void Material::SetInt(const std::string& name, int value)              { m_IntParams[name]   = value; }
    void Material::SetFloat(const std::string& name, float value)          { m_FloatParams[name] = value; }
    void Material::SetVec2(const std::string& name, const glm::vec2& value){ m_Vec2Params[name]  = value; }
    void Material::SetVec3(const std::string& name, const glm::vec3& value){ m_Vec3Params[name]  = value; }
    void Material::SetVec4(const std::string& name, const glm::vec4& value){ m_Vec4Params[name]  = value; }
    void Material::SetMat4(const std::string& name, const glm::mat4& value){ m_Mat4Params[name]  = value; }

    int Material::GetInt(const std::string& name) const {
        auto it = m_IntParams.find(name);
        return (it != m_IntParams.end()) ? it->second : 0;
    }

    float Material::GetFloat(const std::string& name) const {
        auto it = m_FloatParams.find(name);
        return (it != m_FloatParams.end()) ? it->second : 0.0f;
    }

    glm::vec3 Material::GetVec3(const std::string& name) const {
        auto it = m_Vec3Params.find(name);
        return (it != m_Vec3Params.end()) ? it->second : glm::vec3(0.0f);
    }

    glm::vec4 Material::GetVec4(const std::string& name) const {
        auto it = m_Vec4Params.find(name);
        return (it != m_Vec4Params.end()) ? it->second : glm::vec4(0.0f);
    }

    // -------------------------------------------------------------------------
    // UBO payload
    // -------------------------------------------------------------------------

    Material::MaterialUBO Material::GetMaterialBufferData() const {
        MaterialUBO ubo{};
        ubo.Albedo          = glm::vec4(m_Albedo, 1.0f);
        ubo.Metallic        = m_Metallic;
        ubo.Roughness       = m_Roughness;
        ubo.AO              = m_AO;
        ubo.HasAlbedoMap    = m_AlbedoMap    ? 1 : 0;
        ubo.HasNormalMap    = m_NormalMap    ? 1 : 0;
        ubo.HasMetallicMap  = m_MetallicMap  ? 1 : 0;
        ubo.HasRoughnessMap = m_RoughnessMap ? 1 : 0;
        ubo.HasAOMap        = m_AOMap        ? 1 : 0;
        return ubo;
    }

    // -------------------------------------------------------------------------
    // Descriptor writes
    // -------------------------------------------------------------------------

    void Material::WriteDescriptors(VulkanDescriptorWriter& writer,
                                     VkSampler fallbackSampler,
                                     VkImageView fallbackView) const {
        // Helper: build a VkDescriptorImageInfo for a texture map (or fallback).
        auto makeImageInfo = [&](const Ref<Texture2D>& tex) -> VkDescriptorImageInfo {
            VkDescriptorImageInfo info{};
            info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            if (tex) {
                info.imageView = tex->GetImageView();
                info.sampler   = tex->GetSampler();
            } else {
                info.imageView = fallbackView;
                info.sampler   = fallbackSampler;
            }
            return info;
        };

        VkDescriptorImageInfo albedoInfo    = makeImageInfo(m_AlbedoMap);
        VkDescriptorImageInfo normalInfo    = makeImageInfo(m_NormalMap);
        VkDescriptorImageInfo metallicInfo  = makeImageInfo(m_MetallicMap);
        VkDescriptorImageInfo roughnessInfo = makeImageInfo(m_RoughnessMap);
        VkDescriptorImageInfo aoInfo        = makeImageInfo(m_AOMap);

        writer
            .WriteImage(kSlotAlbedo,   &albedoInfo)
            .WriteImage(kSlotNormal,   &normalInfo)
            .WriteImage(kSlotMetallic, &metallicInfo)
            .WriteImage(kSlotRoughness,&roughnessInfo)
            .WriteImage(kSlotAO,       &aoInfo);

        // Custom textures start at kSlotCustomBase
        uint32_t slot = kSlotCustomBase;
        for (const auto& [name, tex] : m_CustomTextures) {
            if (!tex) { slot++; continue; }
            VkDescriptorImageInfo info = makeImageInfo(tex);
            writer.WriteImage(slot, &info);
            slot++;
        }
    }

} // namespace GameEngine
