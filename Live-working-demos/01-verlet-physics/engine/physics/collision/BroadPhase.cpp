#include "BroadPhase.hpp"
#include "../RigidBody.hpp"

namespace engine::physics {

bool BroadPhase::shouldTestPair(RigidBody* bodyA, RigidBody* bodyB) const {
    if (!bodyA || !bodyB || bodyA == bodyB) {
        return false;
    }
    
    // Don't test two static bodies
    if (bodyA->getBodyType() == RigidBody::BodyType::Static && 
        bodyB->getBodyType() == RigidBody::BodyType::Static) {
        return false;
    }
    
    // Don't test two sleeping bodies
    if (bodyA->isSleeping() && bodyB->isSleeping()) {
        return false;
    }
    
    // Both bodies must have collision shapes
    if (!bodyA->getCollisionShape() || !bodyB->getCollisionShape()) {
        return false;
    }
    
    return true;
}

} // namespace engine::physics