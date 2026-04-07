// #include "PhysicsWorld.hpp"
// #include "engine/core/Logger.hpp"
// #include <algorithm>
// #include <unordered_set>

// namespace engine::physics {

// PhysicsWorld::PhysicsWorld() {
//     rigidBodies.reserve(1000);
//     contactPoints.reserve(1000);
//     activePairs.reserve(500);
//     newPairs.reserve(500);
// }

// PhysicsWorld::~PhysicsWorld() {
//     clear();
// }

// void PhysicsWorld::update(float dt) {
//     // Fixed timestep with accumulator
//     accumulator += dt;
    
//     int steps = 0;
//     while (accumulator >= fixedTimeStep && steps < maxSubSteps) {
//         // Apply gravity and other forces
//         integrateForces(fixedTimeStep);
        
//         // Collision detection
//         broadPhaseCollision();
//         narrowPhaseCollision();
        
//         // Collision response
//         resolveCollisions();
        
//         // Integrate velocities to positions
//         integrateBodies(fixedTimeStep);
        
//         // Update collision events
//         updateCollisionEvents();
        
//         accumulator -= fixedTimeStep;
//         steps++;
//     }
    
//     // Clear contact points for next frame
//     contactPoints.clear();
// }

// void PhysicsWorld::clear() {
//     rigidBodies.clear();
//     contactPoints.clear();
//     activePairs.clear();
//     newPairs.clear();
//     accumulator = 0.0f;
// }

// void PhysicsWorld::addRigidBody(std::shared_ptr<RigidBody> body) {
//     if (body && std::find(rigidBodies.begin(), rigidBodies.end(), body) == rigidBodies.end()) {
//         rigidBodies.push_back(body);
//     }
// }

// void PhysicsWorld::removeRigidBody(std::shared_ptr<RigidBody> body) {
//     auto it = std::find(rigidBodies.begin(), rigidBodies.end(), body);
//     if (it != rigidBodies.end()) {
//         rigidBodies.erase(it);
        
//         // Remove any collision pairs involving this body
//         activePairs.erase(
//             std::remove_if(activePairs.begin(), activePairs.end(),
//                 [body](const CollisionPair& pair) {
//                     return pair.bodyA == body.get() || pair.bodyB == body.get();
//                 }),
//             activePairs.end()
//         );
//     }
// }

// bool PhysicsWorld::raycast(const Ray& ray, RaycastHit& hit) const {
//     hit.hit = false;
//     float closestDistance = FLT_MAX;
    
//     for (const auto& body : rigidBodies) {
//         if (!body->getCollisionShape()) continue;
        
//         Transform transform = getRigidBodyTransform(body.get());
//         RaycastHit tempHit;
        
//         if (body->getCollisionShape()->raycast(ray, transform, tempHit)) {
//             if (tempHit.distance < closestDistance) {
//                 closestDistance = tempHit.distance;
//                 hit = tempHit;
//                 hit.body = body.get();
//             }
//         }
//     }
    
//     return hit.hit;
// }

// std::vector<RaycastHit> PhysicsWorld::raycastAll(const Ray& ray) const {
//     std::vector<RaycastHit> hits;
    
//     for (const auto& body : rigidBodies) {
//         if (!body->getCollisionShape()) continue;
        
//         Transform transform = getRigidBodyTransform(body.get());
//         RaycastHit hit;
        
//         if (body->getCollisionShape()->raycast(ray, transform, hit)) {
//             hit.body = body.get();
//             hits.push_back(hit);
//         }
//     }
    
//     // Sort by distance
//     std::sort(hits.begin(), hits.end(),
//         [](const RaycastHit& a, const RaycastHit& b) {
//             return a.distance < b.distance;
//         });
    
//     return hits;
// }

// std::vector<RigidBody*> PhysicsWorld::getOverlappingBodies(const BoundingBox& aabb) const {
//     std::vector<RigidBody*> overlapping;
    
//     for (const auto& body : rigidBodies) {
//         if (!body->getCollisionShape()) continue;
        
//         Transform transform = getRigidBodyTransform(body.get());
//         BoundingBox bodyAABB = body->getCollisionShape()->getAABB(transform);
        
//         if (aabb.intersects(bodyAABB)) {
//             overlapping.push_back(body.get());
//         }
//     }
    
//     return overlapping;
// }

// bool PhysicsWorld::checkOverlap(RigidBody* bodyA, RigidBody* bodyB) const {
//     if (!bodyA->getCollisionShape() || !bodyB->getCollisionShape()) {
//         return false;
//     }
    
//     Transform transformA = getRigidBodyTransform(bodyA);
//     Transform transformB = getRigidBodyTransform(bodyB);
    
//     BoundingBox aabbA = bodyA->getCollisionShape()->getAABB(transformA);
//     BoundingBox aabbB = bodyB->getCollisionShape()->getAABB(transformB);
    
//     return aabbA.intersects(aabbB);
// }

// void PhysicsWorld::broadPhaseCollision() {
//     newPairs.clear();
    
//     // Simple O(n²) broad phase - can be optimized with spatial partitioning later
//     for (size_t i = 0; i < rigidBodies.size(); ++i) {
//         for (size_t j = i + 1; j < rigidBodies.size(); ++j) {
//             RigidBody* bodyA = rigidBodies[i].get();
//             RigidBody* bodyB = rigidBodies[j].get();
            
//             // Skip if both bodies are static or sleeping
//             if ((bodyA->getBodyType() == RigidBody::BodyType::Static && 
//                  bodyB->getBodyType() == RigidBody::BodyType::Static) ||
//                 (bodyA->isSleeping() && bodyB->isSleeping())) {
//                 continue;
//             }
            
//             // AABB test
//             if (checkOverlap(bodyA, bodyB)) {
//                 newPairs.emplace_back(bodyA, bodyB);
//             }
//         }
//     }
// }

// void PhysicsWorld::narrowPhaseCollision() {
//     for (const auto& pair : newPairs) {
//         ContactPoint contact;
//         if (detectCollision(pair.bodyA, pair.bodyB, contact)) {
//             contactPoints.push_back(contact);
//         }
//     }
// }

// bool PhysicsWorld::detectCollision(RigidBody* bodyA, RigidBody* bodyB, ContactPoint& contact) {
//     auto shapeA = bodyA->getCollisionShape();
//     auto shapeB = bodyB->getCollisionShape();
    
//     if (!shapeA || !shapeB) return false;
    
//     auto typeA = shapeA->getType();
//     auto typeB = shapeB->getType();
    
//     // Dispatch to specific collision detection functions
//     if (typeA == CollisionShape::ShapeType::Sphere && typeB == CollisionShape::ShapeType::Sphere) {
//         return sphereVsSphere(bodyA, bodyB, contact);
//     } else if (typeA == CollisionShape::ShapeType::Box && typeB == CollisionShape::ShapeType::Box) {
//         return boxVsBox(bodyA, bodyB, contact);
//     } else if ((typeA == CollisionShape::ShapeType::Sphere && typeB == CollisionShape::ShapeType::Box) ||
//                (typeA == CollisionShape::ShapeType::Box && typeB == CollisionShape::ShapeType::Sphere)) {
//         return sphereVsBox(bodyA, bodyB, contact);
//     }
    
//     return false;
// }

// bool PhysicsWorld::sphereVsSphere(RigidBody* bodyA, RigidBody* bodyB, ContactPoint& contact) {
//     auto sphereA = std::static_pointer_cast<SphereShape>(bodyA->getCollisionShape());
//     auto sphereB = std::static_pointer_cast<SphereShape>(bodyB->getCollisionShape());
    
//     Transform transformA = getRigidBodyTransform(bodyA);
//     Transform transformB = getRigidBodyTransform(bodyB);
    
//     glm::vec3 contactPoint, normal;
//     float penetration;
    
//     if (SphereShape::sphereVsSphere(*sphereA, transformA, *sphereB, transformB, 
//                                    contactPoint, normal, penetration)) {
//         contact.position = contactPoint;
//         contact.normal = normal;
//         contact.penetration = penetration;
//         contact.bodyA = bodyA;
//         contact.bodyB = bodyB;
//         return true;
//     }
    
//     return false;
// }

// bool PhysicsWorld::boxVsBox(RigidBody* bodyA, RigidBody* bodyB, ContactPoint& contact) {
//     // Simplified box vs box collision - this would be implemented using SAT
//     // For now, just return false to focus on the architecture
//     return false;
// }

// bool PhysicsWorld::sphereVsBox(RigidBody* bodyA, RigidBody* bodyB, ContactPoint& contact) {
//     // Simplified sphere vs box collision
//     // For now, just return false to focus on the architecture
//     return false;
// }

// void PhysicsWorld::resolveCollisions() {
//     for (const auto& contact : contactPoints) {
//         resolveContact(contact);
//         separateBodies(contact);
//     }
// }

// void PhysicsWorld::resolveContact(const ContactPoint& contact) {
//     RigidBody* bodyA = contact.bodyA;
//     RigidBody* bodyB = contact.bodyB;
    
//     if (bodyA->getBodyType() != RigidBody::BodyType::Dynamic &&
//         bodyB->getBodyType() != RigidBody::BodyType::Dynamic) {
//         return;
//     }
    
//     // Calculate relative velocity
//     glm::vec3 relativeVelocity = bodyB->getLinearVelocity() - bodyA->getLinearVelocity();
//     float velocityAlongNormal = glm::dot(relativeVelocity, contact.normal);
    
//     // Do not resolve if velocities are separating
//     if (velocityAlongNormal > 0) return;
    
//     // Calculate restitution (bounciness)
//     float restitution = 0.2f;  // Default restitution - could be material property
    
//     // Calculate impulse scalar
//     float impulseScalar = -(1 + restitution) * velocityAlongNormal;
//     impulseScalar /= bodyA->getInverseMass() + bodyB->getInverseMass();
    
//     // Apply impulse
//     glm::vec3 impulse = impulseScalar * contact.normal;
    
//     if (bodyA->getBodyType() == RigidBody::BodyType::Dynamic) {
//         bodyA->applyImpulse(-impulse);
//     }
//     if (bodyB->getBodyType() == RigidBody::BodyType::Dynamic) {
//         bodyB->applyImpulse(impulse);
//     }
// }

// void PhysicsWorld::separateBodies(const ContactPoint& contact) {
//     RigidBody* bodyA = contact.bodyA;
//     RigidBody* bodyB = contact.bodyB;
    
//     float totalInverseMass = bodyA->getInverseMass() + bodyB->getInverseMass();
//     if (totalInverseMass <= 0.0f) return;
    
//     // Calculate separation
//     glm::vec3 separation = contact.normal * contact.penetration / totalInverseMass;
    
//     if (bodyA->getBodyType() == RigidBody::BodyType::Dynamic) {
//         bodyA->setPosition(bodyA->getPosition() - separation * bodyA->getInverseMass());
//     }
//     if (bodyB->getBodyType() == RigidBody::BodyType::Dynamic) {
//         bodyB->setPosition(bodyB->getPosition() + separation * bodyB->getInverseMass());
//     }
// }

// void PhysicsWorld::integrateForces(float dt) {
//     for (auto& body : rigidBodies) {
//         if (body->getBodyType() == RigidBody::BodyType::Dynamic && !body->isSleeping()) {
//             // Apply gravity
//             body->applyForce(gravity * body->getMass());
//         }
//     }
// }

// void PhysicsWorld::integrateBodies(float dt) {
//     for (auto& body : rigidBodies) {
//         if (body->getBodyType() == RigidBody::BodyType::Dynamic && !body->isSleeping()) {
//             body->integrate(dt);
//         }
//     }
// }

// Transform PhysicsWorld::getRigidBodyTransform(RigidBody* body) const {
//     Transform transform;
//     transform.position = body->getPosition();
//     transform.rotation = body->getOrientation();
//     transform.scale = glm::vec3(1.0f);  // Assuming uniform scale for now
//     return transform;
// }

// void PhysicsWorld::updateCollisionEvents() {
//     // Simple event system - could be optimized
//     for (const auto& pair : activePairs) {
//         if (!pairExists(pair, newPairs)) {
//             // Collision ended
//             if (onCollisionExit) {
//                 onCollisionExit(pair.bodyA, pair.bodyB);
//             }
//         }
//     }
    
//     for (const auto& pair : newPairs) {
//         if (pairExists(pair, activePairs)) {
//             // Continuing collision
//             auto it = std::find_if(contactPoints.begin(), contactPoints.end(),
//                 [&pair](const ContactPoint& contact) {
//                     return (contact.bodyA == pair.bodyA && contact.bodyB == pair.bodyB) ||
//                            (contact.bodyA == pair.bodyB && contact.bodyB == pair.bodyA);
//                 });
            
//             if (it != contactPoints.end() && onCollisionStay) {
//                 onCollisionStay(pair.bodyA, pair.bodyB, *it);
//             }
//         } else {
//             // New collision
//             auto it = std::find_if(contactPoints.begin(), contactPoints.end(),
//                 [&pair](const ContactPoint& contact) {
//                     return (contact.bodyA == pair.bodyA && contact.bodyB == pair.bodyB) ||
//                            (contact.bodyA == pair.bodyB && contact.bodyB == pair.bodyA);
//                 });
            
//             if (it != contactPoints.end() && onCollisionEnter) {
//                 onCollisionEnter(pair.bodyA, pair.bodyB, *it);
//             }
//         }
//     }
    
//     activePairs = newPairs;
// }

// bool PhysicsWorld::pairExists(const CollisionPair& pair, const std::vector<CollisionPair>& pairs) const {
//     return std::find(pairs.begin(), pairs.end(), pair) != pairs.end();
// }

// } // namespace engine::physics



#include "PhysicsWorld.hpp"
#include "engine/core/Logger.hpp"
#include <algorithm>
#include <unordered_set>
#include <chrono>
#include <sstream>

namespace engine::physics {

PhysicsWorld::PhysicsWorld() {
    rigidBodies.reserve(1000);
    contactManifolds.reserve(1000);
    activePairs.reserve(500);
    newPairs.reserve(500);
    
    // Initialize with spatial hash broad phase
    broadPhase = std::make_unique<SpatialHashBroadPhase>(5.0f);
}

PhysicsWorld::~PhysicsWorld() {
    clear();
}

void PhysicsWorld::update(float dt) {
    using namespace std::chrono;
    auto frameStart = high_resolution_clock::now();
    
    // Reset performance stats
    perfStats = PerformanceStats{};
    
    // Fixed timestep with accumulator
    accumulator += dt;
    
    int steps = 0;
    while (accumulator >= fixedTimeStep && steps < maxSubSteps) {
        // Apply gravity and other forces
        integrateForces(fixedTimeStep);
        
        // Broad phase collision detection
        auto broadStart = high_resolution_clock::now();
        broadPhaseCollision();
        perfStats.broadPhaseTime = endTimer(broadStart);
        
        // Narrow phase collision detection
        auto narrowStart = high_resolution_clock::now();
        narrowPhaseCollision();
        perfStats.narrowPhaseTime = endTimer(narrowStart);
        
        // Collision resolution
        auto solverStart = high_resolution_clock::now();
        resolveCollisions();
        perfStats.solverTime = endTimer(solverStart);
        
        // Integrate velocities to positions
// Integrate velocities to positions
       integrateBodies(fixedTimeStep);
       
       // Update collision events
       updateCollisionEvents();
       
       accumulator -= fixedTimeStep;
       steps++;
   }
   
   // Update performance stats
   perfStats.totalTime = endTimer(frameStart);
   perfStats.contactsGenerated = contactManifolds.size();
   
   size_t activeCount = 0, sleepingCount = 0;
   for (const auto& body : rigidBodies) {
       if (body->isSleeping()) {
           sleepingCount++;
       } else {
           activeCount++;
       }
   }
   perfStats.bodiesActive = activeCount;
   perfStats.bodiesSleeping = sleepingCount;
   
   // Clear old contact manifolds
   contactManifolds.clear();
}

void PhysicsWorld::clear() {
   rigidBodies.clear();
   contactManifolds.clear();
   activePairs.clear();
   newPairs.clear();
   accumulator = 0.0f;
   perfStats = PerformanceStats{};
   
   if (broadPhase) {
       broadPhase->clear();
   }
}

void PhysicsWorld::addRigidBody(std::shared_ptr<RigidBody> body) {
   if (body && std::find(rigidBodies.begin(), rigidBodies.end(), body) == rigidBodies.end()) {
       rigidBodies.push_back(body);
       
       if (broadPhase) {
           broadPhase->insertBody(body.get());
       }
   }
}

void PhysicsWorld::removeRigidBody(std::shared_ptr<RigidBody> body) {
   auto it = std::find(rigidBodies.begin(), rigidBodies.end(), body);
   if (it != rigidBodies.end()) {
       if (broadPhase) {
           broadPhase->removeBody(body.get());
       }
       
       rigidBodies.erase(it);
       
       // Remove any collision pairs involving this body
       activePairs.erase(
           std::remove_if(activePairs.begin(), activePairs.end(),
               [body](const CollisionPair& pair) {
                   return pair.bodyA == body.get() || pair.bodyB == body.get();
               }),
           activePairs.end()
       );
   }
}

void PhysicsWorld::setBroadPhase(std::unique_ptr<BroadPhase> newBroadPhase) {
   if (newBroadPhase) {
       // Transfer all bodies to new broad phase
       std::vector<RigidBody*> bodiesToTransfer;
       for (const auto& body : rigidBodies) {
           bodiesToTransfer.push_back(body.get());
       }
       
       broadPhase = std::move(newBroadPhase);
       
       for (RigidBody* body : bodiesToTransfer) {
           broadPhase->insertBody(body);
       }
   }
}

bool PhysicsWorld::raycast(const Ray& ray, RaycastHit& hit) const {
   hit.hit = false;
   float closestDistance = FLT_MAX;
   
   for (const auto& body : rigidBodies) {
       if (!body->getCollisionShape()) continue;
       
       Transform transform = getRigidBodyTransform(body.get());
       RaycastHit tempHit;
       
       if (body->getCollisionShape()->raycast(ray, transform, tempHit)) {
           if (tempHit.distance < closestDistance) {
               closestDistance = tempHit.distance;
               hit = tempHit;
               hit.body = body.get();
           }
       }
   }
   
   return hit.hit;
}

std::vector<RaycastHit> PhysicsWorld::raycastAll(const Ray& ray) const {
   std::vector<RaycastHit> hits;
   
   for (const auto& body : rigidBodies) {
       if (!body->getCollisionShape()) continue;
       
       Transform transform = getRigidBodyTransform(body.get());
       RaycastHit hit;
       
       if (body->getCollisionShape()->raycast(ray, transform, hit)) {
           hit.body = body.get();
           hits.push_back(hit);
       }
   }
   
   // Sort by distance
   std::sort(hits.begin(), hits.end(),
       [](const RaycastHit& a, const RaycastHit& b) {
           return a.distance < b.distance;
       });
   
   return hits;
}

std::vector<RigidBody*> PhysicsWorld::getOverlappingBodies(const BoundingBox& aabb) const {
   if (broadPhase) {
       return broadPhase->queryRegion(aabb.min, aabb.max);
   }
   
   // Fallback: linear search
   std::vector<RigidBody*> overlapping;
   for (const auto& body : rigidBodies) {
       if (!body->getCollisionShape()) continue;
       
       Transform transform = getRigidBodyTransform(body.get());
       BoundingBox bodyAABB = body->getCollisionShape()->getAABB(transform);
       
       if (aabb.intersects(bodyAABB)) {
           overlapping.push_back(body.get());
       }
   }
   
   return overlapping;
}

std::vector<RigidBody*> PhysicsWorld::getOverlappingBodies(const glm::vec3& point) const {
   if (broadPhase) {
       return broadPhase->queryPoint(point);
   }
   
   // Fallback: linear search
   std::vector<RigidBody*> overlapping;
   for (const auto& body : rigidBodies) {
       if (!body->getCollisionShape()) continue;
       
       Transform transform = getRigidBodyTransform(body.get());
       BoundingBox bodyAABB = body->getCollisionShape()->getAABB(transform);
       
       if (bodyAABB.contains(point)) {
           overlapping.push_back(body.get());
       }
   }
   
   return overlapping;
}

bool PhysicsWorld::checkOverlap(RigidBody* bodyA, RigidBody* bodyB) const {
   if (!bodyA->getCollisionShape() || !bodyB->getCollisionShape()) {
       return false;
   }
   
   Transform transformA = getRigidBodyTransform(bodyA);
   Transform transformB = getRigidBodyTransform(bodyB);
   
   BoundingBox aabbA = bodyA->getCollisionShape()->getAABB(transformA);
   BoundingBox aabbB = bodyB->getCollisionShape()->getAABB(transformB);
   
   return aabbA.intersects(aabbB);
}

int PhysicsWorld::getContactCount() const {
   int totalContacts = 0;
   for (const auto& manifold : contactManifolds) {
       totalContacts += static_cast<int>(manifold.getContactCount());
   }
   return totalContacts;
}

std::string PhysicsWorld::getDebugInfo() const {
   std::ostringstream oss;
   oss << "PhysicsWorld Debug Info:\n";
   oss << "  Bodies: " << rigidBodies.size() << " (Active: " << perfStats.bodiesActive 
       << ", Sleeping: " << perfStats.bodiesSleeping << ")\n";
   oss << "  Contact Manifolds: " << contactManifolds.size() << "\n";
   oss << "  Contact Points: " << getContactCount() << "\n";
   oss << "  Pairs Processed: " << perfStats.pairsProcessed << "\n";
   oss << "  Performance:\n";
   oss << "    Total Time: " << perfStats.totalTime << " ms\n";
   oss << "    Broad Phase: " << perfStats.broadPhaseTime << " ms\n";
   oss << "    Narrow Phase: " << perfStats.narrowPhaseTime << " ms\n";
   oss << "    Solver: " << perfStats.solverTime << " ms\n";
   
   if (broadPhase) {
       oss << "  Broad Phase Info:\n";
       oss << "    " << broadPhase->getDebugInfo();
   }
   
   return oss.str();
}

void PhysicsWorld::broadPhaseCollision() {
   if (!broadPhase) return;
   
   // Update broad phase with current body positions
   broadPhase->updateAllBodies();
   
   // Find potential collision pairs
   newPairs = broadPhase->findPotentialCollisions();
   perfStats.pairsProcessed = newPairs.size();
}

void PhysicsWorld::narrowPhaseCollision() {
   contactManifolds.clear();
   contactManifolds.reserve(newPairs.size());
   
   for (const auto& pair : newPairs) {
       ContactManifold manifold;
       if (CollisionDetector::detectCollision(pair.bodyA, pair.bodyB, manifold)) {
           if (manifold.hasContacts()) {
               contactManifolds.push_back(manifold);
           }
       }
   }
}

void PhysicsWorld::resolveCollisions() {
   // Prepare all manifolds
   for (auto& manifold : contactManifolds) {
       manifold.prepare(fixedTimeStep);
   }
   
   // Warm start
   for (auto& manifold : contactManifolds) {
       manifold.warmStart();
   }
   
   // Velocity iterations
   for (int i = 0; i < velocityIterations; ++i) {
       for (auto& manifold : contactManifolds) {
           manifold.solve(fixedTimeStep);
       }
   }
   
   // Position correction (simplified)
   for (int i = 0; i < positionIterations; ++i) {
       for (auto& manifold : contactManifolds) {
           // Simple position correction
           if (manifold.hasContacts()) {
               RigidBody* bodyA = manifold.getBodyA();
               RigidBody* bodyB = manifold.getBodyB();
               
               const auto& contacts = manifold.getContacts();
               glm::vec3 normal = manifold.getNormal();
               
               for (const auto& contact : contacts) {
                   if (contact.penetrationDepth > 0.001f) {
                       float correction = contact.penetrationDepth * 0.2f; // 20% correction
                       glm::vec3 correctionVector = normal * correction;
                       
                       if (bodyA->getBodyType() == RigidBody::BodyType::Dynamic) {
                           float massRatio = bodyA->getInverseMass() / 
                                           (bodyA->getInverseMass() + bodyB->getInverseMass());
                           bodyA->setPosition(bodyA->getPosition() - correctionVector * massRatio);
                       }
                       
                       if (bodyB->getBodyType() == RigidBody::BodyType::Dynamic) {
                           float massRatio = bodyB->getInverseMass() / 
                                           (bodyA->getInverseMass() + bodyB->getInverseMass());
                           bodyB->setPosition(bodyB->getPosition() + correctionVector * massRatio);
                       }
                   }
               }
           }
       }
   }
   
   // Post solve
   for (auto& manifold : contactManifolds) {
       manifold.postSolve();
   }
}

void PhysicsWorld::integrateForces(float dt) {
   for (auto& body : rigidBodies) {
       if (body->getBodyType() == RigidBody::BodyType::Dynamic && !body->isSleeping()) {
           // Apply gravity
           body->applyForce(gravity * body->getMass());
       }
   }
}

void PhysicsWorld::integrateBodies(float dt) {
   for (auto& body : rigidBodies) {
       if (body->getBodyType() == RigidBody::BodyType::Dynamic && !body->isSleeping()) {
           body->integrate(dt);
           
           // Update broad phase
           if (broadPhase) {
               broadPhase->updateBody(body.get());
           }
       }
   }
}

void PhysicsWorld::updateCollisionEvents() {
   // Convert manifolds to pairs for event processing
   std::vector<CollisionPair> currentPairs;
   for (const auto& manifold : contactManifolds) {
       currentPairs.emplace_back(manifold.getBodyA(), manifold.getBodyB());
   }
   
   // Find exiting collisions
   for (const auto& pair : activePairs) {
       if (std::find(currentPairs.begin(), currentPairs.end(), pair) == currentPairs.end()) {
           // Collision ended
           if (onCollisionExit) {
               onCollisionExit(pair.bodyA, pair.bodyB);
           }
       }
   }
   
   // Find new and continuing collisions
   for (size_t i = 0; i < currentPairs.size(); ++i) {
       const auto& pair = currentPairs[i];
       const auto& manifold = contactManifolds[i];
       
       if (std::find(activePairs.begin(), activePairs.end(), pair) != activePairs.end()) {
           // Continuing collision
           if (onCollisionStay) {
               onCollisionStay(pair.bodyA, pair.bodyB, manifold);
           }
       } else {
           // New collision
           if (onCollisionEnter) {
               onCollisionEnter(pair.bodyA, pair.bodyB, manifold);
           }
       }
   }
   
   activePairs = currentPairs;
}

Transform PhysicsWorld::getRigidBodyTransform(RigidBody* body) const {
   Transform transform;
   transform.position = body->getPosition();
   transform.rotation = body->getOrientation();
   transform.scale = glm::vec3(1.0f);
   return transform;
}

bool PhysicsWorld::pairExists(const CollisionPair& pair, const std::vector<CollisionPair>& pairs) const {
   return std::find(pairs.begin(), pairs.end(), pair) != pairs.end();
}

void PhysicsWorld::startTimer(std::chrono::high_resolution_clock::time_point& timer) const {
   timer = std::chrono::high_resolution_clock::now();
}

float PhysicsWorld::endTimer(const std::chrono::high_resolution_clock::time_point& timer) const {
   auto end = std::chrono::high_resolution_clock::now();
   return std::chrono::duration<float, std::milli>(end - timer).count();
}

} // namespace engine::physics