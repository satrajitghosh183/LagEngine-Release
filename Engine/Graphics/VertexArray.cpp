#include "VertexArray.hpp"
#include "../Core/Logger.hpp"
#include <cstring>

namespace GameEngine {

    // ========== VertexBuffer ==========

    VertexBuffer::VertexBuffer(const void* data, uint32_t size) {
        CreateBuffer(size, false);
        if (data) {
            UploadData(data, size);
        }
    }

    VertexBuffer::VertexBuffer(float* vertices, uint32_t size) {
        CreateBuffer(size, false);
        if (vertices) {
            UploadData(vertices, size);
        }
    }

    VertexBuffer::VertexBuffer(uint32_t size) {
        CreateBuffer(size, true);
    }

    VertexBuffer::~VertexBuffer() {
        if (m_Buffer != VK_NULL_HANDLE) {
            auto allocator = VulkanDevice::Get().GetAllocator();
            vmaDestroyBuffer(allocator, m_Buffer, m_Allocation);
        }
    }

    void VertexBuffer::CreateBuffer(uint32_t size, bool dynamic) {
        m_Size = size;
        m_Dynamic = dynamic;
        auto allocator = VulkanDevice::Get().GetAllocator();

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo{};
        if (dynamic) {
            // Host-visible for frequent CPU updates (cloth, particles, etc.)
            allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
            allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                              VMA_ALLOCATION_CREATE_MAPPED_BIT;
        } else {
            // Device-local for static geometry (uploaded via staging buffer)
            allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        }

        if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &m_Buffer, &m_Allocation, nullptr) != VK_SUCCESS) {
            GE_CORE_ERROR("Failed to create vertex buffer ({0} bytes)", size);
        }
    }

    void VertexBuffer::UploadData(const void* data, uint32_t size) {
        auto& device = VulkanDevice::Get();
        auto allocator = device.GetAllocator();

        if (m_Dynamic) {
            // Direct map for dynamic buffers
            void* mapped = nullptr;
            vmaMapMemory(allocator, m_Allocation, &mapped);
            memcpy(mapped, data, size);
            vmaUnmapMemory(allocator, m_Allocation);
            vmaFlushAllocation(allocator, m_Allocation, 0, size);
        } else {
            // Stage through host-visible buffer for device-local memory
            VkBuffer stagingBuffer;
            VmaAllocation stagingAlloc;

            VkBufferCreateInfo stagingInfo{};
            stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            stagingInfo.size = size;
            stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

            VmaAllocationCreateInfo stagingAllocInfo{};
            stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
            stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                     VMA_ALLOCATION_CREATE_MAPPED_BIT;

            vmaCreateBuffer(allocator, &stagingInfo, &stagingAllocInfo,
                            &stagingBuffer, &stagingAlloc, nullptr);

            void* mapped = nullptr;
            vmaMapMemory(allocator, stagingAlloc, &mapped);
            memcpy(mapped, data, size);
            vmaUnmapMemory(allocator, stagingAlloc);
            vmaFlushAllocation(allocator, stagingAlloc, 0, size);

            // Copy via single-time command buffer
            VkCommandBuffer cmd = device.BeginSingleTimeCommands();
            VkBufferCopy copyRegion{};
            copyRegion.size = size;
            vkCmdCopyBuffer(cmd, stagingBuffer, m_Buffer, 1, &copyRegion);
            device.EndSingleTimeCommands(cmd);

            vmaDestroyBuffer(allocator, stagingBuffer, stagingAlloc);
        }
    }

    void VertexBuffer::SetData(const void* data, uint32_t size) {
        UploadData(data, size);
    }

    void VertexBuffer::Bind(VkCommandBuffer cmd) const {
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &m_Buffer, &offset);
    }

    // ========== IndexBuffer ==========

    IndexBuffer::IndexBuffer(const uint32_t* indices, uint32_t count)
        : m_Count(count) {
        auto& device = VulkanDevice::Get();
        auto allocator = device.GetAllocator();
        uint32_t size = count * sizeof(uint32_t);

        // Create device-local index buffer
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &m_Buffer, &m_Allocation, nullptr);

        // Stage upload
        VkBuffer stagingBuffer;
        VmaAllocation stagingAlloc;

        VkBufferCreateInfo stagingInfo{};
        stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingInfo.size = size;
        stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo stagingAllocInfo{};
        stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                 VMA_ALLOCATION_CREATE_MAPPED_BIT;

        vmaCreateBuffer(allocator, &stagingInfo, &stagingAllocInfo,
                        &stagingBuffer, &stagingAlloc, nullptr);

        void* mapped = nullptr;
        vmaMapMemory(allocator, stagingAlloc, &mapped);
        memcpy(mapped, indices, size);
        vmaUnmapMemory(allocator, stagingAlloc);
        vmaFlushAllocation(allocator, stagingAlloc, 0, size);

        VkCommandBuffer cmd = device.BeginSingleTimeCommands();
        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(cmd, stagingBuffer, m_Buffer, 1, &copyRegion);
        device.EndSingleTimeCommands(cmd);

        vmaDestroyBuffer(allocator, stagingBuffer, stagingAlloc);
    }

    IndexBuffer::~IndexBuffer() {
        if (m_Buffer != VK_NULL_HANDLE) {
            auto allocator = VulkanDevice::Get().GetAllocator();
            vmaDestroyBuffer(allocator, m_Buffer, m_Allocation);
        }
    }

    void IndexBuffer::Bind(VkCommandBuffer cmd) const {
        vkCmdBindIndexBuffer(cmd, m_Buffer, 0, VK_INDEX_TYPE_UINT32);
    }

    // ========== VertexArray ==========

    void VertexArray::Bind(VkCommandBuffer cmd) const {
        for (const auto& vb : m_VertexBuffers) {
            vb->Bind(cmd);
        }
        if (m_IndexBuffer) {
            m_IndexBuffer->Bind(cmd);
        }
    }

    void VertexArray::AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) {
        GE_CORE_ASSERT(vertexBuffer->GetLayout().GetElements().size(), "Vertex Buffer has no layout!");
        m_VertexBuffers.push_back(vertexBuffer);
    }

    void VertexArray::SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer) {
        m_IndexBuffer = indexBuffer;
    }
}
