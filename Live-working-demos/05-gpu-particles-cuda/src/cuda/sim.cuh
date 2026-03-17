#pragma once
#include <cstddef>
#include <cuda_runtime.h>

struct SimParams {
    float dt          = 0.016f;
    float gravityY    = -9.81f;
    float restitution = 0.7f;
    float damping     = 0.98f;
    float emitY       = 2.0f;
    float floorY      = 0.0f;
    unsigned seed     = 1337u;
    unsigned frame    = 0u;
};

void launchInit(float4* pos, float4* vel, float4* col, size_t N, SimParams p, cudaStream_t stream = 0, int blockSize = 256);
void launchStep(float4* pos, float4* vel, float4* col, size_t N, SimParams p, cudaStream_t stream = 0, int blockSize = 256);
