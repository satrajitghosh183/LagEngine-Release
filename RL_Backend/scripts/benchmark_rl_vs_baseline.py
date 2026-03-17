#!/usr/bin/env python3
"""
Benchmark script comparing RL-optimized settings vs baseline.
Shows concrete frame timing improvements and RL-recommended settings.

Usage:
  python scripts/benchmark_rl_vs_baseline.py --checkpoint rl/runs/FrameSchedulerEnv_ppo_s0/model.pt
  python scripts/benchmark_rl_vs_baseline.py --env WorkStealSchedulerEnv --checkpoint rl/runs/WorkStealSchedulerEnv_awss_s0/model.pt
"""

from __future__ import annotations

import argparse
import json
import numpy as np
import sys
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Tuple

# Add rl directory to path
rl_dir = Path(__file__).parent.parent / "rl"
sys.path.insert(0, str(rl_dir))

@dataclass
class BenchmarkResult:
    name: str
    settings: Dict
    frame_times_ms: List[float] = field(default_factory=list)
    
    @property
    def avg_ms(self) -> float:
        return np.mean(self.frame_times_ms) if self.frame_times_ms else 0.0
    
    @property
    def min_ms(self) -> float:
        return np.min(self.frame_times_ms) if self.frame_times_ms else 0.0
    
    @property
    def max_ms(self) -> float:
        return np.max(self.frame_times_ms) if self.frame_times_ms else 0.0
    
    @property
    def std_ms(self) -> float:
        return np.std(self.frame_times_ms) if self.frame_times_ms else 0.0
    
    @property
    def p95_ms(self) -> float:
        return np.percentile(self.frame_times_ms, 95) if self.frame_times_ms else 0.0


def load_env(env_name: str, body_count: Optional[int] = None):
    """Load environment, trying C++ env first, then fallback."""
    try:
        import rldemo_env
        if env_name == "WorkStealSchedulerEnv":
            from envs.engine_env import WorkStealSchedulerEnv
            env = WorkStealSchedulerEnv(body_count=body_count)
        else:
            env = rldemo_env.FrameSchedulerEnv()
        from utils.env_wrapper import ActionScaleWrapper
        low = 0.01 if env_name == "WorkStealSchedulerEnv" else 0.0
        return ActionScaleWrapper(env, low=low, high=1.0)
    except ImportError as e:
        print(f"Error loading environment: {e}")
        print("Build with: cmake .. -DRL_BACKEND_BUILD_RL=ON && cmake --build . -j")
        sys.exit(1)


def load_agent(checkpoint_path: str, device: str):
    """Load trained RL agent from checkpoint."""
    import torch
    from train import load_checkpoint
    return load_checkpoint(checkpoint_path, device)


def run_baseline_benchmark(env, env_name: str, steps: int = 100) -> List[BenchmarkResult]:
    """Run baseline benchmarks with fixed settings."""
    results = []
    
    if env_name == "FrameSchedulerEnv":
        # Test various cube counts
        test_configs = [
            ("baseline_1k_cubes", {"cube_count": 1000, "cam_angle": 0.5}),
            ("baseline_10k_cubes", {"cube_count": 10000, "cam_angle": 0.5}),
            ("baseline_50k_cubes", {"cube_count": 50000, "cam_angle": 0.5}),
            ("baseline_100k_cubes", {"cube_count": 100000, "cam_angle": 0.5}),
        ]
        
        for name, config in test_configs:
            result = BenchmarkResult(name=name, settings=config)
            # Action: [cube_normalized, cam_angle, 0, 0]
            cube_norm = (config["cube_count"] - 100) / 99900.0
            action = np.array([cube_norm, config["cam_angle"], 0.0, 0.0], dtype=np.float32)
            
            env.reset()
            for _ in range(steps):
                obs, reward, done, info = env.step(action)
                result.frame_times_ms.append(info.get("frame_time", 0.0))
                if done:
                    env.reset()
            results.append(result)
            
    else:  # WorkStealSchedulerEnv
        # Test uniform distribution (baseline)
        test_configs = [
            ("baseline_uniform", {"distribution": "uniform"}),
            ("baseline_front_heavy", {"distribution": "front_heavy"}),
            ("baseline_back_heavy", {"distribution": "back_heavy"}),
        ]
        
        act_dim = env.get_act_dim()
        for name, config in test_configs:
            result = BenchmarkResult(name=name, settings=config)
            
            if config["distribution"] == "uniform":
                action = np.ones(act_dim, dtype=np.float32) / act_dim
            elif config["distribution"] == "front_heavy":
                action = np.array([0.3, 0.2, 0.15, 0.1] + [0.25 / (act_dim - 4)] * (act_dim - 4), dtype=np.float32)
            else:
                action = np.array([0.05] * (act_dim - 4) + [0.1, 0.15, 0.3, 0.4], dtype=np.float32)
            
            env.reset()
            for _ in range(steps):
                obs, reward, done, info = env.step(action)
                result.frame_times_ms.append(info.get("frame_time", 0.0))
                if done:
                    env.reset()
            results.append(result)
    
    return results


def run_rl_benchmark(env, agent, env_name: str, steps: int = 100, discrete: bool = False) -> Tuple[BenchmarkResult, Dict]:
    """Run benchmark with RL agent selecting actions."""
    result = BenchmarkResult(name="rl_optimized", settings={})
    actions_taken = []
    
    obs = env.reset()
    if isinstance(obs, tuple):
        obs = obs[0]
    
    for _ in range(steps):
        if discrete:
            action = agent.select_action(obs, epsilon=0)
        else:
            # select_action may return (action, log_prob, value) tuple - extract just action
            result_action = agent.select_action(obs, deterministic=True)
            action = result_action[0] if isinstance(result_action, tuple) else result_action
        
        actions_taken.append(action.copy() if hasattr(action, 'copy') else action)
        
        step_result = env.step(action)
        if len(step_result) == 5:
            obs, reward, term, trunc, info = step_result
            done = term or trunc
        else:
            obs, reward, done, info = step_result
        
        result.frame_times_ms.append(info.get("frame_time", 0.0))
        
        if done:
            obs = env.reset()
            if isinstance(obs, tuple):
                obs = obs[0]
    
    # Analyze RL recommended settings
    actions_array = np.array(actions_taken)
    recommended = {}
    
    if env_name == "FrameSchedulerEnv":
        avg_action = np.mean(actions_array, axis=0)
        # Clamp action to [0, 1] before calculating cube count (as env does)
        cube_norm = max(0.0, min(1.0, avg_action[0]))
        cube_count = int(100 + cube_norm * 99900)
        cam_angle = max(0.0, min(1.0, avg_action[1]))
        recommended = {
            "cube_count": cube_count,
            "cam_angle": float(cam_angle),
            "action_mean": avg_action.tolist(),
            "action_std": np.std(actions_array, axis=0).tolist(),
        }
    else:
        avg_action = np.mean(actions_array, axis=0)
        # Worker batch factors (first 16 dimensions)
        worker_factors = avg_action[:16]
        recommended = {
            "worker_batch_factors": worker_factors.tolist(),
            "worker_factor_sum": float(np.sum(worker_factors)),
            "action_mean": avg_action.tolist(),
            "action_std": np.std(actions_array, axis=0).tolist(),
        }
    
    result.settings = recommended
    return result, recommended


def print_results(baseline_results: List[BenchmarkResult], rl_result: BenchmarkResult, env_name: str):
    """Print formatted benchmark results."""
    print("\n" + "=" * 80)
    print("BENCHMARK RESULTS: RL vs Baseline")
    print("=" * 80)
    
    print(f"\nEnvironment: {env_name}")
    print(f"Steps per benchmark: {len(rl_result.frame_times_ms)}")
    
    # Baseline results
    print("\n--- Baseline Results ---")
    print(f"{'Config':<25} {'Avg (ms)':<12} {'Min (ms)':<12} {'Max (ms)':<12} {'P95 (ms)':<12} {'Std':<10}")
    print("-" * 83)
    
    for r in baseline_results:
        print(f"{r.name:<25} {r.avg_ms:<12.3f} {r.min_ms:<12.3f} {r.max_ms:<12.3f} {r.p95_ms:<12.3f} {r.std_ms:<10.3f}")
    
    # RL result
    print("\n--- RL-Optimized Results ---")
    print(f"{'Config':<25} {'Avg (ms)':<12} {'Min (ms)':<12} {'Max (ms)':<12} {'P95 (ms)':<12} {'Std':<10}")
    print("-" * 83)
    print(f"{rl_result.name:<25} {rl_result.avg_ms:<12.3f} {rl_result.min_ms:<12.3f} {rl_result.max_ms:<12.3f} {rl_result.p95_ms:<12.3f} {rl_result.std_ms:<10.3f}")
    
    # Improvement analysis
    print("\n--- Improvement Analysis ---")
    best_baseline = min(baseline_results, key=lambda r: r.avg_ms)
    improvement_ms = best_baseline.avg_ms - rl_result.avg_ms
    improvement_pct = (improvement_ms / best_baseline.avg_ms) * 100 if best_baseline.avg_ms > 0 else 0
    
    print(f"Best baseline:        {best_baseline.name} ({best_baseline.avg_ms:.3f} ms)")
    print(f"RL-optimized:         {rl_result.avg_ms:.3f} ms")
    print(f"Improvement:          {improvement_ms:.3f} ms ({improvement_pct:+.1f}%)")
    
    if rl_result.avg_ms < 33.33:
        fps = 1000 / rl_result.avg_ms
        print(f"Achieves:             {fps:.1f} FPS (target: 30 FPS)")
    
    # RL Recommended settings
    print("\n--- RL Recommended Settings ---")
    if env_name == "FrameSchedulerEnv":
        print(f"Cube count:           {rl_result.settings.get('cube_count', 'N/A')}")
        print(f"Camera angle:         {rl_result.settings.get('cam_angle', 'N/A'):.3f}")
    else:
        factors = rl_result.settings.get('worker_batch_factors', [])
        if factors:
            print("Worker batch factors (work distribution per worker):")
            for i, f in enumerate(factors[:8]):  # Show first 8 workers
                bar = "█" * int(f * 50)
                print(f"  Worker {i}: {f:.3f} {bar}")
            if len(factors) > 8:
                print(f"  ... ({len(factors) - 8} more workers)")
    
    print("\n" + "=" * 80)
    
    return {
        "baseline_best": best_baseline.name,
        "baseline_avg_ms": best_baseline.avg_ms,
        "rl_avg_ms": rl_result.avg_ms,
        "improvement_ms": improvement_ms,
        "improvement_pct": improvement_pct,
        "recommended_settings": rl_result.settings,
    }


def main():
    parser = argparse.ArgumentParser(description="Benchmark RL vs Baseline frame scheduling")
    parser.add_argument("--checkpoint", type=str, required=True, help="Path to trained model checkpoint")
    parser.add_argument("--env", type=str, default=None, help="Environment name (auto-detected if not specified)")
    parser.add_argument("--steps", type=int, default=200, help="Steps per benchmark run")
    parser.add_argument("--body-count", type=int, default=2000, help="Body count for WorkStealSchedulerEnv")
    parser.add_argument("--device", type=str, default="cpu", help="Device (cpu/cuda)")
    parser.add_argument("--output", type=str, default=None, help="Output JSON file for results")
    args = parser.parse_args()
    
    # Load agent
    checkpoint_path = Path(args.checkpoint)
    if not checkpoint_path.exists():
        print(f"Checkpoint not found: {checkpoint_path}")
        return 1
    
    print(f"Loading agent from: {checkpoint_path}")
    agent, algo, obs_dim, act_dim = load_agent(str(checkpoint_path), args.device)
    discrete = algo in ["dqn", "double_dqn", "dueling_dqn", "c51"]
    
    # Auto-detect environment
    if args.env is None:
        args.env = "WorkStealSchedulerEnv" if act_dim == 21 else "FrameSchedulerEnv"
    
    print(f"Environment: {args.env}")
    print(f"Algorithm: {algo}")
    print(f"Obs dim: {obs_dim}, Act dim: {act_dim}")
    
    # Load environment
    env = load_env(args.env, body_count=args.body_count if args.env == "WorkStealSchedulerEnv" else None)
    
    # Run baseline benchmarks
    print("\nRunning baseline benchmarks...")
    baseline_results = run_baseline_benchmark(env, args.env, steps=args.steps)
    
    # Run RL benchmark
    print("Running RL-optimized benchmark...")
    rl_result, recommended = run_rl_benchmark(env, agent, args.env, steps=args.steps, discrete=discrete)
    
    # Print and save results
    summary = print_results(baseline_results, rl_result, args.env)
    
    if args.output:
        output_path = Path(args.output)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        
        full_results = {
            "environment": args.env,
            "algorithm": algo,
            "checkpoint": str(checkpoint_path),
            "steps": args.steps,
            "summary": summary,
            "baseline_results": [
                {"name": r.name, "avg_ms": r.avg_ms, "min_ms": r.min_ms, "max_ms": r.max_ms, "p95_ms": r.p95_ms}
                for r in baseline_results
            ],
            "rl_result": {
                "avg_ms": rl_result.avg_ms,
                "min_ms": rl_result.min_ms,
                "max_ms": rl_result.max_ms,
                "p95_ms": rl_result.p95_ms,
                "recommended_settings": rl_result.settings,
            },
        }
        
        with open(output_path, "w") as f:
            json.dump(full_results, f, indent=2)
        print(f"\nResults saved to: {output_path}")
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
