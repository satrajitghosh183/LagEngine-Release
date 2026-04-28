//
// Created by barth on 19/09/2022.
//
// Vulkan port: bind() no longer sets GL state.

#include "Material.h"

Material::Material(const char *shaderFolder) : _shader(shaderFolder) {}

void Material::bind() {
    _shader.bind();
    // Wireframe and back-face culling are handled by selecting the
    // appropriate Vulkan pipeline at draw time.
}
