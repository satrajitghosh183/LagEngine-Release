"""
Evaluate a trained RL model on the CUDA GL Demo environment
"""
import os
import argparse
import numpy as np
from stable_baselines3 import PPO
from stable_baselines3.common.vec_env import DummyVecEnv
import json

from CudaGLEnvironment import CudaGLEnvironment


def evaluate_model(
    model_path: str,
    executable_path: str = None,
    num_frames: int = 100,
    num_episodes: int = 10,
    output_file: str = "evaluation_results.json"
):
    """Evaluate a trained model"""
    
    print("=" * 60)
    print("CUDA GL Demo - RL Model Evaluation")
    print("=" * 60)
    print(f"Model: {model_path}")
    print(f"Executable: {executable_path}")
    print(f"Episodes: {num_episodes}")
    print("=" * 60)
    
    # Load model
    print("\nLoading model...")
    model = PPO.load(model_path)
    
    # Create environment
    print("Creating environment...")
    env = DummyVecEnv([
        lambda: CudaGLEnvironment(
            executable_path=executable_path,
            num_frames=num_frames,
            target_fps=60.0,
            fixed_particle_count=500000  # Fixed high particle count
        )
    ])
    
    # Evaluate
    print(f"\nRunning {num_episodes} evaluation episodes...\n")
    
    results = []
    total_reward = 0.0
    
    for episode in range(num_episodes):
        obs = env.reset()
        episode_reward = 0.0
        episode_metrics = None
        
        done = False
        step_count = 0
        
        while not done and step_count < 10:  # Limit steps per episode
            action, _states = model.predict(obs, deterministic=True)
            obs, reward, done, info = env.step(action)
            episode_reward += reward[0]
            step_count += 1
            
            if len(info) > 0 and 'metrics' in info[0]:
                episode_metrics = info[0]['metrics']
        
        total_reward += episode_reward
        
        result = {
            "episode": episode + 1,
            "reward": float(episode_reward),
            "metrics": episode_metrics
        }
        results.append(result)
        
        if episode_metrics:
            print(f"Episode {episode + 1}:")
            print(f"  Reward: {episode_reward:.4f}")
            print(f"  Particle Count: {episode_metrics.get('particle_count', 'N/A')}")
            print(f"  Block Size: {episode_metrics.get('block_size', 'N/A')}")
            print(f"  FPS: {episode_metrics.get('avg_fps', 0):.2f}")
            print(f"  Frame Time: {episode_metrics.get('avg_frame_time_ms', 0):.2f} ms")
            print()
    
    avg_reward = total_reward / num_episodes
    
    # Save results
    output_data = {
        "model_path": model_path,
        "num_episodes": num_episodes,
        "average_reward": float(avg_reward),
        "episodes": results
    }
    
    with open(output_file, 'w') as f:
        json.dump(output_data, f, indent=2)
    
    print("=" * 60)
    print(f"Evaluation complete!")
    print(f"Average Reward: {avg_reward:.4f}")
    print(f"Results saved to: {output_file}")
    print("=" * 60)
    
    env.close()
    
    return output_data


def main():
    parser = argparse.ArgumentParser(description="Evaluate RL model for CUDA GL Demo")
    parser.add_argument(
        "model_path",
        type=str,
        help="Path to trained model"
    )
    parser.add_argument(
        "--executable",
        type=str,
        default=None,
        help="Path to cuda_gl_demo executable (default: auto-detect)"
    )
    parser.add_argument(
        "--frames",
        type=int,
        default=100,
        help="Number of frames per simulation (default: 100)"
    )
    parser.add_argument(
        "--episodes",
        type=int,
        default=10,
        help="Number of evaluation episodes (default: 10)"
    )
    parser.add_argument(
        "--output",
        type=str,
        default="evaluation_results.json",
        help="Output file for results (default: evaluation_results.json)"
    )
    
    args = parser.parse_args()
    
    evaluate_model(
        model_path=args.model_path,
        executable_path=args.executable,
        num_frames=args.frames,
        num_episodes=args.episodes,
        output_file=args.output
    )


if __name__ == "__main__":
    main()

