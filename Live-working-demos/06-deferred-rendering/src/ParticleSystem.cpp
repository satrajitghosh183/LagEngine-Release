#include "ParticleSystem.h"
#include "Shader.h"
#include "VulkanBase.hpp"
#include <cuda_runtime.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <iostream>
#include <random>
#include <cmath>
#include <algorithm>
#include <cstring>

#ifdef _WIN32
#include <vulkan/vulkan_win32.h>
#endif

// Forward declaration of CUDA kernel (uses raw device pointer, not GL resource)
extern "C" void updateParticleSystemVulkan(void* devicePtr,
                                           float deltaTime, int numParticles,
                                           cudaStream_t stream);

ParticleSystem::ParticleSystem(int maxParticles)
    : m_maxParticles(maxParticles)
    , m_width(1280)
    , m_height(720)
    , m_useCuda(false)
    , m_cpuParticleData(maxParticles * 4, 0.0f)
{
}

ParticleSystem::~ParticleSystem() {
    cleanup();
}

void ParticleSystem::cleanup() {
    if (!m_vkBase) return;
    VkDevice dev = m_vkBase->GetDevice();
    if (dev == VK_NULL_HANDLE) return;

    vkDeviceWaitIdle(dev);

    if (m_cudaExtMemory) {
        cudaDestroyExternalMemory(m_cudaExtMemory);
        m_cudaExtMemory = nullptr;
        m_cudaMappedPtr = nullptr;
    }

    if (m_particleBuffer) { vkDestroyBuffer(dev, m_particleBuffer, nullptr); m_particleBuffer = VK_NULL_HANDLE; }
    if (m_particleMemory) { vkFreeMemory(dev, m_particleMemory, nullptr); m_particleMemory = VK_NULL_HANDLE; }
    if (m_stagingBuffer)  { vkDestroyBuffer(dev, m_stagingBuffer, nullptr); m_stagingBuffer = VK_NULL_HANDLE; }
    if (m_stagingMemory)  { vkFreeMemory(dev, m_stagingMemory, nullptr); m_stagingMemory = VK_NULL_HANDLE; }
}

uint32_t ParticleSystem::findMemoryType(uint32_t filter, VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(m_vkBase->GetPhysicalDevice(), &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if ((filter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & props) == props)
            return i;
    }
    throw std::runtime_error("ParticleSystem: failed to find suitable memory type");
}

void ParticleSystem::init(vkdemo::VulkanBase* vkBase) {
    m_vkBase = vkBase;
    createParticleBuffer();
    initializeParticles();
}

void ParticleSystem::createParticleBuffer() {
    VkDevice dev = m_vkBase->GetDevice();
    m_particleBufferSize = static_cast<VkDeviceSize>(m_maxParticles * 4 * sizeof(float));

    // Device-local particle buffer with external memory export for CUDA interop
    VkBufferCreateInfo bci{};
    bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.size = m_particleBufferSize;
    bci.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Try to create exportable buffer for CUDA interop
    VkExternalMemoryBufferCreateInfo extBufInfo{};
#ifdef _WIN32
    extBufInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO;
    extBufInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
    extBufInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO;
    extBufInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif
    bci.pNext = &extBufInfo;

    if (vkCreateBuffer(dev, &bci, nullptr, &m_particleBuffer) != VK_SUCCESS) {
        // Fallback: no export
        bci.pNext = nullptr;
        vkCreateBuffer(dev, &bci, nullptr, &m_particleBuffer);
    }

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(dev, m_particleBuffer, &memReq);

    VkMemoryAllocateInfo mai{};
    mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.allocationSize = memReq.size;
    mai.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // Try exportable allocation
    VkExportMemoryAllocateInfo exportInfo{};
#ifdef _WIN32
    exportInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
    exportInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
    exportInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
    exportInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif
    mai.pNext = &exportInfo;

    if (vkAllocateMemory(dev, &mai, nullptr, &m_particleMemory) != VK_SUCCESS) {
        // Fallback: non-exportable
        mai.pNext = nullptr;
        vkAllocateMemory(dev, &mai, nullptr, &m_particleMemory);
    }
    vkBindBufferMemory(dev, m_particleBuffer, m_particleMemory, 0);

    // Create staging buffer (host-visible) for CPU fallback uploads
    VkBufferCreateInfo stagingBci{};
    stagingBci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBci.size = m_particleBufferSize;
    stagingBci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(dev, &stagingBci, nullptr, &m_stagingBuffer);

    VkMemoryRequirements stagingReq;
    vkGetBufferMemoryRequirements(dev, m_stagingBuffer, &stagingReq);

    VkMemoryAllocateInfo stagingMai{};
    stagingMai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    stagingMai.allocationSize = stagingReq.size;
    stagingMai.memoryTypeIndex = findMemoryType(stagingReq.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkAllocateMemory(dev, &stagingMai, nullptr, &m_stagingMemory);
    vkBindBufferMemory(dev, m_stagingBuffer, m_stagingMemory, 0);

    // Try to set up CUDA-Vulkan interop via VK_KHR_external_memory
    int deviceCount = 0;
    cudaGetDeviceCount(&deviceCount);
    if (deviceCount > 0 && exportInfo.handleTypes != 0) {
        // Import the Vulkan memory into CUDA
        cudaExternalMemoryHandleDesc extMemDesc{};
        memset(&extMemDesc, 0, sizeof(extMemDesc));

#ifdef _WIN32
        extMemDesc.type = cudaExternalMemoryHandleTypeOpaqueWin32;
        // Get Win32 handle
        VkMemoryGetWin32HandleInfoKHR handleInfo{};
        handleInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
        handleInfo.memory = m_particleMemory;
        handleInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
        auto vkGetMemoryWin32HandleKHR = (PFN_vkGetMemoryWin32HandleKHR)
            vkGetDeviceProcAddr(dev, "vkGetMemoryWin32HandleKHR");
        if (vkGetMemoryWin32HandleKHR) {
            HANDLE handle = nullptr;
            if (vkGetMemoryWin32HandleKHR(dev, &handleInfo, &handle) == VK_SUCCESS) {
                extMemDesc.handle.win32.handle = handle;
                extMemDesc.size = memReq.size;
                extMemDesc.flags = 0;

                cudaError_t err = cudaImportExternalMemory(&m_cudaExtMemory, &extMemDesc);
                if (err == cudaSuccess) {
                    cudaExternalMemoryBufferDesc bufDesc{};
                    bufDesc.offset = 0;
                    bufDesc.size = m_particleBufferSize;
                    bufDesc.flags = 0;
                    err = cudaExternalMemoryGetMappedBuffer(&m_cudaMappedPtr, m_cudaExtMemory, &bufDesc);
                    if (err == cudaSuccess) {
                        m_useCuda = true;
                        std::cout << "CUDA-Vulkan interop established for " << m_maxParticles << " particles." << std::endl;
                    }
                }
            }
        }
#else
        extMemDesc.type = cudaExternalMemoryHandleTypeOpaqueFd;
        // Get FD handle
        VkMemoryGetFdInfoKHR fdInfo{};
        fdInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
        fdInfo.memory = m_particleMemory;
        fdInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
        auto vkGetMemoryFdKHR = (PFN_vkGetMemoryFdKHR)
            vkGetDeviceProcAddr(dev, "vkGetMemoryFdKHR");
        if (vkGetMemoryFdKHR) {
            int fd = -1;
            if (vkGetMemoryFdKHR(dev, &fdInfo, &fd) == VK_SUCCESS && fd >= 0) {
                extMemDesc.handle.fd = fd;
                extMemDesc.size = memReq.size;
                extMemDesc.flags = 0;

                cudaError_t err = cudaImportExternalMemory(&m_cudaExtMemory, &extMemDesc);
                if (err == cudaSuccess) {
                    cudaExternalMemoryBufferDesc bufDesc{};
                    bufDesc.offset = 0;
                    bufDesc.size = m_particleBufferSize;
                    bufDesc.flags = 0;
                    err = cudaExternalMemoryGetMappedBuffer(&m_cudaMappedPtr, m_cudaExtMemory, &bufDesc);
                    if (err == cudaSuccess) {
                        m_useCuda = true;
                        std::cout << "CUDA-Vulkan interop established for " << m_maxParticles << " particles." << std::endl;
                    }
                }
            }
        }
#endif

        if (!m_useCuda) {
            std::cerr << "CUDA-Vulkan interop failed, using CPU fallback for particles." << std::endl;
        }
    } else {
        std::cerr << "No CUDA devices or no external memory support, using CPU fallback." << std::endl;
    }
}

void ParticleSystem::initializeParticles() {
    std::mt19937 rng(12345);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    for (int i = 0; i < m_maxParticles; ++i) {
        float angle1 = dist(rng) * 3.14159f * 2.0f;
        float angle2 = dist(rng) * 3.14159f;
        float radius = dist(rng) * 8.0f + 3.0f;

        int idx = i * 4;
        m_cpuParticleData[idx + 0] = radius * sin(angle2) * cos(angle1);
        m_cpuParticleData[idx + 1] = radius * sin(angle2) * sin(angle1);
        m_cpuParticleData[idx + 2] = radius * cos(angle2);
        m_cpuParticleData[idx + 3] = dist(rng) * 0.08f + 0.04f;
    }

    uploadParticlesToGPU();
}

void ParticleSystem::uploadParticlesToGPU() {
    if (!m_vkBase || m_cpuParticleData.empty()) return;

    VkDevice dev = m_vkBase->GetDevice();
    VkDeviceSize dataSize = m_cpuParticleData.size() * sizeof(float);

    // Copy data to staging buffer
    void* mapped = nullptr;
    vkMapMemory(dev, m_stagingMemory, 0, dataSize, 0, &mapped);
    memcpy(mapped, m_cpuParticleData.data(), static_cast<size_t>(dataSize));
    vkUnmapMemory(dev, m_stagingMemory);

    // Copy staging -> device-local
    VkCommandBuffer cmd = m_vkBase->BeginSingleTimeCommands();
    VkBufferCopy copyRegion{0, 0, dataSize};
    vkCmdCopyBuffer(cmd, m_stagingBuffer, m_particleBuffer, 1, &copyRegion);
    m_vkBase->EndSingleTimeCommands(cmd);
}

void ParticleSystem::update(float deltaTime, const glm::mat4& viewMatrix, void* cudaStream) {
    if (m_useCuda && m_cudaMappedPtr != nullptr) {
        cudaStream_t stream = cudaStream ? reinterpret_cast<cudaStream_t>(cudaStream) : 0;
        updateParticleSystemVulkan(m_cudaMappedPtr, deltaTime, m_maxParticles, stream);

        cudaError_t err = cudaGetLastError();
        if (err != cudaSuccess) {
            std::cerr << "CUDA kernel launch error: " << cudaGetErrorString(err) << std::endl;
            m_useCuda = false;
            updateParticlesCPU(deltaTime);
        }
    } else {
        updateParticlesCPU(deltaTime);
    }
}

void ParticleSystem::render(VkCommandBuffer cmd, VkPipelineLayout layout,
                            const glm::mat4& viewProjMatrix) {
    if (m_particleBuffer == VK_NULL_HANDLE) return;

    // Push the VP matrix
    vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(glm::mat4), glm::value_ptr(viewProjMatrix));

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_particleBuffer, &offset);
    vkCmdDraw(cmd, static_cast<uint32_t>(m_maxParticles), 1, 0, 0);
}

void ParticleSystem::updateParticlesCPU(float deltaTime) {
    if (m_cpuParticleData.empty() || m_maxParticles <= 0) return;

    deltaTime = std::max(0.0f, std::min(deltaTime, 0.1f));

    static float time = 0.0f;
    time += deltaTime;

    const float gravity = -2.0f;
    const float turbulence = 1.5f;
    const float centerAttraction = 0.3f;

    for (int i = 0; i < m_maxParticles && (i * 4 + 3) < static_cast<int>(m_cpuParticleData.size()); ++i) {
        int idx = i * 4;

        float speed = (static_cast<float>((i * 5) % 1000) / 1000.0f) * 0.8f + 0.4f;
        float angle = (static_cast<float>((i * 6) % 1000) / 1000.0f) * 3.14159f * 2.0f;
        float phase = (static_cast<float>((i * 7) % 1000) / 1000.0f) * 3.14159f * 2.0f;

        float x = m_cpuParticleData[idx + 0];
        float y = m_cpuParticleData[idx + 1];
        float z = m_cpuParticleData[idx + 2];

        float distFromCenter = std::sqrt(x*x + y*y + z*z);
        float t = time * 8.0f;

        float spiralX = std::cos(angle + t) * speed;
        float spiralY = std::sin(angle + t * 0.7f) * speed;
        float spiralZ = std::cos(angle + t * 0.5f) * speed * 0.5f;

        float gravityY = gravity * deltaTime;

        float turbX = std::sin(t * 3.0f + phase) * turbulence * deltaTime;
        float turbY = std::cos(t * 2.5f + phase * 1.3f) * turbulence * deltaTime;
        float turbZ = std::sin(t * 2.2f + phase * 0.7f) * turbulence * deltaTime;

        float centerX = -x * centerAttraction * deltaTime;
        float centerY = -y * centerAttraction * deltaTime;
        float centerZ = -z * centerAttraction * deltaTime;

        m_cpuParticleData[idx + 0] += (spiralX + turbX + centerX) * deltaTime;
        m_cpuParticleData[idx + 1] += (spiralY + gravityY + turbY + centerY) * deltaTime;
        m_cpuParticleData[idx + 2] += (spiralZ + turbZ + centerZ) * deltaTime;

        const float bounds = 15.0f;
        if (m_cpuParticleData[idx + 0] > bounds)  m_cpuParticleData[idx + 0] -= bounds * 2.0f;
        if (m_cpuParticleData[idx + 0] < -bounds) m_cpuParticleData[idx + 0] += bounds * 2.0f;
        if (m_cpuParticleData[idx + 1] > bounds)  m_cpuParticleData[idx + 1] -= bounds * 2.0f;
        if (m_cpuParticleData[idx + 1] < -bounds) m_cpuParticleData[idx + 1] += bounds * 2.0f;
        if (m_cpuParticleData[idx + 2] > bounds)  m_cpuParticleData[idx + 2] -= bounds * 2.0f;
        if (m_cpuParticleData[idx + 2] < -bounds) m_cpuParticleData[idx + 2] += bounds * 2.0f;

        float sizePhase = t * 3.0f + distFromCenter * 0.5f + i * 0.01f;
        m_cpuParticleData[idx + 3] = 0.03f + 0.04f * (0.5f + 0.5f * std::sin(sizePhase));
    }

    uploadParticlesToGPU();
}

void ParticleSystem::resize(int width, int height) {
    m_width = width;
    m_height = height;
}
