


// Shader.hpp
#pragma once

#include <string>
#include <glm/glm.hpp>

namespace engine::graphics {

class Shader {
public:
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();

    void bind() const;
    void unbind() const;
    void setMaterial(const std::string& prefix, const glm::vec3& ambient, const glm::vec3& diffuse, const glm::vec3& specular, float shininess) const;
    void setLight(const std::string& prefix, const glm::vec3& position, const glm::vec3& ambient, const glm::vec3& diffuse, const glm::vec3& specular) const;

    void setUniformMat4(const std::string& name, const glm::mat4& matrix) const;
    void setUniformVec3(const std::string& name, const glm::vec3& vector) const;
    void setUniformFloat(const std::string& name, float value) const;
    void setUniformInt(const std::string& name, int value) const;
    void setMat4(const std::string& name, const glm::mat4& mat) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setFloat(const std::string& name, float value) const;
    bool isValid() const { return programID != 0; }

private:
    unsigned int programID;

    std::string loadShaderSource(const std::string& filepath) const;
    unsigned int compileShader(unsigned int type, const std::string& source) const;
    void linkProgram(unsigned int vertexShader, unsigned int fragmentShader);
};

} // namespace engine::graphics