#include "Shader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

static std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) { std::cerr << "[Shader] Cannot open: " << path << "\n"; return ""; }
    std::ostringstream ss; ss << f.rdbuf();
    return ss.str();
}

static GLuint compileShader(GLenum type, const std::string& src) {
    GLuint sh = glCreateShader(type);
    const char* c = src.c_str();
    glShaderSource(sh, 1, &c, nullptr);
    glCompileShader(sh);
    GLint ok; glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024]; glGetShaderInfoLog(sh, 1024, nullptr, log);
        std::cerr << "[Shader] Compile error:\n" << log << "\n";
    }
    return sh;
}

Shader::Shader(const std::string& vertPath, const std::string& fragPath) {
    std::string vsrc = readFile(vertPath);
    std::string fsrc = readFile(fragPath);
    if (vsrc.empty() || fsrc.empty()) return;

    GLuint vs = compileShader(GL_VERTEX_SHADER,   vsrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsrc);

    id = glCreateProgram();
    glAttachShader(id, vs); glAttachShader(id, fs);
    glLinkProgram(id);

    GLint ok; glGetProgramiv(id, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024]; glGetProgramInfoLog(id, 1024, nullptr, log);
        std::cerr << "[Shader] Link error:\n" << log << "\n";
    }
    glDeleteShader(vs); glDeleteShader(fs);
}

Shader::~Shader() {
    if (id) glDeleteProgram(id);
}

Shader& Shader::operator=(Shader&& o) noexcept {
    if (this != &o) {
        if (id) glDeleteProgram(id);
        id = o.id;
        o.id = 0;
    }
    return *this;
}

GLint Shader::loc(const std::string& name) const {
    return glGetUniformLocation(id, name.c_str());
}

void Shader::setInt  (const std::string& n, int v)               const { glUniform1i (loc(n), v); }
void Shader::setFloat(const std::string& n, float v)             const { glUniform1f (loc(n), v); }
void Shader::setBool (const std::string& n, bool v)              const { glUniform1i (loc(n), (int)v); }
void Shader::setVec2 (const std::string& n, const glm::vec2& v)  const { glUniform2fv(loc(n), 1, glm::value_ptr(v)); }
void Shader::setVec3 (const std::string& n, const glm::vec3& v)  const { glUniform3fv(loc(n), 1, glm::value_ptr(v)); }
void Shader::setMat3 (const std::string& n, const glm::mat3& v)  const { glUniformMatrix3fv(loc(n), 1, GL_FALSE, glm::value_ptr(v)); }
void Shader::setMat4 (const std::string& n, const glm::mat4& v)  const { glUniformMatrix4fv(loc(n), 1, GL_FALSE, glm::value_ptr(v)); }
