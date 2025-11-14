#pragma once

#include "../Core/Base.hpp"
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

namespace GameEngine {

    /**
     * @brief Shader program wrapper
     * 
     * Features:
     * - Load from file or source string
     * - Compile and link vertex/fragment shaders
     * - Set uniforms with type safety
     * - Automatic uniform location caching
     * - Hot-reloading support
     * 
     * Usage:
     *   auto shader = CreateRef<Shader>("shaders/basic.vert", "shaders/basic.frag");
     *   shader->Bind();
     *   shader->SetUniformMat4("u_ViewProjection", viewProjMatrix);
     *   shader->SetUniformVec3("u_LightPos", lightPosition);
     */
    class Shader {
    public:
        /**
         * @brief Create shader from file paths
         */
        Shader(const std::string& vertexPath, const std::string& fragmentPath);
        
        /**
         * @brief Create shader from source strings
         */
        Shader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
        
        ~Shader();
        
        /**
         * @brief Bind shader for rendering
         */
        void Bind() const;
        
        /**
         * @brief Unbind shader
         */
        void Unbind() const;
        
        /**
         * @brief Get shader name
         */
        const std::string& GetName() const { return m_Name; }
        
        /**
         * @brief Set uniform values
         */
        void SetUniformInt(const std::string& name, int value);
        void SetUniformIntArray(const std::string& name, int* values, uint32_t count);
        void SetUniformFloat(const std::string& name, float value);
        void SetUniformVec2(const std::string& name, const glm::vec2& value);
        void SetUniformVec3(const std::string& name, const glm::vec3& value);
        void SetUniformVec4(const std::string& name, const glm::vec4& value);
        void SetUniformMat3(const std::string& name, const glm::mat3& value);
        void SetUniformMat4(const std::string& name, const glm::mat4& value);
        
        /**
         * @brief Hot-reload shader from disk
         */
        void Reload();
        
    private:
        void CompileShaders(const std::string& vertexSrc, const std::string& fragmentSrc);
        std::string ReadFile(const std::string& filepath);
        int GetUniformLocation(const std::string& name);
        void CheckCompileErrors(uint32_t shader, const std::string& type);
        
    private:
        uint32_t m_RendererID;
        std::string m_Name;
        std::string m_VertexPath;
        std::string m_FragmentPath;
        
        // Cache uniform locations for performance
        mutable std::unordered_map<std::string, int> m_UniformLocationCache;
    };
    
    /**
     * @brief Shader library for managing multiple shaders
     */
    class ShaderLibrary {
    public:
        /**
         * @brief Add shader to library
         */
        void Add(const Ref<Shader>& shader);
        void Add(const std::string& name, const Ref<Shader>& shader);
        
        /**
         * @brief Load shader from files and add to library
         */
        Ref<Shader> Load(const std::string& filepath);
        Ref<Shader> Load(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath);
        
        /**
         * @brief Get shader by name
         */
        Ref<Shader> Get(const std::string& name);
        
        /**
         * @brief Check if shader exists
         */
        bool Exists(const std::string& name) const;
        
    private:
        std::unordered_map<std::string, Ref<Shader>> m_Shaders;
    };
}