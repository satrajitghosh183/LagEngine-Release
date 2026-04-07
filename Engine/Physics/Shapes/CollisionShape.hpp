#pragma once

#include "../../Core/Base.hpp"
#include <glm/glm.hpp>

namespace GameEngine {
namespace Physics {

    /**
     * @brief Base collision shape interface
     */
    class CollisionShape {
    public:
        enum class ShapeType {
            Sphere,
            Box,
            Capsule,
            Plane,
            Mesh
        };
        
        virtual ~CollisionShape() = default;
        
        /**
         * @brief Get shape type
         */
        virtual ShapeType GetType() const = 0;
        
        /**
         * @brief Calculate axis-aligned bounding box
         */
        struct AABB {
            glm::vec3 Min;
            glm::vec3 Max;
        };
        
        virtual AABB CalculateAABB(const glm::vec3& position, const glm::quat& rotation) const = 0;
        
        /**
         * @brief Calculate moment of inertia tensor
         */
        virtual glm::mat3 CalculateInertiaTensor(float mass) const = 0;
        
        /**
         * @brief Raycast against shape
         */
        struct RaycastHit {
            bool Hit;
            float Distance;
            glm::vec3 Point;
            glm::vec3 Normal;
        };
        
        virtual RaycastHit Raycast(const glm::vec3& origin, const glm::vec3& direction, 
                                   const glm::vec3& position, const glm::quat& rotation, 
                                   float maxDistance) const = 0;
        
        /**
         * @brief Get support point for GJK algorithm
         */
        virtual glm::vec3 GetSupportPoint(const glm::vec3& direction, 
                                         const glm::vec3& position, 
                                         const glm::quat& rotation) const = 0;
    };

}}