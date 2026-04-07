
#pragma once
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

namespace engine::graphics {

    class Mesh {
    public:
        std::vector<glm::vec3> vertices;
        std::vector<glm::vec3> normals;  
        std::vector<unsigned int> indices;

        unsigned int VAO = 0, VBO = 0, NBO = 0, EBO = 0;
        bool useIndices = false;

        void upload(bool dynamic = false);
        void updateVertices() const;
        void updateNormals() const;
        void draw(GLenum mode = GL_TRIANGLES) const;
        void destroy();
    };

}
