#pragma once

#include "../Base.hpp"
#include "../../Graphics/Mesh3D.hpp"
#include "../../Graphics/Material.hpp"
#include "../../Graphics/Camera3D.hpp"
#include "../../Graphics/Light.hpp"
#include <glm/glm.hpp>
#include <string>

namespace GameEngine {

    /**
     * @brief Render server interface
     * 
     * Provides rendering operations without exposing implementation details
     */
    class IRenderServer {
    public:
        virtual ~IRenderServer() = default;

        /**
         * @brief Initialize render server
         */
        virtual void Initialize() = 0;

        /**
         * @brief Shutdown render server
         */
        virtual void Shutdown() = 0;

        /**
         * @brief Begin frame rendering
         */
        virtual void BeginFrame() = 0;

        /**
         * @brief End frame rendering
         */
        virtual void EndFrame() = 0;

        /**
         * @brief Set viewport dimensions
         */
        virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;

        /**
         * @brief Submit mesh for rendering
         */
        virtual void SubmitMesh(Ref<Mesh3D> mesh, Ref<Material> material, const glm::mat4& transform) = 0;

        /**
         * @brief Submit light for rendering
         */
        virtual void SubmitLight(const Light& light) = 0;

        /**
         * @brief Set active camera
         */
        virtual void SetCamera(const Camera3D& camera) = 0;

        /**
         * @brief Clear screen
         */
        virtual void Clear(const glm::vec4& color) = 0;
    };

}

