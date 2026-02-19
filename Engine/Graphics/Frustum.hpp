#pragma once

#include <glm/glm.hpp>
#include "Mesh3D.hpp"

namespace GameEngine {

    /**
     * @brief View frustum for culling
     *
     * Extracts six clip planes from a view-projection matrix.
     * Tests AABBs against the frustum to skip off-screen objects.
     */
    class Frustum {
    public:
        Frustum() = default;

        /**
         * @brief Extract frustum planes from a view-projection matrix
         */
        void Update(const glm::mat4& viewProjection);

        /**
         * @brief Test if an AABB is inside or intersects the frustum
         * @param aabb The axis-aligned bounding box
         * @param transform World transform applied to the AABB
         * @return true if any part of the AABB is potentially visible
         */
        bool IsAABBVisible(const Mesh3D::AABB& aabb, const glm::mat4& transform) const;

        /**
         * @brief Test if a sphere is inside or intersects the frustum
         */
        bool IsSphereVisible(const glm::vec3& center, float radius) const;

        /**
         * @brief Test if a point is inside the frustum
         */
        bool IsPointVisible(const glm::vec3& point) const;

    private:
        enum Side { Left, Right, Bottom, Top, Near, Far, Count };
        glm::vec4 m_Planes[Count];

        void NormalizePlane(glm::vec4& plane);
    };

}
