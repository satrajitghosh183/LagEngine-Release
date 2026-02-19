#pragma once

#include <string>
#include <memory>
#include <glm/glm.hpp>

class Shader {
public:
    Shader();
    ~Shader();
    
    bool loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);
    bool loadFromSource(const std::string& vertexSrc, const std::string& fragmentSrc);
    
    void use() const;
    unsigned int getID() const { return m_programID; }
    
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec4(const std::string& name, const glm::vec4& value) const;
    void setMat4(const std::string& name, const glm::mat4& value) const;

private:
    unsigned int m_programID;
    
    std::string readFile(const std::string& path);
    unsigned int compileShader(unsigned int type, const std::string& source);
    bool linkProgram(unsigned int vertex, unsigned int fragment);
};

