"""
CMake Generator
===============

Generate ready-to-use CMakeLists.txt files from project descriptions using local LLMs via Ollama.
"""

from cmake_generator.generator import CMakeGenerator
from cmake_generator.project import ProjectConfig
from cmake_generator.scanner import scan_directory

__all__ = ["CMakeGenerator", "ProjectConfig", "scan_directory"]
__version__ = "1.0.0"
