#version 420 core

layout(location = 0) out vec4 FragColor;

in vec3 v_WorldPos;
in vec3 v_Normal;
in vec2 v_TexCoord;
in mat3 v_TBN;

// Camera
uniform vec3 u_CameraPosition;

// Material
struct MaterialProps {
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;
};
uniform MaterialProps u_Material;

// Texture maps and flags
uniform sampler2D u_AlbedoMap;
uniform sampler2D u_NormalMap;
uniform sampler2D u_MetallicMap;
uniform sampler2D u_RoughnessMap;
uniform sampler2D u_AOMap;

uniform int u_HasAlbedoMap;
uniform int u_HasNormalMap;
uniform int u_HasMetallicMap;
uniform int u_HasRoughnessMap;
uniform int u_HasAOMap;

// Ambient light
struct AmbientLight {
    vec3 color;
    float intensity;
};
uniform AmbientLight u_AmbientLight;

// Lights (matching Renderer3D layout)
#define MAX_LIGHTS 16
struct LightInfo {
    int type;       // 0 = directional, 1 = point, 2 = spot
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
    float range;
    float innerCutoff;
    float outerCutoff;
};
uniform LightInfo u_Lights[MAX_LIGHTS];
uniform int u_LightCount;

// Legacy fallback
uniform vec3 u_Color;
uniform vec3 u_LightDir;
uniform vec3 u_LightColor;

// IBL (Image-Based Lighting)
uniform samplerCube u_IrradianceMap;
uniform samplerCube u_PrefilterMap;
uniform sampler2D u_BRDFLUT;
uniform int u_HasIBL;

const float PI = 3.14159265359;

// --- PBR Functions ---

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return a2 / max(denom, 0.0001);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 CalcLightContribution(vec3 L, vec3 radiance, vec3 N, vec3 V, vec3 albedo, float metallic, float roughness) {
    vec3 H = normalize(V + L);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);

    float NdotL = max(dot(N, L), 0.0);
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001;
    vec3 specular = numerator / denominator;

    return (kD * albedo / PI + specular) * radiance * NdotL;
}

void main() {
    // Sample material properties
    vec3 albedo = u_Material.albedo;
    if (u_HasAlbedoMap != 0) {
        albedo *= pow(texture(u_AlbedoMap, v_TexCoord).rgb, vec3(2.2)); // sRGB -> linear
    }

    float metallic = u_Material.metallic;
    if (u_HasMetallicMap != 0) metallic = texture(u_MetallicMap, v_TexCoord).r;

    float roughness = u_Material.roughness;
    if (u_HasRoughnessMap != 0) roughness = texture(u_RoughnessMap, v_TexCoord).r;

    float ao = u_Material.ao;
    if (u_HasAOMap != 0) ao = texture(u_AOMap, v_TexCoord).r;

    // Normal
    vec3 N = normalize(v_Normal);
    if (u_HasNormalMap != 0) {
        vec3 tangentNormal = texture(u_NormalMap, v_TexCoord).rgb * 2.0 - 1.0;
        N = normalize(v_TBN * tangentNormal);
    }

    vec3 V = normalize(u_CameraPosition - v_WorldPos);

    // Accumulate lighting
    vec3 Lo = vec3(0.0);

    for (int i = 0; i < u_LightCount && i < MAX_LIGHTS; i++) {
        vec3 L;
        float attenuation = 1.0;

        if (u_Lights[i].type == 0) {
            // Directional
            L = normalize(-u_Lights[i].direction);
        } else {
            // Point or Spot
            vec3 toLight = u_Lights[i].position - v_WorldPos;
            float distance = length(toLight);
            L = normalize(toLight);
            attenuation = 1.0 / (u_Lights[i].constant + u_Lights[i].linear * distance + u_Lights[i].quadratic * distance * distance);

            if (u_Lights[i].type == 2) {
                // Spot light cone
                float theta = dot(L, normalize(-u_Lights[i].direction));
                float epsilon = u_Lights[i].innerCutoff - u_Lights[i].outerCutoff;
                float spotIntensity = clamp((theta - u_Lights[i].outerCutoff) / max(epsilon, 0.0001), 0.0, 1.0);
                attenuation *= spotIntensity;
            }
        }

        vec3 radiance = u_Lights[i].color * u_Lights[i].intensity * attenuation;
        Lo += CalcLightContribution(L, radiance, N, V, albedo, metallic, roughness);
    }

    // Ambient term with IBL
    vec3 ambient;
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    if (u_HasIBL != 0) {
        // IBL diffuse
        vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
        vec3 kS = F;
        vec3 kD = (1.0 - kS) * (1.0 - metallic);
        vec3 irradiance = texture(u_IrradianceMap, N).rgb;
        vec3 diffuse = irradiance * albedo;

        // IBL specular
        vec3 R = reflect(-V, N);
        const float MAX_REFLECTION_LOD = 4.0;
        vec3 prefilteredColor = textureLod(u_PrefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
        vec2 brdf = texture(u_BRDFLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
        vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

        ambient = (kD * diffuse + specular) * ao;
    } else {
        ambient = u_AmbientLight.color * u_AmbientLight.intensity * albedo * ao;
    }

    vec3 color = ambient + Lo;

    // HDR tonemapping (Reinhard)
    color = color / (color + vec3(1.0));
    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}
