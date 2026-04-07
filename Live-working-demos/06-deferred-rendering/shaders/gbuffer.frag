#version 420 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedo;

void main() {
    gPosition = FragPos;
    gNormal = normalize(Normal);
    
    // Simple color based on position for demo
    float pattern = sin(FragPos.x * 2.0) * sin(FragPos.z * 2.0);
    gAlbedo = vec4(0.7 + pattern * 0.3, 0.6 + pattern * 0.2, 0.8 + pattern * 0.2, 1.0);
}

