#version 330 core

layout(location = 0) in vec4 aParticle; // x, y, z, size

uniform mat4 viewProjMatrix;

void main() {
    vec3 position = aParticle.xyz;
    gl_Position = viewProjMatrix * vec4(position, 1.0);
    
    // Make particles MUCH bigger - fixed large size for visibility
    // Use a large fixed size so particles are definitely visible
    gl_PointSize = 80.0;  // Large fixed size - particles MUST be visible
}

