"""Dueling DQN - separate value and advantage streams."""

import torch
import torch.nn.functional as F
import numpy as np
from utils.nets import DuelingDQN
from utils.replay_buffer import ReplayBuffer


class DuelingDQNAgent:
    def __init__(self, obs_dim: int, n_actions: int, lr: float = 1e-3, gamma: float = 0.99, tau: float = 1.0,
                 buffer_size: int = 100_000, batch_size: int = 64, device: str = "cpu"):
        self.device = device
        self.gamma = gamma
        self.tau = tau
        self.batch_size = batch_size
        self.n_actions = n_actions
        self.q = DuelingDQN(obs_dim, n_actions).to(device)
        self.q_target = DuelingDQN(obs_dim, n_actions).to(device)
        self.q_target.load_state_dict(self.q.state_dict())
        self.optimizer = torch.optim.Adam(self.q.parameters(), lr=lr)
        self.buffer = ReplayBuffer(buffer_size, obs_dim, 1, device)

    def select_action(self, obs: np.ndarray, epsilon: float = 0.0) -> int:
        if np.random.random() < epsilon:
            return np.random.randint(self.n_actions)
        with torch.no_grad():
            x = torch.FloatTensor(obs).unsqueeze(0).to(self.device)
            return self.q(x).argmax(dim=1).item()

    def store(self, obs: np.ndarray, action: int, reward: float, next_obs: np.ndarray, done: bool):
        self.buffer.add(obs, np.array([action], dtype=np.float32), reward, next_obs, done)

    def update(self) -> dict:
        if self.buffer.size < self.batch_size:
            return {}
        obs, actions, rewards, next_obs, dones = self.buffer.sample(self.batch_size)
        actions = actions.long().squeeze(-1)
        q = self.q(obs).gather(1, actions.unsqueeze(-1)).squeeze(-1)
        with torch.no_grad():
            q_next = self.q_target(next_obs).max(dim=1)[0]
            target = rewards + self.gamma * (1 - dones) * q_next
        loss = F.mse_loss(q, target)
        self.optimizer.zero_grad()
        loss.backward()
        self.optimizer.step()
        for p, pt in zip(self.q.parameters(), self.q_target.parameters()):
            pt.data.copy_(self.tau * p.data + (1 - self.tau) * pt.data)
        return {"q_loss": loss.item()}
