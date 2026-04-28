#include "interop.cuh"
#include <stdexcept>
#include <string>

void cudaCheck(cudaError_t err, const char* msg) {
    if (err != cudaSuccess) {
        throw std::runtime_error(std::string(msg) + ": " + cudaGetErrorString(err));
    }
}

void importVulkanBufferToCuda(CudaVulkanBuffer& buf,
                              void* handle,
                              size_t totalBytes,
                              size_t bufferBytes,
                              size_t offset)
{
    cudaExternalMemoryHandleDesc memDesc = {};
#ifdef _WIN32
    memDesc.type = cudaExternalMemoryHandleTypeOpaqueWin32;
    memDesc.handle.win32.handle = handle;
#else
    memDesc.type = cudaExternalMemoryHandleTypeOpaqueFd;
    memDesc.handle.fd = static_cast<int>(reinterpret_cast<intptr_t>(handle));
#endif
    memDesc.size = totalBytes;
    memDesc.flags = 0;

    cudaError_t err = cudaImportExternalMemory(&buf.extMem, &memDesc);
    cudaCheck(err, "cudaImportExternalMemory");

    cudaExternalMemoryBufferDesc bufDesc = {};
    bufDesc.offset = offset;
    bufDesc.size   = bufferBytes;
    bufDesc.flags  = 0;

    err = cudaExternalMemoryGetMappedBuffer(&buf.devPtr, buf.extMem, &bufDesc);
    cudaCheck(err, "cudaExternalMemoryGetMappedBuffer");

    buf.size = bufferBytes;
}

void destroyCudaVulkanBuffer(CudaVulkanBuffer& buf) {
    if (buf.extMem) {
        cudaDestroyExternalMemory(buf.extMem);
        buf.extMem = nullptr;
    }
    buf.devPtr = nullptr;
    buf.size   = 0;
}
