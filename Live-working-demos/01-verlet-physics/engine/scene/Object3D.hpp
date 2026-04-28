#pragma once
#include <glm/gtc/matrix_transform.hpp>

namespace engine::scene {

    /**
     * @brief Base class for any 3D object in the scene.
     */
    class Object3D {
    public:
        virtual ~Object3D() = default;

        virtual void update(float dt) = 0;

        bool visible = true;
    };

} // namespace engine::scene
