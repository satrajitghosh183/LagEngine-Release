# CUDA GL Demo RL - Command Reference

Complete list of commands for using the Reinforcement Learning system for CUDA GL Demo resource allocation optimization.

---

## Table of Contents

1. [Setup & Installation](#setup--installation)
2. [Building the Project](#building-the-project)
3. [Training the RL Agent](#training-the-rl-agent)
4. [Evaluating Models](#evaluating-models)
5. [Running with Optimal Settings](#running-with-optimal-settings)
6. [Data Collection](#data-collection)
7. [Testing & Debugging](#testing--debugging)
8. [Utility Commands](#utility-commands)

---

## Setup & Installation

### Install Python Dependencies

```bash
cd CUDA_GL_Demo/rl_interface
pip install -r requirements.txt
```

### Verify Installation

```bash
python -c "import stable_baselines3; import gymnasium; print('Dependencies OK')"
```

---

## Building the Project

### Build the CUDA GL Demo (First Time)

```bash
cd CUDA_GL_Demo
mkdir -p build
cd build
cmake ..
make
# On Windows: cmake --build .
```

### Rebuild After Changes

```bash
cd CUDA_GL_Demo/build
make
# On Windows: cmake --build .
```

### Verify Executable Exists

```bash
ls -la ../build/cuda_gl_demo_rl
# On Windows: dir ..\build\cuda_gl_demo_rl.exe
```

---

## Training the RL Agent

### Basic Training (Default Settings)

```bash
cd CUDA_GL_Demo/rl_interface
python train_rl.py --executable ../build/cuda_gl_demo_rl
```

### Training with Custom Particle Count

```bash
python train_rl.py --executable ../build/cuda_gl_demo_rl --particles 500000
```

### Training with Custom Timesteps

```bash
python train_rl.py --executable ../build/cuda_gl_demo_rl --timesteps 2000
```

### Training with Custom Learning Rate

```bash
python train_rl.py --executable ../build/cuda_gl_demo_rl --lr 1e-4
```

### Training with Custom Frames per Episode

```bash
python train_rl.py --executable ../build/cuda_gl_demo_rl --frames 200
```

### Training with Custom Save Path

```bash
python train_rl.py --executable ../build/cuda_gl_demo_rl --save-path ./my_models
```

### Training with Custom Log Directory

```bash
python train_rl.py --executable ../build/cuda_gl_demo_rl --log-dir ./my_logs
```

### Training on GPU (CUDA)

```bash
python train_rl.py --executable ../build/cuda_gl_demo_rl --device cuda
```

### Training on CPU

```bash
python train_rl.py --executable ../build/cuda_gl_demo_rl --device cpu
```

### Complete Training Example (All Options)

```bash
python train_rl.py \
    --executable ../build/cuda_gl_demo_rl \
    --particles 500000 \
    --frames 100 \
    --timesteps 2000 \
    --lr 3e-4 \
    --save-path ./rl_models \
    --log-dir ./rl_logs \
    --device auto
```

---

## Evaluating Models

### Evaluate a Specific Model

```bash
python evaluate_rl.py rl_models/cuda_gl_rl_final.zip
```

### Evaluate with Custom Executable

```bash
python evaluate_rl.py rl_models/cuda_gl_rl_final.zip --executable ../build/cuda_gl_demo_rl
```

### Evaluate with More Episodes

```bash
python evaluate_rl.py rl_models/cuda_gl_rl_final.zip --episodes 20
```

### Evaluate with Custom Frames

```bash
python evaluate_rl.py rl_models/cuda_gl_rl_final.zip --frames 200
```

### Evaluate with Custom Output File

```bash
python evaluate_rl.py rl_models/cuda_gl_rl_final.zip --output my_eval_results.json
```

### Evaluate Best Model from Training

```bash
python evaluate_rl.py rl_logs/PPO_1/best_model.zip
```

---

## Running with Optimal Settings

### Run with Auto-Detected Best Model

```bash
python run_optimal.py
```

### Run with Specific Model

```bash
python run_optimal.py --model-name PPO_1
```

### Run with Model Path

```bash
python run_optimal.py --model rl_models/cuda_gl_rl_final.zip
```

### Run with Custom Particle Count

```bash
python run_optimal.py --particles 1000000
```

### Run with Custom Frame Count

```bash
python run_optimal.py --frames 1000
```

### Run with Custom Executable

```bash
python run_optimal.py --executable ../build/cuda_gl_demo_rl
```

### List Available Models

```bash
python run_optimal.py --list-models
```

### Get Optimal Settings Without Running Demo

```bash
python run_optimal.py --no-demo
```

### Complete Example

```bash
python run_optimal.py \
    --model-name PPO_1 \
    --particles 500000 \
    --frames 500 \
    --executable ../build/cuda_gl_demo_rl \
    --output optimal_results.json
```

---

## Data Collection

### Collect Data with Default Settings

```bash
python collect_data.py
```

### Collect Data with Custom Executable

```bash
python collect_data.py --executable ../build/cuda_gl_demo_rl
```

### Collect Data with Custom Particle Counts

```bash
python collect_data.py --particles 100000 200000 300000 400000 500000
```

### Collect Data with Custom Block Sizes

```bash
python collect_data.py --blocksizes 128 256 512
```

### Collect Data with Custom Frames

```bash
python collect_data.py --frames 200
```

### Collect Data with Custom Output Files

```bash
python collect_data.py --output my_data.json --csv my_data.csv
```

### Complete Data Collection Example

```bash
python collect_data.py \
    --executable ../build/cuda_gl_demo_rl \
    --particles 100000 250000 500000 \
    --blocksizes 256 512 \
    --frames 150 \
    --output training_data.json \
    --csv training_data.csv
```

---

## Testing & Debugging

### Test Executable

```bash
python test_executable.py
```

### Test Specific Executable

```bash
python test_executable.py ../build/cuda_gl_demo_rl
```

### Run Basic Example

```bash
python example_usage.py basic
```

### Run Manual Optimization Example

```bash
python example_usage.py manual
```

### Test Trained Model Example

```bash
python example_usage.py model
```

### View TensorBoard Logs

```bash
tensorboard --logdir rl_logs
```

### View TensorBoard on Specific Port

```bash
tensorboard --logdir rl_logs --port 6006
```

---

## Utility Commands

### Check Python Version

```bash
python --version
```

### Check CUDA Availability

```bash
nvcc --version
nvidia-smi
```

### Check GPU Information

```bash
nvidia-smi
```

### List All Trained Models

```bash
ls -la rl_models/
# On Windows: dir rl_models
```

### List All Logs

```bash
ls -la rl_logs/
# On Windows: dir rl_logs
```

### Clean Up Temporary Files

```bash
rm -f rl_metrics_temp.json temp_metrics.json temp_collect_metrics.json
# On Windows: del rl_metrics_temp.json temp_metrics.json temp_collect_metrics.json
```

### Clean Up All Generated Files

```bash
rm -rf rl_models/ rl_logs/ *.json *.csv
# On Windows: rmdir /s rl_models rl_logs & del *.json *.csv
```

### Check File Sizes

```bash
du -sh rl_models/ rl_logs/
# On Windows: dir /s rl_models rl_logs
```

---

## Quick Start Workflow

### 1. First Time Setup

```bash
# Install dependencies
cd CUDA_GL_Demo/rl_interface
pip install -r requirements.txt

# Build the project
cd ..
mkdir -p build && cd build
cmake ..
make
```

### 2. Test the Executable

```bash
cd ../rl_interface
python test_executable.py ../build/cuda_gl_demo_rl
```

### 3. Train a Model

```bash
python train_rl.py --executable ../build/cuda_gl_demo_rl --particles 500000 --timesteps 1000
```

### 4. Evaluate the Model

```bash
python evaluate_rl.py rl_models/cuda_gl_rl_final.zip --episodes 10
```

### 5. Run with Optimal Settings

```bash
python run_optimal.py --frames 500
```

### 6. View Training Progress

```bash
tensorboard --logdir rl_logs
```

---

## Common Workflows

### Quick Training Test (Fast)

```bash
python train_rl.py \
    --executable ../build/cuda_gl_demo_rl \
    --particles 500000 \
    --frames 50 \
    --timesteps 100
```

### Full Training Run

```bash
python train_rl.py \
    --executable ../build/cuda_gl_demo_rl \
    --particles 500000 \
    --frames 100 \
    --timesteps 5000 \
    --save-path ./rl_models \
    --log-dir ./rl_logs
```

### Compare Multiple Models

```bash
# Train first model
python train_rl.py --executable ../build/cuda_gl_demo_rl --save-path ./models1 --log-dir ./logs1

# Train second model with different settings
python train_rl.py --executable ../build/cuda_gl_demo_rl --lr 1e-4 --save-path ./models2 --log-dir ./logs2

# Evaluate both
python evaluate_rl.py models1/cuda_gl_rl_final.zip --output eval1.json
python evaluate_rl.py models2/cuda_gl_rl_final.zip --output eval2.json
```

### Production Deployment

```bash
# Train final model
python train_rl.py \
    --executable ../build/cuda_gl_demo_rl \
    --particles 500000 \
    --timesteps 10000 \
    --save-path ./production_models

# Get optimal settings
python run_optimal.py \
    --model production_models/cuda_gl_rl_final.zip \
    --no-demo

# Run with optimal settings
python run_optimal.py \
    --model production_models/cuda_gl_rl_final.zip \
    --frames 1000
```

---

## Troubleshooting Commands

### Check if Executable Exists

```bash
test -f ../build/cuda_gl_demo_rl && echo "Found" || echo "Not found"
# On Windows: if exist ..\build\cuda_gl_demo_rl.exe (echo Found) else (echo Not found)
```

### Check Executable Permissions

```bash
ls -l ../build/cuda_gl_demo_rl
# On Windows: icacls ..\build\cuda_gl_demo_rl.exe
```

### Run Executable Manually

```bash
../build/cuda_gl_demo_rl --particles 10000 --blocksize 256 --frames 10 --output test.json
```

### Check Python Environment

```bash
python -c "import sys; print(sys.executable)"
which python
# On Windows: where python
```

### Check CUDA/OpenGL Setup

```bash
python -c "import torch; print(torch.cuda.is_available())"
```

### View Recent Logs

```bash
tail -f rl_logs/PPO_1/events.out.tfevents.*
# On Windows: type rl_logs\PPO_1\events.out.tfevents.* | more
```

---

## Notes

- All commands assume you're in the `CUDA_GL_Demo/rl_interface` directory unless otherwise specified
- Replace `../build/cuda_gl_demo_rl` with the actual path to your executable if different
- On Windows, use backslashes (`\`) instead of forward slashes (`/`) for paths
- Training can be slow - each step runs a full simulation. Start with small timesteps for testing
- Use `Ctrl+C` to stop training early - progress will be saved
- TensorBoard logs are saved automatically if tensorboard is installed

---

## Additional Resources

- See `README.md` for detailed documentation
- See `example_usage.py` for code examples
- Check TensorBoard logs for training visualization
- Review `CudaGLEnvironment.py` for environment implementation details

