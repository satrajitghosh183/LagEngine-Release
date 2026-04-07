#include "SphereShape.hpp"
#include <glm/gtc/quaternion.hpp>

namespace GameEngine {
namespace Physics {

    SphereShape::SphereShape(float radius)
        : m_Radius(radius) {
    }

    CollisionShape::AABB SphereShape::CalculateAABB(const glm::vec3& position, const glm::quat& rotation) const {
        AABB aabb;
        aabb.Min = position - glm::vec3(m_Radius);
        aabb.Max = position + glm::vec3(m_Radius);
        return aabb;
    }

    glm::mat3 SphereShape::CalculateInertiaTensor(float mass) const {
        // Moment of inertia for solid sphere: I = (2/5) * m * r^2
        float inertia = (2.0f / 5.0f) * mass * m_Radius * m_Radius;
        return glm::mat3(
            inertia, 0, 0,
            0, inertia, 0,
            0, 0, inertia
        );
    }

    CollisionShape::RaycastHit SphereShape::Raycast(const glm::vec3& origin, const glm::vec3& direction, 
                                                    const glm::vec3& position, const glm::quat& rotation, 
                                                    float maxDistance) const {
        RaycastHit hit;
        hit.Hit = false;
        
        glm::vec3 oc = origin - position;
        float a = glm::dot(direction, direction);
        float b = 2.0f * glm::dot(oc, direction);
        float c = glm::dot(oc, oc) - m_Radius * m_Radius;
        float discriminant = b * b - 4 * a * c;
        
        if (discriminant < 0) {
            return hit;
        }
        
        float t = (-b - sqrt(discriminant)) / (2.0f * a);
        
        if (t < 0 || t > maxDistance) {
            return hit;
        }
        
        hit.Hit = true;
        hit.Distance = t;
        hit.Point = origin + direction * t;
        hit.Normal = glm::normalize(hit.Point - position);
        
        return hit;
    }

    glm::vec3 SphereShape::GetSupportPoint(const glm::vec3& direction, 
                                          const glm::vec3& position, 
                                          const glm::quat& rotation) const {
        return position + glm::normalize(direction) * m_Radius;
    }

}}