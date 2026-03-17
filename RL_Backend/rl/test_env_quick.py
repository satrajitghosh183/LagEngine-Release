#!/usr/bin/env python3
"""Quick test of rldemo_env"""
import sys
import numpy as np
sys.path.insert(0, ".")

try:
    import rldemo_env
    print("Import OK")
    
    # Test JobSchedulerEnv (new fixed-workload environment)
    print("\n=== Testing JobSchedulerEnv ===")
    env = rldemo_env.JobSchedulerEnv()
    env.set_cube_count(100000)  # 100k cubes
    print(f"Obs dim: {env.get_obs_dim()}, Act dim: {env.get_act_dim()}")
    
    obs = env.reset()
    print(f"Reset OK, obs shape: {len(obs)}")
    
    # Test with uniform distribution
    action = np.ones(16, dtype=np.float32) / 16
    for i in range(5):
        obs, r, d, info = env.step(action)
        print(f"Step {i+1}: reward={r:.2f}, frame_time={info['frame_time']:.2f}ms, workers={info.get('worker_count', 'N/A')}")
    
    print("\nJobSchedulerEnv tests passed!")
    
except Exception as e:
    print(f"Error: {e}")
    import traceback
    traceback.print_exc()
    sys.exit(1)
