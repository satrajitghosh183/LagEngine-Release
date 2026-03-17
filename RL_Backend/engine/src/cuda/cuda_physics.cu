#include "rldemo/CudaPhysics.hpp"
#include "rldemo/NBodySim.hpp"
#include "rldemo/nbody_cuda.cuh"
#include <cuda_runtime.h>
#include <cuda_gl_interop.h>
#include <glad/glad.h>
#include <cstring>
#include <stdexcept>

namespace rldemo {

static void cudaCheck(cudaError_t err, const char* msg) {
    if (err != cudaSuccess) {
        throw std::runtime_error(std::string(msg) + ": " + cudaGetErrorString(err));
    }
}

CudaPhysics::~CudaPhysics() {
    Shutdown();
}

bool CudaPhysics::Init(uint32_t maxBodies) {
    if (m_Initialized) return true;
    cudaError_t e = cudaSetDevice(0);
    if (e != cudaSuccess) return false;
    m_MaxBodies = maxBodies;
    size_t bodyBytes = maxBodies * sizeof(cuda::NBodyDevice);
    e = cudaMalloc(&m_DeviceBodies, bodyBytes);
    if (e != cudaSuccess) return false;

    glGenBuffers(1, &m_InstanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_InstanceVBO);
    glBufferData(GL_ARRAY_BUFFER, maxBodies * 20 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    e = cudaGraphicsGLRegisterBuffer(
        reinterpret_cast<cudaGraphicsResource**>(&m_GraphicsResource),
        m_InstanceVBO,
        cudaGraphicsMapFlagsWriteDiscard);
    if (e != cudaSuccess) {
        cudaFree(m_DeviceBodies);
        glDeleteBuffers(1, &m_InstanceVBO);
        m_InstanceVBO = 0;
        return false;
    }
    m_Initialized = true;
    return true;
}

void CudaPhysics::Step(const std::vector<NBody>& hostBodies, float dt, uint32_t count) {
    if (!m_Initialized || count == 0) return;
    if (count > m_MaxBodies) count = m_MaxBodies;
    m_InstanceCount = count;

    cudaStream_t stream = 0;
    size_t bodyBytes = count * sizeof(cuda::NBodyDevice);
    std::vector<cuda::NBodyDevice> hostCopy(count);
    for (uint32_t i = 0; i < count; ++i) {
        hostCopy[i].pos_x = hostBodies[i].pos.x;
        hostCopy[i].pos_y = hostBodies[i].pos.y;
        hostCopy[i].pos_z = hostBodies[i].pos.z;
        hostCopy[i].vel_x = hostBodies[i].vel.x;
        hostCopy[i].vel_y = hostBodies[i].vel.y;
        hostCopy[i].vel_z = hostBodies[i].vel.z;
        hostCopy[i].acc_x = hostBodies[i].acc.x;
        hostCopy[i].acc_y = hostBodies[i].acc.y;
        hostCopy[i].acc_z = hostBodies[i].acc.z;
        hostCopy[i].mass = hostBodies[i].mass;
        hostCopy[i].radius = hostBodies[i].radius;
    }
    cudaCheck(cudaMemcpy(m_DeviceBodies, hostCopy.data(), bodyBytes, cudaMemcpyHostToDevice), "cudaMemcpy H2D");

    cudaGraphicsResource* res = reinterpret_cast<cudaGraphicsResource*>(m_GraphicsResource);
    cudaCheck(cudaGraphicsMapResources(1, &res, stream), "cudaGraphicsMapResources");
    void* dInstance = nullptr;
    size_t instanceBytes = 0;
    cudaCheck(cudaGraphicsResourceGetMappedPointer(&dInstance, &instanceBytes, res), "cudaGraphicsResourceGetMappedPointer");

    cuda::launchNBodyGravity(reinterpret_cast<cuda::NBodyDevice*>(m_DeviceBodies), static_cast<int>(count), stream);
    cuda::launchCollisionResolve(reinterpret_cast<cuda::NBodyDevice*>(m_DeviceBodies), static_cast<int>(count), stream);
    cuda::launchIntegrateAndInstance(
        reinterpret_cast<cuda::NBodyDevice*>(m_DeviceBodies),
        static_cast<int>(count), dt,
        reinterpret_cast<float*>(dInstance),
        stream);

    cudaCheck(cudaGraphicsUnmapResources(1, &res, stream), "cudaGraphicsUnmapResources");
    cudaCheck(cudaDeviceSynchronize(), "cudaDeviceSynchronize");
}

void CudaPhysics::Shutdown() {
    if (!m_Initialized) return;
    cudaGraphicsResource* res = reinterpret_cast<cudaGraphicsResource*>(m_GraphicsResource);
    if (res) {
        cudaGraphicsUnregisterResource(res);
        m_GraphicsResource = nullptr;
    }
    if (m_DeviceBodies) {
        cudaFree(m_DeviceBodies);
        m_DeviceBodies = nullptr;
    }
    if (m_InstanceVBO) {
        glDeleteBuffers(1, &m_InstanceVBO);
        m_InstanceVBO = 0;
    }
    m_Initialized = false;
}

} // namespace rldemo
