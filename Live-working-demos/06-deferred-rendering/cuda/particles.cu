#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <math.h>

struct Particle {
    float x, y, z, w; // Position (x,y,z) and size (w)
};

__device__ float randomFloat(unsigned int seed, int idx) {
    // Simple hash-based random number generator
    unsigned int hash = seed + idx * 1103515245U;
    hash = (hash >> 16) ^ (hash & 0xFFFF);
    hash = hash * 1103515245U + 12345U;
    return ((float)(hash & 0x7FFFFFFF) / 2147483647.0f);
}

__global__ void initializeParticles(Particle* particles, int numParticles, unsigned int seed) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= numParticles) return;

    // Initialize particles in a sphere/cloud formation
    float angle1 = randomFloat(seed, idx * 2) * 3.14159f * 2.0f;
    float angle2 = randomFloat(seed, idx * 2 + 1) * 3.14159f;
    float radius = randomFloat(seed, idx * 3) * 5.0f + 2.0f;

    particles[idx].x = radius * sin(angle2) * cos(angle1);
    particles[idx].y = radius * sin(angle2) * sin(angle1);
    particles[idx].z = radius * cos(angle2);
    particles[idx].w = randomFloat(seed, idx * 4) * 0.05f + 0.02f; // Size between 0.02 and 0.07
}

__global__ void updateParticles(Particle* particles, float deltaTime, int numParticles, unsigned int seed) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= numParticles) return;

    // Get initial velocity based on particle index (for consistency)
    float speed = randomFloat(seed, idx * 5) * 0.5f + 0.3f;
    float angle = randomFloat(seed, idx * 6) * 3.14159f * 2.0f;

    // Update position with circular/spiral motion
    float time = deltaTime * 10.0f;
    particles[idx].x += cos(angle + time) * speed * deltaTime;
    particles[idx].y += sin(angle + time * 0.7f) * speed * deltaTime;
    particles[idx].z += cos(angle + time * 0.5f) * speed * deltaTime * 0.5f;

    // Wrap around if particles go too far
    if (particles[idx].x > 10.0f) particles[idx].x -= 20.0f;
    if (particles[idx].x < -10.0f) particles[idx].x += 20.0f;
    if (particles[idx].y > 10.0f) particles[idx].y -= 20.0f;
    if (particles[idx].y < -10.0f) particles[idx].y += 20.0f;
    if (particles[idx].z > 10.0f) particles[idx].z -= 20.0f;
    if (particles[idx].z < -10.0f) particles[idx].z += 20.0f;

    // Pulsing size effect
    particles[idx].w = (0.02f + 0.03f * (0.5f + 0.5f * sin(time * 2.0f + idx * 0.1f)));
}

static bool g_particlesInitialized = false;

// Vulkan-compatible entry point: operates on a raw CUDA device pointer
// imported from Vulkan via VK_KHR_external_memory.
extern "C" void updateParticleSystemVulkan(void* devicePtr,
                                           float deltaTime, int numParticles,
                                           cudaStream_t stream) {
    Particle* d_particles = reinterpret_cast<Particle*>(devicePtr);
    if (!d_particles) return;

    int blockSize = 256;
    int numBlocks = (numParticles + blockSize - 1) / blockSize;

    // Initialize particles on first call
    if (!g_particlesInitialized) {
        unsigned int seed = 12345;
        initializeParticles<<<numBlocks, blockSize, 0, stream>>>(d_particles, numParticles, seed);
        cudaStreamSynchronize(stream);
        g_particlesInitialized = true;
    }

    // Update particles on the specified stream (async)
    unsigned int seed = 12345;
    updateParticles<<<numBlocks, blockSize, 0, stream>>>(d_particles, deltaTime, numParticles, seed);
}
