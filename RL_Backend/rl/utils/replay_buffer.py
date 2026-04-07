"""Replay buffers for off-policy RL algorithms."""

import numpy as np
import torch
from typing import Tuple, Optional


class ReplayBuffer:
    """Standard replay buffer for off-policy algorithms."""

    def __init__(self, capacity: int, obs_dim: int, act_dim: int, device: str = "cpu"):
        self.capacity = capacity
        self.device = device
        self.ptr = 0
        self.size = 0
        self.obs = np.zeros((capacity, obs_dim), dtype=np.float32)
        self.actions = np.zeros((capacity, act_dim), dtype=np.float32)
        self.rewards = np.zeros(capacity, dtype=np.float32)
        self.next_obs = np.zeros((capacity, obs_dim), dtype=np.float32)
        self.dones = np.zeros(capacity, dtype=np.float32)

    def add(self, obs: np.ndarray, action: np.ndarray, reward: float, next_obs: np.ndarray, done: bool):
        self.obs[self.ptr] = obs
        self.actions[self.ptr] = action
        self.rewards[self.ptr] = reward
        self.next_obs[self.ptr] = next_obs
        self.dones[self.ptr] = float(done)
        self.ptr = (self.ptr + 1) % self.capacity
        self.size = min(self.size + 1, self.capacity)

    def sample(self, batch_size: int) -> Tuple[torch.Tensor, ...]:
        idx = np.random.randint(0, self.size, size=batch_size)
        return (
            torch.FloatTensor(self.obs[idx]).to(self.device),
            torch.FloatTensor(self.actions[idx]).to(self.device),
            torch.FloatTensor(self.rewards[idx]).to(self.device),
            torch.FloatTensor(self.next_obs[idx]).to(self.device),
            torch.FloatTensor(self.dones[idx]).to(self.device),
        )


class PrioritizedReplayBuffer(ReplayBuffer):
    """Prioritized experience replay (for PPO-QFilter critic)."""

    def __init__(self, capacity: int, obs_dim: int, act_dim: int, alpha: float = 0.6, device: str = "cpu"):
        super().__init__(capacity, obs_dim, act_dim, device)
        self.alpha = alpha
        self.priorities = np.ones(capacity, dtype=np.float32)

    def add(self, obs: np.ndarray, action: np.ndarray, reward: float, next_obs: np.ndarray, done: bool, priority: float = 1.0):
        self.priorities[self.ptr] = max(priority, 1e-6)
        super().add(obs, action, reward, next_obs, done)

    def sample(self, batch_size: int, beta: float = 0.4) -> Tuple[torch.Tensor, ...]:
        probs = self.priorities[:self.size] ** self.alpha
        probs /= probs.sum()
        idx = np.random.choice(self.size, size=batch_size, p=probs, replace=False)
        weights = (self.size * probs[idx]) ** (-beta)
        weights /= weights.max()
        return (
            torch.FloatTensor(self.obs[idx]).to(self.device),
            torch.FloatTensor(self.actions[idx]).to(self.device),
            torch.FloatTensor(self.rewards[idx]).to(self.device),
            torch.FloatTensor(self.next_obs[idx]).to(self.device),
            torch.FloatTensor(self.dones[idx]).to(self.device),
            torch.FloatTensor(weights).to(self.device),
            idx,
        )

    def update_priorities(self, idx: np.ndarray, priorities: np.ndarray):
        self.priorities[idx] = np.maximum(priorities, 1e-6)
