#pragma once

#include "CollisionShape.hpp"

namespace GameEngine {
namespace Physics {

    /**
     * @brief Capsule collision shape (cylinder with hemispherical caps)
     * Perfect for character controllers
     */
    class CapsuleShape : public CollisionShape {
    public:
        CapsuleShape(float radius = 0.5f, float height = 2.0f);
        
        ShapeType GetType() const override { return ShapeType::Capsule; }
        
        AABB CalculateAABB(const glm::vec3& position, const glm::quat& rotation) const override;
        glm::mat3 CalculateInertiaTensor(float mass) const override;
        RaycastHit Raycast(const glm::vec3& origin, const glm::vec3& direction, 
                          const glm::vec3& position, const glm::quat& rotation, 
                          float maxDistance) const override;
        glm::vec3 GetSupportPoint(const glm::vec3& direction, 
                                 const glm::vec3& position, 
                                 const glm::quat& rotation) const override;
        
        float GetRadius() const { return m_Radius; }
        float GetHeight() const { return m_Height; }
        float GetCylinderHeight() const { return m_Height - 2.0f * m_Radius; }
        
        void SetRadius(float radius) { m_Radius = radius; }
        void SetHeight(float height) { m_Height = height; }
        
    private:
        float m_Radius;
        float m_Height;  // Total height including caps
    };

}}