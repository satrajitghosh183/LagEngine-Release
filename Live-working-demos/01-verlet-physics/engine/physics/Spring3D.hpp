#pragma once

#include <memory>
#include "Particle3D.hpp"

namespace engine::physics {

class Spring3D {
public:
    Spring3D(std::shared_ptr<Particle3D> p1, std::shared_ptr<Particle3D> p2, float stiffness, float restLength = -1.0f);

    void solve();
    float getCurrentLength() const;

private:
    std::shared_ptr<Particle3D> particleA;
    std::shared_ptr<Particle3D> particleB;
    float stiffness;
    float restLength;
};

} // namespace engine::physics
