#!/usr/bin/env python3
"""
Multi-Algorithm Comparison for Job Scheduling Optimization

Trains multiple RL algorithms on JobSchedulerEnv and compares their performance
to determine the best algorithm for work-stealing/job distribution optimization.

Algorithms compared:
  1. PPO (Proximal Policy Optimization) - On-policy, stable
  2. SAC (Soft Actor-Critic) - Off-policy, entropy-regularized
  3. TD3 (Twin Delayed DDPG) - Off-policy, twin critics
  4. DDPG (Deep Deterministic Policy Gradient) - Off-policy, continuous
  5. A2C (Advantage Actor-Critic) - On-policy, simple
  6. REINFORCE - On-policy, basic policy gradient

Usage:
  python scripts/compare_algorithms.py --steps 30000 --cube-count 100000
  python scripts/compare_algorithms.py --eval-only  # Just evaluate existing checkpoints
"""

from __future__ import annotations

import argparse
import json
import sys
import time
import numpy as np
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Tuple
from concurrent.futures import ProcessPoolExecutor, as_completed
import subprocess

# Add rl directory to path
rl_dir = Path(__file__).parent.parent / "rl"
sys.path.insert(0, str(rl_dir))


@dataclass
class AlgorithmResult:
    """Results for a single algorithm."""
    name: str
    train_time_sec: float = 0.0
    checkpoint_path: str = ""
    
    # Evaluation metrics
    avg_frame_time_ms: float = float('inf')
    std_frame_time_ms: float = float('inf')
    p95_frame_time_ms: float = float('inf')
    min_frame_time_ms: float = float('inf')
    max_frame_time_ms: float = float('inf')
    
    # Training metrics
    final_return: float = float('-inf')
    
    # Comparison metrics
    improvement_vs_baseline_pct: float = 0.0
    learned_distribution: List[float] = field(default_factory=list)
    
    @property
    def fps(self) -> float:
        return 1000.0 / self.avg_frame_time_ms if self.avg_frame_time_ms > 0 else 0.0


def get_algorithms():
    """Get list of algorithms to compare."""
    return [
        ("ppo", "PPO", True),      # (algo_id, display_name, is_on_policy)
        ("sac", "SAC", False),
        ("td3", "TD3", False),
        ("ddpg", "DDPG", False),
        ("a2c", "A2C", True),
        ("reinforce", "REINFORCE", True),
    ]


def train_algorithm(algo: str, steps: int, cube_count: int, seed: int = 0) -> Tuple[str, float, str]:
    """Train a single algorithm. Returns (algo_name, train_time, checkpoint_path)."""
    import torch
    from train import get_env, get_agent, run_onpolicy, run_offpolicy, seed_all
    from utils.logger import Logger
    
    seed_all(seed)
    
    logdir = Path("runs") / f"JobSchedulerEnv_{algo}_s{seed}"
    logdir.mkdir(parents=True, exist_ok=True)
    
    env = get_env("JobSchedulerEnv", discrete=False, cube_count=cube_count)
    obs_dim = env.get_obs_dim()
    act_dim = env.get_act_dim()
    
    device = "cuda" if torch.cuda.is_available() else "cpu"
    agent = get_agent(algo, obs_dim, act_dim, device)
    logger = Logger(str(logdir), algo)
    
    print(f"  Training {algo.upper()} | obs={obs_dim} act={act_dim} device={device}")
    
    start_time = time.time()
    
    is_on_policy = algo in ["ppo", "a2c", "reinforce", "ppo_qfilter"]
    if is_on_policy:
        run_onpolicy(env, agent, steps, rollout_len=1024, logger=logger, algo=algo)
    else:
        run_offpolicy(env, agent, steps, eval_freq=2000, logger=logger, algo=algo)
    
    train_time = time.time() - start_time
    
    # Save checkpoint
    ckpt_path = logdir / "model.pt"
    state = {
        "algo": algo,
        "obs_dim": obs_dim,
        "act_dim": act_dim,
    }
    
    if hasattr(agent, 'actor'):
        state["actor"] = agent.actor.state_dict()
    if hasattr(agent, 'critic'):
        state["critic"] = agent.critic.state_dict()
    if hasattr(agent, 'policy'):
        state["policy"] = agent.policy.state_dict()
    if hasattr(agent, 'q1'):
        state["q1"] = agent.q1.state_dict()
    if hasattr(agent, 'q2'):
        state["q2"] = agent.q2.state_dict()
    
    torch.save(state, ckpt_path)
    logger.close()
    
    return algo, train_time, str(ckpt_path)


def evaluate_algorithm(algo: str, checkpoint_path: str, cube_count: int, 
                       eval_steps: int = 200) -> AlgorithmResult:
    """Evaluate a trained algorithm."""
    import torch
    from train import load_checkpoint, get_env
    
    result = AlgorithmResult(name=algo)
    result.checkpoint_path = checkpoint_path
    
    if not Path(checkpoint_path).exists():
        print(f"  Checkpoint not found: {checkpoint_path}")
        return result
    
    try:
        agent, loaded_algo, obs_dim, act_dim = load_checkpoint(checkpoint_path, "cpu")
    except Exception as e:
        print(f"  Failed to load {algo}: {e}")
        return result
    
    env = get_env("JobSchedulerEnv", discrete=False, cube_count=cube_count)
    
    frame_times = []
    actions_taken = []
    total_return = 0.0
    
    obs = env.reset()
    if isinstance(obs, tuple):
        obs = obs[0]
    
    for _ in range(eval_steps):
        action_result = agent.select_action(obs, deterministic=True)
        action = action_result[0] if isinstance(action_result, tuple) else action_result
        actions_taken.append(action.copy())
        
        step_result = env.step(action)
        if len(step_result) == 5:
            obs, reward, term, trunc, info = step_result
            done = term or trunc
        else:
            obs, reward, done, info = step_result
        
        frame_times.append(info.get("frame_time", 0.0))
        total_return += reward
        
        if done:
            obs = env.reset()
            if isinstance(obs, tuple):
                obs = obs[0]
    
    result.avg_frame_time_ms = np.mean(frame_times)
    result.std_frame_time_ms = np.std(frame_times)
    result.p95_frame_time_ms = np.percentile(frame_times, 95)
    result.min_frame_time_ms = np.min(frame_times)
    result.max_frame_time_ms = np.max(frame_times)
    result.final_return = total_return / eval_steps
    
    # Get learned distribution
    avg_action = np.mean(actions_taken, axis=0)
    clamped = np.clip(avg_action, 0.01, 1.0)
    normalized = clamped / clamped.sum()
    result.learned_distribution = normalized.tolist()
    
    return result


def run_baseline(cube_count: int, eval_steps: int = 200) -> float:
    """Run uniform baseline and return average frame time."""
    import rldemo_env
    from utils.env_wrapper import ActionScaleWrapper
    
    env = rldemo_env.JobSchedulerEnv()
    env.set_cube_count(cube_count)
    env = ActionScaleWrapper(env, low=0.01, high=1.0)
    
    # Uniform distribution
    action = np.ones(16, dtype=np.float32) / 16
    
    env.reset()
    frame_times = []
    for _ in range(eval_steps):
        obs, reward, done, info = env.step(action)
        frame_times.append(info.get("frame_time", 0.0))
        if done:
            env.reset()
    
    return np.mean(frame_times)


def print_comparison_table(results: List[AlgorithmResult], baseline_ms: float):
    """Print formatted comparison table."""
    print("\n" + "=" * 100)
    print("MULTI-ALGORITHM COMPARISON RESULTS")
    print("=" * 100)
    
    # Sort by average frame time (best first)
    sorted_results = sorted(results, key=lambda r: r.avg_frame_time_ms)
    
    print(f"\nBaseline (uniform distribution): {baseline_ms:.2f} ms")
    print("\n" + "-" * 100)
    print(f"{'Rank':<6} {'Algorithm':<12} {'Avg (ms)':<12} {'Std (ms)':<12} {'P95 (ms)':<12} {'FPS':<10} {'vs Baseline':<15} {'Train Time':<12}")
    print("-" * 100)
    
    for i, r in enumerate(sorted_results):
        improvement = ((baseline_ms - r.avg_frame_time_ms) / baseline_ms) * 100
        r.improvement_vs_baseline_pct = improvement
        
        train_time_str = f"{r.train_time_sec:.0f}s" if r.train_time_sec > 0 else "N/A"
        improvement_str = f"{improvement:+.1f}%" if r.avg_frame_time_ms < float('inf') else "N/A"
        
        print(f"{i+1:<6} {r.name.upper():<12} {r.avg_frame_time_ms:<12.3f} {r.std_frame_time_ms:<12.3f} "
              f"{r.p95_frame_time_ms:<12.3f} {r.fps:<10.1f} {improvement_str:<15} {train_time_str:<12}")
    
    print("-" * 100)
    
    # Best algorithm
    best = sorted_results[0]
    print(f"\n{'='*100}")
    print(f"BEST ALGORITHM: {best.name.upper()}")
    print(f"{'='*100}")
    print(f"  Average Frame Time: {best.avg_frame_time_ms:.3f} ms")
    print(f"  FPS: {best.fps:.1f}")
    print(f"  Improvement vs Baseline: {best.improvement_vs_baseline_pct:+.1f}%")
    print(f"  Consistency (Std): {best.std_frame_time_ms:.3f} ms")
    
    # Show learned distribution for best
    if best.learned_distribution:
        print(f"\n  Learned Work Distribution:")
        num_workers = len([d for d in best.learned_distribution if d > 0.001])
        for i, d in enumerate(best.learned_distribution[:num_workers]):
            bar = "█" * int(d * 100)
            print(f"    Worker {i:2d}: {d:.3f} ({d*100:5.1f}%) {bar}")
    
    print("=" * 100)
    
    return sorted_results


def save_results(results: List[AlgorithmResult], baseline_ms: float, 
                 cube_count: int, output_path: str):
    """Save results to JSON file."""
    data = {
        "environment": "JobSchedulerEnv",
        "cube_count": cube_count,
        "baseline_uniform_ms": baseline_ms,
        "algorithms": [
            {
                "name": r.name,
                "rank": i + 1,
                "avg_frame_time_ms": r.avg_frame_time_ms,
                "std_frame_time_ms": r.std_frame_time_ms,
                "p95_frame_time_ms": r.p95_frame_time_ms,
                "fps": r.fps,
                "improvement_vs_baseline_pct": r.improvement_vs_baseline_pct,
                "train_time_sec": r.train_time_sec,
                "checkpoint_path": r.checkpoint_path,
                "learned_distribution": r.learned_distribution,
            }
            for i, r in enumerate(sorted(results, key=lambda r: r.avg_frame_time_ms))
        ],
        "best_algorithm": min(results, key=lambda r: r.avg_frame_time_ms).name,
    }
    
    with open(output_path, "w") as f:
        json.dump(data, f, indent=2)
    print(f"\nResults saved to: {output_path}")


def main():
    parser = argparse.ArgumentParser(description="Compare RL algorithms for job scheduling")
    parser.add_argument("--steps", type=int, default=30000, help="Training steps per algorithm")
    parser.add_argument("--eval-steps", type=int, default=200, help="Evaluation steps")
    parser.add_argument("--cube-count", type=int, default=100000, help="Number of cubes (workload)")
    parser.add_argument("--seed", type=int, default=0, help="Random seed")
    parser.add_argument("--eval-only", action="store_true", help="Only evaluate existing checkpoints")
    parser.add_argument("--algorithms", type=str, default=None, 
                        help="Comma-separated list of algorithms (default: all)")
    parser.add_argument("--output", type=str, default="algorithm_comparison.json", help="Output file")
    args = parser.parse_args()
    
    all_algorithms = get_algorithms()
    
    # Filter algorithms if specified
    if args.algorithms:
        selected = set(args.algorithms.lower().split(","))
        all_algorithms = [a for a in all_algorithms if a[0] in selected]
    
    print("=" * 80)
    print("RL ALGORITHM COMPARISON FOR JOB SCHEDULING OPTIMIZATION")
    print("=" * 80)
    print(f"\nWorkload: {args.cube_count:,} cubes (non-uniform complexity)")
    print(f"Training steps: {args.steps:,}")
    print(f"Algorithms: {', '.join([a[1] for a in all_algorithms])}")
    print("=" * 80)
    
    results = []
    
    # Training phase
    if not args.eval_only:
        print("\n" + "=" * 80)
        print("PHASE 1: TRAINING")
        print("=" * 80)
        
        for algo_id, display_name, is_on_policy in all_algorithms:
            print(f"\n[{len(results)+1}/{len(all_algorithms)}] Training {display_name}...")
            try:
                algo, train_time, ckpt_path = train_algorithm(
                    algo_id, args.steps, args.cube_count, args.seed
                )
                result = AlgorithmResult(name=algo_id, train_time_sec=train_time, checkpoint_path=ckpt_path)
                results.append(result)
                print(f"  Completed in {train_time:.1f}s")
            except Exception as e:
                print(f"  Failed: {e}")
                import traceback
                traceback.print_exc()
                results.append(AlgorithmResult(name=algo_id))
    else:
        # Just create placeholder results for eval
        for algo_id, _, _ in all_algorithms:
            ckpt = Path("runs") / f"JobSchedulerEnv_{algo_id}_s{args.seed}" / "model.pt"
            results.append(AlgorithmResult(name=algo_id, checkpoint_path=str(ckpt)))
    
    # Evaluation phase
    print("\n" + "=" * 80)
    print("PHASE 2: EVALUATION")
    print("=" * 80)
    
    # Run baseline first
    print("\nRunning baseline (uniform distribution)...")
    baseline_ms = run_baseline(args.cube_count, args.eval_steps)
    print(f"  Baseline: {baseline_ms:.2f} ms")
    
    # Evaluate each algorithm
    evaluated_results = []
    for i, r in enumerate(results):
        print(f"\n[{i+1}/{len(results)}] Evaluating {r.name.upper()}...")
        
        if not r.checkpoint_path or not Path(r.checkpoint_path).exists():
            ckpt = Path("runs") / f"JobSchedulerEnv_{r.name}_s{args.seed}" / "model.pt"
            r.checkpoint_path = str(ckpt)
        
        eval_result = evaluate_algorithm(r.name, r.checkpoint_path, args.cube_count, args.eval_steps)
        eval_result.train_time_sec = r.train_time_sec
        evaluated_results.append(eval_result)
        
        if eval_result.avg_frame_time_ms < float('inf'):
            print(f"  Avg: {eval_result.avg_frame_time_ms:.2f} ms, FPS: {eval_result.fps:.1f}")
        else:
            print(f"  Evaluation failed")
    
    # Print comparison
    sorted_results = print_comparison_table(evaluated_results, baseline_ms)
    
    # Save results
    save_results(sorted_results, baseline_ms, args.cube_count, args.output)
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
