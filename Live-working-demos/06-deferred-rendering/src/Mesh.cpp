#include "Mesh.h"
#include <cmath>

// ---- Mesh implementation (Vulkan) ------------------------------------------

Mesh::~Mesh() {
    cleanup();
}

void Mesh::init(vkdemo::VulkanBase* vkBase,
                const std::vector<Vertex>& vertices,
                const std::vector<unsigned int>& indices)
{
    m_vkBase = vkBase;
    m_indexCount = indices.size();

    m_vertexBuffer = vkBase->CreateVertexBuffer(
        vertices.data(),
        static_cast<VkDeviceSize>(vertices.size() * sizeof(Vertex)));

    m_indexBuffer = vkBase->CreateIndexBuffer(
        indices.data(),
        static_cast<VkDeviceSize>(indices.size() * sizeof(unsigned int)));
}

void Mesh::cleanup() {
    if (m_vkBase) {
        m_vkBase->DestroyBuffer(m_vertexBuffer);
        m_vkBase->DestroyBuffer(m_indexBuffer);
        m_vkBase = nullptr;
    }
}

void Mesh::render(VkCommandBuffer cmd) const {
    if (m_vertexBuffer.Buffer == VK_NULL_HANDLE) return;

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_vertexBuffer.Buffer, &offset);
    vkCmdBindIndexBuffer(cmd, m_indexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmd, static_cast<uint32_t>(m_indexCount), 1, 0, 0, 0);
}

// ---- Factory data generators -----------------------------------------------

MeshData createCubeMeshData() {
    MeshData data;

    data.vertices = {
        // Front face
        {{-1.0f, -1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-1.0f,  1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        // Back face
        {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
        {{ 1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
        {{ 1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
        {{-1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
        // Left face
        {{-1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{-1.0f, -1.0f,  1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{-1.0f,  1.0f,  1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        {{-1.0f,  1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        // Right face
        {{ 1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{ 1.0f, -1.0f,  1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        {{ 1.0f,  1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        // Top face
        {{-1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
        {{-1.0f,  1.0f,  1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        // Bottom face
        {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
        {{ 1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
        {{ 1.0f, -1.0f,  1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
        {{-1.0f, -1.0f,  1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}}
    };

    data.indices = {
        0, 1, 2,  2, 3, 0,     // Front
        4, 5, 6,  6, 7, 4,     // Back
        8, 9, 10, 10, 11, 8,   // Left
        12, 13, 14, 14, 15, 12,// Right
        16, 17, 18, 18, 19, 16,// Top
        20, 21, 22, 22, 23, 20 // Bottom
    };

    return data;
}

MeshData createSphereMeshData(int segments) {
    MeshData data;
    const float PI = 3.14159265359f;

    for (int y = 0; y <= segments; ++y) {
        for (int x = 0; x <= segments; ++x) {
            float xSeg = (float)x / (float)segments;
            float ySeg = (float)y / (float)segments;
            float xPos = std::cos(xSeg * 2.0f * PI) * std::sin(ySeg * PI);
            float yPos = std::cos(ySeg * PI);
            float zPos = std::sin(xSeg * 2.0f * PI) * std::sin(ySeg * PI);

            Vertex v;
            v.position = glm::vec3(xPos, yPos, zPos);
            v.normal   = glm::vec3(xPos, yPos, zPos);
            v.texCoords = glm::vec2(xSeg, ySeg);
            data.vertices.push_back(v);
        }
    }

    for (int y = 0; y < segments; ++y) {
        for (int x = 0; x < segments; ++x) {
            unsigned int topLeft  = y * (segments + 1) + x;
            unsigned int topRight = topLeft + 1;
            unsigned int botLeft  = (y + 1) * (segments + 1) + x;
            unsigned int botRight = botLeft + 1;

            data.indices.push_back(topLeft);
            data.indices.push_back(botLeft);
            data.indices.push_back(topRight);

            data.indices.push_back(topRight);
            data.indices.push_back(botLeft);
            data.indices.push_back(botRight);
        }
    }

    return data;
}

MeshData createQuadMeshData() {
    MeshData data;

    // Fullscreen quad: position in xy, normal unused, texcoord in texCoords
    data.vertices = {
        {{-1.0f,  1.0f, 0.0f}, {0,0,1}, {0.0f, 1.0f}},
        {{-1.0f, -1.0f, 0.0f}, {0,0,1}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f, 0.0f}, {0,0,1}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f, 0.0f}, {0,0,1}, {1.0f, 1.0f}},
    };

    data.indices = {0, 1, 2, 2, 3, 0};
    return data;
}
