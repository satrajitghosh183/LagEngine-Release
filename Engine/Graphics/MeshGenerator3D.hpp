#pragma once

#include "../Core/Base.hpp"
#include "Mesh3D.hpp"
#include <glm/glm.hpp>

namespace GameEngine {

    /**
     * @brief 3D mesh generation utilities
     * 
     * Generate common 3D shapes procedurally
     */
    class MeshGenerator3D {
    public:
        /**
         * @brief Generate cube mesh
         */
        static Ref<Mesh3D> CreateCube(float size = 1.0f);
        
        /**
         * @brief Generate sphere mesh
         */
        static Ref<Mesh3D> CreateSphere(float radius = 1.0f, int segments = 32, int rings = 16);
        
        /**
         * @brief Generate plane mesh
         */
        static Ref<Mesh3D> CreatePlane(float width = 10.0f, float height = 10.0f, int subdivisionsX = 1, int subdivisionsY = 1);
        
        /**
         * @brief Generate cylinder mesh
         */
        static Ref<Mesh3D> CreateCylinder(float radius = 0.5f, float height = 2.0f, int segments = 32);
        
        /**
         * @brief Generate capsule mesh
         */
        static Ref<Mesh3D> CreateCapsule(float radius = 0.5f, float height = 2.0f, int segments = 32, int rings = 8);
    };
}