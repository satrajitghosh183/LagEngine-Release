#include "rldemo/nbody_cuda.cuh"
#include <cuda_runtime.h>

namespace rldemo {
namespace cuda {

__global__ void NBodyGravityKernel(NBodyDevice* __restrict__ bodies, int n) {
    const float G = 1e-4f;
    const float softeningSq = 1e-6f;
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;

    float ax = 0.f, ay = 0.f, az = 0.f;
    float xi = bodies[i].pos_x, yi = bodies[i].pos_y, zi = bodies[i].pos_z;

    for (int j = 0; j < n; ++j) {
        if (j == i) continue;
        float dx = bodies[j].pos_x - xi;
        float dy = bodies[j].pos_y - yi;
        float dz = bodies[j].pos_z - zi;
        float distSq = dx*dx + dy*dy + dz*dz + softeningSq;
        float invDist = 1.f / sqrtf(distSq);
        float f = G * bodies[j].mass * invDist * invDist * invDist;
        ax += dx * f;
        ay += dy * f;
        az += dz * f;
    }
    bodies[i].acc_x = ax;
    bodies[i].acc_y = ay;
    bodies[i].acc_z = az;
}

void launchNBodyGravity(NBodyDevice* d_bodies, int n, cudaStream_t stream) {
    int block = 256;
    int grid = (n + block - 1) / block;
    NBodyGravityKernel<<<grid, block, 0, stream>>>(d_bodies, n);
}

} // namespace cuda
} // namespace rldemo
