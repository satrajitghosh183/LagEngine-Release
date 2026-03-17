# RL_Backend – Multithreaded Rendering + RL Benchmark Suite

**Standalone project** – not part of GameEngine. Build independently.

A high-performance OpenGL engine with:
- **Job system** (Chase-Lev work-stealing deques)
- **Render thread + worker threads** (GL submission on one thread; parallel CPU prep)
- **GPU-driven draw submission** (MultiDrawIndirect, persistent mapped buffers)
- **CUDA–OpenGL interop** (optional)
- **Python RL training suite** (pybind11 env, ≥10 algorithms)

---

## Build (Linux / WSL / macOS)

```bash
cd RL_Backend
mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)
```

## Build (Windows – Visual Studio)

Open **Developer Command Prompt for VS**:

```cmd
cd RL_Backend
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

---

## Run Demos

From the `build/` directory:

```bash
cd build
./bin/demo_hello_triangle      # Hello triangle + ImGui overlay
./bin/demo_indirectdraw        # 10k–100k cubes, parallel CPU prep
./bin/demo_physics_slow        # Single-threaded N-body baseline
./bin/demo_physics_parallel    # Parallel N-body
./bin/demo_physics_rl          # N-body + work distribution UI
./bin/jobbench                 # Job system microbenchmark (headless)
```

---

## CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `RL_BACKEND_BUILD_CUDA` | OFF | Build CUDA interop (particles, cloth) |
| `RL_BACKEND_BUILD_RL` | ON | Build pybind11 env + RL bindings |
| `RL_BACKEND_BUILD_TESTS` | ON | Build unit tests |

---

## Tests

```bash
ctest --test-dir build
# or
./bin/test_jobs
```

---

## RL Environment ↔ Parallel Demos (Connected)

The RL `FrameSchedulerEnv` uses the **same parallel workload pattern** as `demo_indirectdraw`:

- **demo_indirectdraw**: Workers build instance data (matrices + colors) in batches; each job processes a range of cubes and calls `IndirectDrawBuilder::AddInstance` (shared mutex).
- **FrameSchedulerEnv**: Runs the same pattern headless—workers process batches, compute matrices, add to shared staging buffer (mutex). No OpenGL; CPU-only for Python training.

The agent controls **cube count** (100–100k, like the demo slider) and **cam angle** (affects work per instance). Reward = −frame_time. Training optimizes the same scheduling behavior you see in the visual demo.

---

## Python RL Suite

From the **GameEngine** directory:

```bash
# Path has spaces - use quotes on Windows/WSL
cd "RL_Backend/rl"

python -m venv .venv
.venv/Scripts/activate   # Windows
# source .venv/bin/activate   # Linux/WSL/macOS
pip install -r requirements.txt

# Train (requires rldemo_env built with -DRL_BACKEND_BUILD_RL=ON)
python train.py --algo ppo --env FrameSchedulerEnv --steps 50000 --seed 0

# Evaluate trained checkpoint
python eval.py --checkpoint runs/FrameSchedulerEnv_ppo_s0/model.pt --episodes 10 --output results.csv

# Train AWSS on WorkStealSchedulerEnv (curriculum)
python train.py --algo awss --env WorkStealSchedulerEnv --steps 50000 --curriculum

# View TensorBoard
tensorboard --logdir runs
```

**Note:** If you see `GLIBCXX` or libstdc++ errors when importing `rldemo_env`, use a Python environment that matches the compiler used for the C++ build (e.g. system Python, or a conda env with `libstdcxx-ng`).

Or run from GameEngine root without cd:
```bash
pip install -r RL_Backend/rl/requirements.txt
python RL_Backend/rl/train.py --algo ppo --env FrameSchedulerEnv --steps 50000
```

**Algorithms:** `reinforce`, `dqn`, `double_dqn`, `dueling_dqn`, `c51`, `a2c`, `ppo`, `ddpg`, `td3`, `sac`, `ppo_qfilter`, `awss`

---

## Structure

```
RL_Backend/
├── engine/           # C++20 engine (JobSystem, IndirectDrawBuilder, Window, Shader)
├── demos/
│   ├── hello_triangle/
│   ├── demo_indirectdraw/
│   └── jobbench/
├── bindings/          # pybind11 → FrameSchedulerEnv
├── rl/                # Python RL algorithms + train/eval
├── tests/
└── third_party/       # spdlog, Catch2, pybind11
```

---

## Implementation Status

- [x] Job system (Chase-Lev work-stealing)
- [x] Hello triangle + ImGui
- [x] Indirect draw demo (parallel CPU prep)
- [x] JobBench microbenchmark
- [x] pybind11 env + FrameSchedulerEnv
- [x] RL skeleton (train.py, eval.py, algos/)
- [x] PPO-QFilter hybrid (skeleton)
- [ ] CUDA interop (particles, cloth) – enable with -DRL_BACKEND_BUILD_CUDA=ON
