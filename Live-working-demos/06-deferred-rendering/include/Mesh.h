#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <memory>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
};

class Mesh {
public:
    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
    ~Mesh();
    
    void render() const;
    void renderInstanced(int count) const;

private:
    unsigned int m_VAO;
    unsigned int m_VBO;
    unsigned int m_EBO;
    size_t m_indexCount;
    
    void setupMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
};

// Factory functions
std::shared_ptr<Mesh> createCubeMesh();
std::shared_ptr<Mesh> createSphereMesh(int segments = 32);

// Fullscreen quad rendering (for post-processing)
void renderFullscreenQuad();

