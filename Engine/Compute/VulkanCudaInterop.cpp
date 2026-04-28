#include "VulkanCudaInterop.hpp"
#include "CudaContext.hpp"

namespace GameEngine {

#ifdef GE_HAS_CUDA

    bool VulkanCudaBuffer::Create(VkDevice device, VmaAllocator allocator,
                                   VkDeviceSize size, VkBufferUsageFlags usage) {
        Size = size;

        // Create Vulkan buffer with external memory export
        VkExternalMemoryBufferCreateInfo extMemInfo{};
        extMemInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO;
#ifdef _WIN32
        extMemInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
        extMemInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.pNext = &extMemInfo;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        // We need dedicated allocation for external memory export
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

        if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &Buffer, &Allocation, nullptr) != VK_SUCCESS) {
            GE_CORE_ERROR("Failed to create Vulkan-CUDA shared buffer");
            return false;
        }

        // Get the underlying VkDeviceMemory
        VmaAllocationInfo vmaInfo{};
        vmaGetAllocationInfo(allocator, Allocation, &vmaInfo);
        Memory = vmaInfo.deviceMemory;

        // Export Vulkan memory to CUDA
#ifdef _WIN32
        VkMemoryGetWin32HandleInfoKHR handleInfo{};
        handleInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
        handleInfo.memory = Memory;
        handleInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

        HANDLE handle;
        auto vkGetMemoryWin32HandleKHR = (PFN_vkGetMemoryWin32HandleKHR)
            vkGetDeviceProcAddr(device, "vkGetMemoryWin32HandleKHR");
        if (!vkGetMemoryWin32HandleKHR || vkGetMemoryWin32HandleKHR(device, &handleInfo, &handle) != VK_SUCCESS) {
            GE_CORE_ERROR("Failed to get Win32 handle for Vulkan memory");
            return false;
        }

        cudaExternalMemoryHandleDesc cudaExtMemDesc{};
        cudaExtMemDesc.type = cudaExternalMemoryHandleTypeOpaqueWin32;
        cudaExtMemDesc.handle.win32.handle = handle;
        cudaExtMemDesc.size = size;
#else
        VkMemoryGetFdInfoKHR fdInfo{};
        fdInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
        fdInfo.memory = Memory;
        fdInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

        int fd;
        auto vkGetMemoryFdKHR = (PFN_vkGetMemoryFdKHR)
            vkGetDeviceProcAddr(device, "vkGetMemoryFdKHR");
        if (!vkGetMemoryFdKHR || vkGetMemoryFdKHR(device, &fdInfo, &fd) != VK_SUCCESS) {
            GE_CORE_ERROR("Failed to get fd for Vulkan memory");
            return false;
        }

        cudaExternalMemoryHandleDesc cudaExtMemDesc{};
        cudaExtMemDesc.type = cudaExternalMemoryHandleTypeOpaqueFd;
        cudaExtMemDesc.handle.fd = fd;
        cudaExtMemDesc.size = size;
#endif

        if (cudaImportExternalMemory(&CudaExtMem, &cudaExtMemDesc) != cudaSuccess) {
            GE_CORE_ERROR("Failed to import Vulkan memory into CUDA");
            return false;
        }

        // Map the external memory to a CUDA device pointer
        cudaExternalMemoryBufferDesc bufDesc{};
        bufDesc.offset = 0;
        bufDesc.size = size;
        bufDesc.flags = 0;

        if (cudaExternalMemoryGetMappedBuffer(&CudaDevicePtr, CudaExtMem, &bufDesc) != cudaSuccess) {
            GE_CORE_ERROR("Failed to map CUDA external memory");
            return false;
        }

        GE_CORE_INFO("Vulkan-CUDA shared buffer created ({0} bytes)", size);
        return true;
    }

    void VulkanCudaBuffer::Destroy(VkDevice device, VmaAllocator allocator) {
        (void)device;
        if (CudaDevicePtr) {
            cudaFree(CudaDevicePtr);
            CudaDevicePtr = nullptr;
        }
        if (CudaExtMem) {
            cudaDestroyExternalMemory(CudaExtMem);
            CudaExtMem = nullptr;
        }
        if (Buffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(allocator, Buffer, Allocation);
            Buffer = VK_NULL_HANDLE;
            Allocation = VK_NULL_HANDLE;
        }
        Size = 0;
    }

    bool VulkanCudaSemaphore::Create(VkDevice device) {
        // Create Vulkan semaphore with external export
        VkExportSemaphoreCreateInfo exportInfo{};
        exportInfo.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO;
#ifdef _WIN32
        exportInfo.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
        exportInfo.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif

        VkSemaphoreCreateInfo semInfo{};
        semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semInfo.pNext = &exportInfo;

        if (vkCreateSemaphore(device, &semInfo, nullptr, &VkSem) != VK_SUCCESS) {
            GE_CORE_ERROR("Failed to create exportable Vulkan semaphore");
            return false;
        }

        // Import into CUDA
#ifdef _WIN32
        HANDLE handle;
        VkSemaphoreGetWin32HandleInfoKHR handleInfo{};
        handleInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR;
        handleInfo.semaphore = VkSem;
        handleInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;

        auto vkGetSemaphoreWin32HandleKHR = (PFN_vkGetSemaphoreWin32HandleKHR)
            vkGetDeviceProcAddr(device, "vkGetSemaphoreWin32HandleKHR");
        if (!vkGetSemaphoreWin32HandleKHR ||
            vkGetSemaphoreWin32HandleKHR(device, &handleInfo, &handle) != VK_SUCCESS) {
            GE_CORE_ERROR("Failed to get Win32 semaphore handle");
            return false;
        }

        cudaExternalSemaphoreHandleDesc cudaSemDesc{};
        cudaSemDesc.type = cudaExternalSemaphoreHandleTypeOpaqueWin32;
        cudaSemDesc.handle.win32.handle = handle;
#else
        int fd;
        VkSemaphoreGetFdInfoKHR fdInfo{};
        fdInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR;
        fdInfo.semaphore = VkSem;
        fdInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;

        auto vkGetSemaphoreFdKHR = (PFN_vkGetSemaphoreFdKHR)
            vkGetDeviceProcAddr(device, "vkGetSemaphoreFdKHR");
        if (!vkGetSemaphoreFdKHR || vkGetSemaphoreFdKHR(device, &fdInfo, &fd) != VK_SUCCESS) {
            GE_CORE_ERROR("Failed to get fd semaphore handle");
            return false;
        }

        cudaExternalSemaphoreHandleDesc cudaSemDesc{};
        cudaSemDesc.type = cudaExternalSemaphoreHandleTypeOpaqueFd;
        cudaSemDesc.handle.fd = fd;
#endif

        if (cudaImportExternalSemaphore(&CudaSem, &cudaSemDesc) != cudaSuccess) {
            GE_CORE_ERROR("Failed to import Vulkan semaphore into CUDA");
            return false;
        }

        return true;
    }

    void VulkanCudaSemaphore::Destroy(VkDevice device) {
        if (CudaSem) {
            cudaDestroyExternalSemaphore(CudaSem);
            CudaSem = nullptr;
        }
        if (VkSem != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, VkSem, nullptr);
            VkSem = VK_NULL_HANDLE;
        }
    }

    void VulkanCudaSemaphore::CudaWait() {
        if (!CudaSem) return;
        cudaExternalSemaphoreWaitParams params{};
        cudaWaitExternalSemaphoresAsync(&CudaSem, &params, 1, nullptr);
    }

    void VulkanCudaSemaphore::CudaSignal() {
        if (!CudaSem) return;
        cudaExternalSemaphoreSignalParams params{};
        cudaSignalExternalSemaphoresAsync(&CudaSem, &params, 1, nullptr);
    }

    bool VulkanCudaInterop::IsSupported() {
        return CudaContext::IsAvailable() && CudaContext::GetDeviceInfo().SupportsVulkanInterop;
    }

    bool VulkanCudaInterop::CreateSharedBuffer(VulkanCudaBuffer& buffer,
                                                VkDevice device,
                                                VmaAllocator allocator,
                                                VkDeviceSize size,
                                                VkBufferUsageFlags usage) {
        return buffer.Create(device, allocator, size, usage);
    }

    bool VulkanCudaInterop::CreateSharedSemaphore(VulkanCudaSemaphore& semaphore,
                                                   VkDevice device) {
        return semaphore.Create(device);
    }

    void VulkanCudaInterop::DestroySharedBuffer(VulkanCudaBuffer& buffer,
                                                 VkDevice device,
                                                 VmaAllocator allocator) {
        buffer.Destroy(device, allocator);
    }

    void VulkanCudaInterop::DestroySharedSemaphore(VulkanCudaSemaphore& semaphore,
                                                    VkDevice device) {
        semaphore.Destroy(device);
    }

#else // !GE_HAS_CUDA — stubs

    bool VulkanCudaBuffer::Create(VkDevice, VmaAllocator, VkDeviceSize, VkBufferUsageFlags) {
        GE_CORE_WARN("Vulkan-CUDA interop not available (no CUDA)");
        return false;
    }
    void VulkanCudaBuffer::Destroy(VkDevice, VmaAllocator) {}

    bool VulkanCudaSemaphore::Create(VkDevice) { return false; }
    void VulkanCudaSemaphore::Destroy(VkDevice) {}
    void VulkanCudaSemaphore::CudaWait() {}
    void VulkanCudaSemaphore::CudaSignal() {}

    bool VulkanCudaInterop::IsSupported() { return false; }
    bool VulkanCudaInterop::CreateSharedBuffer(VulkanCudaBuffer&, VkDevice, VmaAllocator, VkDeviceSize, VkBufferUsageFlags) { return false; }
    bool VulkanCudaInterop::CreateSharedSemaphore(VulkanCudaSemaphore&, VkDevice) { return false; }
    void VulkanCudaInterop::DestroySharedBuffer(VulkanCudaBuffer&, VkDevice, VmaAllocator) {}
    void VulkanCudaInterop::DestroySharedSemaphore(VulkanCudaSemaphore&, VkDevice) {}

#endif

} // namespace GameEngine
