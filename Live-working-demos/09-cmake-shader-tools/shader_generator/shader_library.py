"""
Shader Library
==============

A curated collection of working GLSL shaders organized by category.
Used for finding similar shaders and providing examples to the LLM.
"""

from dataclasses import dataclass
from typing import List, Dict, Optional
import re


@dataclass
class ShaderExample:
    """A shader example with metadata."""
    name: str
    description: str
    tags: List[str]
    vertex: str
    fragment: str
    

# Standard vertex shader used by most examples
STANDARD_VERTEX = """#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;

out vec2 TexCoord;
out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    TexCoord = aTexCoord;
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(model) * aNormal;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
"""

SIMPLE_VERTEX = """#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    TexCoord = aTexCoord;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
"""


# ============================================================================
# SHADER LIBRARY - Curated working examples
# ============================================================================

SHADER_LIBRARY: List[ShaderExample] = [
    
    # -------------------------------------------------------------------------
    # PATTERNS
    # -------------------------------------------------------------------------
    
    ShaderExample(
        name="Polka Dots",
        description="Red polka dots on white background",
        tags=["pattern", "dots", "polka", "circles", "procedural"],
        vertex=SIMPLE_VERTEX,
        fragment="""#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

void main() {
    vec2 uv = TexCoord * 8.0;
    vec2 local = fract(uv) - 0.5;
    float dist = length(local);
    float circle = 1.0 - smoothstep(0.2, 0.25, dist);
    vec3 color = mix(vec3(0.95), vec3(1.0, 0.2, 0.2), circle);
    FragColor = vec4(color, 1.0);
}
"""
    ),
    
    ShaderExample(
        name="Checkerboard",
        description="Black and white checkerboard pattern",
        tags=["pattern", "checker", "checkerboard", "grid", "tiles", "procedural"],
        vertex=SIMPLE_VERTEX,
        fragment="""#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

void main() {
    vec2 uv = TexCoord * 8.0;
    float checker = mod(floor(uv.x) + floor(uv.y), 2.0);
    vec3 color = mix(vec3(0.1), vec3(0.9), checker);
    FragColor = vec4(color, 1.0);
}
"""
    ),
    
    ShaderExample(
        name="Stripes",
        description="Animated diagonal stripes",
        tags=["pattern", "stripes", "lines", "diagonal", "animated"],
        vertex=SIMPLE_VERTEX,
        fragment="""#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform float time;

void main() {
    float stripe = sin((TexCoord.x + TexCoord.y) * 20.0 - time * 2.0) * 0.5 + 0.5;
    stripe = step(0.5, stripe);
    vec3 color = mix(vec3(0.2, 0.4, 0.8), vec3(1.0, 0.9, 0.3), stripe);
    FragColor = vec4(color, 1.0);
}
"""
    ),
    
    ShaderExample(
        name="Hexagonal Grid",
        description="Hexagonal honeycomb pattern",
        tags=["pattern", "hexagon", "honeycomb", "grid", "procedural"],
        vertex=SIMPLE_VERTEX,
        fragment="""#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

void main() {
    vec2 uv = TexCoord * 6.0;
    vec2 r = vec2(1.0, 1.73205);
    vec2 h = r * 0.5;
    vec2 a = mod(uv, r) - h;
    vec2 b = mod(uv - h, r) - h;
    vec2 gv = length(a) < length(b) ? a : b;
    float d = max(abs(gv.x), abs(gv.y * 0.866 + abs(gv.x) * 0.5));
    float hex = smoothstep(0.4, 0.42, d);
    vec3 color = mix(vec3(1.0, 0.8, 0.2), vec3(0.2, 0.15, 0.1), hex);
    FragColor = vec4(color, 1.0);
}
"""
    ),
    
    ShaderExample(
        name="Brick Wall",
        description="Procedural brick wall pattern",
        tags=["pattern", "brick", "wall", "procedural", "texture"],
        vertex=SIMPLE_VERTEX,
        fragment="""#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

void main() {
    vec2 uv = TexCoord * vec2(4.0, 8.0);
    float row = floor(uv.y);
    uv.x += mod(row, 2.0) * 0.5;
    vec2 brick = fract(uv);
    float mortar = step(0.05, brick.x) * step(0.05, brick.y);
    mortar *= step(brick.x, 0.95) * step(brick.y, 0.9);
    vec3 brickColor = vec3(0.7, 0.25, 0.15);
    vec3 mortarColor = vec3(0.8, 0.8, 0.75);
    vec3 color = mix(mortarColor, brickColor, mortar);
    FragColor = vec4(color, 1.0);
}
"""
    ),
    
    ShaderExample(
        name="Concentric Circles",
        description="Animated concentric rings",
        tags=["pattern", "circles", "rings", "concentric", "animated", "ripple"],
        vertex=SIMPLE_VERTEX,
        fragment="""#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform float time;

void main() {
    vec2 uv = TexCoord - 0.5;
    float dist = length(uv);
    float rings = sin(dist * 30.0 - time * 3.0) * 0.5 + 0.5;
    vec3 color = mix(vec3(0.1, 0.2, 0.4), vec3(0.4, 0.8, 1.0), rings);
    FragColor = vec4(color, 1.0);
}
"""
    ),
    
    # -------------------------------------------------------------------------
    # ANIMATED / PSYCHEDELIC
    # -------------------------------------------------------------------------
    
    ShaderExample(
        name="Plasma",
        description="Classic animated plasma effect with swirling colors",
        tags=["animated", "plasma", "psychedelic", "colorful", "swirl"],
        vertex=SIMPLE_VERTEX,
        fragment="""#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform float time;

void main() {
    vec2 uv = TexCoord * 4.0;
    float v = sin(uv.x + time);
    v += sin(uv.y + time * 0.5);
    v += sin(uv.x + uv.y + time * 0.3);
    v += sin(length(uv - vec2(2.0)) * 2.0 + time);
    v *= 0.25;
    vec3 color = vec3(
        sin(v * 3.14159 + time) * 0.5 + 0.5,
        sin(v * 3.14159 + time + 2.094) * 0.5 + 0.5,
        sin(v * 3.14159 + time + 4.188) * 0.5 + 0.5
    );
    FragColor = vec4(color, 1.0);
}
"""
    ),
    
    ShaderExample(
        name="Rainbow Spiral",
        description="Rotating rainbow spiral pattern",
        tags=["animated", "rainbow", "spiral", "colorful", "rotation", "psychedelic"],
        vertex=SIMPLE_VERTEX,
        fragment="""#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform float time;

vec3 hsv2rgb(vec3 c) {
    vec3 p = abs(fract(c.xxx + vec3(0.0, 2.0/3.0, 1.0/3.0)) * 6.0 - 3.0);
    return c.z * mix(vec3(1.0), clamp(p - 1.0, 0.0, 1.0), c.y);
}

void main() {
    vec2 uv = TexCoord - 0.5;
    float angle = atan(uv.y, uv.x);
    float radius = length(uv);
    float hue = fract((angle / 6.28318) + radius * 3.0 - time * 0.3);
    vec3 color = hsv2rgb(vec3(hue, 0.8, 0.9));
    FragColor = vec4(color, 1.0);
}
"""
    ),
    
    ShaderExample(
        name="Kaleidoscope",
        description="Animated kaleidoscope pattern",
        tags=["animated", "kaleidoscope", "psychedelic", "mirror", "symmetry"],
        vertex=SIMPLE_VERTEX,
        fragment="""#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform float time;

void main() {
    vec2 uv = TexCoord - 0.5;
    float angle = atan(uv.y, uv.x);
    float segments = 6.0;
    angle = mod(angle, 6.28318 / segments);
    angle = abs(angle - 3.14159 / segments);
    float r = length(uv);
    vec2 p = vec2(cos(angle), sin(angle)) * r;
    float pattern = sin(p.x * 20.0 + time) * sin(p.y * 20.0 + time * 0.7);
    vec3 color = vec3(
        sin(pattern * 3.0 + time) * 0.5 + 0.5,
        sin(pattern * 3.0 + time + 2.0) * 0.5 + 0.5,
        sin(pattern * 3.0 + time + 4.0) * 0.5 + 0.5
    );
    FragColor = vec4(color, 1.0);
}
"""
    ),
    
    ShaderExample(
        name="Lava Lamp",
        description="Blobby lava lamp effect",
        tags=["animated", "lava", "blob", "organic", "metaball"],
        vertex=SIMPLE_VERTEX,
        fragment="""#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform float time;

void main() {
    vec2 uv = TexCoord;
    float v = 0.0;
    for(int i = 0; i < 5; i++) {
        float fi = float(i);
        vec2 center = vec2(
            0.5 + 0.3 * sin(time * 0.5 + fi * 1.3),
            0.5 + 0.3 * cos(time * 0.4 + fi * 1.7)
        );
        float d = length(uv - center);
        v += 0.1 / (d + 0.05);
    }
    v = smoothstep(1.5, 2.0, v);
    vec3 color = mix(vec3(0.8, 0.2, 0.1), vec3(1.0, 0.8, 0.2), v);
    FragColor = vec4(color, 1.0);
}
"""
    ),
    
    # -------------------------------------------------------------------------
    # LIGHTING / 3D EFFECTS
    # -------------------------------------------------------------------------
    
    ShaderExample(
        name="Fresnel Glow",
        description="Edge glow effect using fresnel",
        tags=["lighting", "fresnel", "glow", "edge", "rim", "3d"],
        vertex=STANDARD_VERTEX,
        fragment="""#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;
uniform vec3 cameraPos;
uniform float time;

void main() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(cameraPos - FragPos);
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    
    float diff = max(dot(norm, lightDir), 0.0);
    float fresnel = pow(1.0 - max(dot(viewDir, norm), 0.0), 3.0);
    
    vec3 baseColor = vec3(0.2, 0.4, 0.8);
    vec3 color = baseColor * (0.3 + diff * 0.7);
    color += vec3(0.4, 0.8, 1.0) * fresnel;
    
    FragColor = vec4(color, 1.0);
}
"""
    ),
    
    ShaderExample(
        name="Toon Shading",
        description="Cartoon cel-shading effect",
        tags=["lighting", "toon", "cel", "cartoon", "anime", "3d"],
        vertex=STANDARD_VERTEX,
        fragment="""#version 330 core
out vec4 FragColor;
in vec3 FragPos;
in vec3 Normal;
uniform vec3 cameraPos;

void main() {
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(vec3(1.0, 1.0, 0.5));
    vec3 viewDir = normalize(cameraPos - FragPos);
    
    float diff = max(dot(norm, lightDir), 0.0);
    float bands = floor(diff * 4.0) / 4.0;
    bands = max(bands, 0.2);
    
    float rim = 1.0 - max(dot(viewDir, norm), 0.0);
    rim = smoothstep(0.6, 1.0, rim);
    
    vec3 color = vec3(0.9, 0.4, 0.3) * bands;
    color += vec3(1.0) * rim * 0.3;
    
    FragColor = vec4(color, 1.0);
}
"""
    ),
    
    ShaderExample(
        name="Phong Lighting",
        description="Classic Phong lighting with specular",
        tags=["lighting", "phong", "specular", "diffuse", "3d", "realistic"],
        vertex=STANDARD_VERTEX,
        fragment="""#version 330 core
out vec4 FragColor;
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
uniform vec3 cameraPos;

void main() {
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(vec3(2.0, 3.0, 2.0) - FragPos);
    vec3 viewDir = normalize(cameraPos - FragPos);
    vec3 halfDir = normalize(lightDir + viewDir);
    
    float ambient = 0.15;
    float diff = max(dot(norm, lightDir), 0.0);
    float spec = pow(max(dot(norm, halfDir), 0.0), 32.0);
    
    vec3 baseColor = vec3(0.7, 0.3, 0.2);
    vec3 color = baseColor * (ambient + diff * 0.7) + vec3(1.0) * spec * 0.5;
    
    FragColor = vec4(color, 1.0);
}
"""
    ),
    
    ShaderExample(
        name="Hologram",
        description="Sci-fi hologram effect with scan lines",
        tags=["lighting", "hologram", "scifi", "scanlines", "futuristic", "glow"],
        vertex=STANDARD_VERTEX,
        fragment="""#version 330 core
out vec4 FragColor;
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
uniform vec3 cameraPos;
uniform float time;

void main() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(cameraPos - FragPos);
    
    float fresnel = pow(1.0 - max(dot(viewDir, norm), 0.0), 2.0);
    float scanline = sin(FragPos.y * 50.0 + time * 5.0) * 0.5 + 0.5;
    scanline = smoothstep(0.3, 0.7, scanline);
    
    float flicker = sin(time * 20.0) * 0.1 + 0.9;
    
    vec3 color = vec3(0.2, 0.8, 1.0) * (fresnel + 0.2) * scanline * flicker;
    float alpha = fresnel * 0.8 + 0.2;
    
    FragColor = vec4(color, alpha);
}
"""
    ),
    
    # -------------------------------------------------------------------------
    # NATURE / ELEMENTS
    # -------------------------------------------------------------------------
    
    ShaderExample(
        name="Fire",
        description="Animated fire effect",
        tags=["nature", "fire", "flame", "animated", "procedural", "noise"],
        vertex=SIMPLE_VERTEX,
        fragment="""#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform float time;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(hash(i), hash(i + vec2(1.0, 0.0)), f.x),
        mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), f.x),
        f.y
    );
}

void main() {
    vec2 uv = TexCoord;
    uv.y = 1.0 - uv.y;
    
    float n = 0.0;
    n += noise(uv * 8.0 + vec2(0.0, time * 2.0)) * 0.5;
    n += noise(uv * 16.0 + vec2(0.0, time * 3.0)) * 0.25;
    n += noise(uv * 32.0 + vec2(0.0, time * 4.0)) * 0.125;
    
    float fire = n - uv.y * 0.7;
    fire = clamp(fire * 2.5, 0.0, 1.0);
    
    vec3 color = mix(vec3(1.0, 0.0, 0.0), vec3(1.0, 1.0, 0.0), fire);
    color = mix(vec3(0.0), color, smoothstep(0.0, 0.3, fire));
    
    FragColor = vec4(color, 1.0);
}
"""
    ),
    
    ShaderExample(
        name="Water",
        description="Animated water surface with waves",
        tags=["nature", "water", "waves", "ocean", "animated", "3d"],
        vertex="""#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;

out vec2 TexCoord;
out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time;

void main() {
    vec3 pos = aPos;
    pos.y += sin(pos.x * 4.0 + time * 2.0) * 0.05;
    pos.y += sin(pos.z * 3.0 + time * 1.5) * 0.05;
    
    TexCoord = aTexCoord;
    FragPos = vec3(model * vec4(pos, 1.0));
    Normal = mat3(model) * aNormal;
    gl_Position = projection * view * model * vec4(pos, 1.0);
}
""",
        fragment="""#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;
uniform vec3 cameraPos;
uniform float time;

void main() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(cameraPos - FragPos);
    vec3 lightDir = normalize(vec3(1.0, 1.0, 0.5));
    
    float fresnel = pow(1.0 - max(dot(viewDir, norm), 0.0), 3.0);
    float spec = pow(max(dot(reflect(-lightDir, norm), viewDir), 0.0), 64.0);
    
    vec3 deepColor = vec3(0.0, 0.2, 0.4);
    vec3 shallowColor = vec3(0.0, 0.5, 0.7);
    vec3 waterColor = mix(deepColor, shallowColor, fresnel);
    
    vec3 color = waterColor + vec3(1.0) * spec * 0.6;
    
    FragColor = vec4(color, 0.9);
}
"""
    ),
    
    ShaderExample(
        name="Clouds",
        description="Animated procedural clouds",
        tags=["nature", "clouds", "sky", "animated", "noise", "procedural"],
        vertex=SIMPLE_VERTEX,
        fragment="""#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform float time;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(hash(i), hash(i + vec2(1.0, 0.0)), f.x),
        mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), f.x),
        f.y
    );
}

float fbm(vec2 p) {
    float v = 0.0;
    float a = 0.5;
    for(int i = 0; i < 5; i++) {
        v += a * noise(p);
        p *= 2.0;
        a *= 0.5;
    }
    return v;
}

void main() {
    vec2 uv = TexCoord + vec2(time * 0.02, 0.0);
    float clouds = fbm(uv * 3.0);
    clouds = smoothstep(0.4, 0.6, clouds);
    
    vec3 skyColor = vec3(0.4, 0.6, 0.9);
    vec3 cloudColor = vec3(1.0);
    vec3 color = mix(skyColor, cloudColor, clouds);
    
    FragColor = vec4(color, 1.0);
}
"""
    ),
    
    # -------------------------------------------------------------------------
    # RETRO / GLITCH
    # -------------------------------------------------------------------------
    
    ShaderExample(
        name="CRT Screen",
        description="Old CRT TV screen effect with scanlines",
        tags=["retro", "crt", "scanlines", "tv", "vintage", "screen"],
        vertex=SIMPLE_VERTEX,
        fragment="""#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform float time;
uniform sampler2D texture0;

void main() {
    vec2 uv = TexCoord;
    
    // Slight barrel distortion
    vec2 dc = uv - 0.5;
    uv = uv + dc * dot(dc, dc) * 0.1;
    
    vec3 color = texture(texture0, uv).rgb;
    
    // Scanlines
    float scanline = sin(uv.y * 400.0) * 0.1;
    color -= scanline;
    
    // Vignette
    float vignette = 1.0 - dot(dc, dc) * 1.5;
    color *= vignette;
    
    // Slight color separation
    color.r = texture(texture0, uv + vec2(0.002, 0.0)).r;
    color.b = texture(texture0, uv - vec2(0.002, 0.0)).b;
    
    // Flicker
    color *= 0.95 + 0.05 * sin(time * 10.0);
    
    FragColor = vec4(color, 1.0);
}
"""
    ),
    
    ShaderExample(
        name="Glitch",
        description="Digital glitch effect with RGB split",
        tags=["retro", "glitch", "rgb", "split", "digital", "distortion"],
        vertex=SIMPLE_VERTEX,
        fragment="""#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform float time;

float rand(float n) {
    return fract(sin(n) * 43758.5453);
}

void main() {
    vec2 uv = TexCoord;
    
    // Random horizontal offset
    float glitchStrength = step(0.95, rand(floor(time * 10.0)));
    float offset = (rand(floor(uv.y * 20.0) + time) - 0.5) * 0.1 * glitchStrength;
    
    // RGB split
    float r = sin((uv.x + offset) * 10.0 + time) * 0.5 + 0.5;
    float g = sin(uv.x * 10.0 + time + 2.0) * 0.5 + 0.5;
    float b = sin((uv.x - offset) * 10.0 + time + 4.0) * 0.5 + 0.5;
    
    // Scanline glitch
    float scanline = step(0.98, rand(floor(uv.y * 100.0) + floor(time * 5.0)));
    
    vec3 color = vec3(r, g, b);
    color = mix(color, vec3(1.0), scanline * glitchStrength);
    
    FragColor = vec4(color, 1.0);
}
"""
    ),
    
    ShaderExample(
        name="Matrix Rain",
        description="Matrix-style falling code effect",
        tags=["retro", "matrix", "rain", "code", "digital", "green"],
        vertex=SIMPLE_VERTEX,
        fragment="""#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform float time;

float rand(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    vec2 uv = TexCoord;
    uv.x = floor(uv.x * 40.0) / 40.0;
    
    float speed = rand(vec2(uv.x, 0.0)) * 0.5 + 0.5;
    float offset = rand(vec2(uv.x, 1.0));
    
    float y = fract(uv.y + time * speed + offset);
    float brightness = pow(y, 8.0);
    
    // Add some variation
    float flicker = step(0.98, rand(vec2(uv.x, floor(time * 10.0))));
    brightness = max(brightness, flicker);
    
    vec3 color = vec3(0.2, 1.0, 0.3) * brightness;
    
    FragColor = vec4(color, 1.0);
}
"""
    ),
    
    # -------------------------------------------------------------------------
    # GRADIENTS / SIMPLE
    # -------------------------------------------------------------------------
    
    ShaderExample(
        name="Gradient",
        description="Simple animated color gradient",
        tags=["simple", "gradient", "color", "animated", "basic"],
        vertex=SIMPLE_VERTEX,
        fragment="""#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform float time;

void main() {
    float t = sin(time) * 0.5 + 0.5;
    vec3 color1 = vec3(0.2, 0.4, 0.8);
    vec3 color2 = vec3(0.8, 0.2, 0.6);
    vec3 color = mix(color1, color2, TexCoord.x + t * 0.3);
    color *= 0.7 + 0.3 * TexCoord.y;
    FragColor = vec4(color, 1.0);
}
"""
    ),
    
    ShaderExample(
        name="Solid Color",
        description="Simple solid color",
        tags=["simple", "solid", "color", "basic", "red", "blue", "green"],
        vertex=SIMPLE_VERTEX,
        fragment="""#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

void main() {
    FragColor = vec4(0.8, 0.2, 0.3, 1.0);
}
"""
    ),
    
    ShaderExample(
        name="Pulsing Glow",
        description="Pulsing glow effect",
        tags=["animated", "pulse", "glow", "breathing", "simple"],
        vertex=STANDARD_VERTEX,
        fragment="""#version 330 core
out vec4 FragColor;
in vec3 FragPos;
in vec3 Normal;
uniform vec3 cameraPos;
uniform float time;

void main() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(cameraPos - FragPos);
    
    float fresnel = pow(1.0 - max(dot(viewDir, norm), 0.0), 2.0);
    float pulse = sin(time * 3.0) * 0.5 + 0.5;
    
    vec3 glowColor = vec3(0.2, 0.8, 1.0) * (0.5 + pulse * 0.5);
    vec3 color = glowColor * fresnel + vec3(0.1, 0.2, 0.3) * (1.0 - fresnel);
    
    FragColor = vec4(color, 1.0);
}
"""
    ),
    
    # -------------------------------------------------------------------------
    # FRACTALS / MATH
    # -------------------------------------------------------------------------
    
    ShaderExample(
        name="Mandelbrot Zoom",
        description="Mandelbrot fractal with animated zoom and smooth coloring",
        tags=["fractal", "mandelbrot", "math", "complex", "zoom", "animated", "resolution"],
        vertex=SIMPLE_VERTEX,
        fragment="""#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform float time;
uniform vec2 resolution;

void main() {
    float aspect = resolution.x / resolution.y;
    vec2 uv = TexCoord - 0.5;
    uv.x *= aspect;
    float zoom = 0.5 + 0.3 * sin(time * 0.5);
    vec2 c = uv * (2.0 + zoom * 4.0) - vec2(0.5, 0.0);
    vec2 z = vec2(0.0);
    
    int maxIter = 100;
    int iter = 0;
    for(int i = 0; i < 100; i++) {
        if(dot(z, z) > 4.0) break;
        z = vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + c;
        iter = i + 1;
    }
    
    float t = float(iter) / 100.0;
    float smoothT = t - log2(log2(dot(z, z))) * 0.25;
    vec3 color = vec3(
        sin(smoothT * 6.28 + time) * 0.5 + 0.5,
        sin(smoothT * 6.28 + time + 2.0) * 0.5 + 0.5,
        sin(smoothT * 6.28 + time + 4.0) * 0.5 + 0.5
    );
    if(iter >= 99) color = vec3(0.0);
    FragColor = vec4(color, 1.0);
}
"""
    ),
    
    ShaderExample(
        name="Mandelbrot",
        description="Mandelbrot fractal set",
        tags=["fractal", "mandelbrot", "math", "complex"],
        vertex=SIMPLE_VERTEX,
        fragment="""#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform float time;

void main() {
    vec2 c = (TexCoord - 0.5) * 3.0 - vec2(0.5, 0.0);
    vec2 z = vec2(0.0);
    
    int maxIter = 100;
    int iter = 0;
    
    for(int i = 0; i < 100; i++) {
        if(dot(z, z) > 4.0) break;
        z = vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + c;
        iter++;
    }
    
    float t = float(iter) / float(maxIter);
    vec3 color = vec3(
        sin(t * 6.28 + time) * 0.5 + 0.5,
        sin(t * 6.28 + time + 2.0) * 0.5 + 0.5,
        sin(t * 6.28 + time + 4.0) * 0.5 + 0.5
    );
    
    if(iter == maxIter) color = vec3(0.0);
    
    FragColor = vec4(color, 1.0);
}
"""
    ),
    
    ShaderExample(
        name="Voronoi",
        description="Voronoi cell pattern",
        tags=["fractal", "voronoi", "cells", "procedural", "pattern"],
        vertex=SIMPLE_VERTEX,
        fragment="""#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform float time;

vec2 random2(vec2 p) {
    return fract(sin(vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)))) * 43758.5453);
}

void main() {
    vec2 uv = TexCoord * 5.0;
    vec2 i_uv = floor(uv);
    vec2 f_uv = fract(uv);
    
    float minDist = 1.0;
    vec2 minPoint;
    
    for(int y = -1; y <= 1; y++) {
        for(int x = -1; x <= 1; x++) {
            vec2 neighbor = vec2(float(x), float(y));
            vec2 point = random2(i_uv + neighbor);
            point = 0.5 + 0.5 * sin(time + 6.28 * point);
            float dist = length(neighbor + point - f_uv);
            if(dist < minDist) {
                minDist = dist;
                minPoint = point;
            }
        }
    }
    
    vec3 color = vec3(minPoint.x, minPoint.y, 1.0 - minDist);
    
    FragColor = vec4(color, 1.0);
}
"""
    ),
]


# ============================================================================
# SEARCH FUNCTIONS
# ============================================================================

def search_shaders(query: str, limit: int = 3) -> List[ShaderExample]:
    """
    Search for shaders matching the query.
    
    Args:
        query: Search query (natural language)
        limit: Maximum number of results
        
    Returns:
        List of matching ShaderExample objects
    """
    query_lower = query.lower()
    query_words = set(re.findall(r'\w+', query_lower))
    
    scored = []
    for shader in SHADER_LIBRARY:
        score = 0
        
        # Check name match
        if query_lower in shader.name.lower():
            score += 10
        
        # Check description match
        desc_lower = shader.description.lower()
        for word in query_words:
            if word in desc_lower:
                score += 3
        
        # Check tag matches
        for tag in shader.tags:
            if tag in query_lower:
                score += 5
            for word in query_words:
                if word in tag or tag in word:
                    score += 2
        
        if score > 0:
            scored.append((score, shader))
    
    # Sort by score descending
    scored.sort(key=lambda x: x[0], reverse=True)
    
    return [shader for _, shader in scored[:limit]]


def get_shader_by_name(name: str) -> Optional[ShaderExample]:
    """Get a shader by exact name."""
    for shader in SHADER_LIBRARY:
        if shader.name.lower() == name.lower():
            return shader
    return None


def list_shader_names() -> List[str]:
    """Get list of all shader names."""
    return [s.name for s in SHADER_LIBRARY]


def get_random_shaders(count: int = 3) -> List[ShaderExample]:
    """Get random shaders from the library."""
    import random
    return random.sample(SHADER_LIBRARY, min(count, len(SHADER_LIBRARY)))
