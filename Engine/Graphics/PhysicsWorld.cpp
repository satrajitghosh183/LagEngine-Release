#include "PhysicsWorld.hpp"
#include "../Core/Logger.hpp"

namespace GameEngine {
namespace Physics {

    PhysicsWorld::PhysicsWorld()
        : m_Gravity(0.0f, -9.81f, 0.0f)
        , m_Iterations(10) {
        
        GE_CORE_INFO("PhysicsWorld created");
    }

    PhysicsWorld::~PhysicsWorld() {
        m_RigidBodies.clear();
        GE_CORE_INFO("PhysicsWorld destroyed");
    }

    void PhysicsWorld::Update(float deltaTime) {
        // Apply gravity
        ApplyGravity();
        
        // Integrate velocities
        IntegrateVelocities(deltaTime);
        
        // Collision detection (basic)
        DetectCollisions();
        
        // Resolve collisions
        ResolveCollisions();
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

    void PhysicsWorld::ApplyGravity() {
        for (auto& body : m_RigidBodies) {
            if (body->GetUseGravity() && body->GetBodyType() == RigidBody::BodyType::Dynamic) {
                body->ApplyForce(m_Gravity * body->GetMass());
            }
        }
    }

    void PhysicsWorld::IntegrateVelocities(float deltaTime) {
        for (auto& body : m_RigidBodies) {
            body->Integrate(deltaTime);
        }
    }

    void PhysicsWorld::DetectCollisions() {
        // TODO: Implement collision detection
        // For now, this is a placeholder
    }

    void PhysicsWorld::ResolveCollisions() {
        // TODO: Implement collision resolution
        // For now, this is a placeholder
    }

}}