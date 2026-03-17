#pragma once
#include <glad/glad.h>
#include <cstddef>

struct RendererConfig {
    size_t particleCount = 100000;
};

struct Renderer {
    GLuint vao = 0;
    GLuint vboPositions = 0; // float4 per particle (xyz, w)
    GLuint vboColors   = 0;  // float4 per particle (rgba)
    GLuint program = 0;

    bool   init(size_t particleCount);
    void   draw(size_t particleCount);
    void   shutdown();
};
