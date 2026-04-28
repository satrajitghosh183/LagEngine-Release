#pragma once

#include "../Core/Base.hpp"
#include "../Core/Logger.hpp"
#include <cstdint>
#include <cstddef>
#include <vector>

#ifdef GE_HAS_CUDA
#include <cuda_runtime.h>
#endif

namespace GameEngine {

    /**
     * @brief RAII wrapper for CUDA device memory
     *
     * Provides typed GPU buffer allocation with upload/download.
     * Supports both device-only and managed (unified) memory.
     *
     * Usage:
     *   CudaBuffer<float> positions(1024);
     *   positions.Upload(cpuData.data(), cpuData.size());
     *   // launch kernel with positions.DevicePtr()
     *   positions.Download(cpuData.data(), cpuData.size());
     */
    template <typename T>
    class CudaBuffer {
    public:
        CudaBuffer() = default;

        explicit CudaBuffer(size_t count, bool useUnifiedMemory = false) {
            Allocate(count, useUnifiedMemory);
        }

        ~CudaBuffer() { Free(); }

        CudaBuffer(const CudaBuffer&) = delete;
        CudaBuffer& operator=(const CudaBuffer&) = delete;

        CudaBuffer(CudaBuffer&& other) noexcept
            : m_DevicePtr(other.m_DevicePtr)
            , m_Count(other.m_Count)
            , m_Unified(other.m_Unified) {
            other.m_DevicePtr = nullptr;
            other.m_Count = 0;
        }

        CudaBuffer& operator=(CudaBuffer&& other) noexcept {
            if (this != &other) {
                Free();
                m_DevicePtr = other.m_DevicePtr;
                m_Count = other.m_Count;
                m_Unified = other.m_Unified;
                other.m_DevicePtr = nullptr;
                other.m_Count = 0;
            }
            return *this;
        }

        void Allocate(size_t count, bool useUnifiedMemory = false) {
#ifdef GE_HAS_CUDA
            Free();
            m_Count = count;
            m_Unified = useUnifiedMemory;
            size_t bytes = count * sizeof(T);

            cudaError_t err;
            if (useUnifiedMemory) {
                err = cudaMallocManaged(&m_DevicePtr, bytes);
            } else {
                err = cudaMalloc(&m_DevicePtr, bytes);
            }

            if (err != cudaSuccess) {
                GE_CORE_ERROR("CUDA malloc failed ({0} bytes): {1}", bytes, cudaGetErrorString(err));
                m_DevicePtr = nullptr;
                m_Count = 0;
            }
#else
            (void)count; (void)useUnifiedMemory;
            GE_CORE_WARN("CudaBuffer::Allocate called without CUDA support");
#endif
        }

        void Free() {
#ifdef GE_HAS_CUDA
            if (m_DevicePtr) {
                cudaFree(m_DevicePtr);
                m_DevicePtr = nullptr;
                m_Count = 0;
            }
#endif
        }

        void Upload(const T* hostData, size_t count, size_t offset = 0) {
#ifdef GE_HAS_CUDA
            GE_CORE_ASSERT(m_DevicePtr, "Buffer not allocated");
            GE_CORE_ASSERT(offset + count <= m_Count, "Upload exceeds buffer size");
            cudaMemcpy(m_DevicePtr + offset, hostData, count * sizeof(T), cudaMemcpyHostToDevice);
#else
            (void)hostData; (void)count; (void)offset;
#endif
        }

        void Download(T* hostData, size_t count, size_t offset = 0) const {
#ifdef GE_HAS_CUDA
            GE_CORE_ASSERT(m_DevicePtr, "Buffer not allocated");
            GE_CORE_ASSERT(offset + count <= m_Count, "Download exceeds buffer size");
            cudaMemcpy(hostData, m_DevicePtr + offset, count * sizeof(T), cudaMemcpyDeviceToHost);
#else
            (void)hostData; (void)count; (void)offset;
#endif
        }

        void Memset(int value, size_t count = 0) {
#ifdef GE_HAS_CUDA
            if (count == 0) count = m_Count;
            GE_CORE_ASSERT(m_DevicePtr && count <= m_Count, "Invalid memset");
            cudaMemset(m_DevicePtr, value, count * sizeof(T));
#else
            (void)value; (void)count;
#endif
        }

        T* DevicePtr() { return m_DevicePtr; }
        const T* DevicePtr() const { return m_DevicePtr; }

        size_t Count() const { return m_Count; }
        size_t SizeBytes() const { return m_Count * sizeof(T); }
        bool IsValid() const { return m_DevicePtr != nullptr; }
        bool IsUnified() const { return m_Unified; }

    private:
        T* m_DevicePtr = nullptr;
        size_t m_Count = 0;
        bool m_Unified = false;
    };

} // namespace GameEngine
