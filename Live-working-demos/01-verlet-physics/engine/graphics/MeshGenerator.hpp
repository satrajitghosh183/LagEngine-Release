#pragma once
#include <vector>
#include "Mesh.hpp"
#include <glm/glm.hpp>

namespace engine::graphics {

    class MeshGenerator {
    public:
        static Mesh Grid(int width, int height, float spacing);
        static Mesh Sphere(float radius, int stacks, int slices);
        static Mesh Plane(float size = 1.0f);
    };

} // namespace engine::graphics
