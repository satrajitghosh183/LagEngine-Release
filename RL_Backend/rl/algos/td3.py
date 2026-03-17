"""TD3 - Twin Delayed DDPG."""

import torch
import torch.nn.functional as F
import numpy as np
from typing import Tuple
from utils.nets import Actor, Critic
from utils.replay_buffer import ReplayBuffer


class TD3Agent:
    def __init__(self, obs_dim: int, act_dim: int, lr_actor: float = 1e-3, lr_critic: float = 1e-3,
                 gamma: float = 0.99, tau: float = 0.005, policy_delay: int = 2, noise_clip: float = 0.5,
                 expl_noise: float = 0.1, buffer_size: int = 1_000_000, batch_size: int = 256,
                 device: str = "cpu"):
        self.device = device
        self.gamma = gamma
        self.tau = tau
        self.policy_delay = policy_delay
        self.noise_clip = noise_clip
        self.expl_noise = expl_noise
        self.batch_size = batch_size
        self.actor = Actor(obs_dim, act_dim).to(device)
        self.actor_target = Actor(obs_dim, act_dim).to(device)
        self.actor_target.load_state_dict(self.actor.state_dict())
        self.critic1 = Critic(obs_dim, act_dim).to(device)
        self.critic2 = Critic(obs_dim, act_dim).to(device)
        self.critic1_target = Critic(obs_dim, act_dim).to(device)
        self.critic2_target = Critic(obs_dim, act_dim).to(device)
        self.critic1_target.load_state_dict(self.critic1.state_dict())
        self.critic2_target.load_state_dict(self.critic2.state_dict())
        self.actor_opt = torch.optim.Adam(self.actor.parameters(), lr=lr_actor)
        self.critic_opt = torch.optim.Adam(
            list(self.critic1.parameters()) + list(self.critic2.parameters()), lr=lr_critic
        )
        self.buffer = ReplayBuffer(buffer_size, obs_dim, act_dim, device)
        self._update_counter = 0

    def select_action(self, obs: np.ndarray, noise: float = None, deterministic: bool = False) -> np.ndarray:
        if deterministic:
            n = 0.0
        elif noise is not None:
            n = noise
        else:
            n = self.expl_noise
        with torch.no_grad():
            x = torch.FloatTensor(obs).unsqueeze(0).to(self.device)
            action, _ = self.actor.get_action(x, deterministic=True)
            action = action.cpu().numpy()[0]
        if n > 0:
            action = action + n * np.random.randn(len(action))
        return np.clip(action, -1, 1)

    def store(self, obs: np.ndarray, action: np.ndarray, reward: float, next_obs: np.ndarray, done: bool):
        self.buffer.add(obs, action, reward, next_obs, done)

    def update(self) -> dict:
        if self.buffer.size < self.batch_size:
            return {}
        obs, actions, rewards, next_obs, dones = self.buffer.sample(self.batch_size)
        with torch.no_grad():
            next_actions, _ = self.actor_target.get_action(next_obs, deterministic=True)
            noise = (torch.randn_like(next_actions) * self.noise_clip).clamp(-self.noise_clip, self.noise_clip)
            next_actions = (next_actions + noise).clamp(-1, 1)
            target_q1 = self.critic1_target(next_obs, next_actions)
            target_q2 = self.critic2_target(next_obs, next_actions)
            target_q = torch.min(target_q1, target_q2)
            target_q = rewards + self.gamma * (1 - dones) * target_q
        q1 = self.critic1(obs, actions)
        q2 = self.critic2(obs, actions)
        critic_loss = F.mse_loss(q1, target_q) + F.mse_loss(q2, target_q)
        self.critic_opt.zero_grad()
        critic_loss.backward()
        self.critic_opt.step()

        self._update_counter += 1
        actor_loss = torch.tensor(0.0, device=self.device)
        if self._update_counter % self.policy_delay == 0:
            actor_actions, _ = self.actor.get_action(obs, deterministic=True)
            actor_loss = -self.critic1(obs, actor_actions).mean()
            self.actor_opt.zero_grad()
            actor_loss.backward()
            self.actor_opt.step()
            for p, pt in zip(self.actor.parameters(), self.actor_target.parameters()):
                pt.data.copy_(self.tau * p.data + (1 - self.tau) * pt.data)
            for p, pt in zip(self.critic1.parameters(), self.critic1_target.parameters()):
                pt.data.copy_(self.tau * p.data + (1 - self.tau) * pt.data)
            for p, pt in zip(self.critic2.parameters(), self.critic2_target.parameters()):
                pt.data.copy_(self.tau * p.data + (1 - self.tau) * pt.data)

        return {"actor_loss": actor_loss.item(), "critic_loss": critic_loss.item()}
