#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

struct cudaGraphicsResource;
class Shader;

class ParticleSystem {
public:
    ParticleSystem(int maxParticles = 10000);
    ~ParticleSystem();
    
    void initialize();
    void update(float deltaTime, const glm::mat4& viewMatrix, void* cudaStream = nullptr);
    void render(const glm::mat4& viewProjMatrix, const glm::vec3& cameraPos, const glm::vec3& cameraRight, const glm::vec3& cameraUp);
    
    void resize(int width, int height);
    
    unsigned int getParticleTexture() const { return m_particleTexture; }
    cudaGraphicsResource* getCudaResource() const { return m_cudaResource; }
    int getMaxParticles() const { return m_maxParticles; }

private:
    int m_maxParticles;
    int m_width;
    int m_height;
    
    unsigned int m_particleVAO;
    unsigned int m_particleVBO;
    unsigned int m_particleTexture;
    cudaGraphicsResource* m_cudaResource; // CUDA-OpenGL interop resource
    
    void* m_cudaParticleData; // Device pointer
    std::shared_ptr<Shader> m_particleShader;
    
    bool m_useCuda; // Whether CUDA is available
    std::vector<float> m_cpuParticleData; // CPU fallback particle data
    
    void createParticleBuffer();
    void initializeParticles();
    void updateParticlesCPU(float deltaTime);
    void uploadParticlesToGPU();
};

