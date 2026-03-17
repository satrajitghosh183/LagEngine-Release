"""
Gymnasium-style wrapper for RL_Backend C++ environments
"""

import numpy as np
from typing import Optional, Tuple, Any, Dict

try:
    import rldemo_env
    HAS_CPP_ENV = True
except ImportError:
    HAS_CPP_ENV = False


class WorkStealSchedulerEnv:
    """Gymnasium-style WorkStealSchedulerEnv (N-body physics, work distribution)."""

    def __init__(self, seed: Optional[int] = None, body_count: Optional[int] = None):
        if not HAS_CPP_ENV:
            raise ImportError("rldemo_env not built. Build with -DRL_BACKEND_BUILD_RL=ON")
        self._env = rldemo_env.WorkStealSchedulerEnv()
        self.observation_space = self._make_box(self._env.get_obs_dim(), low=0, high=1)
        self.action_space = self._make_box(self._env.get_act_dim(), low=0.01, high=1)
        if body_count is not None:
            self._env.set_body_count(body_count)
        if seed is not None:
            np.random.seed(seed)

    def _make_box(self, dim: int, low: float, high: float):
        try:
            from gymnasium.spaces import Box
            return Box(low=np.float32(low), high=np.float32(high), shape=(dim,), dtype=np.float32)
        except ImportError:
            return None

    def reset(self, seed: Optional[int] = None, options: Optional[Dict] = None) -> Tuple[np.ndarray, Dict]:
        obs = np.array(self._env.reset(), dtype=np.float32)
        return obs, {}

    def step(self, action: np.ndarray) -> Tuple[np.ndarray, float, bool, bool, Dict]:
        action = np.asarray(action, dtype=np.float32)
        obs, reward, done, info = self._env.step(action)
        return np.array(obs), float(reward), bool(done), False, dict(info)

    def get_obs_dim(self) -> int:
        return self._env.get_obs_dim()

    def get_act_dim(self) -> int:
        return self._env.get_act_dim()

    def set_body_count(self, n: int) -> None:
        self._env.set_body_count(n)


class FrameSchedulerEnv:
    """Gymnasium-style FrameSchedulerEnv."""

    def __init__(self, seed: Optional[int] = None):
        if not HAS_CPP_ENV:
            raise ImportError("rldemo_env not built. Build with -DRL_BACKEND_BUILD_RL=ON")
        self._env = rldemo_env.FrameSchedulerEnv()
        self.observation_space = self._make_box(self._env.get_obs_dim(), low=0, high=1)
        self.action_space = self._make_box(self._env.get_act_dim(), low=0, high=1)
        if seed is not None:
            np.random.seed(seed)

    def _make_box(self, dim: int, low: float, high: float):
        try:
            from gymnasium.spaces import Box
            return Box(low=np.float32(low), high=np.float32(high), shape=(dim,), dtype=np.float32)
        except ImportError:
            return None

    def reset(self, seed: Optional[int] = None, options: Optional[Dict] = None) -> Tuple[np.ndarray, Dict]:
        obs = np.array(self._env.reset(), dtype=np.float32)
        return obs, {}

    def step(self, action: np.ndarray) -> Tuple[np.ndarray, float, bool, bool, Dict]:
        action = np.asarray(action, dtype=np.float32)
        obs, reward, done, info = self._env.step(action)
        return np.array(obs), float(reward), bool(done), False, dict(info)
