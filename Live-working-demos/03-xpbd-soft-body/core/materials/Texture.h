//
// Created by barth on 30/09/2022.
//
// Vulkan port: Texture is a stub. Image loading is retained for future use
// but no GPU texture objects are created. The handle is unused.
//

#ifndef FEATHERGL_TEXTURE_H
#define FEATHERGL_TEXTURE_H

#include <vector>
#include <string>
#include <glm/vec4.hpp>

enum TextureType {
    DEPTH,
    RGBA
};

class Texture {
public:
    explicit Texture() : _handle(0) {};

    explicit Texture(const char *filepath) : _filepath(filepath), _handle(0) {
        // In the Vulkan port we do not create GPU textures for now
    }

    explicit Texture(int id) : _handle(id) {};

    ~Texture() {
        // No GPU resource to free
    }

    Texture(unsigned int width, unsigned int height, TextureType type = RGBA, bool generateMipMap = true)
        : _type(type), _handle(0) {
        // No GPU texture created in Vulkan stub
        (void)width;
        (void)height;
        (void)generateMipMap;
    }

    void resize(int width, int height) {
        // No-op stub
        (void)width;
        (void)height;
    }

    void bind(int texShaderId) {
        // No-op stub
        (void)texShaderId;
    }

    void unbind() {
        // No-op stub
    }

    unsigned int handle() const {
        return _handle;
    }

    unsigned int &handlePtr() {
        return _handle;
    }

private:
    unsigned int _handle{};
    std::string _filepath;
    TextureType _type = RGBA;
};

#endif //FEATHERGL_TEXTURE_H
