// Mesh3D.hpp -- Vulkan-compatible 3D mesh data container
//
// Stores interleaved vertex data (position + normal + texCoord) and
// indices on the CPU side.  GPU buffer management and draw-command
// recording use VulkanBase helpers.
#pragma once

#include <vector>
#include <cstring>
#include <glm/glm.hpp>

#include <vulkan/vulkan.h>
#include <VulkanBase.hpp>

namespace engine::graphics {

struct Vertex3D {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

class Mesh3D {
public:
    Mesh3D() = default;
    ~Mesh3D() = default;

    /// Upload interleaved vertex + index data into host-visible Vulkan buffers.
    void uploadData(vkdemo::VulkanBase& vk,
                    const std::vector<Vertex3D>& vertices,
                    const std::vector<unsigned int>& indices) {
        m_VertexCount = vertices.size();
        m_IndexCount = indices.size();

        if (!vertices.empty()) {
            VkDeviceSize size = vertices.size() * sizeof(Vertex3D);
            m_VertexBuffer = vk.CreateBuffer(size,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            vk.CopyToBuffer(m_VertexBuffer, vertices.data(), size);
        }

        if (!indices.empty()) {
            VkDeviceSize size = indices.size() * sizeof(unsigned int);
            m_IndexBuffer = vk.CreateBuffer(size,
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            vk.CopyToBuffer(m_IndexBuffer, indices.data(), size);
        }
    }

    /// Legacy overload without VulkanBase -- stores counts only.
    void uploadData(const std::vector<Vertex3D>& vertices,
                    const std::vector<unsigned int>& indices) {
        m_VertexCount = vertices.size();
        m_IndexCount = indices.size();
    }

    /// Re-upload vertex data to the existing GPU buffer.
    void updateVertices(vkdemo::VulkanBase& vk,
                        const std::vector<Vertex3D>& vertices) {
        if (m_VertexBuffer.Buffer && !vertices.empty()) {
            vk.CopyToBuffer(m_VertexBuffer, vertices.data(),
                            vertices.size() * sizeof(Vertex3D));
        }
    }

    /// Legacy overload (no-op without VulkanBase).
    void updateVertices(const std::vector<Vertex3D>&) {}

    /// Record indexed draw commands into a Vulkan command buffer.
    void draw(VkCommandBuffer cmd) const {
        if (m_VertexBuffer.Buffer == VK_NULL_HANDLE) return;

        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &m_VertexBuffer.Buffer, &offset);

        if (m_IndexBuffer.Buffer != VK_NULL_HANDLE && m_IndexCount > 0) {
            vkCmdBindIndexBuffer(cmd, m_IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(cmd, static_cast<uint32_t>(m_IndexCount), 1, 0, 0, 0);
        } else {
            vkCmdDraw(cmd, static_cast<uint32_t>(m_VertexCount), 1, 0, 0);
        }
    }

    /// Destroy GPU resources.
    void destroy(vkdemo::VulkanBase& vk) {
        vk.DestroyBuffer(m_VertexBuffer);
        vk.DestroyBuffer(m_IndexBuffer);
    }

private:
    vkdemo::GPUBuffer m_VertexBuffer{};
    vkdemo::GPUBuffer m_IndexBuffer{};
    size_t m_VertexCount = 0;
    size_t m_IndexCount = 0;
};

} // namespace engine::graphics
