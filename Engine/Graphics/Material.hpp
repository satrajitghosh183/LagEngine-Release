#pragma once

#include "../Core/Base.hpp"
#include "Shader.hpp"
#include "Texture2D.hpp"
#include <glm/glm.hpp>
#include <unordered_map>
#include <string>

namespace GameEngine {

    /**
     * @brief Material class
     * 
     * Encapsulates shader, textures, and material properties
     * 
     * Features:
     * - Shader reference
     * - Multiple texture slots
     * - Material properties (colors, roughness, metallic, etc.)
     * - Easy binding for rendering
     * 
     * Usage:
     *   auto material = CreateRef<Material>(shader);
     *   material->SetTexture("u_DiffuseMap", texture);
     *   material->SetVec3("u_Color", glm::vec3(1, 0, 0));
     *   material->SetFloat("u_Roughness", 0.5f);
     *   material->Bind();
     */
    class Material {
    public:
        Material(const Ref<Shader>& shader);
        ~Material() = default;
        
        /**
         * @brief Bind material (shader + textures + uniforms)
         */
        void Bind() const;
        
        /**
         * @brief Unbind material
         */
        void Unbind() const;
        
        /**
         * @brief Get shader
         */
        Ref<Shader> GetShader() const { return m_Shader; }
        
        /**
         * @brief Set shader
         */
        void SetShader(const Ref<Shader>& shader) { m_Shader = shader; }
        
        /**
         * @brief Set texture
         */
        void SetTexture(const std::string& name, const Ref<Texture2D>& texture, uint32_t slot = 0);
        
        /**
         * @brief Set material properties
         */
        void SetInt(const std::string& name, int value);
        void SetFloat(const std::string& name, float value);
        void SetVec2(const std::string& name, const glm::vec2& value);
        void SetVec3(const std::string& name, const glm::vec3& value);
        void SetVec4(const std::string& name, const glm::vec4& value);
        void SetMat4(const std::string& name, const glm::mat4& value);
        
        /**
         * @brief Get material properties
         */
        int GetInt(const std::string& name) const;
        float GetFloat(const std::string& name) const;
        glm::vec3 GetVec3(const std::string& name) const;
        glm::vec4 GetVec4(const std::string& name) const;
        
    private:
        Ref<Shader> m_Shader;
        
        struct TextureSlot {
            Ref<Texture2D> Texture;
            uint32_t Slot;
        };
        
        std::unordered_map<std::string, TextureSlot> m_Textures;
        std::unordered_map<std::string, int> m_IntParams;
        std::unordered_map<std::string, float> m_FloatParams;
        std::unordered_map<std::string, glm::vec2> m_Vec2Params;
        std::unordered_map<std::string, glm::vec3> m_Vec3Params;
        std::unordered_map<std::string, glm::vec4> m_Vec4Params;
        std::unordered_map<std::string, glm::mat4> m_Mat4Params;
    };
}