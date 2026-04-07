#version 330 core

out vec4 FragColor;

void main() {
    // SIMPLIFIED: Just make bright, visible particles
    vec2 coord = gl_PointCoord - vec2(0.5);
    float dist = length(coord);
    
    // Simple circular particle - no complex calculations
    if (dist > 0.5) {
        discard;  // Outside circle
    }
    
    // Bright, SOLID color - NO transparency for maximum visibility
    vec3 color = vec3(1.0, 0.7, 0.3);  // Bright orange/yellow
    
    // Simple fade at edges
    float alpha = 1.0 - smoothstep(0.3, 0.5, dist);
    
    FragColor = vec4(color, alpha);
}

