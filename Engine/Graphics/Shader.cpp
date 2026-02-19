#include "Shader.hpp"
#include "../Core/Logger.hpp"
#include "../Core/RuntimePaths.hpp"
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>

namespace GameEngine {

    // ========== Shader ==========

    Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath)
        : m_VertexPath(vertexPath), m_FragmentPath(fragmentPath) {
        
        // Extract name from file path
        auto lastSlash = vertexPath.find_last_of("/\\");
        lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
        auto lastDot = vertexPath.rfind('.');
        auto count = lastDot == std::string::npos ? vertexPath.size() - lastSlash : lastDot - lastSlash;
        m_Name = vertexPath.substr(lastSlash, count);
        
        std::string resolvedVert = RuntimePaths::ResolveShader(vertexPath);
        std::string resolvedFrag = RuntimePaths::ResolveShader(fragmentPath);
        std::string vertexSrc = ReadFile(resolvedVert);
        std::string fragmentSrc = ReadFile(resolvedFrag);
        
        CompileShaders(vertexSrc, fragmentSrc);
        
        GE_CORE_INFO("Shader '{0}' created successfully", m_Name);
    }

    Shader::Shader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc)
        : m_Name(name) {
        CompileShaders(vertexSrc, fragmentSrc);
        GE_CORE_INFO("Shader '{0}' created from source", m_Name);
    }

    Shader::~Shader() {
        glDeleteProgram(m_RendererID);
    }

    void Shader::CompileShaders(const std::string& vertexSrc, const std::string& fragmentSrc) {
        // Vertex Shader
        uint32_t vertexShader = glCreateShader(GL_VERTEX_SHADER);
        const char* vertexSource = vertexSrc.c_str();
        glShaderSource(vertexShader, 1, &vertexSource, nullptr);
        glCompileShader(vertexShader);
        CheckCompileErrors(vertexShader, "VERTEX");
        
        // Fragment Shader
        uint32_t fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        const char* fragmentSource = fragmentSrc.c_str();
        glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
        glCompileShader(fragmentShader);
        CheckCompileErrors(fragmentShader, "FRAGMENT");
        
        // Shader Program
        m_RendererID = glCreateProgram();
        glAttachShader(m_RendererID, vertexShader);
        glAttachShader(m_RendererID, fragmentShader);
        glLinkProgram(m_RendererID);
        CheckCompileErrors(m_RendererID, "PROGRAM");
        
        // Clean up
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    std::string Shader::ReadFile(const std::string& filepath) {
        std::ifstream file(filepath);
        
        if (!file.is_open()) {
            GE_CORE_ERROR("Failed to open shader file: {0}", filepath);
            return "";
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        
        return buffer.str();
    }

    void Shader::CheckCompileErrors(uint32_t shader, const std::string& type) {
        int success;
        char infoLog[1024];
        
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
                GE_CORE_ERROR("Shader compilation error ({0}):\n{1}", type, infoLog);
            }
        } else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
                GE_CORE_ERROR("Shader linking error:\n{0}", infoLog);
            }
        }
    }

    void Shader::Bind() const {
        glUseProgram(m_RendererID);
    }

    void Shader::Unbind() const {
        glUseProgram(0);
    }

    void Shader::Reload() {
        if (!m_VertexPath.empty() && !m_FragmentPath.empty()) {
            GE_CORE_INFO("Reloading shader '{0}'...", m_Name);
            
            std::string resolvedVert = RuntimePaths::ResolveShader(m_VertexPath);
            std::string resolvedFrag = RuntimePaths::ResolveShader(m_FragmentPath);
            std::string vertexSrc = ReadFile(resolvedVert);
            std::string fragmentSrc = ReadFile(resolvedFrag);
            
            // Delete old program
            glDeleteProgram(m_RendererID);
            
            // Clear cache
            m_UniformLocationCache.clear();
            
            // Recompile
            CompileShaders(vertexSrc, fragmentSrc);
            
            GE_CORE_INFO("Shader '{0}' reloaded successfully", m_Name);
        }
    }

    void Shader::ReloadFromSource(const std::string& vertexSrc, const std::string& fragmentSrc) {
        GE_CORE_INFO("Reloading shader '{0}' from source...", m_Name);
        glDeleteProgram(m_RendererID);
        m_UniformLocationCache.clear();
        CompileShaders(vertexSrc, fragmentSrc);
        GE_CORE_INFO("Shader '{0}' reloaded from source successfully", m_Name);
    }

    int Shader::GetUniformLocation(const std::string& name) {
        // Check cache first
        if (m_UniformLocationCache.find(name) != m_UniformLocationCache.end())
            return m_UniformLocationCache[name];
        
        // Query OpenGL
        int location = glGetUniformLocation(m_RendererID, name.c_str());
        
        if (location == -1) {
            GE_CORE_WARN("Uniform '{0}' not found in shader '{1}'", name, m_Name);
        }
        
        m_UniformLocationCache[name] = location;
        return location;
    }

    void Shader::SetUniformBool(const std::string& name, bool value) {
        glUniform1i(GetUniformLocation(name), value ? 1 : 0);
    }

    void Shader::SetUniformInt(const std::string& name, int value) {
        glUniform1i(GetUniformLocation(name), value);
    }

    void Shader::SetUniformIntArray(const std::string& name, int* values, uint32_t count) {
        glUniform1iv(GetUniformLocation(name), count, values);
    }

    void Shader::SetUniformFloat(const std::string& name, float value) {
        glUniform1f(GetUniformLocation(name), value);
    }

    void Shader::SetUniformVec2(const std::string& name, const glm::vec2& value) {
        glUniform2f(GetUniformLocation(name), value.x, value.y);
    }

    void Shader::SetUniformVec3(const std::string& name, const glm::vec3& value) {
        glUniform3f(GetUniformLocation(name), value.x, value.y, value.z);
    }

    void Shader::SetUniformVec4(const std::string& name, const glm::vec4& value) {
        glUniform4f(GetUniformLocation(name), value.x, value.y, value.z, value.w);
    }

    void Shader::SetUniformMat3(const std::string& name, const glm::mat3& value) {
        glUniformMatrix3fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
    }

    void Shader::SetUniformMat4(const std::string& name, const glm::mat4& value) {
        glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
    }

    // ========== ShaderLibrary ==========

    void ShaderLibrary::Add(const Ref<Shader>& shader) {
        auto& name = shader->GetName();
        Add(name, shader);
    }

    void ShaderLibrary::Add(const std::string& name, const Ref<Shader>& shader) {
        GE_CORE_ASSERT(!Exists(name), "Shader already exists!");
        m_Shaders[name] = shader;
    }

    Ref<Shader> ShaderLibrary::Load(const std::string& filepath) {
        auto shader = CreateRef<Shader>(filepath, filepath);
        Add(shader);
        return shader;
    }

    Ref<Shader> ShaderLibrary::Load(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath) {
        auto shader = CreateRef<Shader>(vertexPath, fragmentPath);
        Add(name, shader);
        return shader;
    }

    Ref<Shader> ShaderLibrary::Get(const std::string& name) {
        GE_CORE_ASSERT(Exists(name), "Shader not found!");
        return m_Shaders[name];
    }

    bool ShaderLibrary::Exists(const std::string& name) const {
        return m_Shaders.find(name) != m_Shaders.end();
    }
}