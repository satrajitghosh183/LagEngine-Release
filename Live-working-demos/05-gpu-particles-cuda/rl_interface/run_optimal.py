"""
Run CUDA GL Demo with optimal settings from trained RL models
This script loads trained models (PPO_1, PPO_2, etc.) and runs the demo with optimal settings
"""
import os
import sys
import argparse
import json
import numpy as np
from pathlib import Path
from stable_baselines3 import PPO
from stable_baselines3.common.vec_env import DummyVecEnv
import subprocess

from CudaGLEnvironment import CudaGLEnvironment


def find_trained_models(log_dir: str = "./rl_logs") -> list:
    """Find all trained models in the logs directory"""
    models = []
    log_path = Path(log_dir)
    
    if not log_path.exists():
        return models
    
    # Look for model directories (PPO_1, PPO_2, etc.)
    for item in log_path.iterdir():
        if item.is_dir() and item.name.startswith("PPO_"):
            # Look for best_model.zip or final model
            best_model = item / "best_model.zip"
            if best_model.exists():
                models.append({
                    "name": item.name,
                    "path": str(best_model),
                    "type": "best"
                })
    
    # Also check rl_models directory
    models_dir = Path("./rl_models")
    if models_dir.exists():
        final_model = models_dir / "cuda_gl_rl_final.zip"
        if final_model.exists():
            models.append({
                "name": "final",
                "path": str(final_model),
                "type": "final"
            })
        
        # Look for checkpoint models
        for item in models_dir.glob("cuda_gl_rl_*.zip"):
            if "final" not in item.name:
                models.append({
                    "name": item.stem.replace("cuda_gl_rl_", ""),
                    "path": str(item),
                    "type": "checkpoint"
                })
    
    return models


def get_optimal_action(model, env, deterministic: bool = True):
    """Get optimal action from the trained model"""
    obs = env.reset()
    action, _states = model.predict(obs, deterministic=deterministic)
    return action[0], obs[0]


def run_demo_with_settings(executable_path: str, particle_count: int, block_size: int, 
                           num_frames: int = 100, output_file: str = "optimal_run.json"):
    """Run the CUDA GL demo with specific settings"""
    metrics_path = os.path.abspath(output_file)
    
    cmd = [
        executable_path,
        "--particles", str(particle_count),
        "--blocksize", str(block_size),
        "--frames", str(num_frames),
        "--output", metrics_path
    ]
    
    print(f"Running CUDA GL Demo with optimal settings...")
    print(f"  Command: {' '.join(cmd)}")
    print()
    
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=120.0,
            cwd=os.path.dirname(executable_path) or "."
        )
        
        if result.returncode != 0:
            print(f"ERROR: Demo failed with return code {result.returncode}")
            if result.stderr:
                print(f"Error output: {result.stderr}")
            return None
        
        if os.path.exists(metrics_path):
            with open(metrics_path, 'r') as f:
                metrics = json.load(f)
            return metrics
        else:
            print(f"ERROR: Metrics file not found: {metrics_path}")
            return None
            
    except Exception as e:
        print(f"ERROR: {e}")
        return None


def main():
    parser = argparse.ArgumentParser(
        description="Run CUDA GL Demo with optimal settings from trained RL models"
    )
    parser.add_argument(
        "--model",
        type=str,
        default=None,
        help="Path to trained model (default: auto-detect best model)"
    )
    parser.add_argument(
        "--model-name",
        type=str,
        default=None,
        help="Name of model to use (PPO_1, PPO_2, final, etc.)"
    )
    parser.add_argument(
        "--executable",
        type=str,
        default=None,
        help="Path to cuda_gl_demo executable (default: auto-detect)"
    )
    parser.add_argument(
        "--particles",
        type=int,
        default=500000,
        help="Fixed particle count (default: 500000)"
    )
    parser.add_argument(
        "--frames",
        type=int,
        default=500,
        help="Number of frames to run (default: 500)"
    )
    parser.add_argument(
        "--list-models",
        action="store_true",
        help="List all available trained models"
    )
    parser.add_argument(
        "--output",
        type=str,
        default="optimal_run.json",
        help="Output file for metrics (default: optimal_run.json)"
    )
    parser.add_argument(
        "--no-demo",
        action="store_true",
        help="Only show optimal settings, don't run the demo"
    )
    
    args = parser.parse_args()
    
    # List models if requested
    if args.list_models:
        print("=" * 60)
        print("Available Trained Models:")
        print("=" * 60)
        models = find_trained_models()
        if not models:
            print("No trained models found.")
            print("Train a model first using: python train_rl.py")
            return
        
        for i, model in enumerate(models, 1):
            print(f"{i}. {model['name']} ({model['type']})")
            print(f"   Path: {model['path']}")
        print("=" * 60)
        return
    
    # Find or load model
    if args.model:
        model_path = args.model
    elif args.model_name:
        models = find_trained_models()
        model = next((m for m in models if m['name'] == args.model_name), None)
        if not model:
            print(f"ERROR: Model '{args.model_name}' not found.")
            print("Available models:")
            for m in models:
                print(f"  - {m['name']}")
            return
        model_path = model['path']
    else:
        # Auto-detect best model
        models = find_trained_models()
        if not models:
            print("ERROR: No trained models found.")
            print("Train a model first using: python train_rl.py")
            return
        
        # Prefer best model, then final, then latest checkpoint
        best_model = next((m for m in models if m['type'] == 'best'), None)
        if not best_model:
            best_model = next((m for m in models if m['type'] == 'final'), None)
        if not best_model:
            best_model = models[-1]  # Latest checkpoint
        
        model_path = best_model['path']
        print(f"Using model: {best_model['name']} ({best_model['type']})")
    
    if not os.path.exists(model_path):
        print(f"ERROR: Model file not found: {model_path}")
        return
    
    print("=" * 60)
    print("CUDA GL Demo - Optimal Settings Runner")
    print("=" * 60)
    print(f"Model: {model_path}")
    print(f"Particle Count: {args.particles:,} (fixed)")
    print("=" * 60)
    print()
    
    # Load model
    print("Loading trained model...")
    try:
        model = PPO.load(model_path)
        print("✓ Model loaded successfully")
    except Exception as e:
        print(f"ERROR: Failed to load model: {e}")
        return
    
    # Create environment to get optimal action
    print("Getting optimal settings from model...")
    env = DummyVecEnv([
        lambda: CudaGLEnvironment(
            executable_path=args.executable,
            num_frames=100,  # Just for getting action, not actual run
            target_fps=60.0,
            fixed_particle_count=args.particles
        )
    ])
    
    # Get optimal action
    action, obs = get_optimal_action(model, env, deterministic=True)
    
    # Decode action
    block_size_idx = int(np.clip(action[0], 0, 3))
    grid_size_factor = action[1]
    
    block_sizes = [128, 256, 512, 1024]
    optimal_block_size = block_sizes[block_size_idx]
    
    print()
    print("=" * 60)
    print("Optimal Settings from RL Model:")
    print("=" * 60)
    print(f"Block Size: {optimal_block_size} (index: {block_size_idx})")
    print(f"Grid Size Factor: {grid_size_factor:.3f}")
    print(f"Particle Count: {args.particles:,} (fixed)")
    print("=" * 60)
    print()
    
    # Show observation details
    print("Model Observation (current state):")
    print(f"  FPS: {obs[0]:.2f}")
    print(f"  Frame Time: {obs[1]:.2f} ms")
    print(f"  CUDA Time: {obs[2]:.2f} ms")
    print(f"  CPU Time: {obs[3]:.2f} ms")
    print(f"  Upload Time: {obs[4]:.2f} ms")
    print(f"  Render Time: {obs[5]:.2f} ms")
    print(f"  Block Size Norm: {obs[6]:.3f}")
    print(f"  GPU Utilization: {obs[7]:.2%}")
    print()
    
    if args.no_demo:
        print("Skipping demo run (--no-demo flag set)")
        return
    
    # Run demo with optimal settings
    print("=" * 60)
    print("Running CUDA GL Demo with Optimal Settings")
    print("=" * 60)
    
    executable_path = args.executable
    if not executable_path:
        # Try to find it
        possible_paths = [
            "../build/cuda_gl_demo_rl",
            "../build/cuda_gl_demo",
            "build/cuda_gl_demo_rl",
            "build/cuda_gl_demo",
        ]
        for path in possible_paths:
            if os.path.exists(path):
                executable_path = os.path.abspath(path)
                break
    
    if not executable_path or not os.path.exists(executable_path):
        print("ERROR: Could not find CUDA GL demo executable")
        print("Please specify --executable or build the project")
        return
    
    metrics = run_demo_with_settings(
        executable_path,
        args.particles,
        optimal_block_size,
        args.frames,
        args.output
    )
    
    if metrics:
        print()
        print("=" * 60)
        print("Performance Results:")
        print("=" * 60)
        print(f"Particle Count: {metrics.get('particle_count', 0):,}")
        print(f"Block Size: {metrics.get('block_size', 0)}")
        print(f"Frames: {metrics.get('frames', 0)}")
        print()
        print(f"Average FPS: {metrics.get('avg_fps', 0):.2f}")
        print(f"Average Frame Time: {metrics.get('avg_frame_time_ms', 0):.2f} ms")
        print()
        print("Component Times:")
        print(f"  CUDA Time: {metrics.get('avg_cuda_time_ms', 0):.2f} ms")
        print(f"  CPU Time: {metrics.get('avg_cpu_time_ms', 0):.2f} ms")
        print(f"  Upload Time: {metrics.get('avg_upload_time_ms', 0):.2f} ms")
        print(f"  Render Time: {metrics.get('avg_render_time_ms', 0):.2f} ms")
        print()
        
        frame_time = metrics.get('avg_frame_time_ms', 1.0)
        cuda_time = metrics.get('avg_cuda_time_ms', 0.0)
        gpu_util = (cuda_time / frame_time * 100) if frame_time > 0 else 0.0
        print(f"GPU Utilization: {gpu_util:.2f}%")
        print("=" * 60)
        print()
        print(f"Full metrics saved to: {args.output}")
    else:
        print("ERROR: Failed to run demo or collect metrics")
    
    env.close()


if __name__ == "__main__":
    main()

