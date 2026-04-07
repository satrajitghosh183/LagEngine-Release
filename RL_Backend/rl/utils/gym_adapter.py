"""Adapter to make gymnasium envs compatible with rldemo_env interface."""

import numpy as np
from typing import Tuple, Any, Dict


class GymAdapter:
    """Wraps gymnasium env to provide get_obs_dim(), get_act_dim(), reset(), step()."""

    def __init__(self, env):
        self._env = env
        obs_space = env.observation_space
        act_space = env.action_space
        if hasattr(obs_space, "shape"):
            self._obs_dim = int(np.prod(obs_space.shape))
        else:
            self._obs_dim = obs_space.n
        if hasattr(act_space, "n"):
            self._act_dim = act_space.n
            self._discrete = True
            self._act_scale = None
        else:
            self._act_dim = int(np.prod(act_space.shape))
            self._discrete = False
            # Scale agent output [-1,1] to env's action range (e.g. Pendulum: [-2,2])
            if hasattr(act_space, "low") and hasattr(act_space, "high"):
                low, high = np.array(act_space.low), np.array(act_space.high)
                self._act_scale = (high - low) / 2.0
                self._act_bias = (high + low) / 2.0
            else:
                self._act_scale = None
                self._act_bias = None

    def get_obs_dim(self) -> int:
        return self._obs_dim

    def get_act_dim(self) -> int:
        return self._act_dim

    def reset(self) -> np.ndarray:
        obs, info = self._env.reset()
        return np.array(obs, dtype=np.float32)

    def step(self, action) -> Tuple[np.ndarray, float, bool, Dict]:
        if not self._discrete and self._act_scale is not None:
            action = self._act_scale * np.asarray(action) + self._act_bias
        result = self._env.step(action)
        if len(result) == 5:
            obs, reward, term, trunc, info = result
            done = term or trunc
        else:
            obs, reward, done, info = result
        return np.array(obs, dtype=np.float32), float(reward), bool(done), dict(info or {})
