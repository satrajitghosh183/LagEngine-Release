#include "CharacterController.hpp"
#include "../../core/Logger.hpp"
#include <algorithm>
#include <cmath>

namespace engine::physics {

CharacterController::CharacterController(PhysicsWorld* physicsWorld, float radius, float height, float mass)
    : world(physicsWorld) {
    
    if (!world) {
        throw std::invalid_argument("PhysicsWorld cannot be null");
    }
    
    // Create capsule shape
    capsule = std::make_shared<CapsuleShape>(radius, height);
    
    // Create rigid body
    rigidBody = std::make_shared<RigidBody>(RigidBody::BodyType::Dynamic, mass);
    rigidBody->setCollisionShape(capsule);
    
    // Configure rigid body for character controller
    rigidBody->setLinearDamping(0.0f);  // We handle damping manually
    rigidBody->setAngularDamping(1.0f); // Prevent rotation
    
    // Add to physics world
    world->addRigidBody(rigidBody);
    
    // Initialize ground detection
    updateGroundDetection(0.016f);
}

CharacterController::~CharacterController() {
    if (world && rigidBody) {
        world->removeRigidBody(rigidBody);
    }
}

void CharacterController::move(const glm::vec3& displacement, float dt) {
    if (debugEnabled) {
        clearDebugInfo();
        debugInfo.intendedMovement = displacement;
    }
    
    // Update ground detection first
    updateGroundDetection(dt);
    
    // Handle movement based on current state
    handleMovement(displacement, dt);
    
    // Handle moving platforms
    handleMovingPlatforms(dt);
    
    // Apply gravity and air resistance
    applyGravityAndDrag(dt);
    
    // Update character state
    updateState();
    
    // Update timing
    if (state == CharacterState::Airborne) {
        timeInAir += dt;
    } else {
        timeInAir = 0.0f;
    }
    
    timeSinceJump += dt;
    
    if (debugEnabled) {
        debugInfo.actualMovement = rigidBody->getLinearVelocity() * dt;
    }
}

void CharacterController::jump(float force) {
    if (isGrounded() && timeSinceJump > 0.1f) { // Prevent rapid jumping
        glm::vec3 jumpVelocity = glm::vec3(0, force, 0);
        
        // Add current velocity to jump
        glm::vec3 currentVel = rigidBody->getLinearVelocity();
        currentVel.y = jumpVelocity.y;
        
        rigidBody->setLinearVelocity(currentVel);
        
        timeSinceJump = 0.0f;
        state = CharacterState::Airborne;
        
        if (onJumped) {
            onJumped();
        }
        
        engine::core::log::Logger::log("Character jumped with force: " + std::to_string(force), 
                                      engine::core::log::LogLevel::Info);
    }
}

void CharacterController::setVelocity(const glm::vec3& vel) {
    velocity = vel;
    rigidBody->setLinearVelocity(vel + externalVelocity);
}

void CharacterController::addVelocity(const glm::vec3& vel) {
    velocity += vel;
    rigidBody->setLinearVelocity(velocity + externalVelocity);
}

void CharacterController::setPosition(const glm::vec3& position) {
    rigidBody->setPosition(position);
    
    // Force ground detection update
    updateGroundDetection(0.016f);
}

glm::vec3 CharacterController::getPosition() const {
    return rigidBody->getPosition();
}

glm::vec3 CharacterController::getVelocity() const {
    return rigidBody->getLinearVelocity();
}

void CharacterController::updateGroundDetection(float dt) {
    previousGroundInfo = groundInfo;
    
    if (performGroundCheck(groundInfo)) {
        groundInfo.slopeAngle = calculateSlopeAngle(groundInfo.normal);
        groundInfo.isMovingPlatform = isMovingPlatform(groundInfo.groundBody);
        
        if (groundInfo.isMovingPlatform) {
            groundInfo.platformVelocity = getPlatformVelocity(groundInfo.groundBody);
        }
    } else {
        groundInfo = GroundInfo{}; // Reset to default
    }
}

bool CharacterController::performGroundCheck(GroundInfo& info) {
    glm::vec3 characterPos = rigidBody->getPosition();
    glm::vec3 rayStart = characterPos;
    glm::vec3 rayEnd = characterPos - glm::vec3(0, groundCheckDistance + skinWidth, 0);
    
    // Perform multiple raycasts for better detection
    std::vector<glm::vec3> rayOffsets = {
        glm::vec3(0, 0, 0),                    // Center
        glm::vec3(capsule->getRadius() * 0.7f, 0, 0),      // Right
        glm::vec3(-capsule->getRadius() * 0.7f, 0, 0),     // Left
        glm::vec3(0, 0, capsule->getRadius() * 0.7f),      // Forward
        glm::vec3(0, 0, -capsule->getRadius() * 0.7f)      // Back
    };
    
    RaycastHit bestHit;
    bool foundGround = false;
    float closestDistance = FLT_MAX;
    
    for (const glm::vec3& offset : rayOffsets) {
        glm::vec3 start = rayStart + offset;
        glm::vec3 end = rayEnd + offset;
        
        Ray groundRay(start, glm::vec3(0, -1, 0), glm::length(end - start));
        RaycastHit hit;
        
        if (world->raycast(groundRay, hit)) {
            if (hit.distance < closestDistance && hit.normal.y > 0.1f) {
                closestDistance = hit.distance;
                bestHit = hit;
                foundGround = true;
            }
        }
        
        if (debugEnabled) {
            addDebugRaycast(start, end, foundGround ? hit.normal : glm::vec3(0));
        }
    }
    
    if (foundGround) {
        info.isGrounded = true;
        info.normal = bestHit.normal;
        info.point = bestHit.point;
        info.distance = bestHit.distance;
        info.groundBody = bestHit.body;
        
        return true;
    }
    
    return false;
}

void CharacterController::handleMovement(const glm::vec3& displacement, float dt) {
   if (glm::length(displacement) < 0.001f) {
       return; // No movement requested
   }
   
   glm::vec3 movement = displacement;
   
   // Apply air control if airborne
   if (state == CharacterState::Airborne) {
       movement *= airControl;
   }
   
   // Handle slope movement
   if (state == CharacterState::OnSlope || state == CharacterState::Grounded) {
       handleSlopes(movement);
   }
   
   // Attempt movement with collision detection
   glm::vec3 currentPos = rigidBody->getPosition();
   glm::vec3 targetPos = currentPos + movement;
   
   // Perform sweep test
   RaycastHit sweepHit;
   if (performSweepTest(currentPos, targetPos, sweepHit)) {
       // Hit something, try to handle it
       if (canStepUp(sweepHit.point, sweepHit.normal, stepHeight)) {
           handleSteps(movement, dt);
       } else {
           // Slide along the surface
           movement = slideAlongSurface(movement, sweepHit.normal);
           
           // Check if this is a wall hit
           if (sweepHit.normal.y < 0.7f && onHitWall) {
               onHitWall(sweepHit.normal);
           }
       }
   }
   
   // Apply the final movement
   glm::vec3 currentVelocity = rigidBody->getLinearVelocity();
   glm::vec3 newVelocity = movement / dt;
   
   // Preserve Y velocity if not grounded (for gravity/jumping)
   if (state == CharacterState::Airborne) {
       newVelocity.y = currentVelocity.y;
   }
   
   rigidBody->setLinearVelocity(newVelocity + externalVelocity);
}

void CharacterController::handleSlopes(glm::vec3& movement) {
   if (!groundInfo.isGrounded) return;
   
   float slopeAngle = calculateSlopeAngle(groundInfo.normal);
   
   if (slopeAngle > maxSlopeAngle) {
       // Too steep, slide down
       glm::vec3 slideDirection = projectOntoPlane(glm::vec3(0, -1, 0), groundInfo.normal);
       movement = slideDirection * glm::length(movement) * 0.5f;
       return;
   }
   
   if (slopeAngle > 1.0f) { // On a slope
       movement = calculateSlopeMovement(movement, groundInfo.normal);
       state = CharacterState::OnSlope;
   }
}

void CharacterController::handleSteps(glm::vec3& movement, float dt) {
   if (debugEnabled) {
       debugInfo.stepClimbAttempts++;
   }
   
   glm::vec3 currentPos = rigidBody->getPosition();
   glm::vec3 horizontalMovement = glm::vec3(movement.x, 0, movement.z);
   
   // Step 1: Move up by step height
   glm::vec3 stepUpPos = currentPos + glm::vec3(0, stepHeight, 0);
   
   // Step 2: Move horizontally
   glm::vec3 stepForwardPos = stepUpPos + horizontalMovement;
   
   // Step 3: Check if we can move down to ground
   Ray stepDownRay(stepForwardPos, glm::vec3(0, -1, 0), stepHeight + groundCheckDistance);
   RaycastHit stepDownHit;
   
   if (world->raycast(stepDownRay, stepDownHit)) {
       // Found ground, perform step
       glm::vec3 finalPos = stepDownHit.point + glm::vec3(0, skinWidth, 0);
       
       // Verify the step is valid
       if (finalPos.y > currentPos.y - 0.01f && finalPos.y <= currentPos.y + stepHeight) {
           rigidBody->setPosition(finalPos);
           
           if (onSteppedOnObject && stepDownHit.body) {
               onSteppedOnObject(stepDownHit.body);
           }
           
           // Zero out the movement since we've already moved
           movement = glm::vec3(0);
       }
   }
}

void CharacterController::handleMovingPlatforms(float dt) {
   if (groundInfo.isMovingPlatform && groundInfo.groundBody) {
       glm::vec3 platformVel = getPlatformVelocity(groundInfo.groundBody);
       
       // If platform velocity changed significantly, update our external velocity
       if (glm::length(platformVel - groundInfo.platformVelocity) > 0.1f) {
           externalVelocity = platformVel;
           
           if (previousGroundInfo.groundBody != groundInfo.groundBody && onMovingPlatformEnter) {
               onMovingPlatformEnter(groundInfo.groundBody);
           }
       }
   } else {
       // Not on a moving platform anymore
       if (previousGroundInfo.isMovingPlatform && onMovingPlatformExit) {
           onMovingPlatformExit(previousGroundInfo.groundBody);
       }
       
       // Gradually reduce external velocity
       externalVelocity *= std::pow(0.1f, dt);
       
       if (glm::length(externalVelocity) < 0.01f) {
           externalVelocity = glm::vec3(0);
       }
   }
}

void CharacterController::applyGravityAndDrag(float dt) {
   glm::vec3 currentVel = rigidBody->getLinearVelocity();
   
   // Apply gravity if not grounded
   if (!isGrounded()) {
       glm::vec3 gravity = world->getGravity();
       currentVel += gravity * dt;
   } else {
       // Ground friction/damping
       currentVel.x *= std::pow(0.1f, dt); // Horizontal damping
       currentVel.z *= std::pow(0.1f, dt);
       
       // Prevent vertical movement when grounded (unless jumping)
       if (timeSinceJump > 0.1f) {
           currentVel.y = glm::max(0.0f, currentVel.y);
       }
   }
   
   rigidBody->setLinearVelocity(currentVel);
}

void CharacterController::updateState() {
   previousState = state;
   
   if (groundInfo.isGrounded) {
       float slopeAngle = calculateSlopeAngle(groundInfo.normal);
       
       if (groundInfo.isMovingPlatform) {
           state = CharacterState::OnMovingPlatform;
       } else if (slopeAngle > slopeLimit) {
           state = CharacterState::OnSlope;
       } else {
           state = CharacterState::Grounded;
       }
       
       // Landing event
       if (previousState == CharacterState::Airborne && onLanded) {
           onLanded();
       }
       
   } else {
       state = CharacterState::Airborne;
   }
   
   // State change events
   if (state != previousState) {
       switch (state) {
           case CharacterState::Grounded:
               enterGroundedState();
               break;
           case CharacterState::Airborne:
               enterAirborneState();
               break;
           case CharacterState::OnSlope:
               enterSlopeState();
               break;
           case CharacterState::OnMovingPlatform:
               enterMovingPlatformState();
               break;
       }
   }
}

bool CharacterController::performSweepTest(const glm::vec3& start, const glm::vec3& end, RaycastHit& hit, float radius) {
   if (radius < 0) {
       radius = capsule->getRadius();
   }
   
   glm::vec3 direction = end - start;
   float distance = glm::length(direction);
   
   if (distance < 0.001f) return false;
   
   direction /= distance;
   
   // Use multiple rays around the capsule perimeter for more accurate detection
   std::vector<glm::vec3> rayOffsets;
   const int numRays = 8;
   
   for (int i = 0; i < numRays; ++i) {
       float angle = (2.0f * glm::pi<float>() * i) / numRays;
       glm::vec3 offset = glm::vec3(
           std::cos(angle) * radius * 0.9f,
           0,
           std::sin(angle) * radius * 0.9f
       );
       rayOffsets.push_back(offset);
   }
   
   // Add center ray
   rayOffsets.push_back(glm::vec3(0));
   
   RaycastHit bestHit;
   bool foundHit = false;
   float closestDistance = FLT_MAX;
   
   for (const glm::vec3& offset : rayOffsets) {
       Ray sweepRay(start + offset, direction, distance + skinWidth);
       RaycastHit rayHit;
       
       if (world->raycast(sweepRay, rayHit)) {
           if (rayHit.distance < closestDistance) {
               closestDistance = rayHit.distance;
               bestHit = rayHit;
               foundHit = true;
           }
       }
       
       if (debugEnabled) {
           addDebugRaycast(start + offset, start + offset + direction * distance, 
                          foundHit ? rayHit.normal : glm::vec3(0));
       }
   }
   
   if (foundHit) {
       hit = bestHit;
       return true;
   }
   
   return false;
}

glm::vec3 CharacterController::slideAlongSurface(const glm::vec3& movement, const glm::vec3& normal) {
   // Project movement onto the surface plane
   return movement - glm::dot(movement, normal) * normal;
}

bool CharacterController::canStepUp(const glm::vec3& hitPoint, const glm::vec3& hitNormal, float maxStepHeight) {
   // Check if the surface is not too steep for stepping
   if (hitNormal.y < 0.7f) return false;
   
   glm::vec3 currentPos = rigidBody->getPosition();
   float heightDifference = hitPoint.y - currentPos.y;
   
   return heightDifference > 0.01f && heightDifference <= maxStepHeight;
}

glm::vec3 CharacterController::projectOntoPlane(const glm::vec3& vector, const glm::vec3& normal) {
   return vector - glm::dot(vector, normal) * normal;
}

bool CharacterController::isWalkableSlope(const glm::vec3& normal) const {
   return calculateSlopeAngle(normal) <= maxSlopeAngle;
}

float CharacterController::calculateSlopeAngle(const glm::vec3& normal) const {
   float dot = glm::clamp(glm::dot(normal, glm::vec3(0, 1, 0)), -1.0f, 1.0f);
   return glm::degrees(std::acos(dot));
}

glm::vec3 CharacterController::calculateSlopeMovement(const glm::vec3& input, const glm::vec3& slopeNormal) {
   // Project input movement onto the slope plane
   glm::vec3 slopeMovement = projectOntoPlane(input, slopeNormal);
   
   // Maintain the intended speed
   float inputSpeed = glm::length(input);
   if (glm::length(slopeMovement) > 0.001f) {
       slopeMovement = glm::normalize(slopeMovement) * inputSpeed;
   }
   
   return slopeMovement;
}

bool CharacterController::isMovingPlatform(RigidBody* body) const {
   if (!body) return false;
   
   // Consider a body a moving platform if it has non-zero velocity
   // and is kinematic or has significant mass
   glm::vec3 velocity = body->getLinearVelocity();
   
   return (glm::length(velocity) > 0.1f) && 
          (body->getBodyType() == RigidBody::BodyType::Kinematic || 
           body->getMass() > 100.0f);
}

glm::vec3 CharacterController::getPlatformVelocity(RigidBody* body) const {
   if (!body) return glm::vec3(0);
   return body->getLinearVelocity();
}

void CharacterController::enterGroundedState() {
   engine::core::log::Logger::log("Character entered grounded state", 
                                 engine::core::log::LogLevel::Info);
}

void CharacterController::enterAirborneState() {
   timeInAir = 0.0f;
   engine::core::log::Logger::log("Character entered airborne state", 
                                 engine::core::log::LogLevel::Info);
}

void CharacterController::enterSlopeState() {
   engine::core::log::Logger::log("Character entered slope state, angle: " + 
                                 std::to_string(groundInfo.slopeAngle), 
                                 engine::core::log::LogLevel::Info);
}

void CharacterController::enterMovingPlatformState() {
   engine::core::log::Logger::log("Character entered moving platform state", 
                                 engine::core::log::LogLevel::Info);
}

void CharacterController::clearDebugInfo() {
   debugInfo = DebugInfo{};
}

void CharacterController::addDebugRaycast(const glm::vec3& start, const glm::vec3& end, const glm::vec3& normal) {
   debugInfo.raycastPoints.push_back(start);
   debugInfo.raycastPoints.push_back(end);
   debugInfo.raycastNormals.push_back(normal);
}

} // namespace engine::physics