# 05 - GPU Particles (CUDA)

CUDA-accelerated particle simulation with OpenGL interop for real-time rendering
of 100K+ particles. Includes a Python reinforcement learning interface for
optimizing CUDA kernel parameters using stable-baselines3.

## Features

- CUDA/OpenGL interop for zero-copy GPU particle rendering
- Velocity-based particle coloring
- Real-time simulation of 100,000+ particles
- Python RL training interface for kernel parameter optimization

## Executables

| Target | Description |
|--------|-------------|
| `gpu_particles` | Interactive particle simulation |
| `gpu_particles_rl` | RL training interface (communicates with Python agent) |

## Dependencies

- CUDA Toolkit
- OpenGL
- GLFW (fetched automatically via CMake FetchContent)

Python dependencies (for RL interface only):

```
pip install -r rl_interface/requirements.txt
```

## Build

```bash
mkdir build && cd build
cmake ..
make
```

### Running the RL interface

1. Build and run `gpu_particles_rl`.
2. In a separate terminal:
   ```bash
   cd rl_interface
   python train_rl.py
   ```
