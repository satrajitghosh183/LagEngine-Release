"""REINFORCE - vanilla policy gradient."""

import torch
import torch.nn as nn
import torch.nn.functional as F
import numpy as np
from typing import List, Tuple
from utils.nets import Actor, layer_init


class REINFORCE:
    def __init__(self, obs_dim: int, act_dim: int, lr: float = 3e-4, gamma: float = 0.99, device: str = "cpu"):
        self.device = device
        self.gamma = gamma
        self.actor = Actor(obs_dim, act_dim).to(device)
        self.optimizer = torch.optim.Adam(self.actor.parameters(), lr=lr)

    def select_action(self, obs: np.ndarray, deterministic: bool = False) -> Tuple[np.ndarray, float]:
        with torch.no_grad():
            x = torch.FloatTensor(obs).unsqueeze(0).to(self.device)
            action, log_prob = self.actor.get_action(x, deterministic=deterministic)
        return action.cpu().numpy()[0], log_prob.item()

    def update(self, obs: List[np.ndarray], actions: List[np.ndarray], rewards: List[float]) -> dict:
        returns = []
        R = 0
        for r in reversed(rewards):
            R = r + self.gamma * R
            returns.insert(0, R)
        returns = torch.FloatTensor(returns).to(self.device)
        returns = (returns - returns.mean()) / (returns.std() + 1e-8)

        obs_t = torch.FloatTensor(np.array(obs)).to(self.device)
        act_t = torch.FloatTensor(np.array(actions)).to(self.device)
        log_prob = self.actor.log_prob(obs_t, act_t)
        loss = -(log_prob * returns).mean()
        self.optimizer.zero_grad()
        loss.backward()
        self.optimizer.step()
        return {"loss": loss.item()}
