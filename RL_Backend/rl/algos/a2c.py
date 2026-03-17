"""A2C - Advantage Actor-Critic."""

import torch
import torch.nn.functional as F
import numpy as np
from typing import Tuple
from utils.nets import Actor, Critic


class A2CAgent:
    def __init__(self, obs_dim: int, act_dim: int, lr: float = 3e-4, gamma: float = 0.99, value_coef: float = 0.5,
                 entropy_coef: float = 0.01, device: str = "cpu"):
        self.device = device
        self.gamma = gamma
        self.value_coef = value_coef
        self.entropy_coef = entropy_coef
        self.actor = Actor(obs_dim, act_dim).to(device)
        self.critic = Critic(obs_dim, 0).to(device)  # V(s) only
        self.optimizer = torch.optim.Adam(
            list(self.actor.parameters()) + list(self.critic.parameters()), lr=lr
        )

    def select_action(self, obs: np.ndarray, deterministic: bool = False) -> Tuple[np.ndarray, float, float]:
        with torch.no_grad():
            x = torch.FloatTensor(obs).unsqueeze(0).to(self.device)
            action, log_prob = self.actor.get_action(x, deterministic=deterministic)
            value = self.critic(x)
        return action.cpu().numpy()[0], log_prob.item(), value.item()

    def update(self, obs: np.ndarray, actions: np.ndarray, log_probs: np.ndarray, rewards: np.ndarray,
               dones: np.ndarray, values: np.ndarray, next_obs: np.ndarray, next_done: bool) -> dict:
        with torch.no_grad():
            next_value = self.critic(torch.FloatTensor(next_obs).unsqueeze(0).to(self.device)).item()
            advantages = []
            lastgaelam = 0
            for t in reversed(range(len(rewards))):
                if t == len(rewards) - 1:
                    next_val = next_value
                    next_nonterminal = 1.0 - next_done
                else:
                    next_val = values[t + 1]
                    next_nonterminal = 1.0 - dones[t + 1]
                delta = rewards[t] + self.gamma * next_val * next_nonterminal - values[t]
                lastgaelam = delta + self.gamma * 0.95 * lastgaelam * next_nonterminal
                advantages.insert(0, lastgaelam)
            advantages = torch.FloatTensor(advantages).to(self.device)
            returns = advantages + torch.FloatTensor(values).to(self.device)

        obs_t = torch.FloatTensor(obs).to(self.device)
        act_t = torch.FloatTensor(actions).to(self.device)
        advantages = (advantages - advantages.mean()) / (advantages.std() + 1e-8)

        log_prob = self.actor.log_prob(obs_t, act_t)
        value = self.critic(obs_t)
        
        # Get entropy from actor's forward pass (log_std is state-dependent)
        _, log_std = self.actor.forward(obs_t)
        entropy = (0.5 * (1 + np.log(2 * np.pi) + 2 * log_std)).sum(-1).mean()

        policy_loss = -(log_prob * advantages).mean()
        value_loss = F.mse_loss(value, returns)
        loss = policy_loss + self.value_coef * value_loss - self.entropy_coef * entropy
        self.optimizer.zero_grad()
        loss.backward()
        self.optimizer.step()
        return {"policy_loss": policy_loss.item(), "value_loss": value_loss.item(), "entropy": entropy.item()}
