#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

struct cudaExternalMemory_st;
typedef struct cudaExternalMemory_st* cudaExternalMemory_t;

namespace vkdemo { class VulkanBase; }
class GBuffer;

class SSAO {
public:
    SSAO() = default;
    ~SSAO();

    void init(vkdemo::VulkanBase* vkBase, int width, int height);
    void cleanup();

    void process(GBuffer* gbuffer, const glm::mat4& projectionMatrix);

    VkImageView getResultView() const { return m_resultView; }
    VkSampler   getSampler()    const { return m_sampler; }

    void resize(int width, int height);

    // Legacy compat
    unsigned int getResultTexture() const { return 0; }

private:
    vkdemo::VulkanBase* m_vkBase = nullptr;
    int m_width  = 0;
    int m_height = 0;

    // Result image (R32F, sampled by lighting pass)
    VkImage        m_resultImage  = VK_NULL_HANDLE;
    VkDeviceMemory m_resultMemory = VK_NULL_HANDLE;
    VkImageView    m_resultView   = VK_NULL_HANDLE;
    VkSampler      m_sampler      = VK_NULL_HANDLE;

    // CUDA-Vulkan interop via VK_KHR_external_memory
    cudaExternalMemory_t m_cudaExtMemory = nullptr;
    void* m_cudaMappedPtr = nullptr;

    // Noise texture
    VkImage        m_noiseImage  = VK_NULL_HANDLE;
    VkDeviceMemory m_noiseMemory = VK_NULL_HANDLE;
    VkImageView    m_noiseView   = VK_NULL_HANDLE;

    std::vector<glm::vec3> m_kernel;
    std::vector<glm::vec3> m_noise;

    void generateKernel();
    void generateNoise();

    void createImage(uint32_t w, uint32_t h, VkFormat fmt, VkImageUsageFlags usage,
                     VkImage& image, VkDeviceMemory& memory,
                     bool exportable = false);
    VkImageView createImageView(VkImage image, VkFormat fmt, VkImageAspectFlags aspect);
    uint32_t findMemoryType(uint32_t filter, VkMemoryPropertyFlags props);
};
