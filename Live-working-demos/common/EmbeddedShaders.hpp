#pragma once

/**
 * Embedded SPIR-V shaders for demos.
 *
 * These are compiled from trivial GLSL shaders:
 *
 * --- basic.vert ---
 * #version 450
 * layout(push_constant) uniform PC { mat4 mvp; } pc;
 * layout(location = 0) in vec3 inPos;
 * layout(location = 1) in vec3 inColor;
 * layout(location = 0) out vec3 fragColor;
 * void main() {
 *     gl_Position = pc.mvp * vec4(inPos, 1.0);
 *     fragColor = inColor;
 * }
 *
 * --- basic.frag ---
 * #version 450
 * layout(location = 0) in vec3 fragColor;
 * layout(location = 0) out vec4 outColor;
 * void main() {
 *     outColor = vec4(fragColor, 1.0);
 * }
 *
 * For production: precompile with glslangValidator and embed the binary.
 * For these demos we compile at runtime via shaderc if available,
 * or provide GLSL source strings that VulkanBase can compile.
 */

#include <string>

namespace vkdemo {

    // Basic vertex shader (position + color, push constant MVP)
    inline const std::string kBasicVertexShader = R"(
#version 450

layout(push_constant) uniform PushConstants {
    mat4 mvp;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = pc.mvp * vec4(inPosition, 1.0);
    fragColor = inColor;
}
)";

    // Basic fragment shader (pass-through color)
    inline const std::string kBasicFragmentShader = R"(
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
)";

    // Point/particle vertex shader (position + color + point size)
    inline const std::string kPointVertexShader = R"(
#version 450

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    float pointSize;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = pc.mvp * vec4(inPosition, 1.0);
    gl_PointSize = pc.pointSize;
    fragColor = inColor;
}
)";

    // Lit vertex shader (position + normal + color, push constant MVP + model)
    inline const std::string kLitVertexShader = R"(
#version 450

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    mat4 model;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragWorldPos;

void main() {
    gl_Position = pc.mvp * vec4(inPosition, 1.0);
    fragWorldPos = vec3(pc.model * vec4(inPosition, 1.0));
    fragNormal = mat3(transpose(inverse(pc.model))) * inNormal;
    fragColor = inColor;
}
)";

    // Lit fragment shader (simple diffuse + ambient)
    inline const std::string kLitFragmentShader = R"(
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 lightDir = normalize(vec3(1.0, 1.0, 0.5));
    vec3 normal = normalize(fragNormal);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 ambient = 0.15 * fragColor;
    vec3 diffuse = diff * fragColor;
    outColor = vec4(ambient + diffuse, 1.0);
}
)";

    // Flat color fragment shader (uniform color via push constant)
    inline const std::string kFlatColorFragmentShader = R"(
#version 450

layout(push_constant) uniform PushConstants {
    layout(offset = 64) vec4 color;
} pc;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = pc.color;
}
)";

    // Line vertex shader (position only, push constant MVP + color)
    inline const std::string kLineVertexShader = R"(
#version 450

layout(push_constant) uniform PushConstants {
    mat4 mvp;
} pc;

layout(location = 0) in vec3 inPosition;

void main() {
    gl_Position = pc.mvp * vec4(inPosition, 1.0);
}
)";

} // namespace vkdemo
