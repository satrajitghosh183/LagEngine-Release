#pragma once

#include <memory>
#include "Scene3D.hpp"

namespace engine::scene {

class SceneManager3D {
private:
    std::shared_ptr<Scene3D> currentScene;

public:
    SceneManager3D(float aspectRatio);

    void update(float dt);

    std::shared_ptr<Scene3D> getCurrentScene() const;

    std::shared_ptr<engine::physics::PhysicsWorld3D> getPhysicsWorld() const;
};

} // namespace engine::scene
