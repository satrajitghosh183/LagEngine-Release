"""
Easy Shader Generator API
=========================

Simple one-liner API for generating GLSL shaders.

Usage:
    from shader_generator.easy import shader, preview, list_shaders
    
    # Generate a custom shader
    vert, frag = shader("blue fire with sparks")
    
    # Preview it
    preview("rainbow spiral")
    
    # Use a built-in shader
    vert, frag = shader("Plasma")  # exact name from library
    
    # List all available built-in shaders
    list_shaders()
"""

from typing import Tuple, Optional, List
from .shader_library import SHADER_LIBRARY, search_shaders, get_shader_by_name
from .generator import ShaderGenerator, ShaderPair
from .ollama_client import OllamaConfig


# Global generator instance (lazy loaded)
_generator: Optional[ShaderGenerator] = None


def _get_generator(model: str = "qwen2.5:7b-instruct-q4_K_M") -> ShaderGenerator:
    """Get or create the global generator instance."""
    global _generator
    if _generator is None:
        _generator = ShaderGenerator(
            ollama_config=OllamaConfig(model=model),
            verbose=False
        )
    return _generator


def shader(
    description: str,
    model: str = "qwen2.5:7b-instruct-q4_K_M"
) -> Tuple[str, str]:
    """
    Generate or retrieve a shader.
    
    Args:
        description: Either a built-in shader name (e.g., "Plasma", "Fire")
                    or a custom description (e.g., "blue fire with sparks")
        model: Ollama model to use for custom generation
        
    Returns:
        Tuple of (vertex_shader, fragment_shader) as strings
        
    Examples:
        # Use built-in shader
        vert, frag = shader("Polka Dots")
        
        # Generate custom shader
        vert, frag = shader("green plasma with lightning")
        
        # Generate with specific model
        vert, frag = shader("metallic surface", model="llama3:8b")
    """
    # First, check if it's an exact match to a built-in shader
    builtin = get_shader_by_name(description)
    if builtin:
        return builtin.vertex, builtin.fragment
    
    # Otherwise, generate with LLM
    gen = _get_generator(model)
    result = gen.generate(description)
    
    if result.success and result.shaders:
        return result.shaders.vertex, result.shaders.fragment
    else:
        raise RuntimeError(f"Failed to generate shader: {result.error}")


def preview(
    description: str,
    model: str = "qwen2.5:7b-instruct-q4_K_M"
) -> None:
    """
    Generate a shader and show it in a preview window.
    
    Args:
        description: Shader name or description
        model: Ollama model for custom generation
        
    Examples:
        preview("Fire")
        preview("neon grid with glow")
    """
    vert, frag = shader(description, model)
    
    from .preview import preview_shader
    from .generator import ShaderPair
    
    preview_shader(ShaderPair(vertex=vert, fragment=frag))


def list_shaders() -> List[str]:
    """
    List all available built-in shaders.
    
    Returns:
        List of shader names
        
    Example:
        names = list_shaders()
        print(names)  # ['Polka Dots', 'Checkerboard', 'Fire', ...]
    """
    return [s.name for s in SHADER_LIBRARY]


def find_shaders(query: str, limit: int = 5) -> List[str]:
    """
    Search for shaders matching a query.
    
    Args:
        query: Search term
        limit: Max results
        
    Returns:
        List of matching shader names
        
    Example:
        find_shaders("glow")  # ['Fresnel Glow', 'Pulsing Glow', 'Hologram']
    """
    results = search_shaders(query, limit=limit)
    return [s.name for s in results]


def get_shader_info(name: str) -> Optional[dict]:
    """
    Get detailed info about a built-in shader.
    
    Args:
        name: Shader name
        
    Returns:
        Dict with shader info or None if not found
        
    Example:
        info = get_shader_info("Fire")
        print(info['description'])  # "Animated fire effect"
        print(info['tags'])  # ['nature', 'fire', 'flame', ...]
    """
    shader = get_shader_by_name(name)
    if shader:
        return {
            'name': shader.name,
            'description': shader.description,
            'tags': shader.tags,
            'vertex': shader.vertex,
            'fragment': shader.fragment
        }
    return None


def save_shader(
    description: str,
    path: str,
    name: str = "shader",
    model: str = "qwen2.5:7b-instruct-q4_K_M"
) -> Tuple[str, str]:
    """
    Generate a shader and save to files.
    
    Args:
        description: Shader name or description
        path: Directory to save to
        name: Base filename (creates {name}.vert and {name}.frag)
        model: Ollama model for custom generation
        
    Returns:
        Tuple of (vertex_path, fragment_path)
        
    Example:
        save_shader("Fire", "./shaders", "fire_effect")
        # Creates: ./shaders/fire_effect.vert and ./shaders/fire_effect.frag
    """
    from pathlib import Path
    
    vert, frag = shader(description, model)
    
    out_path = Path(path)
    out_path.mkdir(parents=True, exist_ok=True)
    
    vert_file = out_path / f"{name}.vert"
    frag_file = out_path / f"{name}.frag"
    
    vert_file.write_text(vert)
    frag_file.write_text(frag)
    
    return str(vert_file), str(frag_file)


# Quick aliases
generate = shader
show = preview
search = find_shaders
