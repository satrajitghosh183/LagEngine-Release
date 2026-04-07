#version 330 core

out vec4 FragColor;
in  vec2 TexCoord;

uniform sampler2D image;
uniform bool      horizontal;

// 9-tap Gaussian weights
const float weight[5] = float[](0.227027, 0.194595, 0.121622, 0.054054, 0.016216);

void main() {
    vec2 texOffset = 1.0 / textureSize(image, 0);
    vec3 result    = texture(image, TexCoord).rgb * weight[0];

    if (horizontal) {
        for (int i = 1; i < 5; ++i) {
            result += texture(image, TexCoord + vec2(texOffset.x * i, 0.0)).rgb * weight[i];
            result += texture(image, TexCoord - vec2(texOffset.x * i, 0.0)).rgb * weight[i];
        }
    } else {
        for (int i = 1; i < 5; ++i) {
            result += texture(image, TexCoord + vec2(0.0, texOffset.y * i)).rgb * weight[i];
            result += texture(image, TexCoord - vec2(0.0, texOffset.y * i)).rgb * weight[i];
        }
    }
    FragColor = vec4(result, 1.0);
}
