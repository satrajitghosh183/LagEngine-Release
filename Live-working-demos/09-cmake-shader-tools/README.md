# 09 - CMake and Shader Tools

Two Python utilities developed during engine prototyping as part of the LAGEngine
project.

1. **cmake_generator** -- scans C++ project directories and auto-generates
   `CMakeLists.txt` files with correct source lists, include paths, and library
   links.
2. **shader_generator** -- generates GLSL shaders from natural language
   descriptions using a local LLM served by Ollama, with a built-in library of
   20+ ready-made shaders.

## cmake_generator

Recursively walks a project directory, detects `.cpp`/`.hpp`/`.h` source files,
identifies library dependencies from `#include` directives, and emits a working
`CMakeLists.txt`.

```bash
python -m cmake_generator --scan /path/to/project
```

## shader_generator

Sends a natural language prompt to a locally running Ollama model, parses the
returned GLSL, and optionally renders a live preview using ModernGL and Pygame.

```bash
python -m shader_generator --preview "blue fire"
```

### Built-in shader library

The tool ships with 20+ pre-built shaders that work without an LLM:

| Category | Shaders |
|---|---|
| Patterns | Polka Dots, Checkerboard, Stripes, Hexagonal Grid, Brick Wall, Concentric Circles |
| Animated | Plasma, Rainbow Spiral, Kaleidoscope, Lava Lamp |
| Lighting | Fresnel Glow, Toon Shading, Phong Lighting, Hologram |
| Nature | Fire, Water, Clouds |
| Retro | CRT Screen, Glitch, Matrix Rain |
| Fractals | Mandelbrot, Voronoi |
| Simple | Gradient, Solid Color, Pulsing Glow |

```bash
python -m shader_generator --list
python -m shader_generator --use "Fire" --preview
python -m shader_generator --search "glow"
```

### Python API

```python
from shader_generator import shader, preview, list_shaders, save_shader

vert, frag = shader("blue fire with electric sparks")
preview("rainbow spiral animation")
save_shader("neon grid", "./shaders", "neon_grid")
print(list_shaders())
```

## Demo projects

The `demos/` directory contains small self-contained C++ projects used to test
the cmake_generator:

- `hello_sfml` -- minimal SFML application
- `matrix_viz` -- matrix math visualization
- `physics_sim` -- simple 2D physics simulation
- `raytracer` -- basic CPU raytracer

## Dependencies

- Python 3.8+
- `requests`
- `moderngl`
- `pygame`
- `numpy`
- [Ollama](https://ollama.com/) (required only for LLM-based shader generation)

## Setup

```bash
pip install -r requirements.txt
```

Ollama must be running locally (`ollama serve`) for the shader generator's LLM
mode. The cmake_generator and the built-in shader library work without it.

## Preview controls

- **ESC** -- close window
- **SPACE** -- toggle auto-rotation
- **Mouse drag** -- rotate manually
- **Scroll** -- zoom

## Language

Python
