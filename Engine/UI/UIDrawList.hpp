#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <cstdint>

namespace GameEngine {
namespace UI {

    // Draw commands recorded by widgets, flushed to BatchRenderer2D per frame.
    struct UIDrawCommand {
        enum class Kind : uint8_t {
            Rect,           // Solid or rounded-corner rectangle
            BorderedRect,   // Rect with outline
            TexturedRect,   // Textured (sprite) rectangle
            Text,           // Glyph-by-glyph text render
            Line,           // Thick line segment
            PushScissor,    // Start a clipping rect
            PopScissor      // Pop the last clipping rect
        };

        Kind Type;
        glm::vec4 Rect;         // x, y, w, h (top-left origin)
        glm::vec4 Color;
        glm::vec4 Color2;       // Used for border color etc.
        float CornerRadius = 0.0f;
        float BorderWidth = 0.0f;
        float LineThickness = 1.0f;
        uint32_t TextureID = 0; // For TexturedRect / Text (font atlas)
        glm::vec4 UVRect{0, 0, 1, 1};
        std::string Text;       // For Text commands
        float FontSize = 14.0f;
        std::string FontName;
    };

    class UIDrawList {
    public:
        void Clear() { m_Commands.clear(); }
        const std::vector<UIDrawCommand>& Commands() const { return m_Commands; }

        void AddRect(const glm::vec4& rect, const glm::vec4& color, float cornerRadius = 0.0f) {
            UIDrawCommand c;
            c.Type = UIDrawCommand::Kind::Rect;
            c.Rect = rect;
            c.Color = color;
            c.CornerRadius = cornerRadius;
            m_Commands.push_back(std::move(c));
        }

        void AddBorderedRect(const glm::vec4& rect, const glm::vec4& fill,
                             const glm::vec4& border, float borderWidth = 1.0f,
                             float cornerRadius = 0.0f) {
            UIDrawCommand c;
            c.Type = UIDrawCommand::Kind::BorderedRect;
            c.Rect = rect;
            c.Color = fill;
            c.Color2 = border;
            c.BorderWidth = borderWidth;
            c.CornerRadius = cornerRadius;
            m_Commands.push_back(std::move(c));
        }

        void AddTexturedRect(const glm::vec4& rect, uint32_t textureID,
                             const glm::vec4& tint = glm::vec4(1.0f),
                             const glm::vec4& uv = glm::vec4(0, 0, 1, 1)) {
            UIDrawCommand c;
            c.Type = UIDrawCommand::Kind::TexturedRect;
            c.Rect = rect;
            c.Color = tint;
            c.TextureID = textureID;
            c.UVRect = uv;
            m_Commands.push_back(std::move(c));
        }

        void AddText(const std::string& text, const glm::vec2& pos, float size,
                     const glm::vec4& color, const std::string& fontName = "default") {
            UIDrawCommand c;
            c.Type = UIDrawCommand::Kind::Text;
            c.Rect = { pos.x, pos.y, 0, 0 };
            c.Color = color;
            c.FontSize = size;
            c.Text = text;
            c.FontName = fontName;
            m_Commands.push_back(std::move(c));
        }

        void AddLine(const glm::vec2& a, const glm::vec2& b, const glm::vec4& color,
                     float thickness = 1.0f) {
            UIDrawCommand c;
            c.Type = UIDrawCommand::Kind::Line;
            c.Rect = { a.x, a.y, b.x, b.y };
            c.Color = color;
            c.LineThickness = thickness;
            m_Commands.push_back(std::move(c));
        }

        void PushScissor(const glm::vec4& rect) {
            UIDrawCommand c;
            c.Type = UIDrawCommand::Kind::PushScissor;
            c.Rect = rect;
            m_Commands.push_back(std::move(c));
        }
        void PopScissor() {
            UIDrawCommand c;
            c.Type = UIDrawCommand::Kind::PopScissor;
            m_Commands.push_back(std::move(c));
        }

    private:
        std::vector<UIDrawCommand> m_Commands;
    };

}}
