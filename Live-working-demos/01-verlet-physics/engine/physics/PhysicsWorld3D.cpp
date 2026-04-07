#include "engine/physics/PhysicsWorld3D.hpp"

namespace engine::physics {

PhysicsWorld3D::PhysicsWorld3D()
    : gravity(glm::vec3(0.0f, -9.81f, 0.0f)), solverIterations(20)
{}

void PhysicsWorld3D::setGravity(const glm::vec3& g) {
    gravity = g;
}

void PhysicsWorld3D::update(float dt, int solverIterations_) {
    solverIterations = solverIterations_;

    // Now simply use the base class update
    VertletSystem3D::update(dt, gravity, solverIterations);
}

void PhysicsWorld3D::addCloth(const std::shared_ptr<ClothSolver3D>& cloth) {
    cloths.push_back(cloth);
    // Automatically register particles and springs
    for (const auto& particle : cloth->getParticles()) {
        addParticle(particle);
    }
    for (const auto& spring : cloth->getSprings()) {
        addSpring(spring);
    }
}

void PhysicsWorld3D::addBall(const std::shared_ptr<engine::objects::Ball3D>& ball) {
    balls.push_back(ball);
    // If Ball3D should interact, register its particles/springs similarly
}

} // namespace engine::physics


void PhysicsWorld3D::addRigidBody(std::shared_ptr<RigidBody> body) {
    rigidBodies.push_back(body);
}

void PhysicsWorld3D::removeRigidBody(std::shared_ptr<RigidBody> body) {
    auto it = std::find(rigidBodies.begin(), rigidBodies.end(), body);
    if (it != rigidBodies.end()) {
        rigidBodies.erase(it);
    }
}

// In the update method, add:
void PhysicsWorld3D::update(float dt, int solverIterations_) {
    // ... existing cloth code ...
    
    // Update rigid bodies
    for (auto& body : rigidBodies) {
        if (body->getBodyType() == RigidBody::BodyType::Dynamic) {
            body->applyForce(gravity * body->getMass());
            body->integrate(dt);
        }
    }
}