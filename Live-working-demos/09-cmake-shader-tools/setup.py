#!/usr/bin/env python3
"""
Setup script for the GLSL Shader Generator package.
"""

from setuptools import setup, find_packages
from pathlib import Path

# Read README for long description
readme_path = Path(__file__).parent / "README.md"
long_description = ""
if readme_path.exists():
    long_description = readme_path.read_text(encoding="utf-8")

setup(
    name="shader-generator",
    version="1.0.0",
    author="Your Name",
    author_email="your.email@example.com",
    description="Generate GLSL shaders from natural language using local LLMs",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/yourusername/shader-generator",
    packages=find_packages(),
    classifiers=[
        "Development Status :: 4 - Beta",
        "Intended Audience :: Developers",
        "Topic :: Multimedia :: Graphics :: 3D Rendering",
        "Topic :: Scientific/Engineering :: Artificial Intelligence",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
    ],
    python_requires=">=3.8",
    install_requires=[
        "requests>=2.28.0",
    ],
    extras_require={
        "opengl": [
            "moderngl>=5.8.0",
        ],
        "preview": [
            "moderngl>=5.8.0",
            "pygame>=2.5.0",
            "numpy>=1.24.0",
        ],
        "dev": [
            "pytest>=7.0.0",
            "black>=23.0.0",
            "mypy>=1.0.0",
        ],
        "all": [
            "moderngl>=5.8.0",
            "pygame>=2.5.0",
            "numpy>=1.24.0",
        ],
    },
    entry_points={
        "console_scripts": [
            "shader-gen=shader_generator.cli:main",
        ],
    },
    include_package_data=True,
    package_data={
        "shader_generator": ["*.txt"],
    },
)
