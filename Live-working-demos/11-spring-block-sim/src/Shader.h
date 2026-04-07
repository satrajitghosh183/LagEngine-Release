#pragma once
#include <string>
#include <glm/glm.hpp>
#include <glad/glad.h>

class Shader {
public:
    GLuint id = 0;

    Shader() = default;
    Shader(const std::string& vertPath, const std::string& fragPath);
    ~Shader();

    // Non-copyable, movable
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& o) noexcept : id(o.id) { o.id = 0; }
    Shader& operator=(Shader&& o) noexcept;

    void use() const { glUseProgram(id); }

    void setInt  (const std::string& name, int v)              const;
    void setFloat(const std::string& name, float v)            const;
    void setBool (const std::string& name, bool v)             const;
    void setVec2 (const std::string& name, const glm::vec2& v) const;
    void setVec3 (const std::string& name, const glm::vec3& v) const;
    void setMat3 (const std::string& name, const glm::mat3& v) const;
    void setMat4 (const std::string& name, const glm::mat4& v) const;

private:
    GLint loc(const std::string& name) const;
};
