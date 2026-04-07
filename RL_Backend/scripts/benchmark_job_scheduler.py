#!/usr/bin/env python3
"""
Benchmark script for JobSchedulerEnv: RL-optimized work distribution vs baselines.

This demonstrates how RL can optimize thread work allocation to minimize frame time
when the workload is FIXED (100k cubes). The agent learns optimal batch factors
for each worker thread.

Usage:
  python scripts/benchmark_job_scheduler.py --checkpoint rl/runs/JobSchedulerEnv_ppo_s0/model.pt
  python scripts/benchmark_job_scheduler.py --train --steps 30000  # Train first, then benchmark
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
    distribution: str
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
    
    @property
    def p99_ms(self) -> float:
        return np.percentile(self.frame_times_ms, 99) if self.frame_times_ms else 0.0


def load_env(cube_count: int = 100000):
    """Load JobSchedulerEnv."""
    try:
        import rldemo_env
        env = rldemo_env.JobSchedulerEnv()
        env.set_cube_count(cube_count)
        from utils.env_wrapper import ActionScaleWrapper
        return ActionScaleWrapper(env, low=0.01, high=1.0)
    except ImportError as e:
        print(f"Error loading environment: {e}")
        print("Build with: cmake .. -DRL_BACKEND_BUILD_RL=ON && cmake --build . -j")
        sys.exit(1)


def load_agent(checkpoint_path: str, device: str):
    """Load trained RL agent from checkpoint."""
    import torch
    from train import load_checkpoint
    return load_checkpoint(checkpoint_path, device)


def create_distribution(name: str, num_workers: int) -> np.ndarray:
    """Create different work distribution strategies."""
    if name == "uniform":
        # Equal work for all workers
        return np.ones(num_workers) / num_workers
    elif name == "front_heavy":
        # More work to first workers
        weights = np.array([1.0 / (i + 1) for i in range(num_workers)])
        return weights / weights.sum()
    elif name == "back_heavy":
        # More work to last workers (SHOULD BE GOOD for non-uniform workload)
        weights = np.array([1.0 / (num_workers - i) for i in range(num_workers)])
        return weights / weights.sum()
    elif name == "alternating":
        # Alternating high/low
        weights = np.array([2.0 if i % 2 == 0 else 1.0 for i in range(num_workers)])
        return weights / weights.sum()
    elif name == "single_heavy":
        # One worker gets most work
        weights = np.ones(num_workers)
        weights[0] = num_workers
        return weights / weights.sum()
    elif name == "complexity_aware":
        # Optimal for our workload: first 30% cubes are 3x complex
        # So first ~30% of work takes ~60% of total time
        # Give first workers (handling complex cubes) LESS work
        weights = np.ones(num_workers)
        complex_workers = max(1, num_workers * 3 // 10)  # First 30% of workers
        for i in range(complex_workers):
            weights[i] = 0.5  # Give them half the normal amount
        return weights / weights.sum()
    elif name == "linear_decrease":
        # Linearly decreasing work (first worker gets least)
        weights = np.array([i + 1 for i in range(num_workers)], dtype=float)
        return weights / weights.sum()
    else:
        return np.ones(num_workers) / num_workers


def run_baseline_benchmarks(env, num_workers: int, steps: int = 100) -> List[BenchmarkResult]:
    """Run benchmarks with different fixed distributions."""
    distributions = ["uniform", "front_heavy", "back_heavy", "alternating", "single_heavy", 
                     "complexity_aware", "linear_decrease"]
    results = []
    
    for dist_name in distributions:
        result = BenchmarkResult(name=f"baseline_{dist_name}", distribution=dist_name)
        action = create_distribution(dist_name, num_workers)
        # Pad to 16 dims if needed
        if len(action) < 16:
            action = np.concatenate([action, np.zeros(16 - len(action))])
        action = action.astype(np.float32)
        
        env.reset()
        for _ in range(steps):
            obs, reward, done, info = env.step(action)
            result.frame_times_ms.append(info.get("frame_time", 0.0))
            if done:
                env.reset()
        results.append(result)
        print(f"  {dist_name}: avg={result.avg_ms:.2f}ms, std={result.std_ms:.2f}ms")
    
    return results


def run_rl_benchmark(env, agent, steps: int = 100) -> Tuple[BenchmarkResult, np.ndarray]:
    """Run benchmark with RL agent selecting work distribution."""
    result = BenchmarkResult(name="rl_optimized", distribution="learned")
    actions_taken = []
    
    obs = env.reset()
    if isinstance(obs, tuple):
        obs = obs[0]
    
    for _ in range(steps):
        # Get action from agent
        action_result = agent.select_action(obs, deterministic=True)
        action = action_result[0] if isinstance(action_result, tuple) else action_result
        
        actions_taken.append(action.copy())
        
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
    
    avg_action = np.mean(actions_taken, axis=0)
    return result, avg_action


def print_results(baseline_results: List[BenchmarkResult], rl_result: BenchmarkResult, 
                  rl_distribution: np.ndarray, num_workers: int, cube_count: int):
    """Print formatted benchmark results."""
    print("\n" + "=" * 90)
    print("JOB SCHEDULER OPTIMIZATION BENCHMARK")
    print("=" * 90)
    print(f"\nWorkload: {cube_count:,} cubes (FIXED)")
    print(f"Workers: {num_workers}")
    print(f"Steps per benchmark: {len(rl_result.frame_times_ms)}")
    
    print("\n" + "-" * 90)
    print("BASELINE DISTRIBUTIONS (fixed work allocation strategies)")
    print("-" * 90)
    print(f"{'Strategy':<20} {'Avg (ms)':<12} {'Std (ms)':<12} {'P95 (ms)':<12} {'P99 (ms)':<12} {'Min-Max':<20}")
    print("-" * 90)
    
    for r in baseline_results:
        minmax = f"{r.min_ms:.2f}-{r.max_ms:.2f}"
        print(f"{r.distribution:<20} {r.avg_ms:<12.3f} {r.std_ms:<12.3f} {r.p95_ms:<12.3f} {r.p99_ms:<12.3f} {minmax:<20}")
    
    print("\n" + "-" * 90)
    print("RL-OPTIMIZED DISTRIBUTION (learned work allocation)")
    print("-" * 90)
    minmax = f"{rl_result.min_ms:.2f}-{rl_result.max_ms:.2f}"
    print(f"{'learned':<20} {rl_result.avg_ms:<12.3f} {rl_result.std_ms:<12.3f} {rl_result.p95_ms:<12.3f} {rl_result.p99_ms:<12.3f} {minmax:<20}")
    
    # Find best baseline
    best_baseline = min(baseline_results, key=lambda r: r.avg_ms)
    improvement_ms = best_baseline.avg_ms - rl_result.avg_ms
    improvement_pct = (improvement_ms / best_baseline.avg_ms) * 100 if best_baseline.avg_ms > 0 else 0
    
    print("\n" + "-" * 90)
    print("IMPROVEMENT ANALYSIS")
    print("-" * 90)
    print(f"Best baseline:     {best_baseline.distribution} ({best_baseline.avg_ms:.3f} ms)")
    print(f"RL-optimized:      {rl_result.avg_ms:.3f} ms")
    
    if improvement_ms > 0:
        print(f"Improvement:       {improvement_ms:.3f} ms faster ({improvement_pct:+.1f}%)")
    else:
        print(f"Difference:        {-improvement_ms:.3f} ms slower ({improvement_pct:+.1f}%)")
    
    # Variance improvement
    if rl_result.std_ms < best_baseline.std_ms:
        var_improvement = (1 - rl_result.std_ms / best_baseline.std_ms) * 100
        print(f"Variance:          {var_improvement:.1f}% more consistent")
    
    fps = 1000 / rl_result.avg_ms if rl_result.avg_ms > 0 else 0
    print(f"Achieved FPS:      {fps:.1f}")
    
    # Show learned distribution
    print("\n" + "-" * 90)
    print("RL LEARNED WORK DISTRIBUTION (batch factors per worker)")
    print("-" * 90)
    
    # Clamp and normalize (like the environment does)
    clamped = np.clip(rl_distribution[:num_workers], 0.01, 1.0)
    dist_sum = clamped.sum()
    normalized = clamped / dist_sum if dist_sum > 0 else clamped
    
    for i in range(num_workers):
        pct = normalized[i] * 100
        bar_len = int(pct * 3)  # Scale bar length
        bar = "█" * min(bar_len, 50)  # Cap at 50 chars
        print(f"Worker {i:2d}: {normalized[i]:.3f} ({pct:5.1f}%) {bar}")
    
    # Compare to uniform
    uniform = 1.0 / num_workers
    print(f"\n(Uniform would be: {uniform:.3f} = {100/num_workers:.1f}% per worker)")
    
    print("\n" + "=" * 90)
    
    return {
        "best_baseline": best_baseline.distribution,
        "baseline_avg_ms": best_baseline.avg_ms,
        "rl_avg_ms": rl_result.avg_ms,
        "improvement_ms": improvement_ms,
        "improvement_pct": improvement_pct,
        "rl_distribution": normalized.tolist(),
    }


def train_agent(env, steps: int, seed: int = 0):
    """Train a PPO agent on the environment."""
    from utils.seeding import seed_all
    from utils.logger import Logger
    from train import get_agent, run_onpolicy
    
    seed_all(seed)
    
    obs_dim = env.get_obs_dim()
    act_dim = env.get_act_dim()
    
    logdir = Path("runs") / f"JobSchedulerEnv_ppo_s{seed}"
    logdir.mkdir(parents=True, exist_ok=True)
    logger = Logger(str(logdir), "ppo")
    
    agent = get_agent("ppo", obs_dim, act_dim, "cpu")
    print(f"Training PPO on JobSchedulerEnv | obs={obs_dim} act={act_dim}")
    print(f"Steps: {steps}, Logdir: {logdir}")
    
    run_onpolicy(env, agent, steps, rollout_len=1024, logger=logger, algo="ppo")
    
    # Save checkpoint
    import torch
    ckpt_path = logdir / "model.pt"
    state = {
        "algo": "ppo",
        "obs_dim": obs_dim,
        "act_dim": act_dim,
        "actor": agent.actor.state_dict(),
        "critic": agent.critic.state_dict(),
    }
    torch.save(state, ckpt_path)
    logger.close()
    
    print(f"Training complete. Checkpoint saved to {ckpt_path}")
    return agent, str(ckpt_path)


def main():
    parser = argparse.ArgumentParser(description="Benchmark RL vs baseline job scheduling")
    parser.add_argument("--checkpoint", type=str, default=None, help="Path to trained model checkpoint")
    parser.add_argument("--train", action="store_true", help="Train a new agent before benchmarking")
    parser.add_argument("--steps", type=int, default=30000, help="Training steps if --train")
    parser.add_argument("--benchmark-steps", type=int, default=200, help="Steps per benchmark run")
    parser.add_argument("--cube-count", type=int, default=100000, help="Number of cubes (fixed workload)")
    parser.add_argument("--seed", type=int, default=0, help="Random seed")
    parser.add_argument("--output", type=str, default=None, help="Output JSON file")
    args = parser.parse_args()
    
    print("Loading environment...")
    env = load_env(cube_count=args.cube_count)
    
    # Get actual worker count from env
    obs = env.reset()
    if isinstance(obs, tuple):
        obs = obs[0]
    # Worker count is encoded in observation
    obs_dim = env.get_obs_dim()
    act_dim = env.get_act_dim()
    num_workers = act_dim  # Action dim = number of workers (16 max)
    
    # For actual benchmarking, we'll use the real worker count
    import rldemo_env
    temp_env = rldemo_env.JobSchedulerEnv()
    temp_env.reset()
    # Get from the info dict
    _, _, _, info = temp_env.step(np.ones(16, dtype=np.float32) * 0.5)
    actual_workers = info.get("worker_count", 8)
    del temp_env
    
    print(f"Environment: JobSchedulerEnv")
    print(f"Cube count: {args.cube_count:,} (fixed)")
    print(f"Worker threads: {actual_workers}")
    print(f"Observation dim: {obs_dim}, Action dim: {act_dim}")
    
    # Train or load agent
    if args.train:
        print(f"\nTraining new agent ({args.steps} steps)...")
        agent, checkpoint_path = train_agent(env, args.steps, args.seed)
        args.checkpoint = checkpoint_path
    elif args.checkpoint:
        checkpoint_path = Path(args.checkpoint)
        if not checkpoint_path.exists():
            print(f"Checkpoint not found: {checkpoint_path}")
            return 1
        print(f"\nLoading agent from: {checkpoint_path}")
        agent, algo, _, _ = load_agent(str(checkpoint_path), "cpu")
    else:
        # Check for existing checkpoint
        default_ckpt = Path("rl/runs/JobSchedulerEnv_ppo_s0/model.pt")
        if default_ckpt.exists():
            print(f"\nLoading existing checkpoint: {default_ckpt}")
            agent, algo, _, _ = load_agent(str(default_ckpt), "cpu")
            args.checkpoint = str(default_ckpt)
        else:
            print("\nNo checkpoint found. Training new agent...")
            agent, checkpoint_path = train_agent(env, args.steps, args.seed)
            args.checkpoint = checkpoint_path
    
    # Run baseline benchmarks
    print(f"\nRunning baseline benchmarks ({args.benchmark_steps} steps each)...")
    baseline_results = run_baseline_benchmarks(env, actual_workers, steps=args.benchmark_steps)
    
    # Run RL benchmark
    print(f"\nRunning RL-optimized benchmark...")
    rl_result, rl_distribution = run_rl_benchmark(env, agent, steps=args.benchmark_steps)
    print(f"  RL: avg={rl_result.avg_ms:.2f}ms, std={rl_result.std_ms:.2f}ms")
    
    # Print and save results
    summary = print_results(baseline_results, rl_result, rl_distribution, actual_workers, args.cube_count)
    
    if args.output:
        output_path = Path(args.output)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        
        full_results = {
            "environment": "JobSchedulerEnv",
            "cube_count": args.cube_count,
            "worker_count": actual_workers,
            "benchmark_steps": args.benchmark_steps,
            "checkpoint": args.checkpoint,
            "summary": summary,
            "baseline_results": [
                {"name": r.name, "distribution": r.distribution, "avg_ms": r.avg_ms, 
                 "std_ms": r.std_ms, "p95_ms": r.p95_ms}
                for r in baseline_results
            ],
            "rl_result": {
                "avg_ms": rl_result.avg_ms,
                "std_ms": rl_result.std_ms,
                "p95_ms": rl_result.p95_ms,
                "learned_distribution": summary["rl_distribution"],
            },
        }
        
        with open(output_path, "w") as f:
            json.dump(full_results, f, indent=2)
        print(f"\nResults saved to: {output_path}")
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
