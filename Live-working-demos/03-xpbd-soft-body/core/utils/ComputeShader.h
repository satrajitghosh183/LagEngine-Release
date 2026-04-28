//
// Created by barth on 13/10/23.
//
// Vulkan port: ComputeShader is a stub. Compute pipelines require a
// separate Vulkan compute pipeline setup.
//

#ifndef FEATHERGL_COMPUTESHADER_H
#define FEATHERGL_COMPUTESHADER_H

#include <glm/fwd.hpp>
#include <glm/detail/type_vec2.hpp>
#include <vector>
#include <iostream>
#include "Texture.h"

class ComputeShader {
public:
    ComputeShader(const char *shaderPath, glm::ivec2 size) : work_size(size) {
        // No-op stub
        (void)shaderPath;
    }

    ~ComputeShader() {}

    void set_values(float *values) {
        (void)values;
    }

    std::vector<float> get_values() {
        return std::vector<float>(work_size.x * work_size.y * 4, 0.0f);
    }

    void use() {}

    void dispatch() const {}

    void wait() {}

protected:
    glm::ivec2 work_size;
};

#endif //FEATHERGL_COMPUTESHADER_H
