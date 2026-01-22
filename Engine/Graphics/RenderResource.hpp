#pragma once

#include "../Core/Base.hpp"
#include <cstdint>

namespace GameEngine {

    /**
     * @brief Strong type wrapper for resource handles
     */
    template<typename Tag>
    struct Handle {
        uint32_t value = 0;
        
        Handle() = default;
        explicit Handle(uint32_t v) : value(v) {}
        
        bool operator==(const Handle& other) const { return value == other.value; }
        bool operator!=(const Handle& other) const { return value != other.value; }
        explicit operator bool() const { return value != 0; }
        operator uint32_t() const { return value; }
    };

    // Tag types for strong typing
    struct TextureTag {};
    struct BufferTag {};
    struct RenderTargetTag {};

    /**
     * @brief Render resource handles
     */
    using TextureHandle = Handle<TextureTag>;
    using BufferHandle = Handle<BufferTag>;
    using RenderTargetHandle = Handle<RenderTargetTag>;

    /**
     * @brief Invalid handle constants
     */
    inline const TextureHandle INVALID_TEXTURE_HANDLE{0};
    inline const BufferHandle INVALID_BUFFER_HANDLE{0};
    inline const RenderTargetHandle INVALID_RENDER_TARGET_HANDLE{0};

}

