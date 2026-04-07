#pragma once
#include "../CollisonShape.hpp"


namespace engine::physics {

/**
 * @brief Capsule collision shape (cylinder with hemispheres on ends)
 * Perfect for character controllers and elongated objects
 */
class CapsuleShape : public CollisionShape {
public:
    CapsuleShape(float radius, float height);

    // CollisionShape interface
    ShapeType getType() const override { return ShapeType::Capsule; }
    BoundingBox getAABB(const Transform& transform) const override;
    bool raycast(const Ray& ray, const Transform& transform, RaycastHit& hit) const override;
    glm::vec3 support(const glm::vec3& direction, const Transform& transform) const override;
    float calculateVolume() const override;
    glm::mat3 calculateInertiaTensor(float mass) const override;

    // Capsule-specific
    float getRadius() const { return radius; }
    float getHeight() const { return height; }
    float getTotalHeight() const { return height + 2.0f * radius; }
    
    void setRadius(float r) { radius = std::max(r, 0.001f); }
    void setHeight(float h) { height = std::max(h, 0.001f); }
    
    // Utility functions
    glm::vec3 getTopCenter(const Transform& transform) const;
    glm::vec3 getBottomCenter(const Transform& transform) const;
    void getEndpoints(const Transform& transform, glm::vec3& top, glm::vec3& bottom) const;

private:
    float radius;
    float height;  // Height of cylindrical section (not including caps)
    
    // Internal helpers
    bool raycastCylinder(const Ray& localRay, float& t) const;
    bool raycastSphere(const Ray& localRay, const glm::vec3& center, float& t) const;
    glm::vec3 closestPointOnLineSegment(const glm::vec3& point, 
                                       const glm::vec3& a, const glm::vec3& b) const;
};

} // namespace engine::physics