#include "Material.hpp"

namespace GameEngine {

    Material::Material(const Ref<Shader>& shader)
        : m_Shader(shader) {
    }

    void Material::Bind() const {
        m_Shader->Bind();
        
        // Bind textures
        for (const auto& [name, slot] : m_Textures) {
            slot.Texture->Bind(slot.Slot);
            m_Shader->SetUniformInt(name, slot.Slot);
        }
        
        // Set uniforms
        for (const auto& [name, value] : m_IntParams) {
            m_Shader->SetUniformInt(name, value);
        }
        
        for (const auto& [name, value] : m_FloatParams) {
            m_Shader->SetUniformFloat(name, value);
        }
        
        for (const auto& [name, value] : m_Vec2Params) {
            m_Shader->SetUniformVec2(name, value);
        }
        
        for (const auto& [name, value] : m_Vec3Params) {
            m_Shader->SetUniformVec3(name, value);
        }
        
        for (const auto& [name, value] : m_Vec4Params) {
            m_Shader->SetUniformVec4(name, value);
        }
        
        for (const auto& [name, value] : m_Mat4Params) {
            m_Shader->SetUniformMat4(name, value);
        }
    }

    void Material::Unbind() const {
        m_Shader->Unbind();
        
        // Unbind textures
        for (const auto& [name, slot] : m_Textures) {
            slot.Texture->Unbind();
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