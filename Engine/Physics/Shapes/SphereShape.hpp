#pragma once

#include "CollisionShape.hpp"

namespace GameEngine {
namespace Physics {

    /**
     * @brief Sphere collision shape
     */
    class SphereShape : public CollisionShape {
    public:
        SphereShape(float radius = 0.5f);
        
        ShapeType GetType() const override { return ShapeType::Sphere; }
        
        AABB CalculateAABB(const glm::vec3& position, const glm::quat& rotation) const override;
        glm::mat3 CalculateInertiaTensor(float mass) const override;
        RaycastHit Raycast(const glm::vec3& origin, const glm::vec3& direction, 
                          const glm::vec3& position, const glm::quat& rotation, 
                          float maxDistance) const override;
        glm::vec3 GetSupportPoint(const glm::vec3& direction, 
                                 const glm::vec3& position, 
                                 const glm::quat& rotation) const override;
        
        float GetRadius() const { return m_Radius; }
        void SetRadius(float radius) { m_Radius = radius; }
        
    private:
        float m_Radius;
    };

}}