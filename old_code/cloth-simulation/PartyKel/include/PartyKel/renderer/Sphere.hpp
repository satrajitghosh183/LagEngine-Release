#pragma once

#include <vector>
#include <GL/glew.h>
#include "PartyKel/glm.hpp"

namespace PartyKel {


class Sphere {
  
    void build(GLfloat radius, GLsizei discLat, GLsizei discLong);

public:
    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
    };

    std::vector<glm::vec3> _positions;

  
    Sphere(GLfloat radius, GLsizei discLat, GLsizei discLong):
        m_nVertexCount(0) {
        build(radius, discLat, discLong); 
    }

   
    const Vertex* getDataPointer() const {
        return &m_Vertices[0];
    }


    GLsizei getVertexCount() const {
        return m_nVertexCount;
    }

private:
    std::vector<Vertex> m_Vertices;
    GLsizei m_nVertexCount; 
};

struct SphereHandler{
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> colors;
    std::vector<float> radius;
};
    
}
