#pragma once

#include "../../Core/Base.hpp"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <cstdint>

namespace GameEngine { namespace RHI {

    enum class BufferUsage : uint8_t { Vertex, Index, Uniform, Storage };
    enum class BufferAccess : uint8_t { Static, Dynamic, Stream };

    enum class TextureFormat : uint8_t {
        R8, RG8, RGB8, RGBA8,
        R16F, RG16F, RGB16F, RGBA16F,
        R32F, RGBA32F,
        Depth24Stencil8, Depth32F
    };

    enum class TextureWrap : uint8_t { Repeat, Clamp, Mirror };
    enum class TextureFilter : uint8_t { Nearest, Linear, Trilinear };
    enum class PrimitiveTopology : uint8_t { Triangles, TriangleStrip, Lines, LineStrip, Points };

    enum class BlendFactor : uint8_t { Zero, One, SrcAlpha, OneMinusSrcAlpha, DstAlpha };
    enum class BlendOp : uint8_t { Add, Subtract, ReverseSubtract };
    enum class CompareOp : uint8_t { Never, Less, Equal, LessEqual, Greater, NotEqual, GreaterEqual, Always };
    enum class CullMode : uint8_t { None, Front, Back };
    enum class FillMode : uint8_t { Solid, Wireframe };
    enum class ShaderStage : uint8_t { Vertex, Fragment, Geometry, Compute };

    enum class VertexAttributeFormat : uint8_t { Float, Float2, Float3, Float4, Int, UInt };

    struct VertexAttribute {
        std::string Name;
        VertexAttributeFormat Format = VertexAttributeFormat::Float3;
        uint32_t Offset = 0;
        uint32_t Location = 0;
    };

    struct VertexLayout {
        std::vector<VertexAttribute> Attributes;
        uint32_t Stride = 0;
    };

    struct Viewport {
        float X = 0, Y = 0, Width = 0, Height = 0;
        float MinDepth = 0.0f, MaxDepth = 1.0f;
    };

    struct ScissorRect {
        int X = 0, Y = 0, Width = 0, Height = 0;
    };

    struct BlendState {
        bool Enabled = false;
        BlendFactor SrcColor = BlendFactor::SrcAlpha;
        BlendFactor DstColor = BlendFactor::OneMinusSrcAlpha;
        BlendOp ColorOp = BlendOp::Add;
        BlendFactor SrcAlpha = BlendFactor::One;
        BlendFactor DstAlpha = BlendFactor::Zero;
        BlendOp AlphaOp = BlendOp::Add;
    };

    struct DepthState {
        bool TestEnabled = true;
        bool WriteEnabled = true;
        CompareOp CompareFunc = CompareOp::Less;
    };

    struct RasterizerState {
        CullMode Culling = CullMode::Back;
        FillMode Fill = FillMode::Solid;
        bool ScissorTest = false;
    };

    inline uint32_t getFormatSize(VertexAttributeFormat fmt) {
        switch (fmt) {
            case VertexAttributeFormat::Float:  return 4;
            case VertexAttributeFormat::Float2: return 8;
            case VertexAttributeFormat::Float3: return 12;
            case VertexAttributeFormat::Float4: return 16;
            case VertexAttributeFormat::Int:    return 4;
            case VertexAttributeFormat::UInt:   return 4;
        }
        return 0;
    }

}} // namespace GameEngine::RHI
