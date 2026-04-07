#version 410 core

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;

uniform sampler2D uBlendMap;    // texture unit 0
uniform sampler2D uTexLayer0;   // texture unit 1
uniform sampler2D uTexLayer1;   // texture unit 2
uniform sampler2D uTexLayer2;   // texture unit 3
uniform sampler2D uTexLayer3;   // texture unit 4

uniform vec4 uTextureScales;    // scale for each layer
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbient;
uniform vec3 uCameraPos;

out vec4 FragColor;

void main() {
    vec3 normal = normalize(vNormal);

    // Sample blend map
    vec4 blend = texture(uBlendMap, vTexCoord);
    float backAmount = 1.0 - (blend.r + blend.g + blend.b);
    backAmount = max(backAmount, 0.0);

    // Sample each texture layer with its own scale
    vec3 tex0 = texture(uTexLayer0, vTexCoord * uTextureScales.x).rgb;
    vec3 tex1 = texture(uTexLayer1, vTexCoord * uTextureScales.y).rgb;
    vec3 tex2 = texture(uTexLayer2, vTexCoord * uTextureScales.z).rgb;
    vec3 tex3 = texture(uTexLayer3, vTexCoord * uTextureScales.w).rgb;

    // Blend textures
    vec3 albedo = tex0 * backAmount + tex1 * blend.r + tex2 * blend.g + tex3 * blend.b;

    // Blinn-Phong lighting
    vec3 lightDir = normalize(-uLightDir);
    float diff = max(dot(normal, lightDir), 0.0);

    vec3 viewDir = normalize(uCameraPos - vWorldPos);
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);

    vec3 ambient = uAmbient * albedo;
    vec3 diffuse = diff * uLightColor * albedo;
    vec3 specular = spec * uLightColor * 0.2;

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}
