
"""
Multi-Token Context Model (MTCM) for joint view selection, pose regression, and NeRF supervision.
"""

# Configuration
from .config import MTCMConfig

# Core model components
from .model import MTCM_MAE

# Prediction heads
from .heads import ReconstructionHead, SelectionHead, PoseRegressionHead

# Utilities
from .masking import random_mask_indices
from .utils import setup_logger, check_tensor_shape

__all__ = [
    'MTCMConfig',
    'MTCM_MAE',
    'ReconstructionHead',
    'SelectionHead',
    'PoseRegressionHead',
    'random_mask_indices',
    'setup_logger',
    'check_tensor_shape'
]

__version__ = '1.0.0'