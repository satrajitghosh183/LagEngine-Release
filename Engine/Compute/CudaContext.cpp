#include "CudaContext.hpp"

namespace GameEngine {

    bool CudaContext::s_Initialized = false;
    CudaDeviceInfo CudaContext::s_DeviceInfo{};

#ifdef GE_HAS_CUDA

    bool CudaContext::Initialize(int preferredDevice) {
        if (s_Initialized) return true;

        int deviceCount = 0;
        cudaError_t err = cudaGetDeviceCount(&deviceCount);
        if (err != cudaSuccess || deviceCount == 0) {
            GE_CORE_WARN("No CUDA-capable devices found");
            return false;
        }

        // Select device
        int selectedDevice = 0;
        if (preferredDevice >= 0 && preferredDevice < deviceCount) {
            selectedDevice = preferredDevice;
        } else {
            // Pick device with highest compute capability
            int bestScore = -1;
            for (int i = 0; i < deviceCount; i++) {
                cudaDeviceProp props;
                cudaGetDeviceProperties(&props, i);
                int score = props.major * 100 + props.minor * 10 + props.multiProcessorCount;
                if (score > bestScore) {
                    bestScore = score;
                    selectedDevice = i;
                }
            }
        }

        err = cudaSetDevice(selectedDevice);
        if (err != cudaSuccess) {
            GE_CORE_ERROR("Failed to set CUDA device {0}: {1}", selectedDevice, cudaGetErrorString(err));
            return false;
        }

        // Query device properties
        cudaDeviceProp props;
        cudaGetDeviceProperties(&props, selectedDevice);

        s_DeviceInfo.DeviceID = selectedDevice;
        s_DeviceInfo.Name = props.name;
        s_DeviceInfo.TotalMemory = props.totalGlobalMem;
        s_DeviceInfo.ComputeCapabilityMajor = props.major;
        s_DeviceInfo.ComputeCapabilityMinor = props.minor;
        s_DeviceInfo.MultiprocessorCount = props.multiProcessorCount;
        s_DeviceInfo.MaxThreadsPerBlock = props.maxThreadsPerBlock;
        s_DeviceInfo.MaxBlockDimX = props.maxThreadsDim[0];
        s_DeviceInfo.MaxBlockDimY = props.maxThreadsDim[1];
        s_DeviceInfo.MaxBlockDimZ = props.maxThreadsDim[2];
        s_DeviceInfo.MaxGridDimX = props.maxGridSize[0];
        s_DeviceInfo.MaxGridDimY = props.maxGridSize[1];
        s_DeviceInfo.MaxGridDimZ = props.maxGridSize[2];
        s_DeviceInfo.WarpSize = props.warpSize;
        s_DeviceInfo.SupportsUnifiedMemory = (props.managedMemory != 0);

        // Check Vulkan interop support (requires external memory extension).
        // The exact attribute name varies between CUDA versions; older toolkits
        // (≤ 11.5) don't expose a query, so we conservatively assume support
        // when the device exposes any external-memory capability.
        s_DeviceInfo.SupportsVulkanInterop = true;

        s_Initialized = true;

        GE_CORE_INFO("CUDA initialized: {0} (SM {1}.{2}, {3} MB, {4} SMs)",
                      s_DeviceInfo.Name,
                      s_DeviceInfo.ComputeCapabilityMajor,
                      s_DeviceInfo.ComputeCapabilityMinor,
                      s_DeviceInfo.TotalMemory / (1024 * 1024),
                      s_DeviceInfo.MultiprocessorCount);

        return true;
    }

    void CudaContext::Shutdown() {
        if (!s_Initialized) return;
        cudaDeviceReset();
        s_Initialized = false;
        s_DeviceInfo = {};
        GE_CORE_INFO("CUDA shutdown");
    }

    bool CudaContext::IsAvailable() {
        return s_Initialized;
    }

    const CudaDeviceInfo& CudaContext::GetDeviceInfo() {
        return s_DeviceInfo;
    }

    int CudaContext::GetDeviceCount() {
        int count = 0;
        cudaGetDeviceCount(&count);
        return count;
    }

    void CudaContext::Synchronize() {
        if (s_Initialized) {
            cudaDeviceSynchronize();
        }
    }

    size_t CudaContext::GetFreeMemory() {
        size_t free = 0, total = 0;
        if (s_Initialized) {
            cudaMemGetInfo(&free, &total);
        }
        return free;
    }

    size_t CudaContext::GetTotalMemory() {
        return s_DeviceInfo.TotalMemory;
    }

    std::string CudaContext::GetErrorString(int errorCode) {
        return cudaGetErrorString(static_cast<cudaError_t>(errorCode));
    }

#else // !GE_HAS_CUDA — stub implementations

    bool CudaContext::Initialize(int) {
        GE_CORE_WARN("CUDA support not compiled — engine built without GE_HAS_CUDA");
        return false;
    }

    void CudaContext::Shutdown() {}
    bool CudaContext::IsAvailable() { return false; }
    const CudaDeviceInfo& CudaContext::GetDeviceInfo() { return s_DeviceInfo; }
    int CudaContext::GetDeviceCount() { return 0; }
    void CudaContext::Synchronize() {}
    size_t CudaContext::GetFreeMemory() { return 0; }
    size_t CudaContext::GetTotalMemory() { return 0; }
    std::string CudaContext::GetErrorString(int) { return "CUDA not available"; }

#endif

} // namespace GameEngine
