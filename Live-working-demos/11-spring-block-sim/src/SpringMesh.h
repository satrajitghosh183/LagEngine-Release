#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <VulkanBase.hpp>

// Generates and uploads a 3D helix-tube mesh for a spring.
// The tube follows a helical path between two world-space endpoints.
// Call rebuild() each frame with updated endpoints.
class SpringMesh {
public:
    SpringMesh() = default;
    ~SpringMesh();

    // Non-copyable (owns Vulkan resources)
    SpringMesh(const SpringMesh&) = delete;
    SpringMesh& operator=(const SpringMesh&) = delete;
    SpringMesh(SpringMesh&& o) noexcept;
    SpringMesh& operator=(SpringMesh&& o) noexcept;

    // Must call once before use
    void init(vkdemo::VulkanBase* vkBase, int numCoils = 8, int ringSegments = 12,
              float coilRadius = 0.12f, float tubeRadius = 0.035f);

    // Re-generates the mesh for the given endpoints and uploads to GPU.
    void rebuild(const glm::vec3& start, const glm::vec3& end);

    // Binds vertex/index buffers and issues vkCmdDrawIndexed
    void draw(VkCommandBuffer cmd) const;

    bool ready() const { return m_vertexBuffer.Buffer != VK_NULL_HANDLE; }

    int indexCount() const { return m_indexCount; }

private:
    vkdemo::VulkanBase* m_vkBase = nullptr;

    // Geometry parameters
    int   m_numCoils     = 8;
    int   m_ringSegs     = 12;
    float m_coilRadius   = 0.12f;
    float m_tubeRadius   = 0.035f;

    // Number of helix sample points
    int m_helixPoints    = 0;

    vkdemo::GPUBuffer m_vertexBuffer{};
    vkdemo::GPUBuffer m_indexBuffer{};
    int    m_indexCount = 0;

    // Interleaved: pos(3) + normal(3) + uv(2)
    struct Vertex { glm::vec3 pos, normal; glm::vec2 uv; };

    void cleanup();
};
