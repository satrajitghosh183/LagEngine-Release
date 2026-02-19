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
        Material();
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
         * @brief Material name
         */
        const std::string& GetName() const { return m_Name; }
        void SetName(const std::string& name) { m_Name = name; }
        
        /**
         * @brief Set texture
         */
        void SetTexture(const std::string& name, const Ref<Texture2D>& texture, uint32_t slot = 0);
        
        /**
         * @brief PBR material properties
         */
        void SetAlbedo(const glm::vec3& color) { m_Albedo = color; }
        glm::vec3 GetAlbedo() const { return m_Albedo; }
        
        void SetMetallic(float value) { m_Metallic = value; }
        float GetMetallic() const { return m_Metallic; }
        
        void SetRoughness(float value) { m_Roughness = value; }
        float GetRoughness() const { return m_Roughness; }
        
        void SetAO(float value) { m_AO = value; }
        float GetAO() const { return m_AO; }
        
        /**
         * @brief PBR texture maps
         */
        void SetAlbedoMap(const Ref<Texture2D>& texture) { m_AlbedoMap = texture; }
        Ref<Texture2D> GetAlbedoMap() const { return m_AlbedoMap; }
        
        void SetNormalMap(const Ref<Texture2D>& texture) { m_NormalMap = texture; }
        Ref<Texture2D> GetNormalMap() const { return m_NormalMap; }
        
        void SetMetallicMap(const Ref<Texture2D>& texture) { m_MetallicMap = texture; }
        Ref<Texture2D> GetMetallicMap() const { return m_MetallicMap; }
        
        void SetRoughnessMap(const Ref<Texture2D>& texture) { m_RoughnessMap = texture; }
        Ref<Texture2D> GetRoughnessMap() const { return m_RoughnessMap; }
        
        void SetAOMap(const Ref<Texture2D>& texture) { m_AOMap = texture; }
        Ref<Texture2D> GetAOMap() const { return m_AOMap; }
        
        /**
         * @brief Set material properties (generic)
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
        std::string m_Name = "Material";
        Ref<Shader> m_Shader;
        
        // PBR properties
        glm::vec3 m_Albedo = glm::vec3(1.0f);
        float m_Metallic = 0.0f;
        float m_Roughness = 0.5f;
        float m_AO = 1.0f;
        
        // PBR texture maps
        Ref<Texture2D> m_AlbedoMap;
        Ref<Texture2D> m_NormalMap;
        Ref<Texture2D> m_MetallicMap;
        Ref<Texture2D> m_RoughnessMap;
        Ref<Texture2D> m_AOMap;
        
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