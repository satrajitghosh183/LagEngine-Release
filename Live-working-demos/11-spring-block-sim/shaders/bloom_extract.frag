#version 330 core

out vec4 FragColor;
in  vec2 TexCoord;

uniform sampler2D hdrBuffer;
uniform float     threshold;   // default 1.0

void main() {
    vec3 color      = texture(hdrBuffer, TexCoord).rgb;
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > threshold)
        FragColor = vec4(color, 1.0);
    else
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
}
