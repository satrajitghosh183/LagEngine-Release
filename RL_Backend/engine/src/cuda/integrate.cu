#include "rldemo/nbody_cuda.cuh"
#include <cuda_runtime.h>

namespace rldemo {
namespace cuda {

__global__ void IntegrateAndInstanceKernel(NBodyDevice* __restrict__ bodies, int n, float dt,
                                            float* __restrict__ instanceData) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;

    NBodyDevice& b = bodies[i];
    b.vel_x += b.acc_x * dt;
    b.vel_y += b.acc_y * dt;
    b.vel_z += b.acc_z * dt;
    b.pos_x += b.vel_x * dt;
    b.pos_y += b.vel_y * dt;
    b.pos_z += b.vel_z * dt;

    float* out = instanceData + i * 20;
    float scale = b.radius * 2.f;
    out[0] = scale;  out[1] = 0.f;   out[2] = 0.f;   out[3] = 0.f;
    out[4] = 0.f;    out[5] = scale; out[6] = 0.f;   out[7] = 0.f;
    out[8] = 0.f;    out[9] = 0.f;   out[10] = scale; out[11] = 0.f;
    out[12] = b.pos_x;
    out[13] = b.pos_y;
    out[14] = b.pos_z;
    out[15] = 1.f;
    float hue = (i % 100) / 100.f;
    out[16] = hue;
    out[17] = 0.5f;
    out[18] = 0.8f;
    out[19] = 1.f;
}

void launchIntegrateAndInstance(NBodyDevice* d_bodies, int n, float dt, float* d_instanceData, cudaStream_t stream) {
    int block = 256;
    int grid = (n + block - 1) / block;
    IntegrateAndInstanceKernel<<<grid, block, 0, stream>>>(d_bodies, n, dt, d_instanceData);
}

} // namespace cuda
} // namespace rldemo
