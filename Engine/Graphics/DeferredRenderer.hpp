#pragma once

#include "../Core/Base.hpp"
#include "GBuffer.hpp"
#include "Shader.hpp"
#include "VertexArray.hpp"
#include "Vulkan/VulkanPipeline.hpp"
#include "Vulkan/VulkanDescriptors.hpp"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

namespace GameEngine {

    /**
     * @brief Vulkan deferred renderer
     *
     * Two-pass rendering:
     *   1. Geometry pass: render scene into G-Buffer (position, normal, albedo, PBR params)
     *   2. Lighting pass: read G-Buffer as input, compute lighting per-pixel
     *
     * G-Buffer attachments are sampled as textures in the lighting pass
     * via descriptor sets (more efficient than OpenGL texture binding).
     */
    class DeferredRenderer {
    public:
        void Init(int width, int height);
        void Resize(int width, int height);
        void Shutdown();

        /**
         * @brief Begin geometry pass (starts G-Buffer render pass)
         */
        void BeginGeometryPass(VkCommandBuffer cmd);

        /**
         * @brief End geometry pass
         */
        void EndGeometryPass(VkCommandBuffer cmd);

        /**
         * @brief Render lighting pass (reads G-Buffer, outputs to current render pass)
         */
        void RenderLightingPass(VkCommandBuffer cmd, const glm::mat4& projection,
                                 const glm::vec3& cameraPos);

        GBuffer& GetGBuffer() { return m_GBuffer; }

        bool SSAOEnabled = true;

    private:
        void CreateFullscreenQuad();

        GBuffer m_GBuffer;

        // Fullscreen quad for lighting pass
        Ref<VertexBuffer> m_QuadVB;
        Ref<IndexBuffer> m_QuadIB;

        int m_Width = 0, m_Height = 0;
    };

}
