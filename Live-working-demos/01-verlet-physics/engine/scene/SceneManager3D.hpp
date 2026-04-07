#pragma once

#include <memory>
#include "Scene3D.hpp"
#include "engine/graphics/Shader.hpp"

namespace engine::scene {

class SceneManager3D {
private:
    std::shared_ptr<Scene3D> currentScene;

public:
    // Constructor
    SceneManager3D(float aspectRatio);

    // Update and render functions
    void update(float dt);
    void render(const engine::graphics::Shader& shader) const;

    // Accessor for the current scene
    std::shared_ptr<Scene3D> getCurrentScene() const;

    // Convenience function to get PhysicsWorld directly
    std::shared_ptr<engine::physics::PhysicsWorld3D> getPhysicsWorld() const;
};

} // namespace engine::scene
