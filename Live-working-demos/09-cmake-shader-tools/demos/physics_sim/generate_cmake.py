#!/usr/bin/env python3
"""
Generate CMakeLists.txt for this physics demo.

Preferred: run the visual demo from the project root:
    python3 -m cmake_generator

Or generate directly:
    python3 -m cmake_generator --scan demos/physics_sim
"""
import sys
from pathlib import Path

root = Path(__file__).parent.parent.parent
sys.path.insert(0, str(root))

from cmake_generator.demo import run_demo
run_demo()
