#include "engine/scene/SceneManager3D.hpp"

namespace engine::scene {

SceneManager3D::SceneManager3D(float aspectRatio) {
    currentScene = std::make_shared<Scene3D>(aspectRatio);
}

void SceneManager3D::update(float dt) {
    if (currentScene) {
        currentScene->update(dt);
    }
}

// void SceneManager3D::render(const engine::graphics::Shader& shader) const {
//     if (currentScene) {
//         currentScene->render(shader);
//     }
// }

void SceneManager3D::render(const engine::graphics::Shader& shader) const {
    if (currentScene) {
        currentScene->render();  // <-- NO ARGUMENT!
    }
}


std::shared_ptr<Scene3D> SceneManager3D::getCurrentScene() const {
    return currentScene;
}

std::shared_ptr<engine::physics::PhysicsWorld3D> SceneManager3D::getPhysicsWorld() const {
    if (currentScene) {
        return currentScene->getPhysicsWorld();
    }
    return nullptr;
}

} // namespace engine::scene
