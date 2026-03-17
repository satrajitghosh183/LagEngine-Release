#!/usr/bin/env python3
"""
RL_Backend training script with full algorithm implementations.
Usage: python train.py --algo ppo --env FrameSchedulerEnv --steps 50000 --seed 0
"""

import argparse
import numpy as np
import torch
import sys
from pathlib import Path
from typing import Optional

# Add rl to path
sys.path.insert(0, str(Path(__file__).parent))

from utils.seeding import seed_all
from utils.logger import Logger
from tqdm import tqdm


def get_env(env_name: str, discrete: bool = False, body_count: Optional[int] = None, cube_count: Optional[int] = None):
    """Create environment. Use discrete wrapper for DQN variants, ActionScaleWrapper for continuous."""
    try:
        import rldemo_env
        if env_name == "WorkStealSchedulerEnv":
            from envs.engine_env import WorkStealSchedulerEnv
            env = WorkStealSchedulerEnv(body_count=body_count)
        elif env_name == "JobSchedulerEnv":
            env = rldemo_env.JobSchedulerEnv()
            if cube_count is not None:
                env.set_cube_count(cube_count)
        else:
            env = rldemo_env.FrameSchedulerEnv()
        if discrete:
            from utils.env_wrapper import DiscreteActionWrapper
            env = DiscreteActionWrapper(env, n_bins=5)
        else:
            from utils.env_wrapper import ActionScaleWrapper
            # JobSchedulerEnv and WorkStealSchedulerEnv use [0.01, 1.0] for batch factors
            low = 0.01 if env_name in ["WorkStealSchedulerEnv", "JobSchedulerEnv"] else 0.0
            env = ActionScaleWrapper(env, low=low, high=1.0)
        return env
    except ImportError:
        # Fallback: gymnasium for testing without C++ env
        try:
            import gymnasium as gym
            from utils.gym_adapter import GymAdapter
            # Use Pendulum (continuous) for PPO/SAC/etc, CartPole (discrete) for DQN
            gym_env = gym.make("Pendulum-v1") if not discrete else gym.make("CartPole-v1")
            env = GymAdapter(gym_env)
            if env_name == "FrameSchedulerEnv":
                print("rldemo_env not found - using", gym_env.spec.id, "as fallback")
            return env
        except ImportError:
            raise ImportError("Need rldemo_env (build with -DRL_BACKEND_BUILD_RL=ON) or gymnasium")


def save_checkpoint(agent, algo: str, obs_dim: int, act_dim: int, path: str):
    """Save agent checkpoint for later evaluation."""
    state = {"algo": algo, "obs_dim": obs_dim, "act_dim": act_dim}
    discrete = algo in ["dqn", "double_dqn", "dueling_dqn", "c51"]
    n_actions = 5 ** 4 if discrete and act_dim > 4 else (act_dim if discrete else act_dim)
    state["n_actions"] = n_actions
    if hasattr(agent, "actor"):
        state["actor"] = agent.actor.state_dict()
    if hasattr(agent, "critic"):
        state["critic"] = agent.critic.state_dict()
    if hasattr(agent, "q"):
        state["q"] = agent.q.state_dict()
        state["q_target"] = agent.q_target.state_dict()
    if hasattr(agent, "critic1"):
        state["critic1"] = agent.critic1.state_dict()
        state["critic2"] = agent.critic2.state_dict()
        state["critic1_target"] = agent.critic1_target.state_dict()
        state["critic2_target"] = agent.critic2_target.state_dict()
    if hasattr(agent, "actor_target"):
        state["actor_target"] = agent.actor_target.state_dict()
    if hasattr(agent, "q_critic") and algo in ("ppo_qfilter", "awss"):
        state["q_critic"] = agent.q_critic.state_dict()
    torch.save(state, path)


def load_checkpoint(path: str, device: str):
    """Load checkpoint and return (agent, algo, obs_dim, act_dim)."""
    ckpt = torch.load(path, map_location=device, weights_only=True)
    algo = ckpt["algo"]
    obs_dim = ckpt["obs_dim"]
    act_dim = ckpt["act_dim"]
    n_actions = ckpt.get("n_actions", act_dim)
    # For discrete algos, pass n_actions as act_dim for correct network sizing
    agent = get_agent(algo, obs_dim, n_actions if algo in ["dqn", "double_dqn", "dueling_dqn", "c51"] else act_dim, device)
    if "actor" in ckpt:
        agent.actor.load_state_dict(ckpt["actor"])
    if "critic" in ckpt:
        agent.critic.load_state_dict(ckpt["critic"])
    if "q" in ckpt:
        agent.q.load_state_dict(ckpt["q"])
        agent.q_target.load_state_dict(ckpt["q_target"])
    if "critic1" in ckpt:
        agent.critic1.load_state_dict(ckpt["critic1"])
        agent.critic2.load_state_dict(ckpt["critic2"])
        agent.critic1_target.load_state_dict(ckpt["critic1_target"])
        agent.critic2_target.load_state_dict(ckpt["critic2_target"])
    if "actor_target" in ckpt:
        agent.actor_target.load_state_dict(ckpt["actor_target"])
    if "q_critic" in ckpt and hasattr(agent, "q_critic"):
        agent.q_critic.load_state_dict(ckpt["q_critic"])  # ppo_qfilter, awss
    return agent, algo, obs_dim, act_dim


def get_agent(algo: str, obs_dim: int, act_dim: int, device: str, **kwargs):
    """Create agent by algorithm name."""
    discrete = algo in ["dqn", "double_dqn", "dueling_dqn", "c51"]
    if discrete and isinstance(act_dim, int) and act_dim <= 1000:
        n_actions = act_dim  # already discrete
    elif discrete:
        n_actions = 5 ** 4  # DiscreteActionWrapper
    else:
        n_actions = act_dim

    if algo == "reinforce":
        from algos.reinforce import REINFORCE
        return REINFORCE(obs_dim, act_dim, device=device)
    elif algo == "dqn":
        from algos.dqn import DQNAgent
        return DQNAgent(obs_dim, n_actions, device=device)
    elif algo == "double_dqn":
        from algos.double_dqn import DoubleDQNAgent
        return DoubleDQNAgent(obs_dim, n_actions, device=device)
    elif algo == "dueling_dqn":
        from algos.dueling_dqn import DuelingDQNAgent
        return DuelingDQNAgent(obs_dim, n_actions, device=device)
    elif algo == "c51":
        from algos.c51 import C51Agent
        return C51Agent(obs_dim, n_actions, device=device)
    elif algo == "a2c":
        from algos.a2c import A2CAgent
        return A2CAgent(obs_dim, act_dim, device=device)
    elif algo == "ppo":
        from algos.ppo import PPOAgent
        return PPOAgent(obs_dim, act_dim, device=device)
    elif algo == "ddpg":
        from algos.ddpg import DDPGAgent
        return DDPGAgent(obs_dim, act_dim, device=device)
    elif algo == "td3":
        from algos.td3 import TD3Agent
        return TD3Agent(obs_dim, act_dim, device=device)
    elif algo == "sac":
        from algos.sac import SACAgent
        return SACAgent(obs_dim, act_dim, device=device)
    elif algo == "ppo_qfilter":
        from algos.ppo_qfilter import PPOQFilterAgent
        return PPOQFilterAgent(obs_dim, act_dim, device=device)
    elif algo == "awss":
        from algos.awss import AWSSAgent
        n_workers = kwargs.get("n_workers", 8)
        return AWSSAgent(obs_dim, act_dim, n_workers=n_workers, device=device)
    else:
        raise ValueError(f"Unknown algo: {algo}")


def run_onpolicy(env, agent, steps: int, rollout_len: int, logger: Logger, algo: str, curriculum: bool = False):
    """On-policy: REINFORCE, A2C, PPO, PPO-QFilter, AWSS."""
    obs_dim = env.get_obs_dim()
    act_dim = env.get_act_dim()
    discrete = algo == "reinforce"  # REINFORCE can work with continuous
    is_reinforce = algo == "reinforce"

    global_step = 0
    eval_returns = []
    curriculum_body = 500
    episodes_done = 0

    while global_step < steps:
        if hasattr(env, "set_body_count") and curriculum:
            env.set_body_count(min(8000, curriculum_body))
        obs = env.reset()
        if isinstance(obs, tuple):
            obs = obs[0]
        obs_buf, act_buf, log_prob_buf, rew_buf, done_buf, val_buf = [], [], [], [], [], []
        ep_rew = 0

        for _ in range(rollout_len):
            if is_reinforce:
                action, log_prob = agent.select_action(obs)
                value = 0
            else:
                action, log_prob, value = agent.select_action(obs)
            obs_buf.append(obs)
            act_buf.append(action)
            log_prob_buf.append(log_prob)
            val_buf.append(value)

            result = env.step(action)
            if len(result) == 5:
                next_obs, reward, term, trunc, info = result
                done = term or trunc
            else:
                next_obs, reward, done, info = result
            rew_buf.append(reward)
            done_buf.append(done)
            ep_rew += reward
            obs = next_obs
            global_step += 1
            if done:
                episodes_done += 1
                if hasattr(env, "set_body_count") and curriculum and episodes_done % 50 == 0:
                    curriculum_body = min(8000, curriculum_body + 500)
                obs = env.reset()
                obs = obs[0] if isinstance(obs, tuple) else obs
                if global_step % 10000 == 0:
                    eval_returns.append(ep_rew)
                    logger.log_scalar("eval/return", ep_rew, global_step)
                ep_rew = 0

        obs_buf = np.array(obs_buf)
        act_buf = np.array(act_buf)
        log_prob_buf = np.array(log_prob_buf)
        rew_buf = np.array(rew_buf)
        done_buf = np.array(done_buf)
        val_buf = np.array(val_buf)

        if is_reinforce:
            metrics = agent.update(obs_buf, act_buf, rew_buf)
        else:
            metrics = agent.update(obs_buf, act_buf, log_prob_buf, rew_buf, done_buf, val_buf, obs, done)
            if (algo == "ppo_qfilter" or algo == "awss") and hasattr(agent, "store_for_q"):
                for i in range(len(obs_buf)):
                    next_o = obs_buf[i + 1] if i + 1 < len(obs_buf) else obs
                    agent.store_for_q(obs_buf[i], act_buf[i], rew_buf[i], next_o, done_buf[i])
        logger.log_scalars({f"train/{k}": v for k, v in metrics.items()}, global_step)
        logger.log_scalar("train/rollout_return", rew_buf.sum(), global_step)

    return eval_returns


def run_offpolicy(env, agent, steps: int, eval_freq: int, logger: Logger, algo: str):
    """Off-policy: DQN, Double DQN, Dueling DQN, C51, DDPG, TD3, SAC."""
    obs_dim = env.get_obs_dim()
    act_dim = env.get_act_dim()
    discrete = algo in ["dqn", "double_dqn", "dueling_dqn", "c51"]
    n_actions = act_dim
    start_steps = 10000 if not discrete else 1000
    epsilon = 1.0
    epsilon_decay = 0.995
    epsilon_min = 0.05

    obs = env.reset()
    ep_rew = 0
    eval_returns = []

    for step in tqdm(range(steps), desc=algo):
        if step < start_steps and discrete:
            action = np.random.randint(n_actions)
        elif step < start_steps:
            action = np.random.uniform(-1, 1, size=act_dim).astype(np.float32)
        else:
            if discrete:
                eps = max(epsilon_min, epsilon)
                action = agent.select_action(obs, epsilon=eps)
                epsilon *= epsilon_decay
            else:
                # SAC uses stochastic policy, TD3/DDPG use noise
                if hasattr(agent, 'autotune_alpha'):  # SAC
                    action = agent.select_action(obs, deterministic=False)
                else:  # TD3, DDPG
                    action = agent.select_action(obs, noise=0.1)

        result = env.step(action)
        if len(result) == 5:
            next_obs, reward, term, trunc, info = result
            done = term or trunc
        else:
            next_obs, reward, done, info = result

        agent.store(obs, action, reward, next_obs, done)
        ep_rew += reward
        obs = next_obs

        metrics = agent.update()
        if metrics:
            logger.log_scalars({f"train/{k}": v for k, v in metrics.items()}, step)

        if done:
            logger.log_scalar("train/ep_return", ep_rew, step)
            obs = env.reset()
            obs = obs[0] if isinstance(obs, tuple) else obs
            ep_rew = 0

        if (step + 1) % eval_freq == 0:
            eval_ret = evaluate(env, agent, n_episodes=5, discrete=discrete)
            eval_returns.append(eval_ret)
            logger.log_scalar("eval/return", eval_ret, step)

    return eval_returns


def evaluate(env, agent, n_episodes: int = 5, discrete: bool = False) -> float:
    """Evaluate agent deterministically."""
    returns = []
    for _ in range(n_episodes):
        obs = env.reset()
        if isinstance(obs, tuple):
            obs = obs[0]
        ep_rew = 0
        while True:
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
            ep_rew += reward
            if done:
                break
        returns.append(ep_rew)
    return np.mean(returns)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--algo", type=str, default="ppo",
                        choices=["reinforce", "dqn", "double_dqn", "dueling_dqn", "c51", "a2c", "ppo", "ddpg", "td3", "sac", "ppo_qfilter", "awss"])
    parser.add_argument("--env", type=str, default="FrameSchedulerEnv",
                        choices=["FrameSchedulerEnv", "JobSchedulerEnv", "WorkStealSchedulerEnv"])
    parser.add_argument("--steps", type=int, default=50000)
    parser.add_argument("--seed", type=int, default=0)
    parser.add_argument("--device", type=str, default="cuda" if torch.cuda.is_available() else "cpu")
    parser.add_argument("--logdir", type=str, default="runs")
    parser.add_argument("--rollout-len", type=int, default=2048, help="On-policy rollout length")
    parser.add_argument("--eval-freq", type=int, default=5000, help="Off-policy eval frequency")
    parser.add_argument("--body-count", type=int, default=None, help="WorkStealSchedulerEnv: fixed body count")
    parser.add_argument("--cube-count", type=int, default=100000, help="JobSchedulerEnv: fixed cube count (default: 100k)")
    parser.add_argument("--curriculum", action="store_true", help="WorkStealSchedulerEnv: increase body count over time")
    parser.add_argument("--n-workers", type=int, default=8, help="AWSS: number of workers (for agent)")
    args = parser.parse_args()

    seed_all(args.seed)
    logdir = Path(args.logdir) / f"{args.env}_{args.algo}_s{args.seed}"
    logdir.mkdir(parents=True, exist_ok=True)
    logger = Logger(str(logdir), args.algo)

    discrete = args.algo in ["dqn", "double_dqn", "dueling_dqn", "c51"]
    env = get_env(args.env, discrete=discrete, body_count=args.body_count, cube_count=args.cube_count)
    obs_dim = env.get_obs_dim()
    act_dim = env.get_act_dim()

    agent = get_agent(args.algo, obs_dim, act_dim, args.device, n_workers=args.n_workers)
    print(f"Training {args.algo} on {args.env} | obs={obs_dim} act={act_dim} | device={args.device}")
    print(f"Logdir: {logdir}")

    onpolicy = args.algo in ["reinforce", "a2c", "ppo", "ppo_qfilter", "awss"]
    if onpolicy:
        run_onpolicy(env, agent, args.steps, args.rollout_len, logger, args.algo, curriculum=args.curriculum)
    else:
        run_offpolicy(env, agent, args.steps, args.eval_freq, logger, args.algo)

    # Save final checkpoint
    ckpt_path = logdir / "model.pt"
    save_checkpoint(agent, args.algo, obs_dim, act_dim, str(ckpt_path))
    logger.close()
    print("Training complete. Checkpoint saved to", ckpt_path)
    print("View logs: tensorboard --logdir", logdir)


if __name__ == "__main__":
    main()
