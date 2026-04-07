#include "SphereShape.hpp"
#include <glm/gtc/constants.hpp>
#include <cmath>

namespace engine::physics {

SphereShape::SphereShape(float r) : radius(std::max(r, 0.001f)) {
    // Ensure minimum radius for numerical stability
}

BoundingBox SphereShape::getAABB(const Transform& transform) const {
    glm::vec3 center = transform.position;
    float scaledRadius = radius * glm::length(transform.scale);
    glm::vec3 extent(scaledRadius + margin);
    
    return BoundingBox(center - extent, center + extent);
}

bool SphereShape::raycast(const Ray& ray, const Transform& transform, RaycastHit& hit) const {
    glm::vec3 center = transform.position;
    float scaledRadius = radius * glm::length(transform.scale);
    
    // Vector from ray origin to sphere center
    glm::vec3 oc = ray.origin - center;
    
    // Quadratic equation coefficients: |origin + t*direction - center|² = radius²
    float a = glm::dot(ray.direction, ray.direction);
    float b = 2.0f * glm::dot(oc, ray.direction);
    float c = glm::dot(oc, oc) - scaledRadius * scaledRadius;
    
    float discriminant = b * b - 4 * a * c;
    
    if (discriminant < 0) {
        return false;  // No intersection
    }
    
    float sqrt_discriminant = std::sqrt(discriminant);
    float t1 = (-b - sqrt_discriminant) / (2.0f * a);
    float t2 = (-b + sqrt_discriminant) / (2.0f * a);
    
    // Find the closest valid intersection
    float t = -1.0f;
    if (t1 >= 0 && t1 <= ray.maxDistance) {
        t = t1;
    } else if (t2 >= 0 && t2 <= ray.maxDistance) {
        t = t2;
    }
    
    if (t < 0) {
        return false;  // No valid intersection
    }
    
    // Fill hit information
    hit.hit = true;
    hit.distance = t;
    hit.point = ray.getPoint(t);
    hit.normal = glm::normalize(hit.point - center);
    
    return true;
}

glm::vec3 SphereShape::support(const glm::vec3& direction, const Transform& transform) const {
    glm::vec3 center = transform.position;
    float scaledRadius = radius * glm::length(transform.scale);
    
    if (glm::length(direction) < 0.0001f) {
        return center;
    }
    
    return center + glm::normalize(direction) * scaledRadius;
}

float SphereShape::calculateVolume() const {
    return (4.0f / 3.0f) * glm::pi<float>() * radius * radius * radius;
}

glm::mat3 SphereShape::calculateInertiaTensor(float mass) const {
    float i = (2.0f / 5.0f) * mass * radius * radius;
    return glm::mat3(
        i, 0, 0,
        0, i, 0,
        0, 0, i
    );
}

void SphereShape::setRadius(float newRadius) {
    radius = std::max(newRadius, 0.001f);
}

bool SphereShape::sphereVsSphere(const SphereShape& a, const Transform& transformA,
                                const SphereShape& b, const Transform& transformB,
                                glm::vec3& contactPoint, glm::vec3& normal, float& penetration) {
    glm::vec3 centerA = transformA.position;
    glm::vec3 centerB = transformB.position;
    float radiusA = a.radius * glm::length(transformA.scale);
    float radiusB = b.radius * glm::length(transformB.scale);
    
    glm::vec3 direction = centerB - centerA;
    float distance = glm::length(direction);
    float combinedRadius = radiusA + radiusB;
    
    if (distance >= combinedRadius) {
        return false;  // No collision
    }
    
    // Calculate collision response
    if (distance > 0.0001f) {
        normal = direction / distance;
    } else {
        // Spheres are at the same position, use arbitrary normal
        normal = glm::vec3(1.0f, 0.0f, 0.0f);
        distance = 0.0f;
    }
    
    penetration = combinedRadius - distance;
    contactPoint = centerA + normal * radiusA;
    
    return true;
}

} // namespace engine::physics