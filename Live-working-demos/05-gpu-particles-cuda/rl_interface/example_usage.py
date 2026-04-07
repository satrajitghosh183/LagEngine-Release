"""
Example usage of the CUDA GL Demo RL system
"""
import os
from CudaGLEnvironment import CudaGLEnvironment
import numpy as np


def example_basic_usage():
    """Basic example of using the environment"""
    print("=" * 60)
    print("Example: Basic Environment Usage")
    print("=" * 60)
    
    # Create environment
    env = CudaGLEnvironment(
        executable_path=None,  # Auto-detect
        num_frames=50,  # Fewer frames for faster testing
        target_fps=60.0
    )
    
    # Reset environment
    obs, info = env.reset()
    print(f"Initial observation: {obs}")
    print(f"Initial info: {info}")
    
    # Take a random action
    action = env.action_space.sample()
    print(f"\nTaking action: {action}")
    
    obs, reward, terminated, truncated, info = env.step(action)
    print(f"Reward: {reward:.4f}")
    print(f"Observation: {obs}")
    if 'metrics' in info:
        metrics = info['metrics']
        print(f"FPS: {metrics.get('avg_fps', 0):.2f}")
        print(f"Frame Time: {metrics.get('avg_frame_time_ms', 0):.2f} ms")
    
    env.close()
    print("\nDone!")


def example_manual_optimization():
    """Example of manually testing different configurations"""
    print("=" * 60)
    print("Example: Manual Configuration Testing")
    print("=" * 60)
    
    env = CudaGLEnvironment(
        num_frames=100,
        target_fps=60.0
    )
    
    # Test different particle counts
    particle_factors = [0.2, 0.4, 0.6, 0.8, 1.0]  # 10k to 500k
    block_size_indices = [1, 2]  # 256, 512
    
    best_reward = float('-inf')
    best_config = None
    
    for pf in particle_factors:
        for bsi in block_size_indices:
            action = np.array([pf, float(bsi)], dtype=np.float32)
            obs, reward, _, _, info = env.step(action)
            
            if 'metrics' in info:
                metrics = info['metrics']
                print(f"Particles: {metrics.get('particle_count', 'N/A')}, "
                      f"Block: {metrics.get('block_size', 'N/A')}, "
                      f"FPS: {metrics.get('avg_fps', 0):.2f}, "
                      f"Reward: {reward:.4f}")
            
            if reward > best_reward:
                best_reward = reward
                best_config = (pf, bsi, info.get('metrics', {}))
    
    print(f"\nBest configuration:")
    print(f"  Particle Factor: {best_config[0]}")
    print(f"  Block Size Index: {best_config[1]}")
    if best_config[2]:
        print(f"  FPS: {best_config[2].get('avg_fps', 0):.2f}")
        print(f"  Reward: {best_reward:.4f}")
    
    env.close()


def example_load_and_test_model():
    """Example of loading and testing a trained model"""
    print("=" * 60)
    print("Example: Testing Trained Model")
    print("=" * 60)
    
    try:
        from stable_baselines3 import PPO
        
        model_path = "rl_models/cuda_gl_rl_final.zip"
        if not os.path.exists(model_path):
            print(f"Model not found: {model_path}")
            print("Please train a model first using train_rl.py")
            return
        
        # Load model
        model = PPO.load(model_path)
        print(f"Loaded model from: {model_path}")
        
        # Create environment
        env = CudaGLEnvironment(num_frames=100)
        
        # Test model
        obs = env.reset()
        for i in range(5):
            action, _states = model.predict(obs, deterministic=True)
            obs, reward, done, truncated, info = env.step(action)
            
            if 'metrics' in info:
                metrics = info['metrics']
                print(f"\nTest {i+1}:")
                print(f"  Action: {action}")
                print(f"  Particle Count: {metrics.get('particle_count', 'N/A')}")
                print(f"  Block Size: {metrics.get('block_size', 'N/A')}")
                print(f"  FPS: {metrics.get('avg_fps', 0):.2f}")
                print(f"  Frame Time: {metrics.get('avg_frame_time_ms', 0):.2f} ms")
                print(f"  Reward: {reward:.4f}")
        
        env.close()
        print("\nDone!")
        
    except ImportError:
        print("stable-baselines3 not installed. Install with: pip install stable-baselines3")
    except Exception as e:
        print(f"Error: {e}")


if __name__ == "__main__":
    import sys
    
    if len(sys.argv) > 1:
        example_name = sys.argv[1]
        if example_name == "basic":
            example_basic_usage()
        elif example_name == "manual":
            example_manual_optimization()
        elif example_name == "model":
            example_load_and_test_model()
        else:
            print(f"Unknown example: {example_name}")
            print("Available examples: basic, manual, model")
    else:
        print("CUDA GL Demo RL - Example Usage")
        print("\nAvailable examples:")
        print("  python example_usage.py basic   - Basic environment usage")
        print("  python example_usage.py manual  - Manual configuration testing")
        print("  python example_usage.py model   - Test trained model")
        print("\nRunning basic example...\n")
        example_basic_usage()

