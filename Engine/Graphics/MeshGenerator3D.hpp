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
        
        /**
         * @brief Generate cone mesh
         */
        static Ref<Mesh3D> CreateCone(float radius = 0.5f, float height = 1.0f, int segments = 32);
        
        /**
         * @brief Generate torus mesh (donut shape)
         */
        static Ref<Mesh3D> CreateTorus(float majorRadius = 1.0f, float minorRadius = 0.3f, int majorSegments = 32, int minorSegments = 16);
        
        /**
         * @brief Generate quad mesh (single face plane)
         */
        static Ref<Mesh3D> CreateQuad(float width = 1.0f, float height = 1.0f);
        
        /**
         * @brief Generate UV sphere with improved topology
         */
        static Ref<Mesh3D> CreateUVSphere(float radius = 1.0f, int latitudes = 16, int longitudes = 32);
        
        /**
         * @brief Generate icosphere (geodesic sphere)
         */
        static Ref<Mesh3D> CreateIcoSphere(float radius = 1.0f, int subdivisions = 2);
    };
}