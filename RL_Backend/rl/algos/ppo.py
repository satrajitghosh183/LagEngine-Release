"""PPO - Proximal Policy Optimization."""

import torch
import torch.nn.functional as F
import numpy as np
from typing import Tuple
from utils.nets import Actor, Critic


class PPOAgent:
    def __init__(self, obs_dim: int, act_dim: int, lr: float = 3e-4, gamma: float = 0.99, gae_lambda: float = 0.95,
                 clip_eps: float = 0.2, value_coef: float = 0.5, entropy_coef: float = 0.01, max_grad_norm: float = 0.5,
                 device: str = "cpu"):
        self.device = device
        self.gamma = gamma
        self.gae_lambda = gae_lambda
        self.clip_eps = clip_eps
        self.value_coef = value_coef
        self.entropy_coef = entropy_coef
        self.max_grad_norm = max_grad_norm
        self.actor = Actor(obs_dim, act_dim).to(device)
        self.critic = Critic(obs_dim, 0).to(device)
        self.optimizer = torch.optim.Adam(
            list(self.actor.parameters()) + list(self.critic.parameters()), lr=lr
        )

    def select_action(self, obs: np.ndarray, deterministic: bool = False) -> Tuple[np.ndarray, float, float]:
        with torch.no_grad():
            x = torch.FloatTensor(obs).unsqueeze(0).to(self.device)
            action, log_prob = self.actor.get_action(x, deterministic=deterministic)
            value = self.critic(x)
        # Sum log probs across action dimensions for scalar return
        log_prob_scalar = log_prob.sum().item() if log_prob.dim() > 0 else log_prob.item()
        return action.cpu().numpy()[0], log_prob_scalar, value.item()

    def update(self, obs: np.ndarray, actions: np.ndarray, log_probs: np.ndarray, rewards: np.ndarray,
              dones: np.ndarray, values: np.ndarray, next_obs: np.ndarray, next_done: bool) -> dict:
        with torch.no_grad():
            next_value = self.critic(torch.FloatTensor(next_obs).unsqueeze(0).to(self.device)).item()
            advantages = []
            lastgaelam = 0
            for t in reversed(range(len(rewards))):
                next_val = next_value if t == len(rewards) - 1 else values[t + 1]
                next_nonterminal = 1.0 - (next_done if t == len(rewards) - 1 else dones[t + 1])
                delta = rewards[t] + self.gamma * next_val * next_nonterminal - values[t]
                lastgaelam = delta + self.gamma * self.gae_lambda * lastgaelam * next_nonterminal
                advantages.insert(0, lastgaelam)
            advantages = torch.FloatTensor(advantages).to(self.device)
            returns = advantages + torch.FloatTensor(values).to(self.device)
            advantages = (advantages - advantages.mean()) / (advantages.std() + 1e-8)

        obs_t = torch.FloatTensor(obs).to(self.device)
        act_t = torch.FloatTensor(actions).to(self.device)
        old_log_prob = torch.FloatTensor(log_probs).to(self.device)

        log_prob = self.actor.log_prob(obs_t, act_t)
        value = self.critic(obs_t)
        mean, log_std = self.actor(obs_t)
        entropy = (0.5 * (2 * log_std + np.log(2 * np.pi) + 1)).sum(-1).mean()

        ratio = (log_prob - old_log_prob).exp()
        pg1 = advantages * ratio
        pg2 = advantages * torch.clamp(ratio, 1 - self.clip_eps, 1 + self.clip_eps)
        policy_loss = -torch.min(pg1, pg2).mean()
        value_loss = F.mse_loss(value, returns)
        loss = policy_loss + self.value_coef * value_loss - self.entropy_coef * entropy
        self.optimizer.zero_grad()
        loss.backward()
        torch.nn.utils.clip_grad_norm_(list(self.actor.parameters()) + list(self.critic.parameters()), self.max_grad_norm)
        self.optimizer.step()
        return {"policy_loss": policy_loss.item(), "value_loss": value_loss.item(), "entropy": entropy.item()}
