// Texture2D.hpp -- Vulkan-compatible texture stub
//
// Texture loading is not critical for the wireframe / flat-shaded
// physics demos.  This stub keeps the API surface so that any code
// referencing Texture2D still compiles, but all operations are no-ops.
#pragma once

#include <string>

namespace engine::graphics {

class Texture2D {
public:
    Texture2D(const std::string& /*path*/) {}
    ~Texture2D() = default;

    void bind(unsigned int /*slot*/ = 0) const {}
    void unbind() const {}

    int getWidth() const { return 0; }
    int getHeight() const { return 0; }
};

} // namespace engine::graphics
