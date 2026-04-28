#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

struct cudaExternalMemory_st;
typedef struct cudaExternalMemory_st* cudaExternalMemory_t;

namespace vkdemo { class VulkanBase; struct GPUBuffer; }
class Shader;

class ParticleSystem {
public:
    ParticleSystem(int maxParticles = 10000);
    ~ParticleSystem();

    void init(vkdemo::VulkanBase* vkBase);
    void cleanup();

    void update(float deltaTime, const glm::mat4& viewMatrix, void* cudaStream = nullptr);
    void render(VkCommandBuffer cmd, VkPipelineLayout layout, const glm::mat4& viewProjMatrix);

    void resize(int width, int height);
    int getMaxParticles() const { return m_maxParticles; }

private:
    vkdemo::VulkanBase* m_vkBase = nullptr;
    int m_maxParticles;
    int m_width;
    int m_height;

    // Vulkan particle buffer (device-local, exportable for CUDA interop)
    VkBuffer       m_particleBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_particleMemory = VK_NULL_HANDLE;
    VkDeviceSize   m_particleBufferSize = 0;

    // Host-visible staging buffer for CPU fallback uploads
    VkBuffer       m_stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_stagingMemory = VK_NULL_HANDLE;

    // CUDA-Vulkan interop
    cudaExternalMemory_t m_cudaExtMemory = nullptr;
    void* m_cudaMappedPtr = nullptr; // CUDA device pointer into the Vulkan buffer
    bool m_useCuda = false;

    std::vector<float> m_cpuParticleData;

    void createParticleBuffer();
    void initializeParticles();
    void updateParticlesCPU(float deltaTime);
    void uploadParticlesToGPU();

    uint32_t findMemoryType(uint32_t filter, VkMemoryPropertyFlags props);
};
