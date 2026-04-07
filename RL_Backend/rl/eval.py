#!/usr/bin/env python3
"""
Deterministic evaluation of trained RL agents.
Usage:
  python eval.py --checkpoint runs/FrameSchedulerEnv_ppo_s0/model.pt --episodes 10 --output results.csv
  python eval.py --checkpoint path/to/model.pt --env WorkStealSchedulerEnv --episodes 10
"""

import argparse
import numpy as np
import torch
import csv
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))


def get_env(env_name: str, discrete: bool = False, body_count=None):
    """Create environment matching train.py."""
    try:
        import rldemo_env
        if env_name == "WorkStealSchedulerEnv":
            from envs.engine_env import WorkStealSchedulerEnv
            env = WorkStealSchedulerEnv(body_count=body_count)
        else:
            env = rldemo_env.FrameSchedulerEnv()
        if discrete:
            from utils.env_wrapper import DiscreteActionWrapper
            env = DiscreteActionWrapper(env, n_bins=5)
        else:
            from utils.env_wrapper import ActionScaleWrapper
            low = 0.01 if env_name == "WorkStealSchedulerEnv" else 0.0
            env = ActionScaleWrapper(env, low=low, high=1.0)
        return env
    except ImportError:
        raise ImportError("rldemo_env not built. Build with -DRL_BACKEND_BUILD_RL=ON")


def load_agent(checkpoint_path: str, device: str):
    """Load trained agent from checkpoint."""
    from train import load_checkpoint
    return load_checkpoint(checkpoint_path, device)


def run_eval(agent, env, n_episodes: int, discrete: bool, max_steps: int = 1000):
    """Run evaluation episodes and return results."""
    results = []
    for ep in range(n_episodes):
        obs = env.reset()
        if isinstance(obs, tuple):
            obs = obs[0]
        total_reward = 0.0
        steps = 0
        while steps < max_steps:
            if discrete:
                action = agent.select_action(obs, epsilon=0)
            else:
                action = agent.select_action(obs, deterministic=True)
            result = env.step(action)
            if len(result) == 5:
                obs, reward, term, trunc, info = result
                done = term or trunc
            else:
                obs, reward, done, info = result
            total_reward += reward
            steps += 1
            if done:
                break
        results.append({"episode": ep, "return": total_reward, "steps": steps})
    return results


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--checkpoint", type=str, required=True, help="Path to model.pt checkpoint")
    parser.add_argument("--env", type=str, default=None,
                        help="Env name (auto-detected from checkpoint dir if not set)")
    parser.add_argument("--episodes", type=int, default=10)
    parser.add_argument("--seed", type=int, default=0)
    parser.add_argument("--output", type=str, default="eval_results.csv")
    parser.add_argument("--device", type=str, default="cuda" if torch.cuda.is_available() else "cpu")
    parser.add_argument("--body-count", type=int, default=None, help="WorkStealSchedulerEnv: body count")
    args = parser.parse_args()

    from utils.seeding import seed_all
    seed_all(args.seed)

    ckpt_path = Path(args.checkpoint)
    if not ckpt_path.exists():
        print(f"Checkpoint not found: {ckpt_path}")
        return 1

    agent, algo, obs_dim, act_dim = load_agent(str(ckpt_path), args.device)
    discrete = algo in ["dqn", "double_dqn", "dueling_dqn", "c51"]

    if args.env is None:
        args.env = "WorkStealSchedulerEnv" if act_dim == 21 else "FrameSchedulerEnv"
    env = get_env(args.env, discrete=discrete, body_count=args.body_count)

    print(f"Evaluating {algo} on {args.env} | {args.episodes} episodes")
    results = run_eval(agent, env, args.episodes, discrete)

    with open(args.output, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=["episode", "return", "steps"])
        w.writeheader()
        w.writerows(results)

    mean_return = np.mean([r["return"] for r in results])
    print(f"Mean return: {mean_return:.2f} | Saved to {args.output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
