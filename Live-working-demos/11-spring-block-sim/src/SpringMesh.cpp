#include "SpringMesh.h"
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <cstring>

SpringMesh::~SpringMesh() {
    cleanup();
}

void SpringMesh::cleanup() {
    if (m_vkBase) {
        if (m_vertexBuffer.Buffer != VK_NULL_HANDLE)
            m_vkBase->DestroyBuffer(m_vertexBuffer);
        if (m_indexBuffer.Buffer != VK_NULL_HANDLE)
            m_vkBase->DestroyBuffer(m_indexBuffer);
    }
    m_vertexBuffer = {};
    m_indexBuffer = {};
}

SpringMesh::SpringMesh(SpringMesh&& o) noexcept
    : m_vkBase(o.m_vkBase)
    , m_numCoils(o.m_numCoils)
    , m_ringSegs(o.m_ringSegs)
    , m_coilRadius(o.m_coilRadius)
    , m_tubeRadius(o.m_tubeRadius)
    , m_helixPoints(o.m_helixPoints)
    , m_vertexBuffer(o.m_vertexBuffer)
    , m_indexBuffer(o.m_indexBuffer)
    , m_indexCount(o.m_indexCount)
{
    o.m_vertexBuffer = {};
    o.m_indexBuffer = {};
    o.m_vkBase = nullptr;
}

SpringMesh& SpringMesh::operator=(SpringMesh&& o) noexcept {
    if (this != &o) {
        cleanup();
        m_vkBase = o.m_vkBase;
        m_numCoils = o.m_numCoils;
        m_ringSegs = o.m_ringSegs;
        m_coilRadius = o.m_coilRadius;
        m_tubeRadius = o.m_tubeRadius;
        m_helixPoints = o.m_helixPoints;
        m_vertexBuffer = o.m_vertexBuffer;
        m_indexBuffer = o.m_indexBuffer;
        m_indexCount = o.m_indexCount;
        o.m_vertexBuffer = {};
        o.m_indexBuffer = {};
        o.m_vkBase = nullptr;
    }
    return *this;
}

void SpringMesh::init(vkdemo::VulkanBase* vkBase, int numCoils, int ringSegments,
                      float coilRadius, float tubeRadius) {
    m_vkBase     = vkBase;
    m_numCoils   = numCoils;
    m_ringSegs   = ringSegments;
    m_coilRadius = coilRadius;
    m_tubeRadius = tubeRadius;
    m_helixPoints = numCoils * ringSegments + 1;

    // Pre-allocate host-visible buffers large enough for max mesh
    int totalVerts   = m_helixPoints * (m_ringSegs + 1);
    int totalIndices = (m_helixPoints - 1) * m_ringSegs * 6;

    m_vertexBuffer = m_vkBase->CreateBuffer(
        totalVerts * sizeof(Vertex),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    m_indexBuffer = m_vkBase->CreateBuffer(
        totalIndices * sizeof(uint32_t),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

// Build an orthonormal frame for the spring axis
static void buildFrame(const glm::vec3& axis, glm::vec3& perpA, glm::vec3& perpB) {
    glm::vec3 helper = (std::abs(axis.y) < 0.9f) ? glm::vec3(0,1,0) : glm::vec3(1,0,0);
    perpA = glm::normalize(glm::cross(axis, helper));
    perpB = glm::cross(axis, perpA);
}

void SpringMesh::rebuild(const glm::vec3& start, const glm::vec3& end) {
    if (!m_vkBase) return;

    glm::vec3 delta  = end - start;
    float     length = glm::length(delta);
    if (length < 1e-5f) return;

    glm::vec3 axis   = delta / length;
    glm::vec3 perpA, perpB;
    buildFrame(axis, perpA, perpB);

    const float twoPi     = glm::two_pi<float>();
    const int   HP        = m_helixPoints;
    const int   R         = m_ringSegs;

    std::vector<Vertex>    verts;
    std::vector<uint32_t>  indices;
    verts.reserve(HP * (R + 1));
    indices.reserve((HP - 1) * R * 6);

    // Generate helix center points
    std::vector<glm::vec3> centers(HP);
    std::vector<glm::vec3> tangents(HP);

    for (int i = 0; i < HP; ++i) {
        float t      = (float)i / (float)(HP - 1);
        float angle  = t * twoPi * m_numCoils;
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

    // Generate tube rings
    for (int i = 0; i < HP; ++i) {
        glm::vec3 T = tangents[i];
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

    // Generate indices
    int stride = R + 1;
    for (int i = 0; i < HP - 1; ++i) {
        for (int r = 0; r < R; ++r) {
            uint32_t a = i * stride + r;
            uint32_t b = a + 1;
            uint32_t c = (i + 1) * stride + r;
            uint32_t d = c + 1;
            indices.push_back(a); indices.push_back(c); indices.push_back(b);
            indices.push_back(b); indices.push_back(c); indices.push_back(d);
        }
    }
    m_indexCount = (int)indices.size();

    // Upload to host-visible buffers
    m_vkBase->CopyToBuffer(m_vertexBuffer, verts.data(),
                           verts.size() * sizeof(Vertex));
    m_vkBase->CopyToBuffer(m_indexBuffer, indices.data(),
                           indices.size() * sizeof(uint32_t));
}

void SpringMesh::draw(VkCommandBuffer cmd) const {
    if (m_indexCount <= 0 || m_vertexBuffer.Buffer == VK_NULL_HANDLE) return;

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_vertexBuffer.Buffer, &offset);
    vkCmdBindIndexBuffer(cmd, m_indexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmd, static_cast<uint32_t>(m_indexCount), 1, 0, 0, 0);
}
