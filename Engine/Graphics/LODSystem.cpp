#include "LODSystem.hpp"
#include "../Core/Logger.hpp"
#include "Vulkan/VulkanDevice.hpp"
#include <cstring>

namespace GameEngine {

    // =========================================================================
    // OcclusionCullingSystem — Vulkan implementation
    // =========================================================================

    OcclusionCullingSystem::~OcclusionCullingSystem() {
        if (!m_Initialized) return;

        VkDevice device = VulkanDevice::Get().GetDevice();

        if (m_MappedResults && m_ReadbackMemory) {
            vkUnmapMemory(device, m_ReadbackMemory);
            m_MappedResults = nullptr;
        }
        if (m_ReadbackBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, m_ReadbackBuffer, nullptr);
            m_ReadbackBuffer = VK_NULL_HANDLE;
        }
        if (m_ReadbackMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, m_ReadbackMemory, nullptr);
            m_ReadbackMemory = VK_NULL_HANDLE;
        }
        if (m_QueryPool != VK_NULL_HANDLE) {
            vkDestroyQueryPool(device, m_QueryPool, nullptr);
            m_QueryPool = VK_NULL_HANDLE;
        }

        m_Initialized = false;
    }

    void OcclusionCullingSystem::initialize(uint32_t maxObjects) {
        if (m_Initialized) return;
        m_MaxObjects = maxObjects;

        VkDevice device = VulkanDevice::Get().GetDevice();

        // Create query pool -------------------------------------------------
        VkQueryPoolCreateInfo poolInfo{};
        poolInfo.sType      = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        poolInfo.queryType  = VK_QUERY_TYPE_OCCLUSION;
        poolInfo.queryCount = maxObjects;

        if (vkCreateQueryPool(device, &poolInfo, nullptr, &m_QueryPool) != VK_SUCCESS) {
            GE_CORE_ERROR("OcclusionCullingSystem: failed to create VkQueryPool");
            return;
        }

        // Readback buffer (host-visible, host-coherent) ----------------------
        VkDeviceSize bufferSize = static_cast<VkDeviceSize>(maxObjects) * sizeof(uint64_t);

        VkBufferCreateInfo bufCI{};
        bufCI.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufCI.size        = bufferSize;
        bufCI.usage       = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateBuffer(device, &bufCI, nullptr, &m_ReadbackBuffer);

        VkMemoryRequirements memReq;
        vkGetBufferMemoryRequirements(device, m_ReadbackBuffer, &memReq);

        uint32_t memType = VulkanDevice::Get().FindMemoryType(
            memReq.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize  = memReq.size;
        allocInfo.memoryTypeIndex = memType;
        vkAllocateMemory(device, &allocInfo, nullptr, &m_ReadbackMemory);
        vkBindBufferMemory(device, m_ReadbackBuffer, m_ReadbackMemory, 0);

        vkMapMemory(device, m_ReadbackMemory, 0, bufferSize, 0,
                    reinterpret_cast<void**>(&m_MappedResults));

        m_Initialized = true;
        GE_CORE_INFO("OcclusionCullingSystem initialised ({} max objects)", maxObjects);
    }

    void OcclusionCullingSystem::beginFrame(VkCommandBuffer cmd) {
        if (!m_Initialized) return;
        // Reset all query slots so they can be written this frame.
        vkCmdResetQueryPool(cmd, m_QueryPool, 0, m_MaxObjects);
        m_Stats = {};
    }

    void OcclusionCullingSystem::testObject(VkCommandBuffer cmd, uint32_t entityID,
                                             const AABB& /*bounds*/, uint32_t queryIndex) {
        if (!m_Initialized || queryIndex >= m_MaxObjects) return;

        m_Stats.TotalTested++;

        // Begin query — the caller is responsible for drawing the AABB geometry
        // between begin and end so the rasteriser can count covered samples.
        vkCmdBeginQuery(cmd, m_QueryPool, queryIndex, 0);

        // NOTE: Caller draws the bounding box mesh here (depth-only pass).
        // The bounding box draw is intentionally left to the caller so this
        // system stays decoupled from the mesh/pipeline subsystem.

        vkCmdEndQuery(cmd, m_QueryPool, queryIndex);
    }

    void OcclusionCullingSystem::endFrame(VkCommandBuffer cmd, uint32_t queriesUsed) {
        if (!m_Initialized || queriesUsed == 0) return;

        // Copy results into the host-visible readback buffer.
        // VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT ensures we get
        // complete 64-bit sample counts before we read on CPU next frame.
        vkCmdCopyQueryPoolResults(cmd, m_QueryPool, 0, queriesUsed,
                                   m_ReadbackBuffer, 0, sizeof(uint64_t),
                                   VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
    }

    void OcclusionCullingSystem::collectResults(uint32_t queriesUsed,
                                                 const std::vector<uint32_t>& entityOrder) {
        if (!m_Initialized || !m_MappedResults) return;

        uint32_t count = std::min(queriesUsed, static_cast<uint32_t>(entityOrder.size()));
        for (uint32_t i = 0; i < count; i++) {
            bool visible = (m_MappedResults[i] > 0);
            m_VisibilityCache[entityOrder[i]] = visible;
            if (visible)
                m_Stats.TotalVisible++;
            else
                m_Stats.TotalOccluded++;
        }
    }

    bool OcclusionCullingSystem::isVisible(uint32_t entityID) const {
        auto it = m_VisibilityCache.find(entityID);
        if (it != m_VisibilityCache.end()) return it->second;
        return true; // Visible by default until a query result is available
    }

} // namespace GameEngine
