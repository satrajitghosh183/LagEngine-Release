#include <memory>
#include "Particle3D.hpp"

namespace engine::physics {

class Constraint3D {
public:
    Constraint3D(std::shared_ptr<Particle3D> p1, std::shared_ptr<Particle3D> p2, float restDistance);

    void enforce() const;

private:
    std::shared_ptr<Particle3D> particleA;
    std::shared_ptr<Particle3D> particleB;
    float restDistance;
};

} // namespace engine::physics
