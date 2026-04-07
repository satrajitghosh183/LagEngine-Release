#include "SSAO.h"
#include "GBuffer.h"
#include <glad/glad.h>
#include <cuda_runtime.h>
#include <cuda_gl_interop.h>
#include <random>
#include <cmath>
#include <stdexcept>
#include <algorithm>

SSAO::SSAO(int width, int height)
    : m_width(width)
    , m_height(height)
    , m_resultTexture(0)
    , m_cudaResource(nullptr)
    , m_noiseTexture(0)
{
    generateKernel();
    generateNoise();
}

SSAO::~SSAO() {
    if (m_resultTexture != 0) {
        glDeleteTextures(1, &m_resultTexture);
    }
    if (m_noiseTexture != 0) {
        glDeleteTextures(1, &m_noiseTexture);
    }
    if (m_cudaResource != nullptr) {
        cudaGraphicsUnregisterResource(m_cudaResource);
    }
}

void SSAO::initialize() {
    // Create result texture for CUDA output
    glGenTextures(1, &m_resultTexture);
    glBindTexture(GL_TEXTURE_2D, m_resultTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_width, m_height, 0, GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Register with CUDA
    cudaGraphicsGLRegisterImage(&m_cudaResource, m_resultTexture, GL_TEXTURE_2D, cudaGraphicsRegisterFlagsWriteDiscard);
}

void SSAO::generateKernel() {
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
    std::default_random_engine generator;
    
    m_kernel.resize(64);
    for (unsigned int i = 0; i < 64; ++i) {
        glm::vec3 sample(
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator)
        );
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        float scale = (float)i / 64.0f;
        scale = 0.1f + scale * scale * 0.9f;
        sample *= scale;
        m_kernel[i] = sample;
    }
}

void SSAO::generateNoise() {
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
    std::default_random_engine generator;
    
    m_noise.resize(16);
    for (unsigned int i = 0; i < 16; i++) {
        glm::vec3 noise(
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            0.0f
        );
        m_noise[i] = noise;
    }
    
    glGenTextures(1, &m_noiseTexture);
    glBindTexture(GL_TEXTURE_2D, m_noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, &m_noise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void SSAO::process(GBuffer* gbuffer, const glm::mat4& projectionMatrix) {
    // This will be implemented by CUDA kernel
    // For now, just a placeholder
    // The actual SSAO computation will be done in cuda/ssao.cu
}

void SSAO::resize(int width, int height) {
    m_width = width;
    m_height = height;
    
    if (m_resultTexture != 0) {
        glBindTexture(GL_TEXTURE_2D, m_resultTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_width, m_height, 0, GL_RED, GL_FLOAT, nullptr);
    }
}

