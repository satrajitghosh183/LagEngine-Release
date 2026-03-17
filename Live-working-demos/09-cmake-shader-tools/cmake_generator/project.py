"""
Project configuration model for CMake generation.
"""

from dataclasses import dataclass, field
from typing import List, Optional


@dataclass
class ProjectConfig:
    """Describes the project to generate a CMakeLists.txt for."""

    name: str
    description: str
    language: str = "CXX"  # C, CXX, or both
    standard: str = "17"  # C++ standard (11, 14, 17, 20, 23)
    source_dir: str = "src"
    include_dir: str = "include"
    source_files: List[str] = field(default_factory=list)
    header_files: List[str] = field(default_factory=list)
    dependencies: List[str] = field(default_factory=list)
    target_type: str = "executable"  # executable or library
    cmake_minimum: str = "3.16"
    extra_instructions: str = ""

    def to_prompt_context(self) -> str:
        """Format project config as context for the LLM prompt."""
        lines = [
            f"Project Name: {self.name}",
            f"Description: {self.description}",
            f"Language: {self.language}",
            f"C++ Standard: {self.standard}",
            f"Source Directory: {self.source_dir}",
            f"Include Directory: {self.include_dir}",
            f"Target Type: {self.target_type}",
            f"CMake Minimum Version: {self.cmake_minimum}",
        ]
        if self.source_files:
            lines.append(f"Source Files: {', '.join(self.source_files)}")
        if self.header_files:
            lines.append(f"Header Files: {', '.join(self.header_files)}")
        if self.dependencies:
            lines.append(f"Dependencies/Libraries: {', '.join(self.dependencies)}")
        if self.extra_instructions:
            lines.append(f"Additional Requirements: {self.extra_instructions}")
        return "\n".join(lines)
