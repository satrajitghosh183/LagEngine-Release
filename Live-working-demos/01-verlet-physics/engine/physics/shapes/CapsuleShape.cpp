#include "CapsuleShape.hpp"
#include <glm/gtc/constants.hpp>
#include <algorithm>

namespace engine::physics {

CapsuleShape::CapsuleShape(float r, float h) 
    : radius(std::max(r, 0.001f)), height(std::max(h, 0.001f)) {
}

BoundingBox CapsuleShape::getAABB(const Transform& transform) const {
    glm::vec3 center = transform.position;
    glm::vec3 up = transform.rotation * glm::vec3(0, 1, 0);
    
    // Calculate extent in Y direction
    float halfHeight = height * 0.5f;
    glm::vec3 topCenter = center + up * halfHeight;
    glm::vec3 bottomCenter = center - up * halfHeight;
    
    // Expand by radius in all directions
    glm::vec3 radiusExtent(radius + margin);
    
    BoundingBox aabb;
    aabb.expand(topCenter + radiusExtent);
    aabb.expand(topCenter - radiusExtent);
    aabb.expand(bottomCenter + radiusExtent);
    aabb.expand(bottomCenter - radiusExtent);
    
    return aabb;
}

bool CapsuleShape::raycast(const Ray& ray, const Transform& transform, RaycastHit& hit) const {
    // Transform ray to local space
    glm::mat4 invTransform = glm::inverse(transform.getMatrix());
    glm::vec3 localOrigin = glm::vec3(invTransform * glm::vec4(ray.origin, 1.0f));
    glm::vec3 localDirection = glm::vec3(invTransform * glm::vec4(ray.direction, 0.0f));
    Ray localRay(localOrigin, localDirection, ray.maxDistance);
    
    float t = FLT_MAX;
    bool hitFound = false;
    
    // Test cylinder part
    float cylinderT;
    if (raycastCylinder(localRay, cylinderT)) {
        t = std::min(t, cylinderT);
        hitFound = true;
    }
    
    // Test top hemisphere
    float topT;
    glm::vec3 topCenter(0, height * 0.5f, 0);
    if (raycastSphere(localRay, topCenter, topT)) {
        glm::vec3 hitPoint = localRay.getPoint(topT);
        if (hitPoint.y >= height * 0.5f) { // Only count if in upper hemisphere
            t = std::min(t, topT);
            hitFound = true;
        }
    }
    
    // Test bottom hemisphere
    float bottomT;
    glm::vec3 bottomCenter(0, -height * 0.5f, 0);
    if (raycastSphere(localRay, bottomCenter, bottomT)) {
        glm::vec3 hitPoint = localRay.getPoint(bottomT);
        if (hitPoint.y <= -height * 0.5f) { // Only count if in lower hemisphere
            t = std::min(t, bottomT);
            hitFound = true;
        }
    }
    
    if (!hitFound || t > ray.maxDistance) {
        return false;
    }
    
    // Calculate hit information in world space
    glm::vec3 localHitPoint = localRay.getPoint(t);
    
    // Calculate normal
    glm::vec3 localNormal;
    if (localHitPoint.y > height * 0.5f) {
        // Top hemisphere
        localNormal = glm::normalize(localHitPoint - topCenter);
    } else if (localHitPoint.y < -height * 0.5f) {
        // Bottom hemisphere
        localNormal = glm::normalize(localHitPoint - bottomCenter);
    } else {
        // Cylinder
        localNormal = glm::normalize(glm::vec3(localHitPoint.x, 0, localHitPoint.z));
    }
    
    // Transform back to world space
    glm::mat4 worldTransform = transform.getMatrix();
    hit.hit = true;
    hit.distance = t * glm::length(localDirection);
    hit.point = glm::vec3(worldTransform * glm::vec4(localHitPoint, 1.0f));
    hit.normal = glm::normalize(glm::vec3(worldTransform * glm::vec4(localNormal, 0.0f)));
    
    return true;
}

glm::vec3 CapsuleShape::support(const glm::vec3& direction, const Transform& transform) const {
    // Transform direction to local space
    glm::vec3 localDir = glm::inverse(transform.rotation) * direction;
    
    // Find support point along the capsule axis
    float halfHeight = height * 0.5f;
    glm::vec3 center = (localDir.y > 0) ? glm::vec3(0, halfHeight, 0) : glm::vec3(0, -halfHeight, 0);
    
    // Add radius in the direction
    glm::vec3 localSupport = center + glm::normalize(localDir) * radius;
    
    // Transform to world space
    return transform.position + transform.rotation * localSupport;
}

float CapsuleShape::calculateVolume() const {
    float cylinderVolume = glm::pi<float>() * radius * radius * height;
    float sphereVolume = (4.0f / 3.0f) * glm::pi<float>() * radius * radius * radius;
    return cylinderVolume + sphereVolume;
}

glm::mat3 CapsuleShape::calculateInertiaTensor(float mass) const {
    // Simplified inertia tensor calculation for capsule
    float totalHeight = height + 2.0f * radius;
    float cylinderMass = mass * (height / totalHeight);
    float sphereMass = mass * (2.0f * radius / totalHeight);
    
    // Cylinder inertia
    float Ixx_cyl = cylinderMass * (3.0f * radius * radius + height * height) / 12.0f;
    float Iyy_cyl = cylinderMass * radius * radius / 2.0f;
    
    // Sphere inertia (approximated)
    float I_sphere = (2.0f / 5.0f) * sphereMass * radius * radius;
    
    float Ixx = Ixx_cyl + I_sphere;
    float Iyy = Iyy_cyl + I_sphere;
    float Izz = Ixx;
    
    return glm::mat3(
        Ixx, 0, 0,
        0, Iyy, 0,
        0, 0, Izz
    );
}

glm::vec3 CapsuleShape::getTopCenter(const Transform& transform) const {
    glm::vec3 up = transform.rotation * glm::vec3(0, 1, 0);
    return transform.position + up * (height * 0.5f);
}

glm::vec3 CapsuleShape::getBottomCenter(const Transform& transform) const {
    glm::vec3 up = transform.rotation * glm::vec3(0, 1, 0);
    return transform.position - up * (height * 0.5f);
}

void CapsuleShape::getEndpoints(const Transform& transform, glm::vec3& top, glm::vec3& bottom) const {
    glm::vec3 up = transform.rotation * glm::vec3(0, 1, 0);
    float halfHeight = height * 0.5f;
    top = transform.position + up * halfHeight;
    bottom = transform.position - up * halfHeight;
}

bool CapsuleShape::raycastCylinder(const Ray& localRay, float& t) const {
    // Raycast against infinite cylinder, then check height bounds
    glm::vec2 rayOrigin2D(localRay.origin.x, localRay.origin.z);
    glm::vec2 rayDir2D(localRay.direction.x, localRay.direction.z);
    
    float a = glm::dot(rayDir2D, rayDir2D);
    float b = 2.0f * glm::dot(rayOrigin2D, rayDir2D);
    float c = glm::dot(rayOrigin2D, rayOrigin2D) - radius * radius;
    
    float discriminant = b * b - 4 * a * c;
    if (discriminant < 0) return false;
    
    float sqrtD = std::sqrt(discriminant);
    float t1 = (-b - sqrtD) / (2 * a);
    float t2 = (-b + sqrtD) / (2 * a);
    
    float halfHeight = height * 0.5f;
    
    // Check t1
    if (t1 >= 0 && t1 <= localRay.maxDistance) {
        float y = localRay.origin.y + t1 * localRay.direction.y;
        if (y >= -halfHeight && y <= halfHeight) {
            t = t1;
            return true;
        }
    }
    
    // Check t2
    if (t2 >= 0 && t2 <= localRay.maxDistance) {
        float y = localRay.origin.y + t2 * localRay.direction.y;
        if (y >= -halfHeight && y <= halfHeight) {
            t = t2;
            return true;
        }
    }
    
    return false;
}

bool CapsuleShape::raycastSphere(const Ray& localRay, const glm::vec3& center, float& t) const {
    glm::vec3 oc = localRay.origin - center;
    
    float a = glm::dot(localRay.direction, localRay.direction);
    float b = 2.0f * glm::dot(oc, localRay.direction);
    float c = glm::dot(oc, oc) - radius * radius;
    
    float discriminant = b * b - 4 * a * c;
    if (discriminant < 0) return false;
    
    float sqrtD = std::sqrt(discriminant);
    float t1 = (-b - sqrtD) / (2 * a);
    float t2 = (-b + sqrtD) / (2 * a);
    
    if (t1 >= 0 && t1 <= localRay.maxDistance) {
        t = t1;
        return true;
    }
    
    if (t2 >= 0 && t2 <= localRay.maxDistance) {
        t = t2;
        return true;
    }
    
    return false;
}

glm::vec3 CapsuleShape::closestPointOnLineSegment(const glm::vec3& point, 
                                                 const glm::vec3& a, const glm::vec3& b) const {
    glm::vec3 ab = b - a;
    float t = glm::dot(point - a, ab) / glm::dot(ab, ab);
    t = glm::clamp(t, 0.0f, 1.0f);
    return a + t * ab;
}

} // namespace engine::physics