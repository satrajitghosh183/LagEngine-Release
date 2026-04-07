#pragma once

#include <string>

namespace engine::graphics {

class Texture2D {
public:
    Texture2D(const std::string& path);
    ~Texture2D();

    void bind(unsigned int slot = 0) const;
    void unbind() const;

private:
    unsigned int textureID;
    int width;
    int height;
    int channels;
};

} // namespace engine::graphics
