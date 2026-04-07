// engine/graphics/Mesh.cpp
#include "engine/graphics/Mesh.hpp"
#include <glad/glad.h>

namespace engine::graphics {

    void Mesh::upload(bool dynamic) {
        if (VAO == 0) glGenVertexArrays(1, &VAO);
        if (VBO == 0) glGenBuffers(1, &VBO);
        if (NBO == 0) glGenBuffers(1, &NBO);

        glBindVertexArray(VAO);

        // Upload vertex positions
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3),
                     vertices.data(), dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(0);

        // Upload vertex normals
        glBindBuffer(GL_ARRAY_BUFFER, NBO);
        glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3),
                     normals.data(), dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(1);

        // Optional indices
        if (!indices.empty()) {
            if (EBO == 0) glGenBuffers(1, &EBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                         indices.data(), GL_STATIC_DRAW);
            useIndices = true;
        }

        glBindVertexArray(0);
    }

    void Mesh::updateVertices() const {
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(glm::vec3), vertices.data());
    }

    void Mesh::updateNormals() const {
        glBindBuffer(GL_ARRAY_BUFFER, NBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normals.size() * sizeof(glm::vec3), normals.data());
    }

    void Mesh::draw(GLenum mode) const {
        glBindVertexArray(VAO);
        if (useIndices) {
            glDrawElements(mode, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
        } else {
            glDrawArrays(mode, 0, static_cast<GLsizei>(vertices.size()));
        }
        glBindVertexArray(0);
    }

    void Mesh::destroy() {
        if (VBO) glDeleteBuffers(1, &VBO);
        if (NBO) glDeleteBuffers(1, &NBO);
        if (EBO) glDeleteBuffers(1, &EBO);
        if (VAO) glDeleteVertexArrays(1, &VAO);
        VBO = VAO = EBO = NBO = 0;
    }

} // namespace engine::graphics
