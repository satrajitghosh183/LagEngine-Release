"""
Built-in demo project configurations and directory mappings.
"""

from typing import List, Tuple, Optional
from cmake_generator.project import ProjectConfig

# Map demo key → subdirectory under demos/
DEMO_DIRS = {
    "physics":    "demos/physics_sim",
    "raytracer":  "demos/raytracer",
    "matrix-viz": "demos/matrix_viz",
    "error-demo": "demos/hello_sfml",
}

# Binary names produced by each demo's CMakeLists.txt
DEMO_BINARIES = {
    "physics":    "sph_fluid",
    "raytracer":  "miniray",
    "matrix-viz": "matrix_viz",
    "error-demo": "hello_sfml",
}

DEMOS = {
    "physics": ProjectConfig(
        name="SPHFluid",
        description=(
            "2D SPH fluid dynamics — dam-break, splashing, velocity coloring. "
            "Interactive sliders for gravity, viscosity, pressure. Glow rendering."
        ),
        language="CXX",
        standard="17",
        source_dir="src",
        include_dir="include",
        source_files=["main.cpp", "sph.cpp", "renderer.cpp", "gui.cpp"],
        header_files=["vec2.h", "sph.h", "renderer.h", "gui.h"],
        dependencies=["SFML"],
        target_type="executable",
        cmake_minimum="3.16",
        extra_instructions=(
            "Use find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED). "
            "Link with SFML::Graphics SFML::Window SFML::System. "
            "Set output binary name to 'sph_fluid'. Add -O2 for Release."
        ),
    ),
    "raytracer": ProjectConfig(
        name="MiniRay",
        description=(
            "CPU raytracer with reflective spheres, checkerboard floor, and gradient sky. "
            "Progressive scanline rendering visible in real-time. Gamma corrected."
        ),
        language="CXX",
        standard="17",
        source_dir="src",
        include_dir="include",
        source_files=["main.cpp"],
        header_files=["raytracer.h"],
        dependencies=["SFML"],
        target_type="executable",
        cmake_minimum="3.16",
        extra_instructions=(
            "Use find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED). "
            "Link with SFML::Graphics SFML::Window SFML::System. "
            "Set output binary name to 'miniray'. Add -O2 for Release."
        ),
    ),
    "matrix-viz": ProjectConfig(
        name="MatrixViz",
        description=(
            "Interactive 2D matrix transform visualizer. Rotation, scale, shear sliders "
            "with live matrix display, determinant, basis vectors, and shape morphing."
        ),
        language="CXX",
        standard="17",
        source_dir="src",
        include_dir="include",
        source_files=["main.cpp"],
        header_files=["transforms.h"],
        dependencies=["SFML"],
        target_type="executable",
        cmake_minimum="3.16",
        extra_instructions=(
            "Use find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED). "
            "Link with SFML::Graphics SFML::Window SFML::System. "
            "Set output binary name to 'matrix_viz'. Add -O2 for Release."
        ),
    ),
    "error-demo": ProjectConfig(
        name="HelloGraphics",
        description=(
            "Colorful bouncing balls with glow trails and specular highlights. "
            "Simple single-file SFML 3 demo — great intro to graphics programming."
        ),
        language="CXX",
        standard="17",
        source_dir="src",
        source_files=["main.cpp"],
        header_files=[],
        dependencies=["SFML"],
        target_type="executable",
        cmake_minimum="3.16",
        extra_instructions=(
            "Use find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED). "
            "Link with SFML::Graphics SFML::Window SFML::System. "
            "Set output binary name to 'hello_sfml'. Add -O2 for Release."
        ),
    ),
}


def get_demo_config(name: str) -> Optional[ProjectConfig]:
    return DEMOS.get(name)

def get_demo_dir(name: str) -> Optional[str]:
    return DEMO_DIRS.get(name)

def get_demo_binary(name: str) -> Optional[str]:
    return DEMO_BINARIES.get(name)

def list_demos() -> List[Tuple[str, str]]:
    return [(name, cfg.description[:80] + "...") for name, cfg in DEMOS.items()]
