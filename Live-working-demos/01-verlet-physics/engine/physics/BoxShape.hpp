#pragma once

#include "CollisonShape.hpp"
#include <vector>

namespace engine::physics {

/**
 * @brief Axis-aligned box collision shape
 * Efficient for rectangular objects and buildings
 */
class BoxShape : public CollisionShape {
public:
    explicit BoxShape(const glm::vec3& halfExtents);
    explicit BoxShape(float halfX, float halfY, float halfZ);

    // CollisionShape interface
    ShapeType getType() const override { return ShapeType::Box; }
    BoundingBox getAABB(const Transform& transform) const override;
    bool raycast(const Ray& ray, const Transform& transform, RaycastHit& hit) const override;
    glm::vec3 support(const glm::vec3& direction, const Transform& transform) const override;
    float calculateVolume() const override;
    glm::mat3 calculateInertiaTensor(float mass) const override;

    // Box-specific
    const glm::vec3& getHalfExtents() const { return halfExtents; }
    void setHalfExtents(const glm::vec3& extents);
    glm::vec3 getSize() const { return halfExtents * 2.0f; }

    // Utility functions
    std::vector<glm::vec3> getVertices(const Transform& transform) const;
    std::vector<glm::vec3> getFaceNormals(const Transform& transform) const;

private:
    glm::vec3 halfExtents;
    
    // Helper for AABB calculation with rotation
    BoundingBox calculateRotatedAABB(const Transform& transform) const;
};

} // namespace engine::physics