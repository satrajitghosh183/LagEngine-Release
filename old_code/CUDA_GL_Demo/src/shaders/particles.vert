#version 330 core
layout(location=0) in vec4 aPos;   // xyz, w
layout(location=1) in vec4 aColor; // rgba

out vec4 vColor;

uniform mat4 uViewProj;

void main() {
    vColor = aColor;
    gl_Position = uViewProj * vec4(aPos.xyz, 1.0);
    gl_PointSize = 3.0; // tweak as you like
}
