#include "CapsuleShape.hpp"
#include <glm/gtx/quaternion.hpp>

namespace GameEngine {
namespace Physics {

    CapsuleShape::CapsuleShape(float radius, float height)
        : m_Radius(radius), m_Height(height) {
    }

    CollisionShape::AABB CapsuleShape::CalculateAABB(const glm::vec3& position, const glm::quat& rotation) const {
        glm::vec3 up = rotation * glm::vec3(0, 1, 0);
        float halfCylinder = GetCylinderHeight() * 0.5f;
        
        glm::vec3 topCenter = position + up * halfCylinder;
        glm::vec3 bottomCenter = position - up * halfCylinder;
        
        AABB aabb;
        aabb.Min = glm::min(topCenter, bottomCenter) - glm::vec3(m_Radius);
        aabb.Max = glm::max(topCenter, bottomCenter) + glm::vec3(m_Radius);
        
        return aabb;
    }

    glm::mat3 CapsuleShape::CalculateInertiaTensor(float mass) const {
        float cylinderHeight = GetCylinderHeight();
        float r2 = m_Radius * m_Radius;
        float h2 = cylinderHeight * cylinderHeight;
        
        // Approximate as cylinder
        float Ix = (1.0f / 12.0f) * mass * (3.0f * r2 + h2);
        float Iy = 0.5f * mass * r2;
        
        return glm::mat3(
            Ix, 0, 0,
            0, Iy, 0,
            0, 0, Ix
        );
    }

    CollisionShape::RaycastHit CapsuleShape::Raycast(const glm::vec3& origin, const glm::vec3& direction, 
                                                     const glm::vec3& position, const glm::quat& rotation, 
                                                     float maxDistance) const {
        RaycastHit hit;
        hit.Hit = false;
        
        glm::vec3 up = rotation * glm::vec3(0, 1, 0);
        float halfCylinder = GetCylinderHeight() * 0.5f;
        
        glm::vec3 topCenter = position + up * halfCylinder;
        glm::vec3 bottomCenter = position - up * halfCylinder;
        
        // Test against cylinder
        // Test against top sphere
        // Test against bottom sphere
        // Return closest hit
        
        // Simplified: just test against spheres
        glm::vec3 oc = origin - topCenter;
        float a = glm::dot(direction, direction);
        float b = 2.0f * glm::dot(oc, direction);
        float c = glm::dot(oc, oc) - m_Radius * m_Radius;
        float discriminant = b * b - 4 * a * c;
        
        if (discriminant >= 0) {
            float t = (-b - sqrt(discriminant)) / (2.0f * a);
            if (t >= 0 && t <= maxDistance) {
                hit.Hit = true;
                hit.Distance = t;
                hit.Point = origin + direction * t;
                hit.Normal = glm::normalize(hit.Point - topCenter);
                return hit;
            }
        }
        
        // Test bottom sphere
        oc = origin - bottomCenter;
        c = glm::dot(oc, oc) - m_Radius * m_Radius;
        discriminant = b * b - 4 * a * c;
        
        if (discriminant >= 0) {
            float t = (-b - sqrt(discriminant)) / (2.0f * a);
            if (t >= 0 && t <= maxDistance) {
                hit.Hit = true;
                hit.Distance = t;
                hit.Point = origin + direction * t;
                hit.Normal = glm::normalize(hit.Point - bottomCenter);
            }
        }
        
        return hit;
    }

    glm::vec3 CapsuleShape::GetSupportPoint(const glm::vec3& direction, 
                                           const glm::vec3& position, 
                                           const glm::quat& rotation) const {
        glm::vec3 up = rotation * glm::vec3(0, 1, 0);
        float halfCylinder = GetCylinderHeight() * 0.5f;
        
        float dotUp = glm::dot(direction, up);
        glm::vec3 center = position + up * halfCylinder * (dotUp > 0 ? 1.0f : -1.0f);
        
        return center + glm::normalize(direction) * m_Radius;
    }

}}