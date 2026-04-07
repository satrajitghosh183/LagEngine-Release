#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

struct cudaGraphicsResource;

class GBuffer;

class SSAO {
public:
    SSAO(int width, int height);
    ~SSAO();
    
    void initialize();
    void process(GBuffer* gbuffer, const glm::mat4& projectionMatrix);
    
    unsigned int getResultTexture() const { return m_resultTexture; }
    
    void resize(int width, int height);

private:
    int m_width;
    int m_height;
    
    unsigned int m_resultTexture;
    cudaGraphicsResource* m_cudaResource; // CUDA-OpenGL interop resource
    
    std::vector<glm::vec3> m_kernel;
    std::vector<glm::vec3> m_noise;
    unsigned int m_noiseTexture;
    
    void generateKernel();
    void generateNoise();
};

