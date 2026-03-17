# 06 - Deferred Rendering

A deferred rendering pipeline with render graph DAG scheduling, CUDA-accelerated
compute kernels, and an AI-driven pipeline optimization backend.

## Features

- **G-Buffer pass** -- position, normal, albedo, and depth written to multiple
  render targets in a single geometry pass.
- **Screen-Space Ambient Occlusion (SSAO)** -- CUDA compute kernel samples the
  depth/normal buffer to approximate ambient occlusion.
- **Shadow mapping** -- directional light shadow pass with configurable bias and
  PCF filtering.
- **Particle system** -- GPU particle simulation and rendering driven by CUDA
  compute kernels.
- **Post-processing** -- tone mapping and bloom implemented as CUDA kernels
  operating on the lighting output.
- **Render graph / DAG scheduler** -- render passes are declared as nodes in a
  directed acyclic graph; the scheduler resolves dependencies and issues passes
  in the correct order.
- **AI pipeline optimization** -- optional backend that profiles frame timings
  and suggests pass reordering or resolution scaling.
- **ImGui profiler and UI** -- real-time frame profiler, pass timing breakdown,
  and scene controls.

## Dependencies

| Dependency | Source |
|---|---|
| OpenGL 4.2+ | System |
| GLFW | System / vendored |
| GLM | System / vendored |
| CUDA Toolkit | System (nvcc must be on PATH) |
| GLAD | Vendored in `third_party/` |
| Dear ImGui | Vendored in `third_party/` |

## Build

Requires a C++17 compiler and the CUDA toolkit.

```bash
mkdir build && cd build
cmake ..
make
```

The resulting executable will be placed in the build directory. Run it from the
project root so that shader files are found via relative paths.

## Language

C++17, CUDA
