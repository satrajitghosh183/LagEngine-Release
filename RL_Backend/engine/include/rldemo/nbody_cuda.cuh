#pragma once

#include <cuda_runtime.h>

namespace rldemo {
namespace cuda {

struct NBodyDevice {
    float pos_x, pos_y, pos_z;
    float vel_x, vel_y, vel_z;
    float acc_x, acc_y, acc_z;
    float mass;
    float radius;
};

void launchNBodyGravity(NBodyDevice* d_bodies, int n, cudaStream_t stream);
void launchCollisionResolve(NBodyDevice* d_bodies, int n, cudaStream_t stream);
void launchIntegrateAndInstance(NBodyDevice* d_bodies, int n, float dt, float* d_instanceData, cudaStream_t stream);

} // namespace cuda
} // namespace rldemo
