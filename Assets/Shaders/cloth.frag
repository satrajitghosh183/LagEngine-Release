#version 420 core
layout(location = 0) out vec4 FragColor;

in vec3 v_Normal;
in vec3 v_Position;

uniform vec3 u_Color;
uniform vec3 u_LightDir;

void main() {
    vec3 normal = normalize(v_Normal);
    // Two-sided lighting for thin fabric
    float NdotL = dot(normal, -u_LightDir);
    float diff = max(abs(NdotL), 0.3);
    vec3 color = u_Color * diff;
    FragColor = vec4(color, 1.0);
}
