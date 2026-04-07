#include "CharacterController.hpp"
#include "../PhysicsWorld.hpp"
#include "../Shapes/PlaneShape.hpp"
#include "../../Core/Logger.hpp"

namespace GameEngine {
namespace Physics {

    CharacterController::CharacterController(float radius, float height)
        : m_Position(0.0f)
        , m_Velocity(0.0f)
        , m_IsGrounded(false)
        , m_CoyoteTimeCounter(0.0f)
        , m_JumpBufferCounter(0.0f)
        , m_GroundNormal(0, 1, 0)
        , m_OnSlope(false) {
        
        m_CapsuleShape = CreateRef<CapsuleShape>(radius, height);
    }

    void CharacterController::Update(float deltaTime) {
        // Detect ground
        DetectGround();
        
        // Update coyote time
        if (m_IsGrounded) {
            m_CoyoteTimeCounter = CoyoteTime;
        } else {
            m_CoyoteTimeCounter -= deltaTime;
        }
        
        // Update jump buffer
        if (m_JumpBufferCounter > 0) {
            m_JumpBufferCounter -= deltaTime;
        }
        
        // Apply gravity
        if (!m_IsGrounded) {
            ApplyGravity(deltaTime);
        }
        
        // Handle slopes
        if (m_OnSlope) {
            HandleSlopes();
        }
        
        // Update position
        m_Position += m_Velocity * deltaTime;
        
        // Clamp velocity
        if (m_IsGrounded && m_Velocity.y < 0) {
            m_Velocity.y = 0;
        }
    }

    void CharacterController::Move(const glm::vec3& velocity) {
        m_Velocity.x = velocity.x;
        m_Velocity.z = velocity.z;
        
        // If on slope, align movement to slope
        if (m_OnSlope && m_IsGrounded) {
            m_Velocity = glm::normalize(m_Velocity - m_GroundNormal * glm::dot(m_Velocity, m_GroundNormal)) 
                        * glm::length(velocity);
        }
    }

    void CharacterController::Jump(float jumpForce) {
        // Check if can jump (grounded or within coyote time)
        if (m_IsGrounded || m_CoyoteTimeCounter > 0) {
            m_Velocity.y = jumpForce;
            m_IsGrounded = false;
            m_CoyoteTimeCounter = 0;
            m_JumpBufferCounter = 0;
        } else {
            // Buffer jump input
            m_JumpBufferCounter = JumpBufferTime;
        }
    }

    void CharacterController::SetPosition(const glm::vec3& position) {
        m_Position = position;
    }

    void CharacterController::DetectGround() {
        float halfHeight = m_CapsuleShape->GetHeight() * 0.5f - m_CapsuleShape->GetRadius();
        glm::vec3 rayOrigin = m_Position - glm::vec3(0, halfHeight, 0);
        glm::vec3 rayDir(0, -1, 0);
        float maxDist = GroundCheckDistance + SkinWidth + 0.01f;

        if (m_PhysicsWorld) {
            CollisionShape::RaycastHit hit;
            if (m_PhysicsWorld->Raycast(rayOrigin, rayDir, maxDist, hit)) {
                float distanceToGround = hit.Distance;
                m_IsGrounded = distanceToGround <= (GroundCheckDistance + SkinWidth);
                if (m_IsGrounded) {
                    m_GroundNormal = hit.Normal;
                    float slopeAngle = glm::degrees(acos(glm::clamp(glm::dot(m_GroundNormal, glm::vec3(0, 1, 0)), -1.0f, 1.0f)));
                    m_OnSlope = slopeAngle > 0.1f && slopeAngle < SlopeLimit;
                } else {
                    m_OnSlope = false;
                }
            } else {
                m_IsGrounded = false;
                m_OnSlope = false;
            }
        } else {
            // Fallback: assume ground at y=0 when no physics world set
            float groundY = 0.0f;
            float distanceToGround = m_Position.y - m_CapsuleShape->GetRadius() - groundY;
            m_IsGrounded = distanceToGround <= GroundCheckDistance;
            if (m_IsGrounded) {
                m_GroundNormal = glm::vec3(0, 1, 0);
                m_OnSlope = false;
            } else {
                m_OnSlope = false;
            }
        }

        if (m_IsGrounded && m_JumpBufferCounter > 0) {
            Jump();
        }
    }

    void CharacterController::HandleSlopes() {
        // Project velocity onto slope
        glm::vec3 right = glm::cross(m_GroundNormal, glm::vec3(0, 0, 1));
        glm::vec3 forward = glm::cross(right, m_GroundNormal);
        
        // Adjust velocity to stay on slope
        float dot = glm::dot(m_Velocity, m_GroundNormal);
        if (dot < 0) {
            m_Velocity -= m_GroundNormal * dot;
        }
    }

    void CharacterController::ClimbSteps() {
        // TODO: Implement step climbing logic
        // Check for steps ahead and move up if within StepHeight
    }

    void CharacterController::ApplyGravity(float deltaTime) {
        const float gravity = -9.81f;
        m_Velocity.y += gravity * deltaTime;
        
        // Terminal velocity
        const float terminalVelocity = -50.0f;
        if (m_Velocity.y < terminalVelocity) {
            m_Velocity.y = terminalVelocity;
        }
    }

}}