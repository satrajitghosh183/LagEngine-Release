#pragma once

#include "CollisionShape.hpp"

namespace GameEngine {
namespace Physics {

    /**
     * @brief Infinite plane collision shape
     * Defined by normal and distance from origin
     */
    class PlaneShape : public CollisionShape {
    public:
        PlaneShape(const glm::vec3& normal = glm::vec3(0, 1, 0), float distance = 0.0f);
        
        ShapeType GetType() const override { return ShapeType::Plane; }
        
        AABB CalculateAABB(const glm::vec3& position, const glm::quat& rotation) const override;
        glm::mat3 CalculateInertiaTensor(float mass) const override;
        RaycastHit Raycast(const glm::vec3& origin, const glm::vec3& direction, 
                          const glm::vec3& position, const glm::quat& rotation, 
                          float maxDistance) const override;
        glm::vec3 GetSupportPoint(const glm::vec3& direction, 
                                 const glm::vec3& position, 
                                 const glm::quat& rotation) const override;
        
        glm::vec3 GetNormal() const { return m_Normal; }
        float GetDistance() const { return m_Distance; }
        
    private:
        glm::vec3 m_Normal;
        float m_Distance;
    };

}}