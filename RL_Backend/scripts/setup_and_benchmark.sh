#!/bin/bash
# RL_Backend: Full setup, build, train, and benchmark script for WSL/Linux
# Usage: bash scripts/setup_and_benchmark.sh [--quick] [--no-train]

set -e

# Parse arguments
QUICK_MODE=false
NO_TRAIN=false
TRAIN_STEPS=50000

while [[ $# -gt 0 ]]; do
    case $1 in
        --quick)
            QUICK_MODE=true
            TRAIN_STEPS=10000
            shift
            ;;
        --no-train)
            NO_TRAIN=true
            shift
            ;;
        *)
            shift
            ;;
    esac
done

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_header() {
    echo -e "\n${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}\n"
}

print_success() {
    echo -e "${GREEN}[OK] $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}[!] $1${NC}"
}

print_error() {
    echo -e "${RED}[X] $1${NC}"
}

# Find project root
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"
cd "$PROJECT_ROOT"

echo "Project root: $PROJECT_ROOT"

# ========================================
# Step 1: Build C++ project
# ========================================
print_header "Step 1: Building C++ project"

BUILD_DIR="$PROJECT_ROOT/build"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo "Configuring with CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DRL_BACKEND_BUILD_RL=ON \
    -DRL_BACKEND_BUILD_TESTS=ON

# Build
echo "Building..."
NPROC=$(nproc 2>/dev/null || echo 4)
cmake --build . -j$NPROC

if [ -f "$BUILD_DIR/bin/test_jobs" ]; then
    print_success "Build complete: test_jobs found"
else
    print_error "Build failed: test_jobs not found"
    exit 1
fi

if [ -f "$BUILD_DIR/bin/jobbench" ]; then
    print_success "Build complete: jobbench found"
else
    print_warning "jobbench not found - some benchmarks may fail"
fi

# Check for Python bindings
if ls "$BUILD_DIR/lib/"rldemo_env*.so 2>/dev/null; then
    print_success "Python bindings built"
    RLDEMO_LIB_PATH="$BUILD_DIR/lib"
else
    print_warning "Python bindings not found in lib/, checking bindings/"
    if ls "$BUILD_DIR/bindings/"rldemo_env*.so 2>/dev/null; then
        print_success "Python bindings found in bindings/"
        RLDEMO_LIB_PATH="$BUILD_DIR/bindings"
    else
        print_error "Python bindings not found"
        exit 1
    fi
fi

# ========================================
# Step 2: Run unit tests
# ========================================
print_header "Step 2: Running unit tests"

cd "$BUILD_DIR"
if ctest --output-on-failure -V; then
    print_success "All tests passed"
else
    print_warning "Some tests failed - continuing anyway"
fi

# ========================================
# Step 3: Run JobBench
# ========================================
print_header "Step 3: Running JobBench microbenchmark"

if [ -f "$BUILD_DIR/bin/jobbench" ]; then
    if $QUICK_MODE; then
        "$BUILD_DIR/bin/jobbench" --workers 2,4 --depth 3 --breadth 4
    else
        "$BUILD_DIR/bin/jobbench" --workers 1,2,4,8
    fi
    print_success "JobBench complete"
else
    print_warning "Skipping JobBench - executable not found"
fi

# ========================================
# Step 4: Setup Python environment
# ========================================
print_header "Step 4: Setting up Python environment"

cd "$PROJECT_ROOT/rl"

# Create venv if it doesn't exist
if [ ! -d ".venv" ]; then
    echo "Creating Python virtual environment..."
    python3 -m venv .venv
fi

# Activate venv
source .venv/bin/activate

# Install dependencies
echo "Installing Python dependencies..."
pip install --upgrade pip -q
pip install -r requirements.txt -q

# Add library path
export PYTHONPATH="$RLDEMO_LIB_PATH:$PYTHONPATH"

# Test import
echo "Testing rldemo_env import..."
if python3 -c "import rldemo_env; print(f'rldemo_env loaded: obs_dim={rldemo_env.FrameSchedulerEnv().get_obs_dim()}')"; then
    print_success "Python bindings work"
else
    print_error "Failed to import rldemo_env"
    echo "Library path: $RLDEMO_LIB_PATH"
    ls -la "$RLDEMO_LIB_PATH"/*.so 2>/dev/null || true
    exit 1
fi

# ========================================
# Step 5: Train RL agent (if needed)
# ========================================
print_header "Step 5: Training RL agent"

CHECKPOINT_PPO="$PROJECT_ROOT/rl/runs/FrameSchedulerEnv_ppo_s0/model.pt"
CHECKPOINT_AWSS="$PROJECT_ROOT/rl/runs/WorkStealSchedulerEnv_awss_s0/model.pt"

if [ "$NO_TRAIN" = true ]; then
    print_warning "Skipping training (--no-train specified)"
elif [ -f "$CHECKPOINT_PPO" ] && [ -f "$CHECKPOINT_AWSS" ]; then
    print_success "Checkpoints already exist, skipping training"
    echo "  - $CHECKPOINT_PPO"
    echo "  - $CHECKPOINT_AWSS"
else
    echo "Training PPO on FrameSchedulerEnv ($TRAIN_STEPS steps)..."
    python3 train.py --algo ppo --env FrameSchedulerEnv --steps $TRAIN_STEPS --seed 0 --device cpu
    
    echo "Training AWSS on WorkStealSchedulerEnv ($TRAIN_STEPS steps)..."
    python3 train.py --algo awss --env WorkStealSchedulerEnv --steps $TRAIN_STEPS --seed 0 --device cpu --curriculum
    
    print_success "Training complete"
fi

# ========================================
# Step 6: Run RL vs Baseline benchmark
# ========================================
print_header "Step 6: Running RL vs Baseline benchmark"

cd "$PROJECT_ROOT"

BENCH_STEPS=100
if $QUICK_MODE; then
    BENCH_STEPS=50
fi

# FrameSchedulerEnv benchmark
if [ -f "$CHECKPOINT_PPO" ]; then
    echo "Benchmarking FrameSchedulerEnv (PPO)..."
    python3 scripts/benchmark_rl_vs_baseline.py \
        --checkpoint "$CHECKPOINT_PPO" \
        --env FrameSchedulerEnv \
        --steps $BENCH_STEPS \
        --output "benchmark_frame_scheduler_results.json"
else
    print_warning "No PPO checkpoint found, skipping FrameSchedulerEnv benchmark"
fi

# WorkStealSchedulerEnv benchmark
if [ -f "$CHECKPOINT_AWSS" ]; then
    echo "Benchmarking WorkStealSchedulerEnv (AWSS)..."
    python3 scripts/benchmark_rl_vs_baseline.py \
        --checkpoint "$CHECKPOINT_AWSS" \
        --env WorkStealSchedulerEnv \
        --steps $BENCH_STEPS \
        --body-count 2000 \
        --output "benchmark_work_steal_results.json"
else
    print_warning "No AWSS checkpoint found, skipping WorkStealSchedulerEnv benchmark"
fi

# ========================================
# Step 7: Generate final report
# ========================================
print_header "Step 7: Summary"

echo "Build directory:     $BUILD_DIR"
echo "Python environment:  $PROJECT_ROOT/rl/.venv"
echo "Library path:        $RLDEMO_LIB_PATH"

if [ -f "benchmark_frame_scheduler_results.json" ]; then
    echo ""
    echo "FrameSchedulerEnv results saved to: benchmark_frame_scheduler_results.json"
fi

if [ -f "benchmark_work_steal_results.json" ]; then
    echo "WorkStealSchedulerEnv results saved to: benchmark_work_steal_results.json"
fi

echo ""
echo "To view TensorBoard logs:"
echo "  cd $PROJECT_ROOT/rl && source .venv/bin/activate && tensorboard --logdir runs"

print_success "Setup and benchmark complete!"
