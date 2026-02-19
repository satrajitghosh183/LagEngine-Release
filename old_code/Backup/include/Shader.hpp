#pragma once
#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>

class Shader {
public:
    unsigned int ID;
    Shader(const char* vertexPath, const char* fragmentPath);
    void use() const;
    void setMat4(const std::string &name, const glm::mat4 &mat) const;
    void setVec3(const std::string &name, const glm::vec3 &vec) const;
    void setFloat(const std::string &name, float value) const;
};
