#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>

// Generates and uploads a 3D helix-tube mesh for a spring.
// The tube follows a helical path between two world-space endpoints.
// Call rebuild() each frame with updated endpoints.
class SpringMesh {
public:
    SpringMesh() = default;
    ~SpringMesh();

    // Must call once before use
    void init(int numCoils = 8, int ringSegments = 12, float coilRadius = 0.12f, float tubeRadius = 0.035f);

    // Re-generates the mesh for the given endpoints and uploads to GPU.
    void rebuild(const glm::vec3& start, const glm::vec3& end);

    // Draws the mesh (GL_TRIANGLES, uses currently bound shader)
    void draw() const;

    bool ready() const { return m_vao != 0; }

private:
    // Geometry parameters
    int   m_numCoils     = 8;
    int   m_ringSegs     = 12;
    float m_coilRadius   = 0.12f;
    float m_tubeRadius   = 0.035f;

    // Number of helix sample points and total ring verts
    int m_helixPoints    = 0;

    GLuint m_vao = 0, m_vbo = 0, m_ebo = 0;
    int    m_indexCount = 0;

    // Interleaved: pos(3) + normal(3) + uv(2)
    struct Vertex { glm::vec3 pos, normal; glm::vec2 uv; };
};
