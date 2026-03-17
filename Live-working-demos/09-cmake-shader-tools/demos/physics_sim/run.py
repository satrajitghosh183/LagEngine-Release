#!/usr/bin/env python3
"""
One-click build & run for the SPH Fluid Dynamics demo.

Just run:
    python3 demos/physics_sim/run.py
"""

import subprocess
import sys
import os
from pathlib import Path

# Resolve paths
DEMO_DIR  = Path(__file__).parent.resolve()
BUILD_DIR = DEMO_DIR / "build"
BINARY    = BUILD_DIR / "sph_fluid"
NCPUS     = os.cpu_count() or 4

RED   = "\033[91m"
GREEN = "\033[92m"
CYAN  = "\033[96m"
BOLD  = "\033[1m"
DIM   = "\033[2m"
RESET = "\033[0m"


def run(cmd, cwd=None, check=True):
    print(f"  {DIM}$ {' '.join(cmd)}{RESET}")
    result = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)
    if check and result.returncode != 0:
        print(f"{RED}Error:{RESET}\n{result.stderr}")
        sys.exit(1)
    return result


def needs_rebuild():
    """Check if source files are newer than the binary."""
    if not BINARY.exists():
        return True
    bin_mtime = BINARY.stat().st_mtime
    for ext in ("*.cpp", "*.h", "CMakeLists.txt"):
        for f in DEMO_DIR.rglob(ext):
            if "build" not in str(f) and f.stat().st_mtime > bin_mtime:
                return True
    return False


def main():
    print()
    print(f"  {BOLD}{CYAN}╔═══════════════════════════════════════════════╗{RESET}")
    print(f"  {BOLD}{CYAN}║   SPH Fluid Dynamics — Auto Build & Run      ║{RESET}")
    print(f"  {BOLD}{CYAN}╚═══════════════════════════════════════════════╝{RESET}")
    print()

    if not needs_rebuild():
        print(f"  {GREEN}✓{RESET} Binary is up to date, skipping build")
    else:
        # Configure
        BUILD_DIR.mkdir(exist_ok=True)
        print(f"  {CYAN}[1/2]{RESET} Configuring CMake...")
        run(["cmake", ".."], cwd=str(BUILD_DIR))
        print(f"  {GREEN}✓{RESET} CMake configured")

        # Build
        print(f"  {CYAN}[2/2]{RESET} Building ({NCPUS} threads)...")
        run(["make", f"-j{NCPUS}"], cwd=str(BUILD_DIR))
        print(f"  {GREEN}✓{RESET} Build complete")

    # Run
    print()
    print(f"  {BOLD}Launching SPH Fluid Dynamics...{RESET}")
    print(f"  {DIM}Close the window or press Esc to quit{RESET}")
    print()

    os.execv(str(BINARY), [str(BINARY)])


if __name__ == "__main__":
    main()
