#!/bin/bash
# Rebuild bindings for Python 3.12 (miniconda)

set -e

cd "/mnt/d/Master Things/Spring Sem Classes 2026/GameEngine/RL_Backend"

# Find Python 3.12 from miniconda
PYTHON_EXE=$(which python3)
echo "Using Python: $PYTHON_EXE"
$PYTHON_EXE --version

# Clean and rebuild
rm -rf build
mkdir -p build && cd build

# Configure with correct Python
cmake .. \
    -DRL_BACKEND_BUILD_RL=ON \
    -DRL_BACKEND_BUILD_TESTS=ON \
    -DPYTHON_EXECUTABLE=$PYTHON_EXE \
    -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . -j$(nproc)

# Copy to rl folder
cp lib/rldemo_env*.so ../rl/
ls -la ../rl/rldemo_env*.so

echo ""
echo "Build complete! Testing import..."
cd ../rl
$PYTHON_EXE -c "import rldemo_env; print('SUCCESS: rldemo_env imported'); env = rldemo_env.JobSchedulerEnv(); print(f'JobSchedulerEnv: obs_dim={env.get_obs_dim()}, act_dim={env.get_act_dim()}')"
