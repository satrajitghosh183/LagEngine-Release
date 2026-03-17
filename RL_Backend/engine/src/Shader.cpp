#include "rldemo/Shader.hpp"
#include <glad/glad.h>
#include <spdlog/spdlog.h>
#include <cstring>

namespace rldemo {

static uint32_t compileShader(uint32_t type, const char* src) {
    uint32_t id = glCreateShader(type);
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);
    int ok;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[1024];
        glGetShaderInfoLog(id, sizeof(buf), nullptr, buf);
        spdlog::error("Shader compile error: {}", buf);
        glDeleteShader(id);
        return 0;
    }
    return id;
}

Shader::Shader(const char* vertexSrc, const char* fragmentSrc) {
    uint32_t vs = compileShader(GL_VERTEX_SHADER, vertexSrc);
    uint32_t fs = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);
    if (!vs || !fs) return;
    m_Program = glCreateProgram();
    glAttachShader(m_Program, vs);
    glAttachShader(m_Program, fs);
    glLinkProgram(m_Program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    int ok;
    glGetProgramiv(m_Program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char buf[1024];
        glGetProgramInfoLog(m_Program, sizeof(buf), nullptr, buf);
        spdlog::error("Shader link error: {}", buf);
        glDeleteProgram(m_Program);
        m_Program = 0;
    }
}

Shader::~Shader() {
    if (m_Program) glDeleteProgram(m_Program);
}

void Shader::Bind() const {
    glUseProgram(m_Program);
}

void Shader::SetMat4(const char* name, const float* m) {
    int loc = glGetUniformLocation(m_Program, name);
    if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, m);
}

void Shader::SetVec4(const char* name, float x, float y, float z, float w) {
    int loc = glGetUniformLocation(m_Program, name);
    if (loc >= 0) glUniform4f(loc, x, y, z, w);
}

} // namespace rldemo
