// SceneManager2D.cpp
#include "engine/scene/SceneManager2D.hpp"

namespace engine::scene {

    Scene2D* SceneManager2D::get2D() {
        return &mainScene;
    }

    void SceneManager2D::update2D(float dt) {
        mainScene.update(dt);
    }

    std::vector<Object2D::Vertex2D> SceneManager2D::gatherLineVertices() const {
        return mainScene.gatherLineVertices();
    }

    std::vector<Object2D::Vertex2D> SceneManager2D::gatherTriangleVertices() const {
        return mainScene.gatherTriangleVertices();
    }

} // namespace engine::scene
