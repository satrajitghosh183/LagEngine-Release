#pragma once

#include "Component.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace GameEngine {

    /**
     * @brief Transform component - position, rotation, scale
     * 
     * Features:
     * - Local and world space transforms
     * - Hierarchical transformations
     * - Dirty flag for optimization
     * - Euler angles and quaternion rotation
     * - Helper functions (translate, rotate, scale)
     */
    class TransformComponent : public Component {
    public:
        TransformComponent();
        TransformComponent(const glm::vec3& position);
        
        COMPONENT_TYPE(TransformComponent)
        
        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& data) override;
        
        /**
         * @brief Get local transform matrix
         */
        glm::mat4 GetLocalTransform() const;
        
        /**
         * @brief Get world transform matrix (includes parent transforms)
         */
        glm::mat4 GetWorldTransform() const;
        
        /**
         * @brief Get world position
         */
        glm::vec3 GetWorldPosition() const;
        
        /**
         * @brief Get world rotation
         */
        glm::quat GetWorldRotation() const;
        
        /**
         * @brief Get world scale
         */
        glm::vec3 GetWorldScale() const;
        
        /**
         * @brief Set world position
         */
        void SetWorldPosition(const glm::vec3& position);
        
        /**
         * @brief Transform directions
         */
        glm::vec3 GetForward() const;
        glm::vec3 GetRight() const;
        glm::vec3 GetUp() const;
        
        /**
         * @brief Helper functions
         */
        void Translate(const glm::vec3& delta);
        void Rotate(const glm::vec3& axis, float angle);
        void RotateEuler(const glm::vec3& eulerAngles);
        void SetScale(const glm::vec3& scale);
        void LookAt(const glm::vec3& target, const glm::vec3& up = glm::vec3(0, 1, 0));
        
        /**
         * @brief Euler angle helpers
         */
        glm::vec3 GetEulerAngles() const;
        void SetEulerAngles(const glm::vec3& eulerAngles);
        
        /**
         * @brief Mark transform as dirty (needs recalculation)
         */
        void SetDirty();
        
    public:
        // Local space transform
        glm::vec3 Position = glm::vec3(0.0f);
        glm::quat Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Identity quaternion
        glm::vec3 Scale = glm::vec3(1.0f);
        
    private:
        mutable bool m_Dirty = true;
        mutable glm::mat4 m_CachedTransform = glm::mat4(1.0f);
    };
}