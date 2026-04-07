"""Environment wrappers for RL algorithms."""

import numpy as np
from typing import Tuple, Any, Dict, Optional


class ActionScaleWrapper:
    """Maps agent actions from [-1, 1] (tanh) to env's [0, 1] (or [0.01, 1] for WorkSteal)."""

    def __init__(self, env, low: float = 0.0, high: float = 1.0):
        self.env = env
        self.low = low
        self.high = high

    def reset(self):
        result = self.env.reset()
        return result

    def step(self, action):
        # Map [-1, 1] -> [low, high]
        scaled = np.clip((np.asarray(action, dtype=np.float32) + 1) / 2, 0, 1)
        scaled = self.low + scaled * (self.high - self.low)
        return self.env.step(scaled)

    def get_obs_dim(self) -> int:
        return self.env.get_obs_dim()

    def get_act_dim(self) -> int:
        return self.env.get_act_dim()

    def set_body_count(self, n: int):
        if hasattr(self.env, "set_body_count"):
            self.env.set_body_count(n)


class DiscreteActionWrapper:
    """Wraps continuous env to discrete action space for DQN variants."""

    def __init__(self, env, n_bins: int = 5):
        self.env = env
        self.n_bins = n_bins
        self.n_actions = n_bins ** 4  # 4 action dims
        self.obs_dim = env.get_obs_dim()
        self._bins = np.linspace(0, 1, n_bins + 1)[1:-1]  # bin edges

    def _discrete_to_continuous(self, a: int) -> np.ndarray:
        """Convert discrete action index to continuous [0,1]^4."""
        out = np.zeros(4, dtype=np.float32)
        for i in range(4):
            idx = (a // (self.n_bins ** i)) % self.n_bins
            out[i] = np.clip((idx + 0.5) / self.n_bins, 0.01, 0.99)
        return out

    def reset(self) -> np.ndarray:
        result = self.env.reset()
        if isinstance(result, tuple):
            result = result[0]
        return np.array(result, dtype=np.float32)

    def step(self, action: int) -> Tuple[np.ndarray, float, bool, bool, Dict]:
        cont_action = self._discrete_to_continuous(action)
        result = self.env.step(cont_action)
        if len(result) == 5:
            obs, reward, term, trunc, info = result
            done = term or trunc
            return np.array(obs), reward, term, trunc, dict(info)
        obs, reward, done, info = result
        return np.array(obs), reward, done, False, dict(info)

    def get_obs_dim(self) -> int:
        return self.obs_dim

    def get_act_dim(self) -> int:
        return self.n_actions
