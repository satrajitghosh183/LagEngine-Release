#version 330 core
in vec4 vColor;
out vec4 FragColor;

void main() {
    // Bright solid square - no circle clipping
    FragColor = vec4(1.0, 0.0, 0.0, 1.0);  // PURE RED, ignore input color
}
