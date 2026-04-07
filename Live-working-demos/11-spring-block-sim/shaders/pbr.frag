#version 330 core

out vec4 FragColor;

in VS_OUT {
    vec3  WorldPos;
    vec3  Normal;
    vec2  TexCoord;
    vec4  FragPosLightSpace;
} fs_in;

// Material
uniform vec3  albedo;
uniform float metallic;
uniform float roughness;
uniform float ao;          // ambient occlusion (0-1)

// Lights
#define MAX_POINT_LIGHTS 4
uniform vec3 pointLightPositions[MAX_POINT_LIGHTS];
uniform vec3 pointLightColors[MAX_POINT_LIGHTS];
uniform int  numPointLights;

uniform vec3 dirLightDir;
uniform vec3 dirLightColor;

uniform vec3 camPos;

// Shadow map
uniform sampler2D shadowMap;

// Selection highlight
uniform float selectedBrightness; // 0 = normal, 1 = highlighted

const float PI = 3.14159265359;

// ── PCF shadow ───────────────────────────────────────────────────────────────
float shadowCalc(vec4 fragPosLightSpace, vec3 N, vec3 L) {
    vec3 proj = fragPosLightSpace.xyz / fragPosLightSpace.w;
    proj = proj * 0.5 + 0.5;
    if (proj.z > 1.0) return 0.0;

    float bias    = max(0.005 * (1.0 - dot(N, L)), 0.0005);
    float shadow  = 0.0;
    vec2  texelSize = 1.0 / textureSize(shadowMap, 0);

    for (int x = -2; x <= 2; ++x)
    for (int y = -2; y <= 2; ++y) {
        float pcfDepth = texture(shadowMap, proj.xy + vec2(x, y) * texelSize).r;
        shadow += (proj.z - bias > pcfDepth) ? 1.0 : 0.0;
    }
    return shadow / 25.0;
}

// ── Cook-Torrance BRDF helpers ────────────────────────────────────────────────
float DistributionGGX(vec3 N, vec3 H, float rough) {
    float a  = rough * rough;
    float a2 = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float rough) {
    float r = rough + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float rough) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, rough) * GeometrySchlickGGX(NdotL, rough);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ── Evaluate one light contribution ─────────────────────────────────────────
vec3 evalLight(vec3 N, vec3 V, vec3 L, vec3 radiance, vec3 F0) {
    vec3  H     = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0);
    if (NdotL <= 0.0) return vec3(0.0);

    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3  F   = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001;
    vec3  specular    = numerator / denominator;

    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

void main() {
    vec3 N  = normalize(fs_in.Normal);
    vec3 V  = normalize(camPos - fs_in.WorldPos);

    // F0: dialectric = 0.04, metal = albedo
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 Lo = vec3(0.0);

    // Directional light with shadow
    {
        vec3  L        = normalize(-dirLightDir);
        float shadow   = shadowCalc(fs_in.FragPosLightSpace, N, L);
        vec3  radiance = dirLightColor * (1.0 - shadow * 0.85);
        Lo += evalLight(N, V, L, radiance, F0);
    }

    // Point lights
    for (int i = 0; i < numPointLights; ++i) {
        vec3  toLight  = pointLightPositions[i] - fs_in.WorldPos;
        float dist     = length(toLight);
        float atten    = 1.0 / (dist * dist);
        vec3  L        = normalize(toLight);
        vec3  radiance = pointLightColors[i] * atten;
        Lo += evalLight(N, V, L, radiance, F0);
    }

    // Ambient (IBL approximation)
    vec3 ambient = vec3(0.03) * albedo * ao;

    vec3 color = ambient + Lo;

    // Selection highlight: add rim glow
    if (selectedBrightness > 0.0) {
        float rim = 1.0 - max(dot(N, V), 0.0);
        rim = pow(rim, 3.0);
        color += rim * vec3(0.3, 0.7, 1.0) * selectedBrightness * 2.0;
    }

    FragColor = vec4(color, 1.0);
}
