#pragma once

#include "../Core/Base.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <cstdint>
#include <cstddef>
#include <vector>

namespace GameEngine {

    /**
     * @brief Vulkan Uniform Buffer wrapper with per-frame buffering
     *
     * Creates one buffer per frame-in-flight for lockless CPU updates.
     * Uses persistent mapping via VMA for fast writes.
     *
     * Usage:
     *   UniformBuffer ubo;
     *   ubo.Init(sizeof(CameraData), 0);  // binding slot 0
     *   ubo.SetData(&cameraData, sizeof(CameraData));
     *   // Use ubo.GetDescriptorInfo(frameIndex) for descriptor writes
     */
    class UniformBuffer {
    public:
        static constexpr uint32_t MAX_FRAMES = 3;

        UniformBuffer() = default;
        ~UniformBuffer();

        /**
         * @brief Allocate per-frame uniform buffers
         * @param size Size in bytes (matches std140/std430 layout in shaders)
         * @param bindingSlot Descriptor binding index
         * @param frameCount Number of frames in flight (default 2)
         */
        void Init(size_t size, uint32_t bindingSlot, uint32_t frameCount = 2);

        /**
         * @brief Upload data to the current frame's buffer
         */
        void SetData(const void* data, size_t size, size_t offset = 0);

        /**
         * @brief Upload data to a specific frame's buffer
         */
        void SetData(uint32_t frameIndex, const void* data, size_t size, size_t offset = 0);

        /**
         * @brief Set which frame is current (for SetData without frameIndex)
         */
        void SetCurrentFrame(uint32_t frameIndex) { m_CurrentFrame = frameIndex; }

        /**
         * @brief Get descriptor buffer info for a specific frame
         */
        VkDescriptorBufferInfo GetDescriptorInfo(uint32_t frameIndex) const;

        /**
         * @brief Get descriptor buffer info for current frame
         */
        VkDescriptorBufferInfo GetDescriptorInfo() const { return GetDescriptorInfo(m_CurrentFrame); }

        VkBuffer GetBuffer(uint32_t frameIndex) const { return m_Buffers[frameIndex]; }
        uint32_t GetBindingSlot() const { return m_BindingSlot; }
        size_t GetSize() const { return m_Size; }
        bool IsValid() const { return !m_Buffers.empty() && m_Buffers[0] != VK_NULL_HANDLE; }

        void Shutdown();

    private:
        std::vector<VkBuffer> m_Buffers;
        std::vector<VmaAllocation> m_Allocations;
        std::vector<void*> m_MappedPtrs;

        uint32_t m_BindingSlot = 0;
        uint32_t m_FrameCount = 0;
        uint32_t m_CurrentFrame = 0;
        size_t m_Size = 0;
    };
}
