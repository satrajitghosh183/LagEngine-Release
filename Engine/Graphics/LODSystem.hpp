#pragma once

#include "../Core/Base.hpp"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace GameEngine {

    struct LODLevel {
        uint32_t MeshID = 0;
        std::string MeshPath;
        float ScreenSizeThreshold = 1.0f; // Fraction of screen height (0-1)
        float Distance = 0.0f;            // Distance-based threshold
    };

    struct LODStatistics {
        int TotalObjects = 0;
        int ObjectsByLOD[4] = {};
        int CulledObjects = 0;
    };

    class LODGroup {
    public:
        void addLevel(const LODLevel& level) {
            m_Levels.push_back(level);
        }

        int selectLevel(const glm::vec3& cameraPos, const glm::vec3& objectPos,
                         float boundingRadius, float screenHeight) const {
            float dist = glm::length(cameraPos - objectPos);

            for (int i = 0; i < static_cast<int>(m_Levels.size()); i++) {
                float threshold = m_Levels[i].Distance;
                float margin = threshold * m_HysteresisMargin;

                if (i == m_CurrentLevel) {
                    if (dist < threshold + margin) return i;
                } else {
                    if (dist < threshold) return i;
                }
            }

            return static_cast<int>(m_Levels.size()) - 1;
        }

        int getCurrentLevel() const { return m_CurrentLevel; }
        void setCurrentLevel(int level) { m_CurrentLevel = level; }
        const std::vector<LODLevel>& getLevels() const { return m_Levels; }
        void setHysteresisMargin(float margin) { m_HysteresisMargin = margin; }

    private:
        std::vector<LODLevel> m_Levels;
        mutable int m_CurrentLevel = 0;
        float m_HysteresisMargin = 0.1f;
    };

    class LODSystem {
    public:
        void registerGroup(uint32_t entityID, const LODGroup& group) {
            m_Groups[entityID] = group;
        }

        void unregisterGroup(uint32_t entityID) {
            m_Groups.erase(entityID);
        }

        void update(const glm::vec3& cameraPos, float screenHeight) {
            m_Stats = {};
            m_Stats.TotalObjects = static_cast<int>(m_Groups.size());

            for (auto& [id, group] : m_Groups) {
                auto it = m_ObjectPositions.find(id);
                if (it == m_ObjectPositions.end()) continue;

                int level = group.selectLevel(cameraPos, it->second, 1.0f, screenHeight);
                group.setCurrentLevel(level);

                if (level >= 0 && level < 4) {
                    m_Stats.ObjectsByLOD[level]++;
                }
            }
        }

        void setObjectPosition(uint32_t entityID, const glm::vec3& pos) {
            m_ObjectPositions[entityID] = pos;
        }

        int getSelectedLevel(uint32_t entityID) const {
            auto it = m_Groups.find(entityID);
            return it != m_Groups.end() ? it->second.getCurrentLevel() : 0;
        }

        const LODStatistics& getStatistics() const { return m_Stats; }

    private:
        std::unordered_map<uint32_t, LODGroup> m_Groups;
        std::unordered_map<uint32_t, glm::vec3> m_ObjectPositions;
        LODStatistics m_Stats;
    };

    // =========================================================================
    // Occlusion Culling (Vulkan timestamp / conditional rendering)
    //
    // In Vulkan, hardware occlusion queries are driven by VkQueryPool with
    // VK_QUERY_TYPE_OCCLUSION.  Each object's bounding box is rendered in a
    // depth-only pass while a query is active; the result is read back one
    // frame later to avoid GPU stalls.
    // =========================================================================

    struct AABB {
        glm::vec3 Min = glm::vec3(0.0f);
        glm::vec3 Max = glm::vec3(0.0f);

        glm::vec3 center() const { return (Min + Max) * 0.5f; }
        glm::vec3 extent() const { return (Max - Min) * 0.5f; }
    };

    /**
     * @brief GPU occlusion-culling system backed by a VkQueryPool.
     *
     * Usage per frame:
     *   1. beginFrame(cmd)        — reset query pool slots
     *   2. testObject(cmd, id, bounds, pipeline, slot) — vkCmdBeginQuery … render AABB … vkCmdEndQuery
     *   3. endFrame(cmd)          — vkCmdCopyQueryPoolResults into a readback buffer
     *   4. collectResults()       — read the readback buffer on CPU (called next frame)
     *   5. isVisible(id)          — query cached result
     */
    class OcclusionCullingSystem {
    public:
        OcclusionCullingSystem() = default;
        ~OcclusionCullingSystem();

        OcclusionCullingSystem(const OcclusionCullingSystem&) = delete;
        OcclusionCullingSystem& operator=(const OcclusionCullingSystem&) = delete;

        /**
         * @brief Allocate Vulkan resources.
         * @param maxObjects  Maximum number of objects that can be tested per frame.
         */
        void initialize(uint32_t maxObjects);

        /**
         * @brief Reset per-frame query slots at the start of a frame.
         */
        void beginFrame(VkCommandBuffer cmd);

        /**
         * @brief Record an occlusion query for a single AABB.
         * @param cmd          Active command buffer (inside a render pass)
         * @param entityID     Unique identifier for the object
         * @param bounds       World-space AABB
         * @param queryIndex   Slot index within the query pool (caller assigns)
         */
        void testObject(VkCommandBuffer cmd, uint32_t entityID,
                         const AABB& bounds, uint32_t queryIndex);

        /**
         * @brief Copy query results into a host-visible buffer.
         */
        void endFrame(VkCommandBuffer cmd, uint32_t queriesUsed);

        /**
         * @brief Read back results from the previous frame (CPU-side).
         * Call once after the frame's command buffer has finished executing.
         */
        void collectResults(uint32_t queriesUsed,
                             const std::vector<uint32_t>& entityOrder);

        bool isVisible(uint32_t entityID) const;

        struct Stats {
            int TotalTested  = 0;
            int TotalVisible = 0;
            int TotalOccluded = 0;
        };
        const Stats& getStatistics() const { return m_Stats; }

    private:
        VkQueryPool m_QueryPool = VK_NULL_HANDLE;
        uint32_t    m_MaxObjects = 0;

        // Readback buffer: one uint64 per query slot
        VkBuffer      m_ReadbackBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_ReadbackMemory = VK_NULL_HANDLE;
        uint64_t*     m_MappedResults = nullptr; // Persistently mapped

        std::unordered_map<uint32_t, bool> m_VisibilityCache;
        Stats m_Stats;
        bool m_Initialized = false;
    };

} // namespace GameEngine
