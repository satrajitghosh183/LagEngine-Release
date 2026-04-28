#!/usr/bin/env bash
# ── SpringBlockSim Build Script (Vulkan) ─────────────────────────────────────
#  Requires: CMake 3.20+, C++17 compiler, Vulkan SDK (with shaderc), GLFW
#  Usage:   ./build_sim.sh [Release|Debug]
# ─────────────────────────────────────────────────────────────────────────────

set -e
CONFIG="${1:-Release}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "[SpringBlockSim] Configuring ($CONFIG)..."
mkdir -p build
cd build

cmake .. -DCMAKE_BUILD_TYPE="$CONFIG" ${CMAKE_EXTRA_ARGS:+"$CMAKE_EXTRA_ARGS"}
cmake --build . -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"

echo
echo "[SpringBlockSim] Build succeeded."
if [[ -f "SpringBlockSim" ]]; then
  echo "  Executable: build/SpringBlockSim"
  echo "  Run:  ./build/SpringBlockSim"
else
  echo "  Executable: build/$CONFIG/SpringBlockSim (multi-config generator)"
fi
