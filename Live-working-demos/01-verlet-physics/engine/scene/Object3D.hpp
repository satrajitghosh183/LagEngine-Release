#pragma once
#include "engine/graphics/Shader.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace engine::scene {

    /**
     * @brief Base class for any 3D object in the scene.
     *        Inherit from this to create entities like Ball3D, Cloth3D, etc.
     */
    class Object3D {
    public:
        virtual ~Object3D() = default;

        // Called every frame to update physics or logic.
        virtual void update(float dt) = 0;

        // Called every frame to render using provided shader.
        virtual void render(const engine::graphics::Shader& shader) = 0;

        // Optional toggle for in-editor or runtime control
        bool visible = true;
    };

} // namespace engine::scene
