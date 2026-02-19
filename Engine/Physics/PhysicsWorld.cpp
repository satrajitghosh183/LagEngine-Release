#include "PhysicsWorld.hpp"
#include "../Core/Logger.hpp"
#include <algorithm>

namespace GameEngine {
namespace Physics {

    PhysicsWorld::PhysicsWorld()
        : m_Gravity(0.0f, -9.81f, 0.0f)
        , m_Iterations(10) {
        
        m_ConstraintSolver = CreateScope<ConstraintSolver>();
        m_BroadPhase = CreateScope<SpatialHash>(2.0f);
        GE_CORE_INFO("PhysicsWorld created with constraint solver and spatial hash broadphase");
    }

    PhysicsWorld::~PhysicsWorld() {
        m_RigidBodies.clear();
        m_Constraints.clear();
        m_ContactManifolds.clear();
        GE_CORE_INFO("PhysicsWorld destroyed");
    }

    void PhysicsWorld::Update(float deltaTime) {
        // Step 1: Apply gravity to all dynamic bodies
        ApplyGravity();
        
        // Step 2: Integrate forces -> velocities (don't update positions yet)
        for (auto& body : m_RigidBodies) {
            body->IntegrateForces(deltaTime);
        }
        
        // Step 3: Clear previous frame contacts
        m_ContactManifolds.clear();
        
        // Step 4: Broad phase would go here (spatial hashing/BVH)
        // For now, we do O(n^2) narrow phase on all pairs
        
        // Step 5: Narrow phase collision detection
        DetectCollisions();
        
        // Step 6: Resolve collisions and constraints using sequential impulse solver
        ResolveCollisions(deltaTime);
        
        // Step 7: Integrate velocities -> positions (after constraints modify velocities)
        for (auto& body : m_RigidBodies) {
            body->IntegrateVelocities(deltaTime);
        }
    }

    void PhysicsWorld::AddRigidBody(const Ref<RigidBody>& body) {
        m_RigidBodies.push_back(body);
    }

    void PhysicsWorld::RemoveRigidBody(const Ref<RigidBody>& body) {
        auto it = std::find(m_RigidBodies.begin(), m_RigidBodies.end(), body);
        if (it != m_RigidBodies.end()) {
            m_RigidBodies.erase(it);
        }
    }
    
    void PhysicsWorld::AddConstraint(const Ref<Constraint>& constraint) {
        m_Constraints.push_back(constraint);
    }
    
    void PhysicsWorld::RemoveConstraint(const Ref<Constraint>& constraint) {
        auto it = std::find(m_Constraints.begin(), m_Constraints.end(), constraint);
        if (it != m_Constraints.end()) {
            m_Constraints.erase(it);
        }
    }
    
    void PhysicsWorld::SetVelocityIterations(int iterations) {
        m_ConstraintSolver->SetIterations(iterations, m_ConstraintSolver->GetPositionIterations());
    }
    
    void PhysicsWorld::SetPositionIterations(int iterations) {
        m_ConstraintSolver->SetIterations(m_ConstraintSolver->GetVelocityIterations(), iterations);
    }

    bool PhysicsWorld::Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance,
                               CollisionShape::RaycastHit& outHit) const {
        outHit.Hit = false;
        outHit.Distance = maxDistance;
        float closestDist = maxDistance;
        for (const auto& body : m_RigidBodies) {
            if (!body->HasCollisionShape()) continue;
            auto hit = body->GetCollisionShape()->Raycast(
                origin, direction,
                body->GetPosition(), body->GetRotation(),
                maxDistance
            );
            if (hit.Hit && hit.Distance < closestDist && hit.Distance >= 0) {
                closestDist = hit.Distance;
                outHit = hit;
            }
        }
        return outHit.Hit;
    }

    void PhysicsWorld::ApplyGravity() {
        for (auto& body : m_RigidBodies) {
            if (body->GetUseGravity() && body->GetBodyType() == RigidBody::BodyType::Dynamic) {
                body->ApplyForce(m_Gravity * body->GetMass());
            }
        }
    }


    void PhysicsWorld::DetectCollisions() {
        // Get potential collision pairs from broadphase
        std::vector<std::pair<Ref<RigidBody>, Ref<RigidBody>>> potentialPairs;
        
        if (m_UseBroadPhase && m_BroadPhase) {
            // Update broadphase with current positions
            m_BroadPhase->Update(m_RigidBodies);
            
            // Get potential pairs from spatial hash
            potentialPairs = m_BroadPhase->GetPotentialPairs();
        } else {
            // Fallback: O(n^2) brute force
            size_t bodyCount = m_RigidBodies.size();
            for (size_t i = 0; i < bodyCount; i++) {
                for (size_t j = i + 1; j < bodyCount; j++) {
                    potentialPairs.push_back({m_RigidBodies[i], m_RigidBodies[j]});
                }
            }
        }
        
        // Narrow phase: test each potential pair
        for (const auto& [bodyA, bodyB] : potentialPairs) {
            // Skip if both are static
            if (bodyA->GetBodyType() == RigidBody::BodyType::Static &&
                bodyB->GetBodyType() == RigidBody::BodyType::Static) {
                continue;
            }
            
            // Skip if both are asleep
            if (!bodyA->IsAwake() && !bodyB->IsAwake()) {
                continue;
            }
            
            // Skip if no collision shapes
            if (!bodyA->HasCollisionShape() || !bodyB->HasCollisionShape()) {
                continue;
            }
            
            // Create manifold for this pair
            ContactManifold manifold(bodyA.get(), bodyB.get());
            
            // Detect collision
            bool collision = CollisionDetector::DetectCollision(
                bodyA->GetCollisionShape(), bodyA->GetPosition(), bodyA->GetRotation(),
                bodyB->GetCollisionShape(), bodyB->GetPosition(), bodyB->GetRotation(),
                manifold
            );
            
            if (collision && manifold.GetContactCount() > 0) {
                m_ContactManifolds.push_back(manifold);
                
                // Wake up bodies involved in collision
                bodyA->SetAwake(true);
                bodyB->SetAwake(true);
            }
        }
    }

    void PhysicsWorld::ResolveCollisions(float deltaTime) {
        // Use constraint solver to resolve all contacts and constraints
        m_ConstraintSolver->Solve(m_Constraints, m_ContactManifolds, deltaTime);
    }

}}
