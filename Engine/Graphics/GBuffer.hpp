#pragma once

#include "../Core/Base.hpp"
#include <glad/glad.h>
#include <glm/glm.hpp>

namespace GameEngine {

    /**
     * @brief G-Buffer for deferred shading
     *
     * Stores per-pixel geometry data into multiple render targets:
     *  - Position (RGB16F): world-space position
     *  - Normal (RGB16F): world-space normal
     *  - Albedo (RGBA8): base color + alpha
     *  - MetallicRoughness (RG8): metallic in R, roughness in G
     *  - Depth (DEPTH24): depth buffer
     *
     * Usage:
     *   gBuffer.BindForWriting();
     *   // render geometry ...
     *   gBuffer.Unbind();
     *   gBuffer.BindTextures();
     *   // render lighting fullscreen quad ...
     */
    class GBuffer {
    public:
        GBuffer() = default;
        ~GBuffer();

        void Init(int width, int height);
        void Resize(int width, int height);
        void BindForWriting();
        void BindForReading();
        void BindTextures(); // Bind all GBuffer textures to texture units 0-3
        void Unbind();

        uint32_t GetPositionTexture() const { return m_PositionTexture; }
        uint32_t GetNormalTexture() const { return m_NormalTexture; }
        uint32_t GetAlbedoTexture() const { return m_AlbedoTexture; }
        uint32_t GetMetallicRoughnessTexture() const { return m_MetallicRoughnessTexture; }
        uint32_t GetDepthTexture() const { return m_DepthTexture; }
        int GetWidth() const { return m_Width; }
        int GetHeight() const { return m_Height; }

    private:
        void CreateTextures();
        void Cleanup();

        uint32_t m_FBO = 0;
        uint32_t m_PositionTexture = 0;         // RGB16F: world position
        uint32_t m_NormalTexture = 0;            // RGB16F: world normal
        uint32_t m_AlbedoTexture = 0;            // RGBA8: albedo + alpha
        uint32_t m_MetallicRoughnessTexture = 0; // RG8: metallic, roughness
        uint32_t m_DepthTexture = 0;             // DEPTH24
        int m_Width = 0, m_Height = 0;
    };

}
