#include "sim.cuh"
#include <cuda_runtime.h>
#include <math_constants.h>

__host__ __device__ inline float3 f3(float x, float y, float z) { return make_float3(x,y,z); }
__host__ __device__ inline float3 f3(const float4& v) { return make_float3(v.x, v.y, v.z); }
__host__ __device__ inline float4 f4(const float3& v, float w=1.f) { return make_float4(v.x, v.y, v.z, w); }

__host__ __device__ inline float3 add(const float3& a, const float3& b){ return f3(a.x+b.x, a.y+b.y, a.z+b.z); }
__host__ __device__ inline float3 sub(const float3& a, const float3& b){ return f3(a.x-b.x, a.y-b.y, a.z-b.z); }
__host__ __device__ inline float3 mul(const float3& a, float s){ return f3(a.x*s, a.y*s, a.z*s); }
__host__ __device__ inline float  len(const float3& a){ return sqrtf(a.x*a.x + a.y*a.y + a.z*a.z); }

static __device__ float hash11(unsigned int x) {
    x ^= x * 0x3d20adea;
    x ^= x * 0x05526c56;
    x ^= x * 0x53a22864;
    return (x & 0x00FFFFFFu) / float(0x01000000);
}

static __device__ float3 rand3(unsigned int i, unsigned int seed) {
    return make_float3(hash11(i*9781u + seed), hash11(i*6271u + seed*3u), hash11(i*4357u + seed*7u));
}

__global__ void kInit(float4* pos, float4* vel, float4* col, size_t N, SimParams p) {
    size_t i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= N) return;

    float3 r = rand3((unsigned)i, p.seed);
    float theta = r.x * 6.2831853f;
    float phi = (r.y - 0.5f) * 3.14159f * 0.5f;
    float speed = 3.0f + r.z * 4.0f;

    float3 emitVel = f3(
        cosf(theta) * cosf(phi) * speed,
        sinf(phi) * speed + 5.0f,
        sinf(theta) * cosf(phi) * speed
    );

    // Spread initial positions slightly
    float3 emitPos = f3(
        (r.x - 0.5f) * 0.5f,
        0.2f + r.y * 0.3f,
        (r.z - 0.5f) * 0.5f
    );

    pos[i] = f4(emitPos, 1.f);
    vel[i] = f4(emitVel, 0.f);
    
    // Bright initial colors
    float hue = r.x;
    if (hue < 0.33f) {
        col[i] = make_float4(1.0f, 0.3f, 0.1f, 1.0f); // Orange
    } else if (hue < 0.66f) {
        col[i] = make_float4(0.1f, 1.0f, 0.3f, 1.0f); // Green
    } else {
        col[i] = make_float4(0.3f, 0.3f, 1.0f, 1.0f); // Blue
    }
}

__global__ void kStep(float4* pos, float4* vel, float4* col, size_t N, SimParams p) {
    size_t i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= N) return;

    float3 x = f3(pos[i]);
    float3 v = f3(vel[i]);
    float4 c = col[i];

    // Simple gravity
    v.y += p.gravityY * p.dt;
    x = add(x, mul(v, p.dt));

    // Floor collision with bounce
    if (x.y < p.floorY) {
        x.y = p.floorY + 0.01f;
        if (v.y < 0.0f) {
            v.y = -v.y * p.restitution;
            v.x *= p.damping;
            v.z *= p.damping;
        }
        // Yellow on impact
        c = make_float4(1.0f, 1.0f, 0.2f, 1.0f);
    } else {
        // Color based on velocity
        float speed = len(v);
        float t = fminf(1.0f, speed / 8.0f);
        c.x = 0.2f + 0.8f * t;
        c.y = 0.3f + 0.7f * (1.0f - t);
        c.z = 1.0f - 0.5f * t;
        c.w = 1.0f;
    }

    pos[i] = f4(x, 1.0f);
    vel[i] = f4(v, 0.0f);
    col[i] = c;

    // Re-emit if settled
    if (x.y <= p.floorY + 0.02f && len(v) < 0.3f) {
        float3 r = rand3((unsigned)i, p.seed + p.frame);
        float theta = r.x * 6.2831853f;
        float phi = (r.y - 0.5f) * 3.14159f * 0.5f;
        float speed = 3.0f + r.z * 4.0f;

        float3 emitVel = f3(
            cosf(theta) * cosf(phi) * speed,
            sinf(phi) * speed + 5.0f,
            sinf(theta) * cosf(phi) * speed
        );

        float3 emitPos = f3(
            (r.x - 0.5f) * 0.5f,
            0.2f + r.y * 0.3f,
            (r.z - 0.5f) * 0.5f
        );

        pos[i] = f4(emitPos, 1.f);
        vel[i] = f4(emitVel, 0.f);
        
        float hue = r.x;
        if (hue < 0.33f) {
            col[i] = make_float4(1.0f, 0.3f, 0.1f, 1.0f);
        } else if (hue < 0.66f) {
            col[i] = make_float4(0.1f, 1.0f, 0.3f, 1.0f);
        } else {
            col[i] = make_float4(0.3f, 0.3f, 1.0f, 1.0f);
        }
    }
}

void launchInit(float4* pos, float4* vel, float4* col, size_t N, SimParams p, cudaStream_t stream, int blockSize) {
    const int BS = blockSize > 0 ? blockSize : 256;
    int GS = (int)((N + BS - 1) / BS);
    kInit<<<GS,BS,0,stream>>>(pos, vel, col, N, p);
    cudaGetLastError();
}

void launchStep(float4* pos, float4* vel, float4* col, size_t N, SimParams p, cudaStream_t stream, int blockSize) {
    const int BS = blockSize > 0 ? blockSize : 256;
    int GS = (int)((N + BS - 1) / BS);
    kStep<<<GS,BS,0,stream>>>(pos, vel, col, N, p);
    cudaGetLastError();
}
