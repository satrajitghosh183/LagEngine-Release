"""Smoke test for RL_Backend environment."""

import pytest
import numpy as np

def test_env_smoke():
    try:
        import rldemo_env
    except ImportError:
        pytest.skip("rldemo_env not built")
        return

    env = rldemo_env.FrameSchedulerEnv()
    obs = env.reset()
    assert obs.shape == (8,)
    action = np.zeros(4, dtype=np.float32)
    obs2, reward, done, info = env.step(action)
    assert obs2.shape == (8,)
    assert isinstance(reward, (int, float))
    assert isinstance(done, bool)
    assert "frame_time" in info
