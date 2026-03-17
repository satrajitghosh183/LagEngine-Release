"""
GLSL Shader Generator Package
=============================

Generate OpenGL shaders from natural language descriptions using local LLMs via Ollama.

Example usage:
    from shader_generator import ShaderGenerator
    
    generator = ShaderGenerator()
    result = generator.generate("animated fire effect with embers")
    
    if result.success:
        print(result.shaders.vertex)
        print(result.shaders.fragment)
"""

from .generator import ShaderGenerator, ShaderPair, GenerationResult
from .compiler import ShaderCompiler, CompilationResult
from .ollama_client import OllamaClient
from .shader_library import search_shaders, SHADER_LIBRARY, list_shader_names

# Easy API - import these for simple usage
from .easy import shader, preview, list_shaders, find_shaders, save_shader

__version__ = "1.0.0"
__all__ = [
    # Easy API (recommended)
    "shader",
    "preview", 
    "list_shaders",
    "find_shaders",
    "save_shader",
    # Advanced API
    "ShaderGenerator",
    "ShaderPair", 
    "GenerationResult",
    "ShaderCompiler",
    "CompilationResult",
    "OllamaClient",
    "search_shaders",
    "SHADER_LIBRARY",
    "list_shader_names",
]
