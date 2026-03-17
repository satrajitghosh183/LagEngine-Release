#pragma once

#include "Types.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <mutex>
#include <cstdint>

namespace rldemo {

/**
 * Per-instance data (model matrix + color)
 */
struct InstanceData {
    float model[16];  // mat4
    float color[4];   // vec4
};

/**
 * IndirectDrawBuilder - workers build visibility lists, instance data, indirect commands
 * Uses persistent mapped buffers for ring-buffering
 */
class IndirectDrawBuilder {
public:
    IndirectDrawBuilder(uint32_t maxInstances);
    ~IndirectDrawBuilder();

    /** Reset for new frame */
    void BeginFrame();

    /** Prepare for N instances; resizes staging so workers can write to disjoint ranges (no mutex) */
    void PrepareForInstances(uint32_t count);

    /** Pointer to staging buffer; valid after PrepareForInstances. Workers write to [start,end). */
    InstanceData* GetInstanceStagingData();

    /** Add visible instance (called from worker threads - use mutex or per-worker buffers) */
    void AddInstance(const glm::mat4& model, const glm::vec4& color);

    /** Add a batch of instances (one lock per batch; for parallel per-worker staging) */
    void AddInstances(const InstanceData* data, size_t count);

    /** Finalize: build indirect command buffer, return draw count */
    uint32_t EndFrame();

    /** Get GPU buffer IDs for rendering */
    uint32_t GetInstanceVBO() const { return m_InstanceVBO; }
    uint32_t GetInstanceCount() const { return m_DrawCount; }

private:
    uint32_t m_MaxInstances = 0;
    uint32_t m_InstanceVBO = 0;
    uint32_t m_DrawCount = 0;

    std::vector<InstanceData> m_InstanceStaging;
    std::mutex m_StagingMutex;
};

} // namespace rldemo
