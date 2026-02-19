#version 330 core
layout(location=0) in vec4 aPos;
layout(location=1) in vec4 aColor;

out vec4 vColor;

uniform mat4 uViewProj;

void main() {
    vColor = aColor;
    gl_Position = uViewProj * vec4(aPos.xyz, 1.0);
    gl_PointSize = 20.0;  // MASSIVE
}
