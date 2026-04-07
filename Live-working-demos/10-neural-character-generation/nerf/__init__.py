"""
NeRF package for joint view selection, pose regression, and NeRF supervision pipeline.

This package contains a simplified implementation of Neural Radiance Fields (NeRF)
that is designed to be differentiable and work with the joint training pipeline.
"""

from .tiny_nerf import TinyNeRF
from .weighted_tiny_nerf import WeightedTinyNeRF
from .nerf_config import (
    NeRFConfig, 
    WeightedNeRFConfig, 
    JointTrainingConfig,
    create_nerf_config,
    create_weighted_nerf_config,
    create_joint_training_config
)
from .nerf_utils import (
    visualize_camera_poses,
    save_view_selection,
    visualize_nerf_results,
    quaternion_to_rotation_matrix_numpy,
    create_camera_frustum
)

__all__ = [
    'TinyNeRF',
    'WeightedTinyNeRF',
    'NeRFConfig',
    'WeightedNeRFConfig',
    'JointTrainingConfig',
    'create_nerf_config',
    'create_weighted_nerf_config',
    'create_joint_training_config',
    'visualize_camera_poses',
    'save_view_selection',
    'visualize_nerf_results',
    'quaternion_to_rotation_matrix_numpy',
    'create_camera_frustum'
]