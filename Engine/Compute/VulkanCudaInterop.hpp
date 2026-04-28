#pragma once

#include "../Core/Base.hpp"
#include "../Core/Logger.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <cstdint>
#include <cstddef>

#ifdef GE_HAS_CUDA
#include <cuda.h>
#include <cuda_runtime.h>
#endif

namespace GameEngine {

    /**
     * @brief Shared buffer between Vulkan and CUDA
     *
     * Uses VK_KHR_external_memory to export Vulkan buffers as
     * CUDA-accessible device pointers. Both APIs can read/write
     * the same GPU memory with proper synchronization.
     *
     * Usage:
     *   VulkanCudaBuffer shared;
     *   shared.Create(device, allocator, sizeBytes);
     *   // Vulkan side: shared.GetVkBuffer()
     *   // CUDA side:   shared.GetCudaPtr<float>()
     *   // Synchronize with semaphores between API usages
     */
    struct VulkanCudaBuffer {
        VkBuffer Buffer = VK_NULL_HANDLE;
        VmaAllocation Allocation = VK_NULL_HANDLE;
        VkDeviceMemory Memory = VK_NULL_HANDLE;
        VkDeviceSize Size = 0;

#ifdef GE_HAS_CUDA
        cudaExternalMemory_t CudaExtMem = nullptr;
        void* CudaDevicePtr = nullptr;
#endif

        bool Create(VkDevice device, VmaAllocator allocator, VkDeviceSize size,
                     VkBufferUsageFlags usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                                                 VK_BUFFER_USAGE_TRANSFER_DST_BIT);

        void Destroy(VkDevice device, VmaAllocator allocator);

        VkBuffer GetVkBuffer() const { return Buffer; }
        VkDeviceSize GetSize() const { return Size; }

        template <typename T>
        T* GetCudaPtr() const {
#ifdef GE_HAS_CUDA
            return static_cast<T*>(CudaDevicePtr);
#else
            return nullptr;
#endif
        }
    };

    /**
     * @brief Shared semaphore between Vulkan and CUDA
     *
     * Uses VK_KHR_external_semaphore to create timeline or binary
     * semaphores that CUDA can wait on / signal.
     */
    struct VulkanCudaSemaphore {
        VkSemaphore VkSem = VK_NULL_HANDLE;

#ifdef GE_HAS_CUDA
        cudaExternalSemaphore_t CudaSem = nullptr;
#endif

        bool Create(VkDevice device);
        void Destroy(VkDevice device);

        void CudaWait();
        void CudaSignal();
    };

    /**
     * @brief CUDA-Vulkan interop utilities
     */
    class VulkanCudaInterop {
    public:
        static bool IsSupported();

        static bool CreateSharedBuffer(VulkanCudaBuffer& buffer,
                                        VkDevice device,
                                        VmaAllocator allocator,
                                        VkDeviceSize size,
                                        VkBufferUsageFlags usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

        static bool CreateSharedSemaphore(VulkanCudaSemaphore& semaphore,
                                           VkDevice device);

        static void DestroySharedBuffer(VulkanCudaBuffer& buffer,
                                         VkDevice device,
                                         VmaAllocator allocator);

        static void DestroySharedSemaphore(VulkanCudaSemaphore& semaphore,
                                            VkDevice device);
    };

} // namespace GameEngine
