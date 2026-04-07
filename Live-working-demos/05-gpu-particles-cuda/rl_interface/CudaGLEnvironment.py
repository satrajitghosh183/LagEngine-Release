"""
Reinforcement Learning Environment for CUDA GL Demo Resource Allocation
"""
import gymnasium as gym
from gymnasium import spaces
import numpy as np
import json
import subprocess
import os
import time
from pathlib import Path
from typing import Dict, Tuple, Any, Optional
import logging


logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)


class CudaGLEnvironment(gym.Env):
    """
    RL Environment for optimizing CUDA GL Demo resource allocation.
    
    Focus: Optimize CUDA block size (thread stealing/GPU utilization) under fixed high particle count stress.
    
    Action Space: [block_size_idx, grid_size_factor]
    - block_size_idx: 0.0-3.0 (maps to [128, 256, 512, 1024])
    - grid_size_factor: 0.0-1.0 (adjusts grid size multiplier for fine-tuning)
    
    Observation Space: [fps, frame_time, cuda_time, cpu_time, upload_time, render_time, block_size_norm, gpu_utilization_estimate]
    """
    
    metadata = {"render_modes": ["human", "rgb_array"], "render_fps": 60}
    
    def __init__(
        self,
        executable_path: str = None,
        num_frames: int = 100,
        target_fps: float = 60.0,
        fixed_particle_count: int = 500000,  # Fixed high particle count to stress GPU
        render_mode: Optional[str] = None
    ):
        super().__init__()
        
        self.executable_path = executable_path or self._find_executable()
        self.num_frames = num_frames
        self.target_fps = target_fps
        self.fixed_particle_count = fixed_particle_count  # Fixed, not variable
        self.render_mode = render_mode
        
        # Action space: [block_size_idx (0-3), grid_size_factor (0-1)]
        # block_size_idx selects from [128, 256, 512, 1024]
        # grid_size_factor allows fine-tuning grid size (0.8-1.2x multiplier)
        self.action_space = spaces.Box(
            low=np.array([0.0, 0.0], dtype=np.float32),
            high=np.array([3.0, 1.0], dtype=np.float32),
            dtype=np.float32
        )
        
        # Observation space: [fps, frame_time, cuda_time, cpu_time, upload_time, render_time, block_size_norm, gpu_utilization_estimate]
        # gpu_utilization_estimate = cuda_time / frame_time (higher = better GPU utilization)
        self.observation_space = spaces.Box(
            low=np.array([0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], dtype=np.float32),
            high=np.array([200.0, 100.0, 50.0, 50.0, 50.0, 50.0, 1.0, 1.0], dtype=np.float32),
            dtype=np.float32
        )
        
        self.block_sizes = [128, 256, 512, 1024]
        self.metrics_file = "rl_metrics_temp.json"
        self.current_particles = fixed_particle_count  # Always fixed
        self.current_block_size = 256
        self.last_metrics = None
        
    def _find_executable(self) -> str:
        """Find the CUDA GL demo executable"""
        possible_paths = [
            "../build/cuda_gl_demo_rl",
            "../build/cuda_gl_demo",
            "build/cuda_gl_demo_rl",
            "build/cuda_gl_demo",
            "./cuda_gl_demo_rl",
            "./cuda_gl_demo",
            "cuda_gl_demo_rl.exe",
            "cuda_gl_demo.exe"
        ]
        
        for path in possible_paths:
            if os.path.exists(path):
                abs_path = os.path.abspath(path)
                logger.info(f"Found executable: {abs_path}")
                return abs_path
        
        # Print helpful error message
        logger.error("Could not find cuda_gl_demo executable in any of these locations:")
        for path in possible_paths:
            logger.error(f"  - {os.path.abspath(path)}")
        logger.error(f"Current working directory: {os.getcwd()}")
        
        raise FileNotFoundError(
            "Could not find cuda_gl_demo executable. "
            "Please build the project first (cd ../build && cmake .. && make) "
            "or specify executable_path when creating the environment."
        )
    
    def _run_simulation(self, block_size_idx: float, grid_size_factor: float = 1.0) -> Dict[str, Any]:
        """Run the CUDA GL demo with given parameters and return metrics"""
        metrics_path = os.path.abspath(self.metrics_file)
        
        # Convert block_size_idx to actual block size
        block_size_idx_int = min(max(0, int(np.clip(block_size_idx, 0, len(self.block_sizes) - 1))), len(self.block_sizes) - 1)
        actual_block_size = self.block_sizes[block_size_idx_int]
        
        # Use fixed particle count (stress the GPU) - always the same
        particle_count = self.fixed_particle_count
        
        # Check if executable exists
        if not os.path.exists(self.executable_path):
            logger.error(f"Executable not found: {self.executable_path}")
            logger.error(f"Current working directory: {os.getcwd()}")
            return self._default_metrics()
        
        # Check if executable is actually executable
        if not os.access(self.executable_path, os.X_OK):
            logger.error(f"Executable is not executable: {self.executable_path}")
            return self._default_metrics()
        
        cmd = [
            self.executable_path,
            "--particles", str(particle_count),
            "--blocksize", str(actual_block_size),
            "--frames", str(self.num_frames),
            "--output", metrics_path
        ]
        
        logger.info(f"Running simulation: {' '.join(cmd)}")
        logger.info(f"Working directory: {os.path.dirname(self.executable_path) or '.'}")
        logger.info(f"Metrics will be written to: {metrics_path}")
        
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=60.0,  # 60 second timeout
                cwd=os.path.dirname(self.executable_path) or "."
            )
            
            # Log output for debugging
            if result.stdout:
                logger.debug(f"Simulation stdout: {result.stdout}")
            if result.stderr:
                logger.debug(f"Simulation stderr: {result.stderr}")
            
            if result.returncode != 0:
                logger.error(f"Simulation failed with return code {result.returncode}")
                logger.error(f"Command: {' '.join(cmd)}")
                logger.error(f"Stdout: {result.stdout}")
                logger.error(f"Stderr: {result.stderr}")
                return self._default_metrics()
            
            # Read metrics from JSON file
            if os.path.exists(metrics_path):
                try:
                    with open(metrics_path, 'r') as f:
                        metrics = json.load(f)
                    logger.info(f"Successfully loaded metrics: FPS={metrics.get('avg_fps', 0):.2f}, "
                              f"Particles={metrics.get('particle_count', 0)}, "
                              f"BlockSize={metrics.get('block_size', 0)}")
                    return metrics
                except json.JSONDecodeError as e:
                    logger.error(f"Failed to parse JSON metrics file: {e}")
                    logger.error(f"File contents: {open(metrics_path, 'r').read()[:500]}")
                    return self._default_metrics()
            else:
                logger.error(f"Metrics file not found: {metrics_path}")
                logger.error(f"Expected file at: {os.path.abspath(metrics_path)}")
                logger.error(f"Current directory: {os.getcwd()}")
                logger.error(f"Files in current directory: {os.listdir('.')[:10]}")
                return self._default_metrics()
                
        except subprocess.TimeoutExpired:
            logger.error("Simulation timed out after 60 seconds")
            return self._default_metrics()
        except Exception as e:
            logger.error(f"Error running simulation: {e}", exc_info=True)
            return self._default_metrics()
    
    def _default_metrics(self) -> Dict[str, Any]:
        """Return default metrics when simulation fails"""
        return {
            "particle_count": self.fixed_particle_count,
            "block_size": self.current_block_size,
            "frames": self.num_frames,
            "avg_fps": 0.0,
            "avg_frame_time_ms": 1000.0,
            "avg_cuda_time_ms": 50.0,
            "avg_cpu_time_ms": 50.0,
            "avg_upload_time_ms": 50.0,
            "avg_render_time_ms": 50.0,
            "total_time_ms": 1000.0 * self.num_frames
        }
    
    def _compute_reward(self, metrics: Dict[str, Any]) -> float:
        """
        Compute reward based on performance metrics under GPU stress.
        Focus: Maximize GPU utilization (thread stealing efficiency) while maintaining performance.
        """
        fps = metrics.get("avg_fps", 0.0)
        frame_time = metrics.get("avg_frame_time_ms", 1000.0)
        cuda_time = metrics.get("avg_cuda_time_ms", 0.0)
        cpu_time = metrics.get("avg_cpu_time_ms", 0.0)
        upload_time = metrics.get("avg_upload_time_ms", 0.0)
        render_time = metrics.get("avg_render_time_ms", 0.0)
        
        # Base reward: FPS normalized to target (under stress, we want to maximize what we can get)
        fps_reward = min(fps / self.target_fps, 2.0)  # Cap at 2x target
        
        # GPU utilization reward: Higher CUDA time relative to frame time = better GPU utilization
        # This rewards efficient thread block configuration (thread stealing)
        if frame_time > 0:
            gpu_utilization = min(cuda_time / frame_time, 1.0)  # Ratio of GPU work to total time
            gpu_utilization_reward = gpu_utilization * 0.5  # Bonus for high GPU utilization
        else:
            gpu_utilization_reward = 0.0
        
        # Efficiency reward: Lower overhead (CPU + upload + render) relative to CUDA time
        # This rewards configurations that minimize overhead
        total_overhead = cpu_time + upload_time + render_time
        if cuda_time > 0:
            efficiency_ratio = cuda_time / (cuda_time + total_overhead + 0.1)  # Avoid division by zero
            efficiency_reward = efficiency_ratio * 0.3
        else:
            efficiency_reward = 0.0
        
        # Penalty for high frame time (we want to maintain performance under stress)
        frame_time_penalty = max(0, (frame_time - 16.67) / 16.67) * 0.5  # 16.67ms = 60fps
        
        # Penalty if FPS is too low (below 30 FPS is unacceptable)
        if fps < self.target_fps * 0.5:
            fps_penalty = (self.target_fps * 0.5 - fps) / self.target_fps * 2.0
        else:
            fps_penalty = 0.0
        
        # Stability bonus: Reward consistent performance (lower variance in frame times)
        # This is implicitly handled by the reward structure, but we can add explicit stability
        
        reward = fps_reward + gpu_utilization_reward + efficiency_reward - frame_time_penalty - fps_penalty
        
        return float(reward)
    
    def _get_observation(self, metrics: Dict[str, Any]) -> np.ndarray:
        """Convert metrics to observation vector"""
        fps = metrics.get("avg_fps", 0.0)
        frame_time = metrics.get("avg_frame_time_ms", 0.0)
        cuda_time = metrics.get("avg_cuda_time_ms", 0.0)
        cpu_time = metrics.get("avg_cpu_time_ms", 0.0)
        upload_time = metrics.get("avg_upload_time_ms", 0.0)
        render_time = metrics.get("avg_render_time_ms", 0.0)
        block_size = metrics.get("block_size", 256)
        
        # Normalize block size
        block_size_norm = (block_size - 128) / (1024 - 128) if 1024 > 128 else 0.0
        
        # Estimate GPU utilization (cuda_time / frame_time)
        # Higher ratio = better GPU utilization (thread stealing efficiency)
        if frame_time > 0:
            gpu_utilization_estimate = min(cuda_time / frame_time, 1.0)
        else:
            gpu_utilization_estimate = 0.0
        
        obs = np.array([
            fps,
            frame_time,
            cuda_time,
            cpu_time,
            upload_time,
            render_time,
            block_size_norm,
            gpu_utilization_estimate
        ], dtype=np.float32)
        
        return obs
    
    def reset(self, seed: Optional[int] = None, options: Optional[Dict] = None) -> Tuple[np.ndarray, Dict]:
        """Reset the environment"""
        super().reset(seed=seed)
        
        # Start with default parameters
        self.current_particles = self.fixed_particle_count  # Always fixed
        self.current_block_size = 256
        
        # Run initial simulation with default block size (256 = index 1)
        metrics = self._run_simulation(block_size_idx=1.0, grid_size_factor=1.0)
        self.last_metrics = metrics
        
        observation = self._get_observation(metrics)
        info = {"metrics": metrics}
        
        return observation, info
    
    def step(self, action: np.ndarray) -> Tuple[np.ndarray, float, bool, bool, Dict]:
        """Execute one step in the environment"""
        # Decode action: [block_size_idx, grid_size_factor]
        block_size_idx = float(np.clip(action[0], 0.0, 3.0))
        grid_size_factor = float(np.clip(action[1], 0.0, 1.0))
        
        # Map grid_size_factor to multiplier (0.8x to 1.2x for fine-tuning)
        # This allows the RL agent to fine-tune grid size for optimal GPU utilization
        grid_multiplier = 0.8 + (grid_size_factor * 0.4)  # Range: 0.8 to 1.2
        
        # Get actual block size
        block_size_idx_int = int(np.clip(block_size_idx, 0, len(self.block_sizes) - 1))
        self.current_block_size = self.block_sizes[block_size_idx_int]
        
        # Particle count is always fixed (stress the GPU)
        self.current_particles = self.fixed_particle_count
        
        # Run simulation with new block size (grid_size_factor is for future use if we modify the executable)
        metrics = self._run_simulation(block_size_idx, grid_size_factor)
        self.last_metrics = metrics
        
        # Compute reward
        reward = self._compute_reward(metrics)
        
        # Get observation
        observation = self._get_observation(metrics)
        
        # Episode is done after one step (we can make it multi-step if needed)
        terminated = True
        truncated = False
        
        info = {
            "metrics": metrics,
            "particle_count": self.current_particles,  # Always fixed
            "block_size": self.current_block_size,
            "fps": metrics.get("avg_fps", 0.0),
            "gpu_utilization": metrics.get("avg_cuda_time_ms", 0.0) / max(metrics.get("avg_frame_time_ms", 1.0), 1.0)
        }
        
        return observation, reward, terminated, truncated, info
    
    def render(self):
        """Render the environment (not implemented for headless mode)"""
        if self.render_mode == "human":
            logger.info(f"Current metrics: {self.last_metrics}")
        pass
    
    def close(self):
        """Clean up resources"""
        if os.path.exists(self.metrics_file):
            try:
                os.remove(self.metrics_file)
            except:
                pass

