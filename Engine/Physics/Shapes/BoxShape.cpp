#include "BoxShape.hpp"
#include <glm/gtx/quaternion.hpp>
#include <algorithm>

namespace GameEngine {
namespace Physics {

    BoxShape::BoxShape(const glm::vec3& halfExtents)
        : m_HalfExtents(halfExtents) {
    }

    CollisionShape::AABB BoxShape::CalculateAABB(const glm::vec3& position, const glm::quat& rotation) const {
        // Get all 8 corners of the box
        glm::vec3 corners[8] = {
            glm::vec3(-m_HalfExtents.x, -m_HalfExtents.y, -m_HalfExtents.z),
            glm::vec3( m_HalfExtents.x, -m_HalfExtents.y, -m_HalfExtents.z),
            glm::vec3(-m_HalfExtents.x,  m_HalfExtents.y, -m_HalfExtents.z),
            glm::vec3( m_HalfExtents.x,  m_HalfExtents.y, -m_HalfExtents.z),
            glm::vec3(-m_HalfExtents.x, -m_HalfExtents.y,  m_HalfExtents.z),
            glm::vec3( m_HalfExtents.x, -m_HalfExtents.y,  m_HalfExtents.z),
            glm::vec3(-m_HalfExtents.x,  m_HalfExtents.y,  m_HalfExtents.z),
            glm::vec3( m_HalfExtents.x,  m_HalfExtents.y,  m_HalfExtents.z)
        };
        
        // Transform corners and find min/max
        AABB aabb;
        aabb.Min = glm::vec3(std::numeric_limits<float>::max());
        aabb.Max = glm::vec3(std::numeric_limits<float>::lowest());
        
        for (int i = 0; i < 8; i++) {
            glm::vec3 worldCorner = position + rotation * corners[i];
            aabb.Min = glm::min(aabb.Min, worldCorner);
            aabb.Max = glm::max(aabb.Max, worldCorner);
        }
        
        return aabb;
    }

    glm::mat3 BoxShape::CalculateInertiaTensor(float mass) const {
        // Moment of inertia for box: I = (1/12) * m * (h^2 + d^2)
        float x2 = m_HalfExtents.x * m_HalfExtents.x * 4.0f;
        float y2 = m_HalfExtents.y * m_HalfExtents.y * 4.0f;
        float z2 = m_HalfExtents.z * m_HalfExtents.z * 4.0f;
        
        float factor = mass / 12.0f;
        
        return glm::mat3(
            factor * (y2 + z2), 0, 0,
            0, factor * (x2 + z2), 0,
            0, 0, factor * (x2 + y2)
        );
    }

    CollisionShape::RaycastHit BoxShape::Raycast(const glm::vec3& origin, const glm::vec3& direction, 
                                                 const glm::vec3& position, const glm::quat& rotation, 
                                                 float maxDistance) const {
        RaycastHit hit;
        hit.Hit = false;
        
        // Transform ray into box local space
        glm::quat invRotation = glm::inverse(rotation);
        glm::vec3 localOrigin = invRotation * (origin - position);
        glm::vec3 localDirection = invRotation * direction;
        
        // AABB ray intersection in local space
        glm::vec3 invDir = 1.0f / localDirection;
        glm::vec3 t0 = (-m_HalfExtents - localOrigin) * invDir;
        glm::vec3 t1 = ( m_HalfExtents - localOrigin) * invDir;
        
        glm::vec3 tmin = glm::min(t0, t1);
        glm::vec3 tmax = glm::max(t0, t1);
        
        float tEnter = glm::max(glm::max(tmin.x, tmin.y), tmin.z);
        float tExit = glm::min(glm::min(tmax.x, tmax.y), tmax.z);
        
        if (tEnter > tExit || tExit < 0 || tEnter > maxDistance) {
            return hit;
        }
        
        float t = tEnter < 0 ? tExit : tEnter;
        
        hit.Hit = true;
        hit.Distance = t;
        hit.Point = origin + direction * t;
        
        // Calculate normal in world space
        glm::vec3 localHitPoint = localOrigin + localDirection * t;
        glm::vec3 localNormal(0.0f);
        
        float epsilon = 0.0001f;
        if (abs(localHitPoint.x - m_HalfExtents.x) < epsilon) localNormal = glm::vec3(1, 0, 0);
        else if (abs(localHitPoint.x + m_HalfExtents.x) < epsilon) localNormal = glm::vec3(-1, 0, 0);
        else if (abs(localHitPoint.y - m_HalfExtents.y) < epsilon) localNormal = glm::vec3(0, 1, 0);
        else if (abs(localHitPoint.y + m_HalfExtents.y) < epsilon) localNormal = glm::vec3(0, -1, 0);
        else if (abs(localHitPoint.z - m_HalfExtents.z) < epsilon) localNormal = glm::vec3(0, 0, 1);
        else if (abs(localHitPoint.z + m_HalfExtents.z) < epsilon) localNormal = glm::vec3(0, 0, -1);
        
        hit.Normal = rotation * localNormal;
        
        return hit;
    }

    glm::vec3 BoxShape::GetSupportPoint(const glm::vec3& direction, 
                                       const glm::vec3& position, 
                                       const glm::quat& rotation) const {
        // Transform direction to local space
        glm::vec3 localDir = glm::inverse(rotation) * direction;
        
        // Get support point in local space
        glm::vec3 localSupport(
            localDir.x > 0 ? m_HalfExtents.x : -m_HalfExtents.x,
            localDir.y > 0 ? m_HalfExtents.y : -m_HalfExtents.y,
            localDir.z > 0 ? m_HalfExtents.z : -m_HalfExtents.z
        );
        
        // Transform back to world space
        return position + rotation * localSupport;
    }

}}