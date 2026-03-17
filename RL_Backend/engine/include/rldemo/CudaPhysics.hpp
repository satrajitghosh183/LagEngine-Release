#pragma once

#include "Types.hpp"
#include <cstdint>
#include <vector>

namespace rldemo {

struct NBody;

/**
 * CUDA physics backend: N-body gravity, collision, integrate, and instance output.
 * When RL_BACKEND_BUILD_CUDA is OFF, all methods are stubs (Init returns false, GetInstanceVBO returns 0).
 */
class CudaPhysics {
public:
    CudaPhysics() = default;
    ~CudaPhysics();

    /** Initialize CUDA and allocate device memory for maxBodies. Returns false if CUDA unavailable. */
    bool Init(uint32_t maxBodies);

    /** Copy host bodies to device and run one step (gravity, collision, integrate), write instance data to GL buffer. */
    void Step(const std::vector<NBody>& hostBodies, float dt, uint32_t count);

    /** Get the OpenGL VBO ID that holds instance data (mat4+color per body). Valid after Init. */
    uint32_t GetInstanceVBO() const { return m_InstanceVBO; }

    /** Get current instance count (body count last passed to Step). */
    uint32_t GetInstanceCount() const { return m_InstanceCount; }

    /** Shutdown and release resources. */
    void Shutdown();

private:
    uint32_t m_MaxBodies = 0;
    uint32_t m_InstanceVBO = 0;
    uint32_t m_InstanceCount = 0;
    void* m_DeviceBodies = nullptr;
    void* m_GraphicsResource = nullptr;
    bool m_Initialized = false;
};

} // namespace rldemo
