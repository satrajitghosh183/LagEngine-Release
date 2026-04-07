#version 420 core

in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D scene;
uniform sampler2D ssao;

void main() {
    vec3 color = texture(scene, TexCoords).rgb;
    
    // Apply SSAO if available (if texture is white, it means SSAO is disabled)
    float ao = texture(ssao, TexCoords).r;
    // Only apply AO if it's not 1.0 (white texture)
    if (ao < 0.99) {
        color *= ao;
    }
    
    FragColor = vec4(color, 1.0);
}

