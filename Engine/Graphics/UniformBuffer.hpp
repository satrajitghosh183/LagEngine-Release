#pragma once

#include "../Core/Base.hpp"
#include <cstdint>
#include <cstddef>

namespace GameEngine {

    /**
     * @brief OpenGL Uniform Buffer Object wrapper
     *
     * Use for uploading per-frame or per-draw data (camera, lighting)
     * that is shared across many shaders. Bind to a slot and upload data.
     */
    class UniformBuffer {
    public:
        UniformBuffer() = default;
        ~UniformBuffer();

        /**
         * @brief Allocate and initialize the buffer
         * @param size Size in bytes (must match std140 layout if used with shaders)
         * @param bindingSlot GL uniform buffer binding index (e.g. 0, 1)
         * @param dynamic If true, use GL_DYNAMIC_DRAW
         */
        void Init(size_t size, uint32_t bindingSlot, bool dynamic = true);

        /**
         * @brief Upload data to the buffer
         */
        void SetData(const void* data, size_t size, size_t offset = 0);

        /**
         * @brief Bind this UBO to its binding slot
         */
        void Bind() const;

        /**
         * @brief Release GPU resources
         */
        void Shutdown();

        uint32_t GetRendererID() const { return m_RendererID; }
        bool IsValid() const { return m_RendererID != 0; }

    private:
        uint32_t m_RendererID = 0;
        uint32_t m_BindingSlot = 0;
        size_t m_Size = 0;
    };
}
