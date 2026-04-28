#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <memory>
#include <vulkan/vulkan.h>
#include "VulkanBase.hpp"

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
};

class Mesh {
public:
    Mesh() = default;
    ~Mesh();

    // Must call init after VulkanBase is ready
    void init(vkdemo::VulkanBase* vkBase,
              const std::vector<Vertex>& vertices,
              const std::vector<unsigned int>& indices);
    void cleanup();

    void render(VkCommandBuffer cmd) const;

    size_t getIndexCount() const { return m_indexCount; }

private:
    vkdemo::VulkanBase* m_vkBase = nullptr;
    vkdemo::GPUBuffer m_vertexBuffer{};
    vkdemo::GPUBuffer m_indexBuffer{};
    size_t m_indexCount = 0;
};

// Factory helpers (lazy -- caller must call init() with VulkanBase)
struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

MeshData createCubeMeshData();
MeshData createSphereMeshData(int segments = 32);

// Fullscreen-quad mesh data (2 triangles, pos+uv interleaved)
struct QuadVertex {
    glm::vec2 position;
    glm::vec2 texCoord;
};

MeshData createQuadMeshData();
