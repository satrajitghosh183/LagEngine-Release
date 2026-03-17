#version 330 core

out vec4 FragColor;
in  vec2 TexCoord;

uniform sampler2D hdrBuffer;
uniform sampler2D bloomBuffer;
uniform float     exposure;       // default 1.0
uniform float     bloomStrength;  // default 0.15

// ACES filmic tonemapping approximation
vec3 ACESFilm(vec3 x) {
    float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

void main() {
    vec3 hdr   = texture(hdrBuffer,   TexCoord).rgb;
    vec3 bloom = texture(bloomBuffer, TexCoord).rgb;

    // Additive bloom
    hdr += bloom * bloomStrength;

    // Exposure
    hdr *= exposure;

    // ACES tonemap then gamma correct
    vec3 ldr = ACESFilm(hdr);
    ldr      = pow(ldr, vec3(1.0 / 2.2));

    FragColor = vec4(ldr, 1.0);
}
