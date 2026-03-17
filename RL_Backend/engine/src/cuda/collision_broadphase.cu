#include "rldemo/nbody_cuda.cuh"
#include <cuda_runtime.h>

namespace rldemo {
namespace cuda {

__global__ void CollisionResolveKernel(NBodyDevice* __restrict__ bodies, int n) {
    const float floorY = -40.f;
    const float boundsHalf = 50.f;
    const float restitution = 0.5f;

    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;

    float* px = &bodies[i].pos_x, * py = &bodies[i].pos_y, * pz = &bodies[i].pos_z;
    float* vx = &bodies[i].vel_x, * vy = &bodies[i].vel_y, * vz = &bodies[i].vel_z;
    float r = bodies[i].radius;

    if (*py - r < floorY) {
        *py = floorY + r;
        *vy = -(*vy) * restitution;
    }
    if (*px < -boundsHalf + r) { *px = -boundsHalf + r; *vx = -(*vx) * restitution; }
    if (*px > boundsHalf - r)  { *px = boundsHalf - r;  *vx = -(*vx) * restitution; }
    if (*pz < -boundsHalf + r) { *pz = -boundsHalf + r; *vz = -(*vz) * restitution; }
    if (*pz > boundsHalf - r) { *pz = boundsHalf - r;  *vz = -(*vz) * restitution; }

    for (int j = 0; j < n; ++j) {
        if (j == i) continue;
        float dx = bodies[j].pos_x - *px;
        float dy = bodies[j].pos_y - *py;
        float dz = bodies[j].pos_z - *pz;
        float dist = sqrtf(dx*dx + dy*dy + dz*dz);
        float sumR = r + bodies[j].radius;
        if (dist < sumR && dist > 1e-6f) {
            float nx = dx / dist, ny = dy / dist, nz = dz / dist;
            float overlap = sumR - dist;
            float mj = bodies[j].mass;
            float mi = bodies[i].mass;
            float total = mi + mj;
            *px -= nx * (overlap * (mj / total));
            *py -= ny * (overlap * (mj / total));
            *pz -= nz * (overlap * (mj / total));
            float vRel = (*vx - bodies[j].vel_x) * nx + (*vy - bodies[j].vel_y) * ny + (*vz - bodies[j].vel_z) * nz;
            if (vRel < 0.f) {
                float imp = vRel * restitution * (mj / total);
                *vx -= nx * imp;
                *vy -= ny * imp;
                *vz -= nz * imp;
            }
        }
    }
}

void launchCollisionResolve(NBodyDevice* d_bodies, int n, cudaStream_t stream) {
    int block = 256;
    int grid = (n + block - 1) / block;
    CollisionResolveKernel<<<grid, block, 0, stream>>>(d_bodies, n);
}

} // namespace cuda
} // namespace rldemo
