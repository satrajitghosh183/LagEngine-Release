#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform float roughness;
uniform float metallic;

void main() {
    // Normalize inputs
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir = normalize(vec3(0.0, 0.0, 1.0) - FragPos);

    // Ambient lighting
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * lightColor;

    // Wrapped diffuse lighting (fabric scattering)
    float wrap = 0.3;
    float wrappedDiffuse = max(0.0, (dot(norm, lightDir) + wrap) / (1.0 + wrap));
    vec3 diffuse = wrappedDiffuse * lightColor;

    // Specular microfacets (GGX approximation)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float roughnessSq = roughness * roughness;
    float NdotH = max(dot(norm, halfwayDir), 0.0);
    float denom = NdotH * NdotH * (roughnessSq - 1.0) + 1.0;
    float D = roughnessSq / (3.14159 * denom * denom);

    float specularStrength = 0.3 * (1.0 - roughness);
    vec3 specular = specularStrength * D * lightColor;

    // Fake weave pattern (procedural noise)
    float noise = fract(sin(dot(FragPos.xy, vec2(12.9898, 78.233))) * 43758.5453);
    float weavePattern = noise * 0.05;

    // Ambient occlusion based on normal orientation
    float ao = 0.8 + 0.2 * max(0.0, dot(norm, vec3(0.0, 1.0, 0.0)));

    // Rim lighting for cloth edges
    float rim = pow(1.0 - max(dot(viewDir, norm), 0.0), 3.0) * 0.3;

    // Combine everything
    vec3 finalColor = (ambient + diffuse * ao + specular + rim * lightColor) * (objectColor + weavePattern);

    // Subtle fold darkening
    float foldShadow = 1.0 - pow(1.0 - abs(dot(norm, vec3(0.0, 1.0, 0.0))), 2.0) * 0.2;
    finalColor *= foldShadow;

    FragColor = vec4(finalColor, 1.0);
}
