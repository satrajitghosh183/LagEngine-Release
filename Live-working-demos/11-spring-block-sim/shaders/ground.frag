#version 330 core

out vec4 FragColor;

in VS_OUT {
    vec3 WorldPos;
    vec3 Normal;
    vec2 TexCoord;
    vec4 FragPosLightSpace;
} fs_in;

uniform sampler2D shadowMap;
uniform vec3      camPos;
uniform vec3      dirLightDir;
uniform vec3      dirLightColor;

const float PI = 3.14159265359;

// ── Checkerboard pattern ─────────────────────────────────────────────────────
vec3 checkerAlbedo(vec2 uv) {
    float scale = 1.0;
    vec2 grid   = floor(uv * scale);
    float c     = mod(grid.x + grid.y, 2.0);
    return mix(vec3(0.18), vec3(0.28), c);
}

// ── PCF Shadow ───────────────────────────────────────────────────────────────
float shadowCalc(vec4 fragPosLS) {
    vec3 proj = fragPosLS.xyz / fragPosLS.w;
    proj = proj * 0.5 + 0.5;
    if (proj.z > 1.0) return 0.0;
    float bias   = 0.001;
    float shadow = 0.0;
    vec2  ts     = 1.0 / textureSize(shadowMap, 0);
    for (int x = -2; x <= 2; ++x)
    for (int y = -2; y <= 2; ++y) {
        float pcf = texture(shadowMap, proj.xy + vec2(x, y) * ts).r;
        shadow   += (proj.z - bias > pcf) ? 1.0 : 0.0;
    }
    return shadow / 25.0;
}

// ── Simple PBR for ground ────────────────────────────────────────────────────
float DistGGX(vec3 N, vec3 H, float r) {
    float a = r*r; float a2 = a*a;
    float d = max(dot(N,H),0.0); d = d*d;
    float b = d*(a2-1.0)+1.0;
    return a2/(PI*b*b);
}
float GeoSmith(float ndotv, float ndotl, float r) {
    float k = (r+1.0)*(r+1.0)/8.0;
    float gv = ndotv/(ndotv*(1.0-k)+k);
    float gl = ndotl/(ndotl*(1.0-k)+k);
    return gv*gl;
}
vec3 FresnelS(float c, vec3 F0) {
    return F0 + (1.0-F0)*pow(clamp(1.0-c,0.0,1.0),5.0);
}

void main() {
    vec3 N = vec3(0.0, 1.0, 0.0);
    vec3 V = normalize(camPos - fs_in.WorldPos);
    vec3 L = normalize(-dirLightDir);
    vec3 H = normalize(V + L);

    vec3  albedo    = checkerAlbedo(fs_in.WorldPos.xz);
    float roughness = 0.8;
    float metallic  = 0.0;

    vec3  F0        = mix(vec3(0.04), albedo, metallic);
    float shadow    = shadowCalc(fs_in.FragPosLightSpace);
    float NdotL     = max(dot(N, L), 0.0);

    float NDF    = DistGGX(N, H, roughness);
    float G      = GeoSmith(max(dot(N,V),0.0), NdotL, roughness);
    vec3  F      = FresnelS(max(dot(H,V),0.0), F0);
    vec3  spec   = NDF*G*F / (4.0*max(dot(N,V),0.0)*NdotL + 0.0001);
    vec3  kD     = (1.0-F)*(1.0-metallic);
    vec3  diffuse = kD * albedo / PI;

    vec3 radiance = dirLightColor * (1.0 - shadow * 0.85);
    vec3 color    = (diffuse + spec) * radiance * NdotL + vec3(0.02)*albedo;

    FragColor = vec4(color, 1.0);
}
