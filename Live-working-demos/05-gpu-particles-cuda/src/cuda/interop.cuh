#pragma once
#include <cuda_runtime.h>
#include <cstddef>

// Vulkan-CUDA interop buffer.
// The Vulkan side creates an exportable VkBuffer+VkDeviceMemory.
// This struct holds the CUDA-imported external memory and the device pointer.
struct CudaVulkanBuffer {
    cudaExternalMemory_t extMem  = nullptr;
    void*                devPtr  = nullptr;
    size_t               size    = 0;
};

void cudaCheck(cudaError_t err, const char* msg);

// Import a Vulkan-exported memory object into CUDA.
//   handle      - platform-specific handle (fd on Linux, HANDLE on Windows)
//   totalBytes  - allocation size in bytes
//   bufferBytes - mapped region size in bytes
//   offset      - offset into the allocation
void importVulkanBufferToCuda(CudaVulkanBuffer& buf,
                              void* handle,
                              size_t totalBytes,
                              size_t bufferBytes,
                              size_t offset = 0);

// Release the CUDA external memory import (does NOT destroy the Vulkan resources).
void destroyCudaVulkanBuffer(CudaVulkanBuffer& buf);
