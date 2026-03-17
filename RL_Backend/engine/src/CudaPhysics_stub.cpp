#include "rldemo/CudaPhysics.hpp"
#include "rldemo/NBodySim.hpp"

namespace rldemo {

CudaPhysics::~CudaPhysics() {
    Shutdown();
}

bool CudaPhysics::Init(uint32_t /*maxBodies*/) {
    return false;
}

void CudaPhysics::Step(const std::vector<NBody>& /*hostBodies*/, float /*dt*/, uint32_t /*count*/) {}

void CudaPhysics::Shutdown() {
    m_InstanceVBO = 0;
    m_InstanceCount = 0;
    m_DeviceBodies = nullptr;
    m_GraphicsResource = nullptr;
    m_Initialized = false;
}

} // namespace rldemo
