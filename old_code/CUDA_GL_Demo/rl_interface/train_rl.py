"""
Training script for CUDA GL Demo Resource Allocation RL Agent
"""
import os
import argparse
import numpy as np
from stable_baselines3 import PPO
from stable_baselines3.common.callbacks import EvalCallback, CheckpointCallback, CallbackList
from stable_baselines3.common.monitor import Monitor
from stable_baselines3.common.vec_env import DummyVecEnv
import torch

from CudaGLEnvironment import CudaGLEnvironment
import logging

# Set up logging for the environment
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)


def make_env(executable_path: str = None, num_frames: int = 100, fixed_particle_count: int = 500000, rank: int = 0):
    """Create and wrap the environment"""
    def _init():
        env = CudaGLEnvironment(
            executable_path=executable_path,
            num_frames=num_frames,
            target_fps=60.0,
            fixed_particle_count=fixed_particle_count  # Fixed high particle count to stress GPU
        )
        env = Monitor(env, filename=None, allow_early_resets=True)
        return env
    return _init


def train(
    executable_path: str = None,
    num_frames: int = 100,
    fixed_particle_count: int = 500000,  # Fixed high particle count to stress GPU
    total_timesteps: int = 1000,
    learning_rate: float = 3e-4,
    n_steps: int = 128,
    batch_size: int = 64,
    n_epochs: int = 10,
    gamma: float = 0.99,
    gae_lambda: float = 0.95,
    clip_range: float = 0.2,
    ent_coef: float = 0.01,
    vf_coef: float = 0.5,
    max_grad_norm: float = 0.5,
    save_path: str = "./rl_models",
    log_dir: str = "./rl_logs",
    device: str = "auto"
):
    """Train the RL agent"""
    
    print("=" * 60)
    print("CUDA GL Demo - Reinforcement Learning Training")
    print("=" * 60)
    print(f"Executable: {executable_path}")
    print(f"Frames per episode: {num_frames}")
    print(f"Fixed particle count: {fixed_particle_count:,} (stressing GPU)")
    print(f"Total timesteps: {total_timesteps}")
    print(f"Learning rate: {learning_rate}")
    print(f"Device: {device}")
    print("=" * 60)
    print("Focus: Optimizing CUDA block size for GPU utilization (thread stealing)")
    print("=" * 60)
    
    # Create directories
    os.makedirs(save_path, exist_ok=True)
    os.makedirs(log_dir, exist_ok=True)
    
    # Create environment
    print("\nCreating environment...")
    if executable_path:
        print(f"Using executable: {executable_path}")
        if not os.path.exists(executable_path):
            print(f"WARNING: Executable not found at: {executable_path}")
            print("Please check the path or build the project first.")
    else:
        print("Auto-detecting executable...")
    
    env = DummyVecEnv([make_env(executable_path, num_frames, fixed_particle_count, 0)])
    
    # Create evaluation environment
    eval_env = DummyVecEnv([make_env(executable_path, num_frames, fixed_particle_count, 1)])
    
    # Create model
    print("\nCreating PPO model...")
    
    # Check if tensorboard is available
    try:
        import tensorboard
        use_tensorboard = True
        print("TensorBoard logging enabled")
    except ImportError:
        use_tensorboard = False
        print("Warning: TensorBoard not installed. Logging disabled.")
        print("Install with: pip install tensorboard")
    
    model = PPO(
        "MlpPolicy",
        env,
        learning_rate=learning_rate,
        n_steps=n_steps,
        batch_size=batch_size,
        n_epochs=n_epochs,
        gamma=gamma,
        gae_lambda=gae_lambda,
        clip_range=clip_range,
        ent_coef=ent_coef,
        vf_coef=vf_coef,
        max_grad_norm=max_grad_norm,
        verbose=1,
        tensorboard_log=log_dir if use_tensorboard else None,
        device=device,
        policy_kwargs=dict(
            net_arch=[256, 256, 128]  # Custom network architecture
        )
    )
    
    # Create callbacks
    checkpoint_callback = CheckpointCallback(
        save_freq=max(100, total_timesteps // 10),
        save_path=save_path,
        name_prefix="cuda_gl_rl"
    )
    
    eval_callback = EvalCallback(
        eval_env,
        best_model_save_path=save_path,
        log_path=log_dir,
        eval_freq=max(50, total_timesteps // 20),
        deterministic=True,
        render=False
    )
    
    callbacks = CallbackList([checkpoint_callback, eval_callback])
    
    # Train
    print("\nStarting training...")
    print("This may take a while. Each step runs a full simulation.")
    print("Press Ctrl+C to stop early and save progress.\n")
    
    try:
        model.learn(
            total_timesteps=total_timesteps,
            callback=callbacks,
            progress_bar=True
        )
    except KeyboardInterrupt:
        print("\n\nTraining interrupted by user. Saving model...")
    
    # Save final model
    final_model_path = os.path.join(save_path, "cuda_gl_rl_final")
    model.save(final_model_path)
    print(f"\nFinal model saved to: {final_model_path}")
    
    # Test the trained model
    print("\n" + "=" * 60)
    print("Testing trained model...")
    print("=" * 60)
    
    obs = env.reset()
    for i in range(5):
        action, _states = model.predict(obs, deterministic=True)
        obs, rewards, dones, info = env.step(action)
        
        if len(info) > 0 and 'metrics' in info[0]:
            metrics = info[0]['metrics']
            gpu_util = info[0].get('gpu_utilization', 0.0)
            print(f"\nTest {i+1}:")
            print(f"  Particle Count: {metrics.get('particle_count', 'N/A'):,} (fixed)")
            print(f"  Block Size: {metrics.get('block_size', 'N/A')}")
            print(f"  FPS: {metrics.get('avg_fps', 0):.2f}")
            print(f"  Frame Time: {metrics.get('avg_frame_time_ms', 0):.2f} ms")
            print(f"  GPU Utilization: {gpu_util:.2%}")
            print(f"  CUDA Time: {metrics.get('avg_cuda_time_ms', 0):.2f} ms")
            print(f"  Reward: {rewards[0]:.4f}")
    
    env.close()
    eval_env.close()
    
    print("\n" + "=" * 60)
    print("Training complete!")
    print(f"Models saved in: {save_path}")
    if use_tensorboard:
        print(f"Logs saved in: {log_dir}")
        print(f"View logs with: tensorboard --logdir {log_dir}")
    print("=" * 60)


def main():
    parser = argparse.ArgumentParser(description="Train RL agent for CUDA GL Demo")
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
        "--particles",
        type=int,
        default=500000,
        help="Fixed particle count to stress GPU (default: 500000)"
    )
    parser.add_argument(
        "--timesteps",
        type=int,
        default=1000,
        help="Total training timesteps (default: 1000)"
    )
    parser.add_argument(
        "--lr",
        type=float,
        default=3e-4,
        help="Learning rate (default: 3e-4)"
    )
    parser.add_argument(
        "--save-path",
        type=str,
        default="./rl_models",
        help="Path to save models (default: ./rl_models)"
    )
    parser.add_argument(
        "--log-dir",
        type=str,
        default="./rl_logs",
        help="Path to save logs (default: ./rl_logs)"
    )
    parser.add_argument(
        "--device",
        type=str,
        default="auto",
        choices=["auto", "cpu", "cuda"],
        help="Device to use (default: auto)"
    )
    
    args = parser.parse_args()
    
    train(
        executable_path=args.executable,
        num_frames=args.frames,
        fixed_particle_count=args.particles,
        total_timesteps=args.timesteps,
        learning_rate=args.lr,
        save_path=args.save_path,
        log_dir=args.log_dir,
        device=args.device
    )


if __name__ == "__main__":
    main()

