#include "BatchRenderer2D.hpp"
#include "../Core/Logger.hpp"
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <array>

namespace GameEngine {

    static const int MaxQuads = 10000;
    static const int MaxVertices = MaxQuads * 4;
    static const int MaxIndices = MaxQuads * 6;
    static const int MaxTextureSlots = 16;

    struct RendererData {
        uint32_t VAO = 0, VBO = 0, EBO = 0;
        Ref<Shader> SpriteShader;
        uint32_t WhiteTexture = 0;

        BatchRenderer2D::Vertex* VertexBufferBase = nullptr;
        BatchRenderer2D::Vertex* VertexBufferPtr = nullptr;
        int QuadCount = 0;

        std::array<uint32_t, MaxTextureSlots> TextureSlots;
        int TextureSlotIndex = 1; // 0 = white texture

        glm::mat4 CurrentProjection = glm::mat4(1.0f);

        BatchRenderer2D::Stats RenderStats;
    };

    static RendererData s_Data;

    void BatchRenderer2D::Init() {
        s_Data.VertexBufferBase = new Vertex[MaxVertices];

        glGenVertexArrays(1, &s_Data.VAO);
        glBindVertexArray(s_Data.VAO);

        glGenBuffers(1, &s_Data.VBO);
        glBindBuffer(GL_ARRAY_BUFFER, s_Data.VBO);
        glBufferData(GL_ARRAY_BUFFER, MaxVertices * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoord));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Color));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexIndex));

        auto* indices = new uint32_t[MaxIndices];
        uint32_t offset = 0;
        for (int i = 0; i < MaxIndices; i += 6) {
            indices[i + 0] = offset + 0; indices[i + 1] = offset + 1; indices[i + 2] = offset + 2;
            indices[i + 3] = offset + 2; indices[i + 4] = offset + 3; indices[i + 5] = offset + 0;
            offset += 4;
        }
        glGenBuffers(1, &s_Data.EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_Data.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, MaxIndices * sizeof(uint32_t), indices, GL_STATIC_DRAW);
        delete[] indices;

        // White texture
        uint32_t whiteData = 0xFFFFFFFF;
        glGenTextures(1, &s_Data.WhiteTexture);
        glBindTexture(GL_TEXTURE_2D, s_Data.WhiteTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &whiteData);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        s_Data.TextureSlots[0] = s_Data.WhiteTexture;

        const char* vertSrc = R"(
#version 420 core
layout(location=0) in vec2 a_Position;
layout(location=1) in vec2 a_TexCoord;
layout(location=2) in vec4 a_Color;
layout(location=3) in float a_TexIndex;
uniform mat4 u_Projection;
out vec2 v_TexCoord; out vec4 v_Color; out float v_TexIndex;
void main() {
    v_TexCoord = a_TexCoord; v_Color = a_Color; v_TexIndex = a_TexIndex;
    gl_Position = u_Projection * vec4(a_Position, 0.0, 1.0);
})";
        const char* fragSrc = R"(
#version 420 core
in vec2 v_TexCoord; in vec4 v_Color; in float v_TexIndex;
out vec4 FragColor;
uniform sampler2D u_Textures[16];
void main() {
    int idx = int(v_TexIndex);
    FragColor = texture(u_Textures[idx], v_TexCoord) * v_Color;
})";

        s_Data.SpriteShader = CreateRef<Shader>("Sprite2D", vertSrc, fragSrc);
        GE_CORE_INFO("BatchRenderer2D initialized");
    }

    void BatchRenderer2D::Shutdown() {
        delete[] s_Data.VertexBufferBase;
        glDeleteVertexArrays(1, &s_Data.VAO);
        glDeleteBuffers(1, &s_Data.VBO);
        glDeleteBuffers(1, &s_Data.EBO);
        glDeleteTextures(1, &s_Data.WhiteTexture);
    }

    void BatchRenderer2D::BeginBatch(const glm::mat4& projection) {
        s_Data.CurrentProjection = projection;
        s_Data.SpriteShader->Bind();
        s_Data.SpriteShader->SetUniformMat4("u_Projection", projection);
        s_Data.VertexBufferPtr = s_Data.VertexBufferBase;
        s_Data.QuadCount = 0;
        s_Data.TextureSlotIndex = 1;
    }

    void BatchRenderer2D::EndBatch() {
        if (s_Data.QuadCount == 0) return;
        Flush();
    }

    void BatchRenderer2D::Flush() {
        if (s_Data.QuadCount == 0) return;
        uint32_t dataSize = static_cast<uint32_t>((uint8_t*)s_Data.VertexBufferPtr - (uint8_t*)s_Data.VertexBufferBase);
        glBindBuffer(GL_ARRAY_BUFFER, s_Data.VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, dataSize, s_Data.VertexBufferBase);

        for (int i = 0; i < s_Data.TextureSlotIndex; i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, s_Data.TextureSlots[i]);
        }

        glBindVertexArray(s_Data.VAO);
        glDrawElements(GL_TRIANGLES, s_Data.QuadCount * 6, GL_UNSIGNED_INT, nullptr);
        s_Data.RenderStats.DrawCalls++;
    }

    void BatchRenderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color) {
        if (s_Data.QuadCount >= MaxQuads) { EndBatch(); BeginBatch(s_Data.CurrentProjection); }

        float x = position.x, y = position.y, w = size.x, h = size.y;
        auto* v = s_Data.VertexBufferPtr;
        v[0] = {{x, y}, {0, 0}, color, 0};
        v[1] = {{x + w, y}, {1, 0}, color, 0};
        v[2] = {{x + w, y + h}, {1, 1}, color, 0};
        v[3] = {{x, y + h}, {0, 1}, color, 0};
        s_Data.VertexBufferPtr += 4;
        s_Data.QuadCount++;
        s_Data.RenderStats.QuadCount++;
    }

    void BatchRenderer2D::DrawTexturedQuad(const glm::vec2& position, const glm::vec2& size,
                                            uint32_t textureID, const glm::vec4& tint,
                                            const glm::vec2& uvMin, const glm::vec2& uvMax) {
        if (s_Data.QuadCount >= MaxQuads) { EndBatch(); BeginBatch(s_Data.CurrentProjection); }

        float texIndex = 0.0f;
        for (int i = 1; i < s_Data.TextureSlotIndex; i++) {
            if (s_Data.TextureSlots[i] == textureID) { texIndex = (float)i; break; }
        }
        if (texIndex == 0.0f && textureID != s_Data.WhiteTexture) {
            if (s_Data.TextureSlotIndex >= MaxTextureSlots) { EndBatch(); BeginBatch(s_Data.CurrentProjection); }
            texIndex = (float)s_Data.TextureSlotIndex;
            s_Data.TextureSlots[s_Data.TextureSlotIndex++] = textureID;
        }

        float x = position.x, y = position.y, w = size.x, h = size.y;
        auto* v = s_Data.VertexBufferPtr;
        v[0] = {{x, y}, uvMin, tint, texIndex};
        v[1] = {{x + w, y}, {uvMax.x, uvMin.y}, tint, texIndex};
        v[2] = {{x + w, y + h}, uvMax, tint, texIndex};
        v[3] = {{x, y + h}, {uvMin.x, uvMax.y}, tint, texIndex};
        s_Data.VertexBufferPtr += 4;
        s_Data.QuadCount++;
        s_Data.RenderStats.QuadCount++;
    }

    BatchRenderer2D::Stats BatchRenderer2D::GetStats() { return s_Data.RenderStats; }
    void BatchRenderer2D::ResetStats() { s_Data.RenderStats = {}; }

}
