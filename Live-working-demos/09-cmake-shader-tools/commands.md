# GLSL Shader Generator Commands

Complete reference for all commands and usage patterns.

---

## Quick Start

```bash
# Install
pip3 install requests moderngl pygame numpy

# Run Ollama
ollama pull qwen2.5:7b-instruct-q4_K_M
```

---

## Built-in Shaders (Instant, No LLM)

These use the `--use` flag and load instantly from the library:

```bash
# Patterns
python3 -m shader_generator --use "Polka Dots" --preview
python3 -m shader_generator --use "Checkerboard" --preview
python3 -m shader_generator --use "Stripes" --preview
python3 -m shader_generator --use "Hexagonal Grid" --preview
python3 -m shader_generator --use "Brick Wall" --preview
python3 -m shader_generator --use "Concentric Circles" --preview

# Animated
python3 -m shader_generator --use "Plasma" --preview
python3 -m shader_generator --use "Rainbow Spiral" --preview
python3 -m shader_generator --use "Kaleidoscope" --preview
python3 -m shader_generator --use "Lava Lamp" --preview

# Lighting
python3 -m shader_generator --use "Fresnel Glow" --preview
python3 -m shader_generator --use "Toon Shading" --preview
python3 -m shader_generator --use "Phong Lighting" --preview
python3 -m shader_generator --use "Hologram" --preview

# Nature
python3 -m shader_generator --use "Fire" --preview
python3 -m shader_generator --use "Water" --preview
python3 -m shader_generator --use "Clouds" --preview

# Retro
python3 -m shader_generator --use "CRT Screen" --preview
python3 -m shader_generator --use "Glitch" --preview
python3 -m shader_generator --use "Matrix Rain" --preview

# Fractals
python3 -m shader_generator --use "Mandelbrot" --preview
python3 -m shader_generator --use "Voronoi" --preview

# Simple
python3 -m shader_generator --use "Gradient" --preview
python3 -m shader_generator --use "Solid Color" --preview
python3 -m shader_generator --use "Pulsing Glow" --preview
```

---

## List & Search

```bash
# List ALL built-in shaders
python3 -m shader_generator --list

# Search for shaders
python3 -m shader_generator --search "fire"
python3 -m shader_generator --search "glow"
python3 -m shader_generator --search "animated"
python3 -m shader_generator --search "pattern"
python3 -m shader_generator --search "retro"
```

---

## Custom Shaders (LLM Generated)

These use Ollama to generate custom shaders:

### Basic Effects

```bash
python3 -m shader_generator --preview "solid red color"
python3 -m shader_generator --preview "animated rainbow gradient"
python3 -m shader_generator --preview "vertical gradient from black to white"
python3 -m shader_generator --preview "horizontal stripes blue and white"
python3 -m shader_generator --preview "green polka dots on pink background"
python3 -m shader_generator --preview "diagonal lines pattern"
```

### Lighting Effects

```bash
python3 -m shader_generator --preview "phong lighting with bright specular"
python3 -m shader_generator --preview "toon shader with 4 color bands"
python3 -m shader_generator --preview "rim lighting blue glow"
python3 -m shader_generator --preview "cel shading anime style"
python3 -m shader_generator --preview "fresnel edge glow cyan"
python3 -m shader_generator --preview "metallic reflective surface"
python3 -m shader_generator --preview "iridescent soap bubble"
python3 -m shader_generator --preview "subsurface scattering skin"
```

### Animated Effects

```bash
python3 -m shader_generator --preview "purple plasma with swirling colors"
python3 -m shader_generator --preview "animated ocean waves"
python3 -m shader_generator --preview "pulsing heartbeat glow red"
python3 -m shader_generator --preview "fire effect with orange flames"
python3 -m shader_generator --preview "breathing color animation slow"
python3 -m shader_generator --preview "wave distortion ripple effect"
python3 -m shader_generator --preview "rotating rainbow color wheel"
python3 -m shader_generator --preview "bouncing light orb"
python3 -m shader_generator --preview "morphing blob shapes"
python3 -m shader_generator --preview "electric sparks animation"
```

### Procedural Patterns

```bash
python3 -m shader_generator --preview "procedural red brick wall"
python3 -m shader_generator --preview "voronoi cells blue and purple"
python3 -m shader_generator --preview "perlin noise grayscale"
python3 -m shader_generator --preview "spiral pattern rotating clockwise"
python3 -m shader_generator --preview "honeycomb hexagonal grid yellow"
python3 -m shader_generator --preview "wood grain brown texture"
python3 -m shader_generator --preview "marble white with gray veins"
python3 -m shader_generator --preview "circuit board green pattern"
python3 -m shader_generator --preview "camouflage pattern green brown"
python3 -m shader_generator --preview "leopard spots pattern"
```

### Psychedelic & Abstract

```bash
python3 -m shader_generator --preview "psychedelic swirling rainbow"
python3 -m shader_generator --preview "kaleidoscope 8 segments"
python3 -m shader_generator --preview "lava lamp orange blobs"
python3 -m shader_generator --preview "hypnotic spiral black white"
python3 -m shader_generator --preview "trippy color cycling fast"
python3 -m shader_generator --preview "morphing geometric shapes"
python3 -m shader_generator --preview "rainbow vortex spinning"
python3 -m shader_generator --preview "acid trip neon colors"
python3 -m shader_generator --preview "tunnel infinite zoom"
python3 -m shader_generator --preview "fractal zoom animation"
```

### Retro & Glitch

```bash
python3 -m shader_generator --preview "old CRT TV with scanlines"
python3 -m shader_generator --preview "glitch effect RGB split horizontal"
python3 -m shader_generator --preview "VHS tape tracking distortion"
python3 -m shader_generator --preview "scanlines green phosphor"
python3 -m shader_generator --preview "pixel art dithering 8bit"
python3 -m shader_generator --preview "chromatic aberration lens"
python3 -m shader_generator --preview "static noise black white TV"
python3 -m shader_generator --preview "synthwave grid pink purple"
python3 -m shader_generator --preview "retro arcade neon"
python3 -m shader_generator --preview "80s VHS aesthetic"
```

### Sci-Fi & Hologram

```bash
python3 -m shader_generator --preview "hologram blue with scan lines"
python3 -m shader_generator --preview "matrix rain green falling code"
python3 -m shader_generator --preview "energy shield bubble blue"
python3 -m shader_generator --preview "laser beam red pulsing"
python3 -m shader_generator --preview "force field hexagon pattern"
python3 -m shader_generator --preview "portal swirl purple orange"
python3 -m shader_generator --preview "tron neon blue lines"
python3 -m shader_generator --preview "cyberpunk neon pink glow"
python3 -m shader_generator --preview "data stream visualization"
python3 -m shader_generator --preview "sci-fi HUD interface"
python3 -m shader_generator --preview "warp speed stars"
python3 -m shader_generator --preview "teleporter beam effect"
```

### Nature & Elements

```bash
python3 -m shader_generator --preview "underwater caustics blue"
python3 -m shader_generator --preview "ocean surface reflection waves"
python3 -m shader_generator --preview "clouds moving across blue sky"
python3 -m shader_generator --preview "rain drops on glass window"
python3 -m shader_generator --preview "snow falling white particles"
python3 -m shader_generator --preview "lightning bolt flash bright"
python3 -m shader_generator --preview "aurora borealis green purple"
python3 -m shader_generator --preview "sunset gradient orange pink"
python3 -m shader_generator --preview "lava flowing red orange"
python3 -m shader_generator --preview "ice crystal blue frost"
python3 -m shader_generator --preview "grass field waving"
python3 -m shader_generator --preview "starfield twinkling night sky"
```

### Game Effects

```bash
python3 -m shader_generator --preview "dissolve effect with orange edge glow"
python3 -m shader_generator --preview "damage flash red screen"
python3 -m shader_generator --preview "health bar gradient green to red"
python3 -m shader_generator --preview "magic spell sparkles purple"
python3 -m shader_generator --preview "power up golden aura glow"
python3 -m shader_generator --preview "speed lines motion blur radial"
python3 -m shader_generator --preview "explosion shockwave ring"
python3 -m shader_generator --preview "teleport fade out effect"
python3 -m shader_generator --preview "shield hit impact flash"
python3 -m shader_generator --preview "level up glow animation"
python3 -m shader_generator --preview "poison damage green pulse"
python3 -m shader_generator --preview "frozen ice overlay"
```

### Fractals & Math

```bash
python3 -m shader_generator --preview "mandelbrot fractal colored"
python3 -m shader_generator --preview "julia set fractal animated"
python3 -m shader_generator --preview "sierpinski triangle pattern"
python3 -m shader_generator --preview "moire pattern interference lines"
python3 -m shader_generator --preview "sine wave visualization animated"
python3 -m shader_generator --preview "polar coordinates flower pattern"
python3 -m shader_generator --preview "fibonacci spiral golden"
python3 -m shader_generator --preview "lissajous curves animated"
python3 -m shader_generator --preview "fractal tree branching"
python3 -m shader_generator --preview "cellular automata pattern"
```

### Gradients & Colors

```bash
python3 -m shader_generator --preview "animated gradient blue to purple"
python3 -m shader_generator --preview "radial gradient from white center"
python3 -m shader_generator --preview "diagonal gradient pink to blue"
python3 -m shader_generator --preview "color palette cycling rainbow"
python3 -m shader_generator --preview "sunset orange to purple gradient"
python3 -m shader_generator --preview "neon green to cyan transition"
python3 -m shader_generator --preview "pastel pink blue gradient soft"
python3 -m shader_generator --preview "metallic gold shimmer animated"
python3 -m shader_generator --preview "chrome reflection gradient"
python3 -m shader_generator --preview "duotone effect two colors"
```

### Geometric

```bash
python3 -m shader_generator --preview "pulsing circles expanding"
python3 -m shader_generator --preview "rotating squares pattern"
python3 -m shader_generator --preview "concentric rings rainbow"
python3 -m shader_generator --preview "triangle grid tessellation"
python3 -m shader_generator --preview "diamond tiles blue white"
python3 -m shader_generator --preview "star burst pattern yellow"
python3 -m shader_generator --preview "zigzag lines chevron"
python3 -m shader_generator --preview "cross hatch pencil shading"
python3 -m shader_generator --preview "dots grid halftone"
python3 -m shader_generator --preview "op art optical illusion"
```

### Post-Processing Style

```bash
python3 -m shader_generator --preview "vignette dark corners"
python3 -m shader_generator --preview "bloom glow bright areas"
python3 -m shader_generator --preview "film grain noise overlay"
python3 -m shader_generator --preview "sepia tone vintage photo"
python3 -m shader_generator --preview "black and white high contrast"
python3 -m shader_generator --preview "color inversion negative"
python3 -m shader_generator --preview "posterize 4 colors"
python3 -m shader_generator --preview "edge detection outline"
python3 -m shader_generator --preview "blur gaussian soft"
python3 -m shader_generator --preview "sharpen enhance details"
```

### Creative & Artistic

```bash
python3 -m shader_generator --preview "oil painting brush strokes"
python3 -m shader_generator --preview "watercolor bleed effect"
python3 -m shader_generator --preview "sketch pencil drawing"
python3 -m shader_generator --preview "comic book halftone dots"
python3 -m shader_generator --preview "stained glass colorful"
python3 -m shader_generator --preview "mosaic tile pattern"
python3 -m shader_generator --preview "pointillism dots art"
python3 -m shader_generator --preview "neon sign glowing text"
python3 -m shader_generator --preview "graffiti spray paint"
python3 -m shader_generator --preview "abstract expressionist"
```

---

## Python API Examples

### One-Liners

```python
from shader_generator import shader, preview, list_shaders, find_shaders, save_shader

# Get shader code
vert, frag = shader("Fire")                    # Built-in
vert, frag = shader("blue plasma with sparks") # Custom generated

# Preview instantly
preview("Plasma")
preview("neon grid with glow")

# List all built-in
print(list_shaders())

# Search
print(find_shaders("glow"))    # ['Fresnel Glow', 'Pulsing Glow', 'Hologram']
print(find_shaders("fire"))    # ['Fire']
print(find_shaders("pattern")) # ['Polka Dots', 'Checkerboard', ...]

# Save to files
save_shader("rainbow spiral", "./shaders", "rainbow")
```

### Full Script Example

```python
from shader_generator import shader, preview, list_shaders

# Print all available built-in shaders
print("Available shaders:")
for name in list_shaders():
    print(f"  - {name}")

# Generate a custom shader
print("\nGenerating custom shader...")
vert, frag = shader("purple crystal with sparkles")

print("\nVertex Shader:")
print(vert)

print("\nFragment Shader:")
print(frag)

# Save it
from pathlib import Path
Path("output").mkdir(exist_ok=True)
Path("output/crystal.vert").write_text(vert)
Path("output/crystal.frag").write_text(frag)

# Preview it
preview("purple crystal with sparkles")
```

### Batch Generation

```python
from shader_generator import save_shader

effects = [
    ("fire", "Fire"),
    ("water", "Water"),
    ("plasma", "Plasma"),
    ("glow", "Fresnel Glow"),
    ("custom_lava", "red lava with black rocks"),
    ("custom_ice", "frozen ice with blue crystals"),
]

for filename, description in effects:
    print(f"Generating: {description}")
    save_shader(description, "./shaders", filename)
    print(f"  Saved: ./shaders/{filename}.vert/.frag")
```

---

## Utility Commands

```bash
# List available Ollama models
python3 -m shader_generator --list-models

# Use a different model
python3 -m shader_generator --preview -m "llama3:8b" "fire effect"
python3 -m shader_generator --preview -m "qwen2.5:3b-instruct-q4_K_M" "simple gradient"
python3 -m shader_generator --preview -m "codellama:7b" "phong lighting"

# Save to custom location
python3 -m shader_generator --preview --output ./my_shaders --name cool_effect "neon glow"

# More retries for complex shaders
python3 -m shader_generator --preview --retries 5 "complex fractal zoom"

# Skip OpenGL validation (faster but less reliable)
python3 -m shader_generator --preview --no-validate "quick test shader"

# Quiet mode (less output)
python3 -m shader_generator --preview -q "rainbow gradient"

# Custom prompt file
python3 -m shader_generator --preview --prompt-file my_prompt.txt "custom effect"
```

---

## Interactive Mode

```bash
python3 -m shader_generator --interactive
```

In interactive mode:
- Type any shader description → generates it
- `preview` → opens preview of last shader
- `save` → saves last shader to `output/`
- `quit` or `exit` → exit

---

## Preview Controls

| Key | Action |
|-----|--------|
| **ESC** | Close window |
| **SPACE** | Toggle auto-rotation |
| **Mouse drag** | Rotate manually |
| **Scroll** | Zoom in/out |

---

## Troubleshooting

```bash
# Check if Ollama is running
curl http://localhost:11434/api/tags

# Pull a model if needed
ollama pull qwen2.5:7b-instruct-q4_K_M

# Check available models
python3 -m shader_generator --list-models

# Test basic functionality
python3 -c "from shader_generator import list_shaders; print(list_shaders())"
```
