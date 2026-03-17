"""Neural network architectures for RL algorithms."""

import torch
import torch.nn as nn
import torch.nn.functional as F
import numpy as np
from typing import Tuple, Optional


def layer_init(layer: nn.Module, std: float = np.sqrt(2), bias_const: float = 0.0) -> nn.Module:
    nn.init.orthogonal_(layer.weight, std)
    nn.init.constant_(layer.bias, bias_const)
    return layer


class MLP(nn.Module):
    """Multi-layer perceptron."""

    def __init__(self, dims: list, activation=nn.ReLU, output_activation=None):
        super().__init__()
        layers = []
        for i in range(len(dims) - 1):
            layers.append(nn.Linear(dims[i], dims[i + 1]))
            if i < len(dims) - 2:
                layers.append(activation())
            elif output_activation is not None:
                layers.append(output_activation())
        self.net = nn.Sequential(*layers)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        return self.net(x)


# --- Continuous control (Actor-Critic) ---

class Actor(nn.Module):
    """Gaussian policy for continuous actions."""

    def __init__(self, obs_dim: int, act_dim: int, hidden: list = [256, 256], log_std_min: float = -20, log_std_max: float = 2):
        super().__init__()
        self.log_std_min = log_std_min
        self.log_std_max = log_std_max
        dims = [obs_dim] + hidden + [act_dim]
        self.trunk = MLP(dims[:-1])
        self.mean = layer_init(nn.Linear(hidden[-1], act_dim), std=0.01)
        self.log_std = layer_init(nn.Linear(hidden[-1], act_dim), std=0.01)

    def forward(self, x: torch.Tensor) -> Tuple[torch.Tensor, torch.Tensor]:
        h = self.trunk(x)
        mean = self.mean(h)
        log_std = torch.clamp(self.log_std(h), self.log_std_min, self.log_std_max)
        return mean, log_std

    def get_action(self, x: torch.Tensor, deterministic: bool = False) -> Tuple[torch.Tensor, torch.Tensor]:
        mean, log_std = self.forward(x)
        if deterministic:
            return torch.tanh(mean), torch.zeros_like(mean)
        std = log_std.exp()
        normal = torch.distributions.Normal(mean, std)
        x_t = normal.rsample()
        action = torch.tanh(x_t)
        log_prob = (normal.log_prob(x_t).sum(dim=-1) - torch.log(1 - action.pow(2) + 1e-6).sum(dim=-1))
        return action, log_prob

    def log_prob(self, x: torch.Tensor, action: torch.Tensor) -> torch.Tensor:
        mean, log_std = self.forward(x)
        std = log_std.exp()
        x_t = torch.atanh(torch.clamp(action, -0.999, 0.999))
        normal = torch.distributions.Normal(mean, std)
        return (normal.log_prob(x_t).sum(dim=-1) - torch.log(1 - action.pow(2) + 1e-6).sum(dim=-1))


class Critic(nn.Module):
    """Q-function or V-function."""

    def __init__(self, obs_dim: int, act_dim: int = 0, hidden: list = [256, 256]):
        super().__init__()
        inp_dim = obs_dim + act_dim
        dims = [inp_dim] + hidden + [1]
        self.net = MLP(dims)

    def forward(self, obs: torch.Tensor, act: Optional[torch.Tensor] = None) -> torch.Tensor:
        x = torch.cat([obs, act], dim=-1) if act is not None else obs
        return self.net(x).squeeze(-1)


# --- DQN variants ---

class DQN(nn.Module):
    """Standard DQN network."""

    def __init__(self, obs_dim: int, n_actions: int, hidden: list = [256, 256]):
        super().__init__()
        dims = [obs_dim] + hidden + [n_actions]
        self.net = MLP(dims)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        return self.net(x)


class DuelingDQN(nn.Module):
    """Dueling DQN: separate value and advantage streams."""

    def __init__(self, obs_dim: int, n_actions: int, hidden: list = [256, 256]):
        super().__init__()
        self.trunk = MLP([obs_dim] + hidden)
        self.value = nn.Linear(hidden[-1], 1)
        self.advantage = nn.Linear(hidden[-1], n_actions)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        h = self.trunk(x)
        v = self.value(h)
        adv = self.advantage(h)
        return v + (adv - adv.mean(dim=-1, keepdim=True))


class C51Net(nn.Module):
    """C51 / Distributional DQN: outputs distribution over returns."""

    def __init__(self, obs_dim: int, n_actions: int, n_atoms: int = 51, v_min: float = -10, v_max: float = 10, hidden: list = [256, 256]):
        super().__init__()
        self.n_atoms = n_atoms
        self.v_min = v_min
        self.v_max = v_max
        self.support = torch.linspace(v_min, v_max, n_atoms)
        dims = [obs_dim] + hidden + [n_actions * n_atoms]
        self.net = MLP(dims)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        logits = self.net(x)
        return logits.view(-1, logits.shape[-1] // self.n_atoms, self.n_atoms)

    def get_q(self, x: torch.Tensor) -> torch.Tensor:
        dist = self.forward(x)
        probs = F.softmax(dist, dim=-1)
        support = self.support.to(x.device)
        return (probs * support).sum(dim=-1)


# --- Shared utilities ---

def count_parameters(module: nn.Module) -> int:
    return sum(p.numel() for p in module.parameters() if p.requires_grad)
