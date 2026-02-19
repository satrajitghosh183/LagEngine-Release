#!/bin/bash
# Quick start script for CUDA GL Demo RL

set -e

echo "=========================================="
echo "CUDA GL Demo - RL Quick Start"
echo "=========================================="

# Check if Python is available
if ! command -v python3 &> /dev/null; then
    echo "Error: python3 not found. Please install Python 3.8+"
    exit 1
fi

# Check if pip is available
if ! command -v pip3 &> /dev/null; then
    echo "Error: pip3 not found. Please install pip"
    exit 1
fi

# Install dependencies
echo ""
echo "Installing Python dependencies..."
pip3 install -r requirements.txt

# Check if executable exists
EXECUTABLE="../build/cuda_gl_demo"
if [ ! -f "$EXECUTABLE" ]; then
    echo ""
    echo "Warning: Executable not found at $EXECUTABLE"
    echo "Please build the project first:"
    echo "  cd ../build"
    echo "  cmake .."
    echo "  make"
    echo ""
    read -p "Continue anyway? (y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
    EXECUTABLE=""
fi

# Run a quick test
echo ""
echo "Running quick test..."
python3 example_usage.py basic

echo ""
echo "=========================================="
echo "Quick start complete!"
echo "=========================================="
echo ""
echo "Next steps:"
echo "  1. Collect data:    python3 collect_data.py"
echo "  2. Train model:     python3 train_rl.py --timesteps 500"
echo "  3. Evaluate model:  python3 evaluate_rl.py rl_models/cuda_gl_rl_final.zip"
echo ""
echo "For more information, see README.md"
echo ""

