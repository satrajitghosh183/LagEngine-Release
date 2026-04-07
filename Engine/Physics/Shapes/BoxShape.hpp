#pragma once

#include "CollisionShape.hpp"

namespace GameEngine {
namespace Physics {

    /**
     * @brief Oriented bounding box collision shape
     */
    class BoxShape : public CollisionShape {
    public:
        BoxShape(const glm::vec3& halfExtents = glm::vec3(0.5f));
        
        ShapeType GetType() const override { return ShapeType::Box; }
        
        AABB CalculateAABB(const glm::vec3& position, const glm::quat& rotation) const override;
        glm::mat3 CalculateInertiaTensor(float mass) const override;
        RaycastHit Raycast(const glm::vec3& origin, const glm::vec3& direction, 
                          const glm::vec3& position, const glm::quat& rotation, 
                          float maxDistance) const override;
        glm::vec3 GetSupportPoint(const glm::vec3& direction, 
                                 const glm::vec3& position, 
                                 const glm::quat& rotation) const override;
        
        glm::vec3 GetHalfExtents() const { return m_HalfExtents; }
        void SetHalfExtents(const glm::vec3& halfExtents) { m_HalfExtents = halfExtents; }
        
    private:
        glm::vec3 m_HalfExtents;
    };

}}