#pragma once
#include "../RigidBody.hpp"
#include "../shapes/CapsuleShape.hpp"
#include "../PhysicsWorld.hpp"
#include "../collision/ContactManifold.hpp"
#include <functional>
#include <vector>

namespace engine::physics {

enum class CharacterState {
    Grounded,
    Airborne,
    OnSlope,
    OnSteps,
    OnMovingPlatform
};

/**
 * @brief Advanced character controller with slope handling, step climbing, and moving platform support
 */
class CharacterController {
public:
    CharacterController(PhysicsWorld* world, float radius, float height, float mass = 70.0f);
    ~CharacterController();
    
    // Movement
    void move(const glm::vec3& displacement, float dt);
    void jump(float force);
    void setVelocity(const glm::vec3& velocity);
    void addVelocity(const glm::vec3& velocity);
    
    // State queries
    bool isGrounded() const { return state == CharacterState::Grounded || state == CharacterState::OnSlope; }
    bool isOnSlope() const { return state == CharacterState::OnSlope; }
    bool isOnMovingPlatform() const { return state == CharacterState::OnMovingPlatform; }
    CharacterState getState() const { return state; }
    
    // Properties
    void setPosition(const glm::vec3& position);
    glm::vec3 getPosition() const;
    glm::vec3 getVelocity() const;
    glm::vec3 getGroundNormal() const { return groundInfo.normal; }
    float getGroundDistance() const { return groundInfo.distance; }
    
    // Configuration
    void setStepHeight(float height) { stepHeight = glm::max(0.0f, height); }
    float getStepHeight() const { return stepHeight; }
    
    void setSlopeLimit(float angleInDegrees) { slopeLimit = angleInDegrees; }
    float getSlopeLimit() const { return slopeLimit; }
    
    void setGroundCheckDistance(float distance) { groundCheckDistance = glm::max(0.01f, distance); }
    float getGroundCheckDistance() const { return groundCheckDistance; }
    
    void setSkinWidth(float width) { skinWidth = glm::max(0.001f, width); }
    float getSkinWidth() const { return skinWidth; }
    
    void setMaxSlopeAngle(float angleInDegrees) { maxSlopeAngle = angleInDegrees; }
    float getMaxSlopeAngle() const { return maxSlopeAngle; }
    
    // Advanced features
    void setPushForce(float force) { pushForce = force; }
    float getPushForce() const { return pushForce; }
    
    void setAirControl(float control) { airControl = glm::clamp(control, 0.0f, 1.0f); }
    float getAirControl() const { return airControl; }
    
    // Callbacks
    std::function<void()> onLanded;
    std::function<void()> onJumped;
    std::function<void(const glm::vec3&)> onHitWall;
    std::function<void(RigidBody*)> onSteppedOnObject;
    std::function<void(RigidBody*)> onMovingPlatformEnter;
    std::function<void(RigidBody*)> onMovingPlatformExit;
    
    // Debug information
    struct DebugInfo {
        std::vector<glm::vec3> raycastPoints;
        std::vector<glm::vec3> raycastNormals;
        std::vector<RaycastHit> groundHits;
        glm::vec3 intendedMovement;
        glm::vec3 actualMovement;
        int stepClimbAttempts;
        bool wasSliding;
    };
    
    const DebugInfo& getDebugInfo() const { return debugInfo; }
    void enableDebugInfo(bool enable) { debugEnabled = enable; }

private:
    PhysicsWorld* world;
    std::shared_ptr<RigidBody> rigidBody;
    std::shared_ptr<CapsuleShape> capsule;
    
    CharacterState state = CharacterState::Airborne;
    CharacterState previousState = CharacterState::Airborne;
    
    // Configuration
    float stepHeight = 0.3f;
    float slopeLimit = 45.0f;  // degrees
    float maxSlopeAngle = 60.0f;  // Maximum walkable slope
    float groundCheckDistance = 0.1f;
    float skinWidth = 0.08f;
    float pushForce = 10.0f;
    float airControl = 0.2f;  // How much control in air
    
    // Ground detection
    struct GroundInfo {
        bool isGrounded = false;
        glm::vec3 normal{0, 1, 0};
        glm::vec3 point{0};
        float distance = 0.0f;
        float slopeAngle = 0.0f;
        RigidBody* groundBody = nullptr;
        bool isMovingPlatform = false;
        glm::vec3 platformVelocity{0};
    };
    
    GroundInfo groundInfo;
    GroundInfo previousGroundInfo;
    
    // Movement state
    glm::vec3 velocity{0};
    glm::vec3 externalVelocity{0};  // From moving platforms, etc.
    bool wasGroundedLastFrame = false;
    float timeInAir = 0.0f;
    float timeSinceJump = 0.0f;
    
    // Debug
    DebugInfo debugInfo;
    bool debugEnabled = false;
    
    // Internal methods
    void updateGroundDetection(float dt);
    void updateState();
    void handleMovement(const glm::vec3& displacement, float dt);
    void handleSlopes(glm::vec3& movement);
    void handleSteps(glm::vec3& movement, float dt);
    void handleMovingPlatforms(float dt);
    void applyGravityAndDrag(float dt);
    
    // Collision detection
    bool performSweepTest(const glm::vec3& start, const glm::vec3& end, RaycastHit& hit, float radius = -1.0f);
    bool performGroundCheck(GroundInfo& info);
    std::vector<RaycastHit> performMultipleGroundChecks();
    
    // Movement helpers
    void clipMovement(glm::vec3& movement, const glm::vec3& normal, float overbounce = 1.001f);
    glm::vec3 slideAlongSurface(const glm::vec3& movement, const glm::vec3& normal);
    bool canStepUp(const glm::vec3& hitPoint, const glm::vec3& hitNormal, float maxStepHeight);
    glm::vec3 projectOntoPlane(const glm::vec3& vector, const glm::vec3& normal);
    
    // Slope calculations
    bool isWalkableSlope(const glm::vec3& normal) const;
    float calculateSlopeAngle(const glm::vec3& normal) const;
    glm::vec3 calculateSlopeMovement(const glm::vec3& input, const glm::vec3& slopeNormal);
    
    // Platform detection
    bool isMovingPlatform(RigidBody* body) const;
    glm::vec3 getPlatformVelocity(RigidBody* body) const;
    
    // State transitions
    void enterGroundedState();
    void enterAirborneState();
    void enterSlopeState();
    void enterMovingPlatformState();
    
    // Debug helpers
    void clearDebugInfo();
    void addDebugRaycast(const glm::vec3& start, const glm::vec3& end, const glm::vec3& normal = glm::vec3(0));
};

} // namespace engine::physics