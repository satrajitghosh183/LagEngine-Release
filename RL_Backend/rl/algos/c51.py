"""C51 - Distributional DQN (Categorical)."""

import torch
import torch.nn.functional as F
import numpy as np
from utils.nets import C51Net
from utils.replay_buffer import ReplayBuffer


class C51Agent:
    def __init__(self, obs_dim: int, n_actions: int, n_atoms: int = 51, v_min: float = -10, v_max: float = 10,
                 lr: float = 1e-3, gamma: float = 0.99, tau: float = 1.0,
                 buffer_size: int = 100_000, batch_size: int = 64, device: str = "cpu"):
        self.device = device
        self.gamma = gamma
        self.tau = tau
        self.batch_size = batch_size
        self.n_actions = n_actions
        self.n_atoms = n_atoms
        self.v_min = v_min
        self.v_max = v_max
        self.delta_z = (v_max - v_min) / (n_atoms - 1)
        self.support = torch.linspace(v_min, v_max, n_atoms).to(device)
        self.q = C51Net(obs_dim, n_actions, n_atoms, v_min, v_max).to(device)
        self.q_target = C51Net(obs_dim, n_actions, n_atoms, v_min, v_max).to(device)
        self.q_target.load_state_dict(self.q.state_dict())
        self.optimizer = torch.optim.Adam(self.q.parameters(), lr=lr)
        self.buffer = ReplayBuffer(buffer_size, obs_dim, 1, device)

    def select_action(self, obs: np.ndarray, epsilon: float = 0.0) -> int:
        if np.random.random() < epsilon:
            return np.random.randint(self.n_actions)
        with torch.no_grad():
            x = torch.FloatTensor(obs).unsqueeze(0).to(self.device)
            q = self.q.get_q(x)
            return q.argmax(dim=1).item()

    def store(self, obs: np.ndarray, action: int, reward: float, next_obs: np.ndarray, done: bool):
        self.buffer.add(obs, np.array([action], dtype=np.float32), reward, next_obs, done)

    def update(self) -> dict:
        if self.buffer.size < self.batch_size:
            return {}
        obs, actions, rewards, next_obs, dones = self.buffer.sample(self.batch_size)
        actions = actions.long().squeeze(-1)
        batch_size = obs.shape[0]
        dist = self.q(obs)
        log_dist = F.log_softmax(dist, dim=-1)
        log_dist = log_dist.gather(1, actions.unsqueeze(-1).unsqueeze(-1).expand(-1, -1, self.n_atoms)).squeeze(1)

        with torch.no_grad():
            next_q = self.q_target(next_obs)
            next_actions = self.q_target.get_q(next_obs).argmax(dim=1)
            next_dist = next_q.gather(1, next_actions.unsqueeze(-1).unsqueeze(-1).expand(-1, 1, self.n_atoms)).squeeze(1)
            probs = F.softmax(next_dist, dim=-1)
            Tz = rewards.unsqueeze(-1) + self.gamma * (1 - dones).unsqueeze(-1) * self.support.unsqueeze(0)
            Tz = Tz.clamp(self.v_min, self.v_max)
            b = (Tz - self.v_min) / self.delta_z
            l, u = b.floor().long().clamp(0, self.n_atoms - 1), b.ceil().long().clamp(0, self.n_atoms - 1)
            m = torch.zeros(batch_size, self.n_atoms, device=self.device)
            for i in range(batch_size):
                for j in range(self.n_atoms):
                    m[i, l[i, j]] += probs[i, j] * (u[i, j].float() - b[i, j])
                    m[i, u[i, j]] += probs[i, j] * (b[i, j] - l[i, j].float())

        loss = -(m * log_dist).sum(dim=1).mean()
        self.optimizer.zero_grad()
        loss.backward()
        self.optimizer.step()
        for p, pt in zip(self.q.parameters(), self.q_target.parameters()):
            pt.data.copy_(self.tau * p.data + (1 - self.tau) * pt.data)
        return {"q_loss": loss.item()}
