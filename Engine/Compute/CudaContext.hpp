#pragma once

#include "../Core/Base.hpp"
#include "../Core/Logger.hpp"
#include <cstdint>
#include <string>
#include <vector>

// CUDA headers are optional — engine compiles without CUDA SDK
#ifdef GE_HAS_CUDA
#include <cuda.h>
#include <cuda_runtime.h>
#endif

namespace GameEngine {

    struct CudaDeviceInfo {
        int DeviceID = -1;
        std::string Name;
        size_t TotalMemory = 0;
        int ComputeCapabilityMajor = 0;
        int ComputeCapabilityMinor = 0;
        int MultiprocessorCount = 0;
        int MaxThreadsPerBlock = 0;
        int MaxBlockDimX = 0, MaxBlockDimY = 0, MaxBlockDimZ = 0;
        int MaxGridDimX = 0, MaxGridDimY = 0, MaxGridDimZ = 0;
        int WarpSize = 0;
        bool SupportsUnifiedMemory = false;
        bool SupportsVulkanInterop = false;
    };

    /**
     * @brief CUDA context and device management
     *
     * Manages CUDA initialization, device selection, and provides
     * utilities for GPU compute operations. Works alongside Vulkan
     * for CUDA-Vulkan interop (shared buffers, images).
     *
     * Usage:
     *   CudaContext::Initialize();
     *   auto info = CudaContext::GetDeviceInfo();
     *   // ... run compute kernels ...
     *   CudaContext::Shutdown();
     */
    class CudaContext {
    public:
        static bool Initialize(int preferredDevice = -1);
        static void Shutdown();
        static bool IsAvailable();

        static const CudaDeviceInfo& GetDeviceInfo();
        static int GetDeviceCount();

        static void Synchronize();
        static size_t GetFreeMemory();
        static size_t GetTotalMemory();

        static std::string GetErrorString(int errorCode);

    private:
        static bool s_Initialized;
        static CudaDeviceInfo s_DeviceInfo;
    };

} // namespace GameEngine
