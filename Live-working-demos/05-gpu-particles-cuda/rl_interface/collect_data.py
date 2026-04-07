"""
Data collection script for CUDA GL Demo
Collects performance metrics across different parameter configurations
"""
import os
import argparse
import json
import subprocess
import numpy as np
from pathlib import Path
from tqdm import tqdm
import pandas as pd


def find_executable(executable_path: str = None) -> str:
    """Find the CUDA GL demo executable"""
    if executable_path and os.path.exists(executable_path):
        return os.path.abspath(executable_path)
    
    possible_paths = [
        "../build/cuda_gl_demo",
        "build/cuda_gl_demo",
        "./cuda_gl_demo",
        "cuda_gl_demo.exe"
    ]
    
    for path in possible_paths:
        if os.path.exists(path):
            return os.path.abspath(path)
    
    raise FileNotFoundError(
        "Could not find cuda_gl_demo executable. "
        "Please build the project first or specify executable_path."
    )


def run_simulation(executable_path: str, particle_count: int, block_size: int, 
                   num_frames: int = 100, metrics_file: str = "temp_metrics.json") -> dict:
    """Run simulation and return metrics"""
    metrics_path = os.path.abspath(metrics_file)
    
    cmd = [
        executable_path,
        "--particles", str(particle_count),
        "--blocksize", str(block_size),
        "--frames", str(num_frames),
        "--output", metrics_path
    ]
    
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=60.0,
            cwd=os.path.dirname(executable_path) or "."
        )
        
        if result.returncode != 0:
            return None
        
        if os.path.exists(metrics_path):
            with open(metrics_path, 'r') as f:
                metrics = json.load(f)
            return metrics
        else:
            return None
            
    except Exception as e:
        print(f"Error running simulation: {e}")
        return None


def collect_data(
    executable_path: str = None,
    num_frames: int = 100,
    particle_counts: list = None,
    block_sizes: list = None,
    output_file: str = "collected_data.json",
    csv_file: str = "collected_data.csv"
):
    """Collect performance data across different configurations"""
    
    executable_path = find_executable(executable_path)
    
    if particle_counts is None:
        particle_counts = [10000, 25000, 50000, 100000, 200000, 300000, 400000, 500000]
    
    if block_sizes is None:
        block_sizes = [128, 256, 512, 1024]
    
    print("=" * 60)
    print("CUDA GL Demo - Data Collection")
    print("=" * 60)
    print(f"Executable: {executable_path}")
    print(f"Frames per run: {num_frames}")
    print(f"Particle counts: {particle_counts}")
    print(f"Block sizes: {block_sizes}")
    print(f"Total configurations: {len(particle_counts) * len(block_sizes)}")
    print("=" * 60)
    
    data = []
    metrics_file = "temp_collect_metrics.json"
    
    total_configs = len(particle_counts) * len(block_sizes)
    
    with tqdm(total=total_configs, desc="Collecting data") as pbar:
        for particle_count in particle_counts:
            for block_size in block_sizes:
                pbar.set_description(
                    f"Particles: {particle_count}, Block Size: {block_size}"
                )
                
                metrics = run_simulation(
                    executable_path,
                    particle_count,
                    block_size,
                    num_frames,
                    metrics_file
                )
                
                if metrics:
                    metrics['particle_count'] = particle_count
                    metrics['block_size'] = block_size
                    data.append(metrics)
                
                pbar.update(1)
    
    # Clean up temp file
    if os.path.exists(metrics_file):
        os.remove(metrics_file)
    
    # Save as JSON
    with open(output_file, 'w') as f:
        json.dump(data, f, indent=2)
    
    # Save as CSV for easy analysis
    if data:
        df = pd.DataFrame(data)
        df.to_csv(csv_file, index=False)
        print(f"\nData saved to:")
        print(f"  JSON: {output_file}")
        print(f"  CSV: {csv_file}")
        print(f"\nCollected {len(data)} data points")
        print(f"\nSummary statistics:")
        print(df[['particle_count', 'block_size', 'avg_fps', 'avg_frame_time_ms']].describe())
    else:
        print("\nNo data collected!")
    
    return data


def main():
    parser = argparse.ArgumentParser(description="Collect performance data for CUDA GL Demo")
    parser.add_argument(
        "--executable",
        type=str,
        default=None,
        help="Path to cuda_gl_demo executable (default: auto-detect)"
    )
    parser.add_argument(
        "--frames",
        type=int,
        default=100,
        help="Number of frames per simulation (default: 100)"
    )
    parser.add_argument(
        "--particles",
        type=int,
        nargs="+",
        default=None,
        help="Particle counts to test (default: [10000, 25000, 50000, 100000, 200000, 300000, 400000, 500000])"
    )
    parser.add_argument(
        "--blocksizes",
        type=int,
        nargs="+",
        default=None,
        help="Block sizes to test (default: [128, 256, 512, 1024])"
    )
    parser.add_argument(
        "--output",
        type=str,
        default="collected_data.json",
        help="Output JSON file (default: collected_data.json)"
    )
    parser.add_argument(
        "--csv",
        type=str,
        default="collected_data.csv",
        help="Output CSV file (default: collected_data.csv)"
    )
    
    args = parser.parse_args()
    
    collect_data(
        executable_path=args.executable,
        num_frames=args.frames,
        particle_counts=args.particles,
        block_sizes=args.blocksizes,
        output_file=args.output,
        csv_file=args.csv
    )


if __name__ == "__main__":
    main()

