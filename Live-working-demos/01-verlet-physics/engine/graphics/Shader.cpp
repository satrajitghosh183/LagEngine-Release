

// Shader.cpp
#include "engine/graphics/Shader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <glad/glad.h>
#include "engine/core/Logger.hpp"

namespace engine::graphics {

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) : programID(0) {
    using namespace engine::core::log;
    
    // Log the paths we're trying to load
    Logger::log("Loading shaders from: " + vertexPath + " and " + fragmentPath, LogLevel::Info);
    
    std::string vertexSource = loadShaderSource(vertexPath);
    std::string fragmentSource = loadShaderSource(fragmentPath);
    
    // Check if shader sources were loaded successfully
    if (vertexSource.empty()) {
        Logger::log("Failed to load vertex shader source from: " + vertexPath, LogLevel::Error);
        return;
    }
    
    if (fragmentSource.empty()) {
        Logger::log("Failed to load fragment shader source from: " + fragmentPath, LogLevel::Error);
        return;
    }
    
    // Log successful loading
    Logger::log("Loaded shader sources: " + 
                std::to_string(vertexSource.length()) + " bytes vertex, " + 
                std::to_string(fragmentSource.length()) + " bytes fragment", LogLevel::Info);
    
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    
    // Check if compilation was successful
    if (vertexShader == 0 || fragmentShader == 0) {
        Logger::log("Shader compilation failed, cannot link program", LogLevel::Error);
        return;
    }
    
    linkProgram(vertexShader, fragmentShader);
    
    // Cleanup
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

Shader::~Shader() {
    if (programID != 0) {
        glDeleteProgram(programID);
    }
}

void Shader::bind() const {
    if (programID != 0) {
        glUseProgram(programID);
    }
}
void Shader::setMaterial(const std::string& prefix, const glm::vec3& ambient, const glm::vec3& diffuse, const glm::vec3& specular, float shininess) const {
    setUniformVec3(prefix + ".ambient", ambient);
    setUniformVec3(prefix + ".diffuse", diffuse);
    setUniformVec3(prefix + ".specular", specular);
    setUniformFloat(prefix + ".shininess", shininess);
}

void Shader::setLight(const std::string& prefix, const glm::vec3& position, const glm::vec3& ambient, const glm::vec3& diffuse, const glm::vec3& specular) const {
    setUniformVec3(prefix + ".position", position);
    setUniformVec3(prefix + ".ambient", ambient);
    setUniformVec3(prefix + ".diffuse", diffuse);
    setUniformVec3(prefix + ".specular", specular);
}


void Shader::unbind() const {
    glUseProgram(0);
}

void Shader::setUniformMat4(const std::string& name, const glm::mat4& matrix) const {
    if (programID == 0) return;
    int location = glGetUniformLocation(programID, name.c_str());
    glUniformMatrix4fv(location, 1, GL_FALSE, &matrix[0][0]);
}

void Shader::setUniformVec3(const std::string& name, const glm::vec3& vector) const {
    if (programID == 0) return;
    int location = glGetUniformLocation(programID, name.c_str());
    glUniform3fv(location, 1, &vector[0]);
}

void Shader::setUniformFloat(const std::string& name, float value) const {
    if (programID == 0) return;
    int location = glGetUniformLocation(programID, name.c_str());
    glUniform1f(location, value);
}

void Shader::setUniformInt(const std::string& name, int value) const {
    if (programID == 0) return;
    int location = glGetUniformLocation(programID, name.c_str());
    glUniform1i(location, value);
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat) const {
    if (programID == 0) return;
    int location = glGetUniformLocation(programID, name.c_str());
    glUniformMatrix4fv(location, 1, GL_FALSE, &mat[0][0]);
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
    if (programID == 0) return;
    int location = glGetUniformLocation(programID, name.c_str());
    glUniform3fv(location, 1, &value[0]);
}

void Shader::setFloat(const std::string& name, float value) const {
    if (programID == 0) return;
    int location = glGetUniformLocation(programID, name.c_str());
    glUniform1f(location, value);
}

std::string Shader::loadShaderSource(const std::string& filepath) const {
    using namespace engine::core::log;
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        Logger::log("ERROR: Failed to open shader file: " + filepath, LogLevel::Error);
        return ""; // Return empty string on failure
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    std::string content = buffer.str();
    if (content.empty()) {
        Logger::log("WARNING: Shader file is empty: " + filepath, LogLevel::Error);
    } else {
        Logger::log("Successfully loaded shader of length: " + std::to_string(content.length()) + " bytes", LogLevel::Info);
    }
    
    return content;
}

unsigned int Shader::compileShader(unsigned int type, const std::string& source) const {
    using namespace engine::core::log;
    
    if (source.empty()) {
        Logger::log("ERROR: Cannot compile empty shader source", LogLevel::Error);
        return 0; // Return invalid shader ID
    }
    
    unsigned int shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        Logger::log("ERROR: Shader Compilation Failed\n" + std::string(infoLog), LogLevel::Error);
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

void Shader::linkProgram(unsigned int vertexShader, unsigned int fragmentShader) {
    using namespace engine::core::log;
    
    if (vertexShader == 0 || fragmentShader == 0) {
        Logger::log("ERROR: Cannot link with invalid shader IDs", LogLevel::Error);
        programID = 0;
        return;
    }
    
    programID = glCreateProgram();
    glAttachShader(programID, vertexShader);
    glAttachShader(programID, fragmentShader);
    glLinkProgram(programID);
    
    // Check for linking errors
    int success;
    glGetProgramiv(programID, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(programID, 512, nullptr, infoLog);
        Logger::log("ERROR::SHADER::PROGRAM::LINKING_FAILED\n" + std::string(infoLog), LogLevel::Error);
        glDeleteProgram(programID);
        programID = 0;
    }
}

} // namespace engine::graphics