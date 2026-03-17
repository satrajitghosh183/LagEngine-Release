"""
Project Scanner
===============

Auto-detect project settings by scanning a directory for source files,
headers, and build hints.
"""

import os
from pathlib import Path
from typing import List, Optional, Tuple

from cmake_generator.project import ProjectConfig


# Common dependency indicators (filename patterns -> library names)
DEPENDENCY_HINTS = {
    "GL/": "OpenGL",
    "GLFW/": "glfw3",
    "glm/": "glm",
    "SDL": "SDL2",
    "SFML/": "SFML",
    "vulkan/": "Vulkan",
    "imgui": "imgui",
    "opencv": "OpenCV",
    "Eigen/": "Eigen3",
    "boost/": "Boost",
    "nlohmann/json": "nlohmann_json",
    "fmt/": "fmt",
    "spdlog/": "spdlog",
    "pthread": "Threads",
}

CPP_EXTENSIONS = {".cpp", ".cxx", ".cc", ".c++"}
C_EXTENSIONS = {".c"}
HEADER_EXTENSIONS = {".h", ".hpp", ".hxx", ".hh"}


def scan_directory(project_dir: str) -> ProjectConfig:
    """
    Scan a directory and auto-detect project settings.

    Looks for:
    - Source files (.cpp, .c, etc.)
    - Header files (.h, .hpp, etc.)
    - Source/include directory structure
    - #include directives to guess dependencies
    """
    root = Path(project_dir).resolve()
    project_name = root.name

    # Detect source and include directories
    src_dir, include_dir = _detect_dirs(root)

    # Find source and header files
    source_files = []
    header_files = []
    all_includes: set = set()

    for dirpath, _, filenames in os.walk(root):
        rel_dir = Path(dirpath).relative_to(root)
        # Skip build directories and hidden dirs
        if any(part.startswith(".") or part in ("build", "cmake-build", "__pycache__")
               for part in rel_dir.parts):
            continue

        for fname in filenames:
            fpath = Path(dirpath) / fname
            suffix = fpath.suffix.lower()
            rel_path = str(fpath.relative_to(root))

            if suffix in CPP_EXTENSIONS or suffix in C_EXTENSIONS:
                source_files.append(rel_path)
                all_includes.update(_scan_includes(fpath))
            elif suffix in HEADER_EXTENSIONS:
                header_files.append(rel_path)
                all_includes.update(_scan_includes(fpath))

    # Determine language
    has_cpp = any(Path(f).suffix.lower() in CPP_EXTENSIONS for f in source_files)
    has_c = any(Path(f).suffix.lower() in C_EXTENSIONS for f in source_files)
    if has_cpp and has_c:
        language = "C CXX"
    elif has_c:
        language = "C"
    else:
        language = "CXX"

    # Detect dependencies from includes
    dependencies = _detect_dependencies(all_includes)

    # Determine target type (library if no main() found)
    target_type = _detect_target_type(root, source_files)

    # Detect C++ standard hints
    standard = _detect_standard(root, source_files + header_files)

    config = ProjectConfig(
        name=project_name,
        description=f"Auto-detected project from {root}",
        language=language,
        standard=standard,
        source_dir=src_dir,
        include_dir=include_dir,
        source_files=sorted(source_files),
        header_files=sorted(header_files),
        dependencies=sorted(dependencies),
        target_type=target_type,
    )

    return config


def _detect_dirs(root: Path) -> Tuple[str, str]:
    """Detect source and include directory names."""
    src_dir = "src" if (root / "src").is_dir() else "."
    include_dir = "include" if (root / "include").is_dir() else src_dir
    return src_dir, include_dir


def _scan_includes(filepath: Path) -> List[str]:
    """Extract #include directives from a source file."""
    includes = []
    try:
        text = filepath.read_text(errors="replace")
        for line in text.splitlines():
            stripped = line.strip()
            if stripped.startswith("#include"):
                # Extract the included path
                for delim_open, delim_close in [("<", ">"), ('"', '"')]:
                    start = stripped.find(delim_open)
                    end = stripped.find(delim_close, start + 1)
                    if start != -1 and end != -1:
                        includes.append(stripped[start + 1:end])
                        break
    except (OSError, UnicodeDecodeError):
        pass
    return includes


def _detect_dependencies(includes: set) -> List[str]:
    """Match include paths against known dependency patterns."""
    deps = set()
    for inc in includes:
        for pattern, lib in DEPENDENCY_HINTS.items():
            if pattern.lower() in inc.lower():
                deps.add(lib)
    return list(deps)


def _detect_target_type(root: Path, source_files: List[str]) -> str:
    """Check if the project has a main() — executable, otherwise library."""
    for src in source_files:
        try:
            text = (root / src).read_text(errors="replace")
            if "int main(" in text or "int main (" in text:
                return "executable"
        except OSError:
            pass
    return "library"


def _detect_standard(root: Path, files: List[str]) -> str:
    """Guess C++ standard from code features."""
    features_20 = ["concept ", "co_await", "co_yield", "<ranges>", "<span>", "requires "]
    features_17 = ["if constexpr", "std::optional", "std::variant", "std::filesystem",
                    "structured bindings", "std::string_view", "<optional>", "<variant>"]
    features_14 = ["auto ", "decltype(auto)", "std::make_unique"]

    for f in files:
        try:
            text = (root / f).read_text(errors="replace")
            for feat in features_20:
                if feat in text:
                    return "20"
            for feat in features_17:
                if feat in text:
                    return "17"
        except OSError:
            pass
    return "17"  # Default to C++17
