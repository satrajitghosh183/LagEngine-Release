#pragma once

#include "../../Core/Base.hpp"
#include "../RigidBody.hpp"
#include "../Shapes/CapsuleShape.hpp"
#include <glm/glm.hpp>

namespace GameEngine {
namespace Physics {

    class PhysicsWorld;

    /**
     * @brief Character controller for player movement
     * 
     * Features:
     * - Capsule-based collision
     * - Ground detection
     * - Slope handling
     * - Step climbing
     * - Jump buffering
     * - Coyote time (grace period after leaving ground)
     * - Smooth movement
     */
    class CharacterController {
    public:
        CharacterController(float radius = 0.3f, float height = 1.8f);
        ~CharacterController() = default;
        
        /**
         * @brief Update controller
         */
        void Update(float deltaTime);
        
        /**
         * @brief Move character
         */
        void Move(const glm::vec3& velocity);
        
        /**
         * @brief Jump
         */
        void Jump(float jumpForce = 5.0f);
        
        /**
         * @brief Check if grounded
         */
        bool IsGrounded() const { return m_IsGrounded; }
        
        /**
         * @brief Get/Set position
         */
        glm::vec3 GetPosition() const { return m_Position; }
        void SetPosition(const glm::vec3& position);
        
        /**
         * @brief Get velocity
         */
        glm::vec3 GetVelocity() const { return m_Velocity; }
        
        /**
         * @brief Controller properties
         */
        float SlopeLimit = 45.0f;           // Max slope angle in degrees
        float StepHeight = 0.3f;            // Max step height
        float SkinWidth = 0.08f;            // Collision skin
        float GroundCheckDistance = 0.1f;   // Ground detection distance
        float CoyoteTime = 0.1f;            // Grace period after leaving ground
        float JumpBufferTime = 0.1f;        // Jump input buffer
        
        /**
         * @brief Get capsule shape
         */
        Ref<CapsuleShape> GetCapsuleShape() const { return m_CapsuleShape; }

        /**
         * @brief Set physics world for ground detection raycast (optional)
         */
        void SetPhysicsWorld(PhysicsWorld* world) { m_PhysicsWorld = world; }
        PhysicsWorld* GetPhysicsWorld() const { return m_PhysicsWorld; }
        
    private:
        void DetectGround();
        void HandleSlopes();
        void ClimbSteps();
        void ApplyGravity(float deltaTime);
        
    private:
        Ref<CapsuleShape> m_CapsuleShape;
        glm::vec3 m_Position;
        glm::vec3 m_Velocity;
        
        bool m_IsGrounded;
        float m_CoyoteTimeCounter;
        float m_JumpBufferCounter;
        
        glm::vec3 m_GroundNormal;
        bool m_OnSlope;

        PhysicsWorld* m_PhysicsWorld = nullptr;
    };

}}