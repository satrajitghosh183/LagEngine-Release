#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <VulkanBase.hpp>
#include <cstddef>
#include <vector>

#include "cuda/interop.cuh"

// Push constant block matching the vertex shader.
struct ParticlePushConstants {
    float viewProj[16];
    float pointSize;
    float pad[3]; // padding to 16-byte alignment
};

struct Renderer {
    vkdemo::VulkanBase* vkBase = nullptr;

    // Vulkan pipeline objects
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline       pipeline       = VK_NULL_HANDLE;

    // Vertex buffers (created with external memory for CUDA interop)
    VkBuffer         posBuffer      = VK_NULL_HANDLE;
    VkDeviceMemory   posMemory      = VK_NULL_HANDLE;
    VkBuffer         colBuffer      = VK_NULL_HANDLE;
    VkDeviceMemory   colMemory      = VK_NULL_HANDLE;

    // CUDA interop handles
    CudaVulkanBuffer cudaPosBuf{};
    CudaVulkanBuffer cudaColBuf{};

    // Allocation sizes (may differ from buffer sizes due to alignment)
    VkDeviceSize     posAllocSize   = 0;
    VkDeviceSize     colAllocSize   = 0;

    bool init(vkdemo::VulkanBase& base, size_t particleCount);
    void draw(VkCommandBuffer cmd, size_t particleCount, const float* viewProj);
    void shutdown();

    // Get CUDA device pointers for the shared buffers
    float4* getCudaPosPtr() const { return static_cast<float4*>(cudaPosBuf.devPtr); }
    float4* getCudaColPtr() const { return static_cast<float4*>(cudaColBuf.devPtr); }

private:
    VkShaderModule compileGLSL(const std::string& source, bool isVertex);

    void createExternalBuffer(VkDeviceSize size,
                              VkBufferUsageFlags usage,
                              VkBuffer& outBuffer,
                              VkDeviceMemory& outMemory,
                              VkDeviceSize& outAllocSize);

    void* getMemoryHandle(VkDeviceMemory memory);
};
