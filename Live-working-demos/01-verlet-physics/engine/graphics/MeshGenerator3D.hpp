


#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>

#include "Mesh3D.hpp"
#include "../physics/Particle3D.hpp"

namespace engine::graphics {

class MeshGenerator3D {
public:
    static void generateClothMesh(
        int width,
        int height,
        const std::vector<std::shared_ptr<engine::physics::Particle3D>>& particles,
        std::vector<Vertex3D>& outVertices,
        std::vector<unsigned int>& outIndices
    );
};

} // namespace engine::graphics
