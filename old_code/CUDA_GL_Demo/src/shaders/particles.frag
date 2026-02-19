#version 330 core
in vec4 vColor;
out vec4 FragColor;

void main() {
    // round point sprite
    vec2 c = gl_PointCoord*2.0 - 1.0;
    float r2 = dot(c,c);
    if (r2 > 1.0) discard;
    float alpha = smoothstep(1.0, 0.6, r2);
    FragColor = vec4(vColor.rgb, alpha * vColor.a);
}
