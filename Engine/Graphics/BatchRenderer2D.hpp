#pragma once

#include "../Core/Base.hpp"
#include "Shader.hpp"
#include <glm/glm.hpp>
#include <cstdint>

namespace GameEngine {

    class BatchRenderer2D {
    public:
        static void Init();
        static void Shutdown();

        static void BeginBatch(const glm::mat4& projection);
        static void EndBatch();
        static void Flush();

        static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
        static void DrawTexturedQuad(const glm::vec2& position, const glm::vec2& size,
                                     uint32_t textureID, const glm::vec4& tint = glm::vec4(1.0f),
                                     const glm::vec2& uvMin = {0, 0}, const glm::vec2& uvMax = {1, 1});

        struct Stats {
            int DrawCalls = 0;
            int QuadCount = 0;
        };
        static Stats GetStats();
        static void ResetStats();

    private:
        struct Vertex {
            glm::vec2 Position;
            glm::vec2 TexCoord;
            glm::vec4 Color;
            float TexIndex;
        };
    };

}
