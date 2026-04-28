#include "UniformBuffer.hpp"
#include "../Core/Logger.hpp"
#include "Vulkan/VulkanDevice.hpp"
#include <cstring>

namespace GameEngine {

    UniformBuffer::~UniformBuffer() {
        Shutdown();
    }

    void UniformBuffer::Init(size_t size, uint32_t bindingSlot, uint32_t frameCount) {
        if (!m_Buffers.empty()) {
            Shutdown();
        }

        m_Size = size;
        m_BindingSlot = bindingSlot;
        m_FrameCount = frameCount;

        auto allocator = VulkanDevice::Get().GetAllocator();

        m_Buffers.resize(frameCount);
        m_Allocations.resize(frameCount);
        m_MappedPtrs.resize(frameCount, nullptr);

        for (uint32_t i = 0; i < frameCount; i++) {
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = size;
            bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo allocInfo{};
            allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
            allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                              VMA_ALLOCATION_CREATE_MAPPED_BIT;

            VmaAllocationInfo vmaAllocInfo{};
            if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo,
                                &m_Buffers[i], &m_Allocations[i], &vmaAllocInfo) != VK_SUCCESS) {
                GE_CORE_ERROR("Failed to create uniform buffer (frame {0}, {1} bytes)", i, size);
                return;
            }

            // Persistent mapping
            m_MappedPtrs[i] = vmaAllocInfo.pMappedData;
        }
    }

    void UniformBuffer::SetData(const void* data, size_t size, size_t offset) {
        SetData(m_CurrentFrame, data, size, offset);
    }

    void UniformBuffer::SetData(uint32_t frameIndex, const void* data, size_t size, size_t offset) {
        if (!data || frameIndex >= m_FrameCount) return;

        GE_CORE_ASSERT(offset + size <= m_Size, "Uniform buffer write out of bounds");

        if (m_MappedPtrs[frameIndex]) {
            memcpy(static_cast<char*>(m_MappedPtrs[frameIndex]) + offset, data, size);
            vmaFlushAllocation(VulkanDevice::Get().GetAllocator(),
                               m_Allocations[frameIndex], offset, size);
        }
    }

    VkDescriptorBufferInfo UniformBuffer::GetDescriptorInfo(uint32_t frameIndex) const {
        VkDescriptorBufferInfo info{};
        info.buffer = m_Buffers[frameIndex];
        info.offset = 0;
        info.range = m_Size;
        return info;
    }

    void UniformBuffer::Shutdown() {
        auto allocator = VulkanDevice::Get().GetAllocator();

        for (uint32_t i = 0; i < m_FrameCount; i++) {
            if (m_Buffers[i] != VK_NULL_HANDLE) {
                vmaDestroyBuffer(allocator, m_Buffers[i], m_Allocations[i]);
            }
        }

        m_Buffers.clear();
        m_Allocations.clear();
        m_MappedPtrs.clear();
        m_Size = 0;
        m_FrameCount = 0;
    }
}
