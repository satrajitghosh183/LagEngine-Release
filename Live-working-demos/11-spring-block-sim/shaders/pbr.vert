#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out VS_OUT {
    vec3  WorldPos;
    vec3  Normal;
    vec2  TexCoord;
    vec4  FragPosLightSpace;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;
uniform mat3 normalMatrix;

void main() {
    vec4 worldPos          = model * vec4(aPos, 1.0);
    vs_out.WorldPos        = worldPos.xyz;
    vs_out.Normal          = normalize(normalMatrix * aNormal);
    vs_out.TexCoord        = aTexCoord;
    vs_out.FragPosLightSpace = lightSpaceMatrix * worldPos;
    gl_Position            = projection * view * worldPos;
}
