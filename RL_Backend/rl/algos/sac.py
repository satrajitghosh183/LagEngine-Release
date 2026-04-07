"""SAC - Soft Actor-Critic."""

import torch
import torch.nn.functional as F
import numpy as np
from typing import Tuple
from utils.nets import Actor, Critic
from utils.replay_buffer import ReplayBuffer


class SACAgent:
    def __init__(self, obs_dim: int, act_dim: int, lr_actor: float = 3e-4, lr_critic: float = 3e-4,
                 gamma: float = 0.99, tau: float = 0.005, alpha: float = 0.2, autotune_alpha: bool = True,
                 buffer_size: int = 1_000_000, batch_size: int = 256, device: str = "cpu"):
        self.device = device
        self.gamma = gamma
        self.tau = tau
        self.batch_size = batch_size
        self.actor = Actor(obs_dim, act_dim).to(device)
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
        self.autotune_alpha = autotune_alpha
        self.alpha = alpha
        if autotune_alpha:
            self.target_entropy = -act_dim
            self.log_alpha = torch.zeros(1, requires_grad=True, device=device)
            self.alpha_opt = torch.optim.Adam([self.log_alpha], lr=lr_actor)

    def select_action(self, obs: np.ndarray, deterministic: bool = False) -> np.ndarray:
        with torch.no_grad():
            x = torch.FloatTensor(obs).unsqueeze(0).to(self.device)
            action, _ = self.actor.get_action(x, deterministic=deterministic)
        return action.cpu().numpy()[0]

    def store(self, obs: np.ndarray, action: np.ndarray, reward: float, next_obs: np.ndarray, done: bool):
        self.buffer.add(obs, action, reward, next_obs, done)

    def update(self) -> dict:
        if self.buffer.size < self.batch_size:
            return {}
        obs, actions, rewards, next_obs, dones = self.buffer.sample(self.batch_size)
        alpha = self.alpha
        if self.autotune_alpha:
            alpha = self.log_alpha.exp().item()

        with torch.no_grad():
            next_actions, next_log_prob = self.actor.get_action(next_obs, deterministic=False)
            target_q1 = self.critic1_target(next_obs, next_actions)
            target_q2 = self.critic2_target(next_obs, next_actions)
            target_q = torch.min(target_q1, target_q2) - alpha * next_log_prob
            target_q = rewards + self.gamma * (1 - dones) * target_q
        q1 = self.critic1(obs, actions)
        q2 = self.critic2(obs, actions)
        critic_loss = F.mse_loss(q1, target_q) + F.mse_loss(q2, target_q)
        self.critic_opt.zero_grad()
        critic_loss.backward()
        self.critic_opt.step()

        new_actions, log_prob = self.actor.get_action(obs, deterministic=False)
        q1_pi = self.critic1(obs, new_actions)
        q2_pi = self.critic2(obs, new_actions)
        q_pi = torch.min(q1_pi, q2_pi)
        actor_loss = (alpha * log_prob - q_pi).mean()
        self.actor_opt.zero_grad()
        actor_loss.backward()
        self.actor_opt.step()

        if self.autotune_alpha:
            alpha_loss = -(self.log_alpha * (log_prob + self.target_entropy).detach()).mean()
            self.alpha_opt.zero_grad()
            alpha_loss.backward()
            self.alpha_opt.step()

        for p, pt in zip(self.critic1.parameters(), self.critic1_target.parameters()):
            pt.data.copy_(self.tau * p.data + (1 - self.tau) * pt.data)
        for p, pt in zip(self.critic2.parameters(), self.critic2_target.parameters()):
            pt.data.copy_(self.tau * p.data + (1 - self.tau) * pt.data)

        return {"actor_loss": actor_loss.item(), "critic_loss": critic_loss.item(), "alpha": alpha}
