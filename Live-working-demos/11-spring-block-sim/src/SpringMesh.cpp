#include "SpringMesh.h"
#include <glm/gtc/constants.hpp>
#include <cmath>

SpringMesh::~SpringMesh() {
    if (m_ebo) glDeleteBuffers(1, &m_ebo);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
}

void SpringMesh::init(int numCoils, int ringSegments, float coilRadius, float tubeRadius) {
    m_numCoils   = numCoils;
    m_ringSegs   = ringSegments;
    m_coilRadius = coilRadius;
    m_tubeRadius = tubeRadius;
    // number of helix sample points = numCoils * ringSegments + 1
    m_helixPoints = numCoils * ringSegments + 1;

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    // Pre-allocate (rebuild will fill data each frame)
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    int totalVerts   = m_helixPoints * (m_ringSegs + 1);
    int totalIndices = (m_helixPoints - 1) * m_ringSegs * 6;
    glBufferData(GL_ARRAY_BUFFER, totalVerts * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, totalIndices * sizeof(GLuint), nullptr, GL_DYNAMIC_DRAW);

    // pos
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    // normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    // uv
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));

    glBindVertexArray(0);
}

// Build an orthonormal frame for the spring axis
static void buildFrame(const glm::vec3& axis, glm::vec3& perpA, glm::vec3& perpB) {
    // Choose a helper vector not parallel to axis
    glm::vec3 helper = (std::abs(axis.y) < 0.9f) ? glm::vec3(0,1,0) : glm::vec3(1,0,0);
    perpA = glm::normalize(glm::cross(axis, helper));
    perpB = glm::cross(axis, perpA);
}

void SpringMesh::rebuild(const glm::vec3& start, const glm::vec3& end) {
    glm::vec3 delta  = end - start;
    float     length = glm::length(delta);
    if (length < 1e-5f) return;

    glm::vec3 axis   = delta / length;
    glm::vec3 perpA, perpB;
    buildFrame(axis, perpA, perpB);

    const float twoPi     = glm::two_pi<float>();
    const int   HP        = m_helixPoints;
    const int   R         = m_ringSegs;

    std::vector<Vertex>  verts;
    std::vector<GLuint>  indices;
    verts.reserve(HP * (R + 1));
    indices.reserve((HP - 1) * R * 6);

    // ── Generate helix center points ──────────────────────────────────────────
    std::vector<glm::vec3> centers(HP);
    std::vector<glm::vec3> tangents(HP);

    for (int i = 0; i < HP; ++i) {
        float t      = (float)i / (float)(HP - 1);
        float angle  = t * twoPi * m_numCoils;

        // Helix center: moves along axis + circles perpA/perpB
        centers[i]   = start + axis * (length * t)
                     + perpA * (m_coilRadius * std::cos(angle))
                     + perpB * (m_coilRadius * std::sin(angle));
    }

    // Compute tangents via central differences
    for (int i = 0; i < HP; ++i) {
        int prev = std::max(i - 1, 0);
        int next = std::min(i + 1, HP - 1);
        tangents[i] = glm::normalize(centers[next] - centers[prev]);
    }

    // ── Generate tube rings ───────────────────────────────────────────────────
    for (int i = 0; i < HP; ++i) {
        glm::vec3 T = tangents[i];
        // Stable normal to T using perpA projected out of T
        glm::vec3 rawN = glm::normalize(perpA - T * glm::dot(perpA, T));
        if (glm::length(rawN) < 1e-5f)
            rawN = glm::normalize(perpB - T * glm::dot(perpB, T));
        glm::vec3 B = glm::cross(T, rawN);

        for (int r = 0; r <= R; ++r) {
            float phi = twoPi * (float)r / (float)R;
            glm::vec3 tubeN  = rawN * std::cos(phi) + B * std::sin(phi);
            glm::vec3 vPos   = centers[i] + tubeN * m_tubeRadius;

            Vertex v;
            v.pos    = vPos;
            v.normal = glm::normalize(tubeN);
            v.uv     = glm::vec2((float)i / (float)(HP - 1),
                                 (float)r / (float)R);
            verts.push_back(v);
        }
    }

    // ── Generate indices ──────────────────────────────────────────────────────
    int stride = R + 1;
    for (int i = 0; i < HP - 1; ++i) {
        for (int r = 0; r < R; ++r) {
            GLuint a = i * stride + r;
            GLuint b = a + 1;
            GLuint c = (i + 1) * stride + r;
            GLuint d = c + 1;
            indices.push_back(a); indices.push_back(c); indices.push_back(b);
            indices.push_back(b); indices.push_back(c); indices.push_back(d);
        }
    }
    m_indexCount = (int)indices.size();

    // ── Upload ────────────────────────────────────────────────────────────────
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, verts.size() * sizeof(Vertex), verts.data());

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());

    glBindVertexArray(0);
}

void SpringMesh::draw() const {
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}
