// SceneManager2D.hpp
#pragma once
#include "Scene2D.hpp"
#include <memory>

namespace engine::scene {

    class SceneManager2D {
    private:
        Scene2D mainScene;

    public:
        Scene2D* get2D();
        void update2D(float dt);

        /// Gather all line vertices from the scene
        std::vector<Object2D::Vertex2D> gatherLineVertices() const;

        /// Gather all triangle vertices from the scene
        std::vector<Object2D::Vertex2D> gatherTriangleVertices() const;
    };

} // namespace engine::scene
