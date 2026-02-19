#include "Material.hpp"
#include <glad/glad.h>

namespace GameEngine {

    Material::Material() {
        // Default constructor - shader needs to be set later
    }

    Material::Material(const Ref<Shader>& shader)
        : m_Shader(shader) {
    }

    void Material::Bind() const {
        if (!m_Shader) return;
        m_Shader->Bind();

        // Push PBR material properties as uniforms
        m_Shader->SetUniformVec3("u_Material.albedo", m_Albedo);
        m_Shader->SetUniformFloat("u_Material.metallic", m_Metallic);
        m_Shader->SetUniformFloat("u_Material.roughness", m_Roughness);
        m_Shader->SetUniformFloat("u_Material.ao", m_AO);

        // Legacy uniform for simpler shaders
        m_Shader->SetUniformVec3("u_Color", m_Albedo);

        // Fixed slot layout: PBR maps 0-4, custom textures 5+
        constexpr uint32_t kPBRSlotAlbedo = 0;
        constexpr uint32_t kPBRSlotNormal = 1;
        constexpr uint32_t kPBRSlotMetallic = 2;
        constexpr uint32_t kPBRSlotRoughness = 3;
        constexpr uint32_t kPBRSlotAO = 4;
        constexpr uint32_t kCustomTextureBaseSlot = 5;

        int hasAlbedoMap = 0, hasNormalMap = 0, hasMetallicMap = 0, hasRoughnessMap = 0, hasAOMap = 0;
        if (m_AlbedoMap) {
            m_AlbedoMap->Bind(kPBRSlotAlbedo);
            m_Shader->SetUniformInt("u_AlbedoMap", kPBRSlotAlbedo);
            hasAlbedoMap = 1;
        }
        if (m_NormalMap) {
            m_NormalMap->Bind(kPBRSlotNormal);
            m_Shader->SetUniformInt("u_NormalMap", kPBRSlotNormal);
            hasNormalMap = 1;
        }
        if (m_MetallicMap) {
            m_MetallicMap->Bind(kPBRSlotMetallic);
            m_Shader->SetUniformInt("u_MetallicMap", kPBRSlotMetallic);
            hasMetallicMap = 1;
        }
        if (m_RoughnessMap) {
            m_RoughnessMap->Bind(kPBRSlotRoughness);
            m_Shader->SetUniformInt("u_RoughnessMap", kPBRSlotRoughness);
            hasRoughnessMap = 1;
        }
        if (m_AOMap) {
            m_AOMap->Bind(kPBRSlotAO);
            m_Shader->SetUniformInt("u_AOMap", kPBRSlotAO);
            hasAOMap = 1;
        }

        m_Shader->SetUniformInt("u_HasAlbedoMap", hasAlbedoMap);
        m_Shader->SetUniformInt("u_HasNormalMap", hasNormalMap);
        m_Shader->SetUniformInt("u_HasMetallicMap", hasMetallicMap);
        m_Shader->SetUniformInt("u_HasRoughnessMap", hasRoughnessMap);
        m_Shader->SetUniformInt("u_HasAOMap", hasAOMap);

        uint32_t customSlot = kCustomTextureBaseSlot;
        for (const auto& [name, slot] : m_Textures) {
            slot.Texture->Bind(customSlot);
            m_Shader->SetUniformInt(name, static_cast<int>(customSlot));
            customSlot++;
        }

        // Set custom uniforms
        for (const auto& [name, value] : m_IntParams)
            m_Shader->SetUniformInt(name, value);
        for (const auto& [name, value] : m_FloatParams)
            m_Shader->SetUniformFloat(name, value);
        for (const auto& [name, value] : m_Vec2Params)
            m_Shader->SetUniformVec2(name, value);
        for (const auto& [name, value] : m_Vec3Params)
            m_Shader->SetUniformVec3(name, value);
        for (const auto& [name, value] : m_Vec4Params)
            m_Shader->SetUniformVec4(name, value);
        for (const auto& [name, value] : m_Mat4Params)
            m_Shader->SetUniformMat4(name, value);
    }

    void Material::Unbind() const {
        if (!m_Shader) return;
        m_Shader->Unbind();
        // Unbind PBR texture slots (0-4)
        for (uint32_t slot = 0; slot <= 4; slot++) {
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        // Unbind custom texture slots (5+)
        uint32_t customSlot = 5;
        for (const auto& [name, slot] : m_Textures) {
            glActiveTexture(GL_TEXTURE0 + customSlot);
            glBindTexture(GL_TEXTURE_2D, 0);
            customSlot++;
        }
    }

    void Material::SetTexture(const std::string& name, const Ref<Texture2D>& texture, uint32_t slot) {
        m_Textures[name] = { texture, slot };
    }

    void Material::SetInt(const std::string& name, int value) {
        m_IntParams[name] = value;
    }

    void Material::SetFloat(const std::string& name, float value) {
        m_FloatParams[name] = value;
    }

    void Material::SetVec2(const std::string& name, const glm::vec2& value) {
        m_Vec2Params[name] = value;
    }

    void Material::SetVec3(const std::string& name, const glm::vec3& value) {
        m_Vec3Params[name] = value;
    }

    void Material::SetVec4(const std::string& name, const glm::vec4& value) {
        m_Vec4Params[name] = value;
    }

    void Material::SetMat4(const std::string& name, const glm::mat4& value) {
        m_Mat4Params[name] = value;
    }

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
}