#version 420 core
layout(location = 0) out vec4 FragColor;

in vec3 v_Normal;
in vec3 v_Position;

uniform vec3 u_Color;
uniform vec3 u_LightDir;
uniform vec3 u_LightColor;

void main() {
    vec3 normal = normalize(v_Normal);
    float diff = max(dot(normal, -u_LightDir), 0.2);
    vec3 color = u_Color * diff * u_LightColor;
    FragColor = vec4(color, 1.0);
}
