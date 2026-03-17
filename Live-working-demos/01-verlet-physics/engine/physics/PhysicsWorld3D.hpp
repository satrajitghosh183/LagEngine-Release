#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>

#include "VertletSystem3D.hpp"
#include "ClothSolver3D.hpp"
#include "SoftBodySystem3D.hpp"
#include "../objects/Ball3D.hpp"
#include "RigidBody.hpp"


namespace engine::physics {

class PhysicsWorld3D : public VertletSystem3D {
public:
    PhysicsWorld3D();
    // Add rigid body support to existing cloth system
    void addRigidBody(std::shared_ptr<RigidBody> body);
    void removeRigidBody(std::shared_ptr<RigidBody> body);
    const std::vector<std::shared_ptr<RigidBody>>& getRigidBodies() const { return rigidBodies; }


    
    void setGravity(const glm::vec3& gravity);
    void update(float dt, int solverIterations = 5);

    void addCloth(const std::shared_ptr<ClothSolver3D>& cloth);
    void addBall(const std::shared_ptr<engine::objects::Ball3D>& ball);

    const std::vector<std::shared_ptr<ClothSolver3D>>& getCloths() const { return cloths; }
    const std::vector<std::shared_ptr<engine::objects::Ball3D>>& getBalls() const { return balls; }

private:
    glm::vec3 gravity;
    int solverIterations;
    std::vector<std::shared_ptr<RigidBody>> rigidBodies;
    std::vector<std::shared_ptr<ClothSolver3D>> cloths;
    std::vector<std::shared_ptr<engine::objects::Ball3D>> balls;
};

} // namespace engine::physics
