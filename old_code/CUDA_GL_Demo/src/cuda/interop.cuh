#pragma once
#include <cuda_runtime.h>
#include <cuda_gl_interop.h>

// NOTE: Do NOT include <glad/glad.h> in CUDA files.
// cuda_gl_interop.h already provides GL headers.

struct CudaGlBuffer {
    unsigned int glBuffer = 0u;
    cudaGraphicsResource* cudaRes = nullptr;
};

void cudaCheck(cudaError_t err, const char* msg);
void registerGlBuffer(CudaGlBuffer& buf, unsigned int glBuffer);
void unregisterGlBuffer(CudaGlBuffer& buf);
void* mapGlBufferDevicePtr(CudaGlBuffer& buf, size_t& outBytes);
void unmapGlBuffer(CudaGlBuffer& buf);
