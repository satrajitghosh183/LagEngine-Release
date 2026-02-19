#include "ParticleSystem.h"
#include "Shader.h"
#include <glad/glad.h>
#include <cuda_runtime.h>
#include <cuda_gl_interop.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <iostream>
#include <random>
#include <cmath>
#include <algorithm>

// Forward declaration of CUDA function
extern "C" void updateParticleSystem(cudaGraphicsResource_t particleResource,
                                     float deltaTime, int numParticles,
                                     cudaStream_t stream = 0);

ParticleSystem::ParticleSystem(int maxParticles)
    : m_maxParticles(maxParticles)
    , m_width(1280)
    , m_height(720)
    , m_particleVAO(0)
    , m_particleVBO(0)
    , m_particleTexture(0)
    , m_cudaResource(nullptr)
    , m_cudaParticleData(nullptr)
    , m_useCuda(false)
    , m_cpuParticleData(maxParticles * 4, 0.0f)
{
}

ParticleSystem::~ParticleSystem() {
    if (m_particleVAO != 0) {
        glDeleteVertexArrays(1, &m_particleVAO);
    }
    if (m_particleVBO != 0) {
        glDeleteBuffers(1, &m_particleVBO);
    }
    if (m_particleTexture != 0) {
        glDeleteTextures(1, &m_particleTexture);
    }
    if (m_cudaResource != nullptr) {
        cudaGraphicsUnregisterResource(m_cudaResource);
    }
    if (m_cudaParticleData != nullptr) {
        cudaFree(m_cudaParticleData);
    }
}

void ParticleSystem::initialize() {
    createParticleBuffer();
    initializeParticles();
    
    // Create simple particle texture
    glGenTextures(1, &m_particleTexture);
    glBindTexture(GL_TEXTURE_2D, m_particleTexture);
    unsigned char data[] = {255, 255, 255, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Load particle shader
    m_particleShader = std::make_shared<Shader>();
    if (!m_particleShader->loadFromFiles("shaders/particle.vert", "shaders/particle.frag")) {
        std::cerr << "ERROR: Failed to load particle shader" << std::endl;
    } else {
        std::cout << "✓ Particle shader loaded successfully" << std::endl;
    }
}

void ParticleSystem::createParticleBuffer() {
    glGenVertexArrays(1, &m_particleVAO);
    glGenBuffers(1, &m_particleVBO);
    
    glBindVertexArray(m_particleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_particleVBO);
    glBufferData(GL_ARRAY_BUFFER, m_maxParticles * sizeof(float) * 4, nullptr, GL_DYNAMIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    
    glBindVertexArray(0);
    
    // Initialize CUDA device (if not already done)
    int deviceCount = 0;
    cudaGetDeviceCount(&deviceCount);
    if (deviceCount == 0) {
        std::cerr << "Warning: No CUDA devices found. Particles will not be updated." << std::endl;
        return;
    }
    
    // Register with CUDA (modern CUDA handles device selection automatically)
    cudaError_t err = cudaGraphicsGLRegisterBuffer(&m_cudaResource, m_particleVBO, cudaGraphicsRegisterFlagsWriteDiscard);
    if (err != cudaSuccess) {
        std::cerr << "Warning: Failed to register OpenGL buffer with CUDA: " 
                  << cudaGetErrorString(err) << std::endl;
        std::cerr << "Particles will use CPU fallback for updates." << std::endl;
        m_cudaResource = nullptr;
        m_useCuda = false;
    } else {
        std::cout << "Successfully registered particle buffer with CUDA for " 
                  << m_maxParticles << " particles." << std::endl;
        m_useCuda = true;
    }
    
    // Initialize particles on CPU (will be used if CUDA fails or as fallback)
    initializeParticles();
}

void ParticleSystem::update(float deltaTime, const glm::mat4& viewMatrix, void* cudaStream) {
    if (m_useCuda && m_cudaResource != nullptr) {
        // Use CUDA for particle updates on the specified stream
        cudaStream_t stream = cudaStream ? reinterpret_cast<cudaStream_t>(cudaStream) : 0;
        updateParticleSystem(m_cudaResource, deltaTime, m_maxParticles, stream);
        
        // Check for CUDA errors (don't synchronize - let the scheduler handle it)
        cudaError_t err = cudaGetLastError();
        if (err != cudaSuccess) {
            std::cerr << "CUDA kernel launch error: " << cudaGetErrorString(err) << std::endl;
            // Fall back to CPU on error
            m_useCuda = false;
            updateParticlesCPU(deltaTime);
            return;
        }
        // Note: We don't synchronize here - the scheduler will synchronize all streams
    } else {
        // Use CPU fallback for particle updates
        updateParticlesCPU(deltaTime);
    }
}

void ParticleSystem::render(const glm::mat4& viewProjMatrix, const glm::vec3& cameraPos, const glm::vec3& cameraRight, const glm::vec3& cameraUp) {
    if (!m_particleShader) {
        static bool warned = false;
        if (!warned) {
            std::cerr << "ERROR: Particle shader not loaded!" << std::endl;
            warned = true;
        }
        return;
    }
    
    // SIMPLIFIED: Make particles always visible - NO depth test, NO culling
    glDisable(GL_DEPTH_TEST);  // Disable depth test - particles always visible
    glDisable(GL_CULL_FACE);   // Don't cull anything
    glDisable(GL_BLEND);       // NO blending - solid particles for maximum visibility
    glEnable(GL_PROGRAM_POINT_SIZE);
    
    m_particleShader->use();
    
    // Set the view-projection matrix (shader uses "viewProjMatrix" uniform)
    m_particleShader->setMat4("viewProjMatrix", viewProjMatrix);
    
    // Make sure VBO has data
    glBindVertexArray(m_particleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_particleVBO);
    
    // Verify buffer has data
    GLint bufferSize = 0;
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
    
    // Draw particles
    static int frameCount = 0;
    if (frameCount++ < 3) {
        std::cout << "Drawing " << m_maxParticles << " particles" << std::endl;
        std::cout << "Buffer size: " << bufferSize << " bytes" << std::endl;
        if (m_cpuParticleData.size() > 0) {
            std::cout << "First particle position: (" 
                      << m_cpuParticleData[0] << ", " 
                      << m_cpuParticleData[1] << ", " 
                      << m_cpuParticleData[2] << ")" << std::endl;
        }
        std::cout << "ViewProj matrix [0][0]: " << viewProjMatrix[0][0] << std::endl;
    }
    
    // Check for shader errors before drawing
    GLenum err = glGetError();
    if (err != GL_NO_ERROR && frameCount < 5) {
        std::cerr << "OpenGL error before particle draw: " << err << std::endl;
    }
    
    glDrawArrays(GL_POINTS, 0, m_maxParticles);
    
    // Check for errors after drawing
    err = glGetError();
    if (err != GL_NO_ERROR && frameCount < 5) {
        std::cerr << "OpenGL error after particle draw: " << err << std::endl;
    }
    
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    glDisable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_DEPTH_TEST);  // Re-enable depth test
    
    // Final error check
    err = glGetError();
    static int errorCount = 0;
    if (err != GL_NO_ERROR && errorCount++ < 5) {
        std::cerr << "OpenGL error in particle render: " << err << std::endl;
    }
}

void ParticleSystem::initializeParticles() {
    // Initialize particles in a spherical cloud formation
    std::mt19937 rng(12345);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    for (int i = 0; i < m_maxParticles; ++i) {
        float angle1 = dist(rng) * 3.14159f * 2.0f;
        float angle2 = dist(rng) * 3.14159f;
        float radius = dist(rng) * 8.0f + 3.0f;  // Larger initial radius
        
        int idx = i * 4;
        m_cpuParticleData[idx + 0] = radius * sin(angle2) * cos(angle1); // x
        m_cpuParticleData[idx + 1] = radius * sin(angle2) * sin(angle1); // y
        m_cpuParticleData[idx + 2] = radius * cos(angle2); // z
        m_cpuParticleData[idx + 3] = dist(rng) * 0.08f + 0.04f; // Larger initial size
    }
    
    // Upload to GPU
    uploadParticlesToGPU();
}

void ParticleSystem::uploadParticlesToGPU() {
    if (m_particleVBO == 0 || m_cpuParticleData.empty()) {
        return;
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, m_particleVBO);
    size_t dataSize = m_cpuParticleData.size() * sizeof(float);
    glBufferSubData(GL_ARRAY_BUFFER, 0, dataSize, m_cpuParticleData.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ParticleSystem::updateParticlesCPU(float deltaTime) {
    // Safety check
    if (m_cpuParticleData.empty() || m_maxParticles <= 0) {
        return;
    }
    
    // Clamp deltaTime to prevent huge jumps
    deltaTime = std::max(0.0f, std::min(deltaTime, 0.1f));
    
    static float time = 0.0f;
    time += deltaTime;
    
    // More complex physics: gravity, turbulence, and interactions
    const float gravity = -2.0f;
    const float turbulence = 1.5f;
    const float centerAttraction = 0.3f;
    
    // Update particles with more complex physics
    for (int i = 0; i < m_maxParticles && (i * 4 + 3) < static_cast<int>(m_cpuParticleData.size()); ++i) {
        int idx = i * 4;
        
        // Get consistent random values for this particle
        float speed = (static_cast<float>((i * 5) % 1000) / 1000.0f) * 0.8f + 0.4f;
        float angle = (static_cast<float>((i * 6) % 1000) / 1000.0f) * 3.14159f * 2.0f;
        float phase = (static_cast<float>((i * 7) % 1000) / 1000.0f) * 3.14159f * 2.0f;
        
        // Get current position
        float x = m_cpuParticleData[idx + 0];
        float y = m_cpuParticleData[idx + 1];
        float z = m_cpuParticleData[idx + 2];
        
        // Distance from center
        float distFromCenter = std::sqrt(x*x + y*y + z*z);
        
        // Complex motion: spiral + gravity + turbulence + center attraction
        float t = time * 8.0f;
        
        // Spiral motion
        float spiralX = std::cos(angle + t) * speed;
        float spiralY = std::sin(angle + t * 0.7f) * speed;
        float spiralZ = std::cos(angle + t * 0.5f) * speed * 0.5f;
        
        // Gravity effect
        float gravityY = gravity * deltaTime;
        
        // Turbulence (noise-like motion)
        float turbX = std::sin(t * 3.0f + phase) * turbulence * deltaTime;
        float turbY = std::cos(t * 2.5f + phase * 1.3f) * turbulence * deltaTime;
        float turbZ = std::sin(t * 2.2f + phase * 0.7f) * turbulence * deltaTime;
        
        // Center attraction (particles pulled toward origin)
        float centerX = -x * centerAttraction * deltaTime;
        float centerY = -y * centerAttraction * deltaTime;
        float centerZ = -z * centerAttraction * deltaTime;
        
        // Combine all forces
        m_cpuParticleData[idx + 0] += (spiralX + turbX + centerX) * deltaTime;
        m_cpuParticleData[idx + 1] += (spiralY + gravityY + turbY + centerY) * deltaTime;
        m_cpuParticleData[idx + 2] += (spiralZ + turbZ + centerZ) * deltaTime;
        
        // Wrap around with larger bounds
        const float bounds = 15.0f;
        if (m_cpuParticleData[idx + 0] > bounds) m_cpuParticleData[idx + 0] -= bounds * 2.0f;
        if (m_cpuParticleData[idx + 0] < -bounds) m_cpuParticleData[idx + 0] += bounds * 2.0f;
        if (m_cpuParticleData[idx + 1] > bounds) m_cpuParticleData[idx + 1] -= bounds * 2.0f;
        if (m_cpuParticleData[idx + 1] < -bounds) m_cpuParticleData[idx + 1] += bounds * 2.0f;
        if (m_cpuParticleData[idx + 2] > bounds) m_cpuParticleData[idx + 2] -= bounds * 2.0f;
        if (m_cpuParticleData[idx + 2] < -bounds) m_cpuParticleData[idx + 2] += bounds * 2.0f;
        
        // More complex size variation based on position and time
        float sizePhase = t * 3.0f + distFromCenter * 0.5f + i * 0.01f;
        m_cpuParticleData[idx + 3] = 0.03f + 0.04f * (0.5f + 0.5f * std::sin(sizePhase));
    }
    
    // Upload updated data to GPU
    uploadParticlesToGPU();
}

void ParticleSystem::resize(int width, int height) {
    m_width = width;
    m_height = height;
}

