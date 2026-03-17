"""
CMake Generator
===============

Uses Ollama LLMs to generate CMakeLists.txt from project descriptions.
"""

import re
import sys
from typing import Optional
from pathlib import Path

# Reuse the existing Ollama client from shader_generator
sys.path.insert(0, str(Path(__file__).parent.parent))
from shader_generator.ollama_client import OllamaClient, OllamaConfig
from cmake_generator.project import ProjectConfig


SYSTEM_PROMPT = """\
You are a CMake expert. Generate a complete, ready-to-use CMakeLists.txt file based on the project specification below.

Rules:
- Output ONLY the CMakeLists.txt content — no explanations, no markdown fences, no commentary.
- Start with cmake_minimum_required() and project().
- Use modern CMake practices (target-based commands, generator expressions where appropriate).
- Use find_package() for external dependencies when applicable.
- Add appropriate compiler warnings (-Wall -Wextra -Wpedantic).
- Set the C++ standard using target_compile_features() or set(CMAKE_CXX_STANDARD ...).
- Include install rules if the target is a library.
- Keep the output clean, well-commented, and production-ready.
"""


class CMakeGenerator:
    """Generate CMakeLists.txt files using an Ollama LLM."""

    def __init__(self, model: Optional[str] = None, base_url: Optional[str] = None):
        config = OllamaConfig(
            model=model or "qwen2.5:7b-instruct-q4_K_M",
            base_url=base_url or "http://localhost:11434",
            temperature=0.3,  # Lower temp for deterministic code output
            num_predict=4096,
            stop_sequences=["---END---", "## END"],
        )
        self.client = OllamaClient(config)

    def check_ready(self) -> bool:
        """Verify Ollama is running and model is available."""
        if not self.client.is_available():
            print("Error: Ollama is not running. Start it with: ollama serve")
            return False
        if not self.client.has_model():
            models = self.client.list_models()
            print(f"Error: Model not found. Available models: {models}")
            return False
        return True

    def generate(self, config: ProjectConfig) -> str:
        """
        Generate a CMakeLists.txt from a ProjectConfig.

        Returns the CMakeLists.txt content as a string.
        """
        prompt = f"{SYSTEM_PROMPT}\n\n## Project Specification\n{config.to_prompt_context()}\n\n## CMakeLists.txt\n"

        print(f"Generating CMakeLists.txt for '{config.name}' using Ollama...")
        raw = self.client.generate(prompt)
        return self._clean_output(raw)

    def generate_streaming(self, config: ProjectConfig):
        """Generate with streaming output to stdout."""
        prompt = f"{SYSTEM_PROMPT}\n\n## Project Specification\n{config.to_prompt_context()}\n\n## CMakeLists.txt\n"

        print(f"Generating CMakeLists.txt for '{config.name}'...\n")
        chunks = []
        for chunk in self.client.generate_stream(prompt):
            print(chunk, end="", flush=True)
            chunks.append(chunk)
        print()
        return self._clean_output("".join(chunks))

    def generate_and_save(self, config: ProjectConfig, output_dir: str = ".") -> Path:
        """Generate and write CMakeLists.txt to disk."""
        content = self.generate(config)
        output_path = Path(output_dir) / "CMakeLists.txt"
        output_path.write_text(content)
        print(f"Saved to {output_path.resolve()}")
        return output_path

    @staticmethod
    def _clean_output(raw: str) -> str:
        """Strip markdown fences, fix common LLM syntax issues."""
        # Remove markdown code fences if present
        raw = re.sub(r"^```\w*\n?", "", raw, flags=re.MULTILINE)
        raw = re.sub(r"\n?```\s*$", "", raw, flags=re.MULTILINE)

        # Fix common LLM mistake: cmake_minimum_required-Version(X.Y) -> cmake_minimum_required(VERSION X.Y)
        raw = re.sub(
            r"cmake_minimum_required\s*[-–—]\s*Version\s*\(?",
            "cmake_minimum_required(VERSION ",
            raw,
            flags=re.IGNORECASE,
        )
        # Fix double parens: cmake_minimum_required(VERSION (3.16) -> cmake_minimum_required(VERSION 3.16)
        raw = re.sub(
            r"cmake_minimum_required\(VERSION\s+\((\d+\.\d+)\)",
            r"cmake_minimum_required(VERSION \1)",
            raw,
        )

        # Fix SFML 2.x references → SFML 3 (LLMs predate SFML 3)
        raw = re.sub(
            r"find_package\(SFML\s+2[\d.]*\s+",
            "find_package(SFML 3 ",
            raw,
        )
        # Fix lowercase SFML component names → proper case
        raw = re.sub(r"SFML::window\b", "SFML::Window", raw)
        raw = re.sub(r"SFML::system\b", "SFML::System", raw)
        raw = re.sub(r"SFML::graphics\b", "SFML::Graphics", raw)
        # Fix COMPONENTS casing
        raw = re.sub(
            r"COMPONENTS\s+window\s+system\s+graphics",
            "COMPONENTS Graphics Window System",
            raw,
            flags=re.IGNORECASE,
        )

        # Fix target-specific commands used before add_executable/add_library
        # Collect them, remove from original position, re-insert after target definition
        target_cmds = ("target_compile_features(", "target_compile_options(",
                       "target_include_directories(", "target_link_libraries(",
                       "target_compile_definitions(", "set_target_properties(")
        lines = raw.strip().split("\n")
        cleaned = []
        deferred = []
        target_defined = False
        for line in lines:
            if "add_executable(" in line or "add_library(" in line:
                target_defined = True
                cleaned.append(line)
                # Insert any deferred commands right after target definition
                for d in deferred:
                    cleaned.append(d)
                deferred.clear()
                continue
            stripped = line.lstrip()
            if not target_defined and any(stripped.startswith(tc) for tc in target_cmds):
                deferred.append(line)
                continue
            cleaned.append(line)
        # If target was never defined but we deferred lines, just drop them
        return "\n".join(cleaned).strip() + "\n"
