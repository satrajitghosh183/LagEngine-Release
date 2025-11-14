#include "PlaneShape.hpp"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace GameEngine {
namespace Physics {

    PlaneShape::PlaneShape(const glm::vec3& normal, float distance)
        : m_Normal(glm::normalize(normal)), m_Distance(distance) {
    }

    CollisionShape::AABB PlaneShape::CalculateAABB(const glm::vec3& position, const glm::quat& rotation) const {
        // Planes are infinite - return max AABB
        AABB aabb;
        const float large = 100000.0f;
        aabb.Min = glm::vec3(-large);
        aabb.Max = glm::vec3(large);
        return aabb;
    }

    glm::mat3 PlaneShape::CalculateInertiaTensor(float mass) const {
        // Infinite inertia
        return glm::mat3(0.0f);
    }

    CollisionShape::RaycastHit PlaneShape::Raycast(const glm::vec3& origin, const glm::vec3& direction, 
                                                   const glm::vec3& position, const glm::quat& rotation, 
                                                   float maxDistance) const {
        RaycastHit hit;
        hit.Hit = false;
        
        glm::mat3 rotMat = glm::mat3_cast(rotation);
        glm::vec3 worldNormal = rotMat * m_Normal;
        float denom = glm::dot(worldNormal, direction);
        
        if (abs(denom) > 0.0001f) {
            glm::vec3 planePoint = position + worldNormal * m_Distance;
            float t = glm::dot(planePoint - origin, worldNormal) / denom;
            
            if (t >= 0 && t <= maxDistance) {
                hit.Hit = true;
                hit.Distance = t;
                hit.Point = origin + direction * t;
                hit.Normal = worldNormal;
            }
        }
        
        return hit;
    }

    glm::vec3 PlaneShape::GetSupportPoint(const glm::vec3& direction, 
                                         const glm::vec3& position, 
                                         const glm::quat& rotation) const {
        // Planes are infinite - return point in direction
        glm::mat3 rotMat = glm::mat3_cast(rotation);
        glm::vec3 worldNormal = rotMat * m_Normal;
        return position + worldNormal * m_Distance;
    }

}}