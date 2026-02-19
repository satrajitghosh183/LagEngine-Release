#include "interop.cuh"
#include <stdexcept>
#include <string>

void cudaCheck(cudaError_t err, const char* msg) {
    if (err != cudaSuccess) {
        throw std::runtime_error(std::string(msg) + ": " + cudaGetErrorString(err));
    }
}

void registerGlBuffer(CudaGlBuffer& buf, unsigned int glBuffer) {
    buf.glBuffer = glBuffer;
    cudaError_t e = cudaGraphicsGLRegisterBuffer(&buf.cudaRes, glBuffer, cudaGraphicsMapFlagsWriteDiscard);
    cudaCheck(e, "cudaGraphicsGLRegisterBuffer");
}

void unregisterGlBuffer(CudaGlBuffer& buf) {
    if (buf.cudaRes) {
        cudaError_t e = cudaGraphicsUnregisterResource(buf.cudaRes);
        cudaCheck(e, "cudaGraphicsUnregisterResource");
        buf.cudaRes = nullptr;
    }
}

void* mapGlBufferDevicePtr(CudaGlBuffer& buf, size_t& outBytes) {
    cudaError_t e = cudaGraphicsMapResources(1, &buf.cudaRes, 0);
    cudaCheck(e, "cudaGraphicsMapResources");
    void* dptr = nullptr;
    e = cudaGraphicsResourceGetMappedPointer(&dptr, &outBytes, buf.cudaRes);
    cudaCheck(e, "cudaGraphicsResourceGetMappedPointer");
    return dptr;
}

void unmapGlBuffer(CudaGlBuffer& buf) {
    cudaError_t e = cudaGraphicsUnmapResources(1, &buf.cudaRes, 0);
    cudaCheck(e, "cudaGraphicsUnmapResources");
}
