
#pragma once

#include <vector>
#include <glm/glm.hpp>

namespace engine::graphics {

struct Vertex3D {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

class Mesh3D {
public:
    Mesh3D();
    ~Mesh3D();

    void uploadData(const std::vector<Vertex3D>& vertices, const std::vector<unsigned int>& indices);
    void updateVertices(const std::vector<Vertex3D>& vertices);
    void draw() const;

private:
    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;

    size_t vertexCount;
    size_t indexCount;
};

} // namespace engine::graphics
