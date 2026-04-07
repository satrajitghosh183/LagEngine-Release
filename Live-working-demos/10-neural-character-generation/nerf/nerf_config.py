"""
Configuration classes for TinyNeRF and WeightedTinyNeRF modules.
"""

from dataclasses import dataclass
from typing import Optional, Tuple


@dataclass
class NeRFConfig:
    """
    Configuration for TinyNeRF model.
    
    Attributes:
        num_encoding_functions (int): Number of positional encoding functions
        hidden_dim (int): Hidden dimension for MLP layers
        num_layers (int): Number of MLP layers
        image_height (int): Height of input/output images
        image_width (int): Width of input/output images
        near_plane (float): Near plane distance for ray sampling
        far_plane (float): Far plane distance for ray sampling
        num_samples_per_ray (int): Number of sample points per ray
        num_rays_per_batch (int): Number of rays to sample per batch during training
    """
    num_encoding_functions: int = 10
    hidden_dim: int = 128
    num_layers: int = 4
    image_height: int = 256
    image_width: int = 256
    near_plane: float = 2.0
    far_plane: float = 6.0
    num_samples_per_ray: int = 64
    num_rays_per_batch: int = 1024
    


@dataclass
class WeightedNeRFConfig(NeRFConfig):
    """
    Configuration for WeightedTinyNeRF model.
    
    Extends the base NeRFConfig with parameters for view selection.
    
    Additional Attributes:
        top_k (int): Number of views to select
    """
    top_k: int = 5


@dataclass
class JointTrainingConfig:
    """
    Configuration for the joint training of Transformer + NeRF.
    
    Attributes:
        batch_size (int): Batch size for training
        num_epochs (int): Number of training epochs
        learning_rate (float): Learning rate for optimizer
        weight_decay (float): Weight decay for optimizer
        checkpoint_interval (int): Save checkpoints every N epochs
        validation_interval (int): Run validation every N epochs
        pose_loss_weight (float): Weight for pose regression loss
        nerf_loss_weight (float): Weight for NeRF reconstruction loss
        selection_entropy_weight (float): Weight for selection entropy regularization
        pose_supervision (bool): Whether to use ground truth pose supervision
        mixed_precision (bool): Whether to use mixed precision training
        render_interval (int): Render visualization every N epochs
        warmup_epochs (int): Number of warmup epochs for NeRF training
        nerf_ray_batch_size (int): Batch size for NeRF ray sampling
        min_selection_temp (float): Minimum temperature for selection softmax
        max_selection_temp (float): Maximum temperature for selection softmax
        selection_temp_decay (float): Decay rate for selection temperature
    """
    # Training parameters
    batch_size: int = 8
    top_k: int = 5
    num_epochs: int = 50
    learning_rate: float = 1e-4
    weight_decay: float = 1e-5
    checkpoint_interval: int = 5
    validation_interval: int = 2
    image_height: int = 256
    image_width: int = 256
    # Loss weights
    pose_loss_weight: float = 1.0
    nerf_loss_weight: float = 10.0
    selection_entropy_weight: float = 0.01
    diversity_loss_weight: float = 0.1  # Add this line
    
    # Training options
    pose_supervision: bool = True
    mixed_precision: bool = True
    render_interval: int = 5
    warmup_epochs: int = 5
    nerf_ray_batch_size: int = 1024
    
    # Selection temperature annealing
    min_selection_temp: float = 0.5
    max_selection_temp: float = 5.0
    selection_temp_decay: float = 0.9


def create_nerf_config(config_dict=None):
    """
    Create a NeRFConfig instance from a dictionary.
    
    Args:
        config_dict (dict, optional): Dictionary of configuration parameters
        
    Returns:
        NeRFConfig: Configuration object
    """
    if config_dict is None:
        return NeRFConfig()
    
    return NeRFConfig(**{
        k: v for k, v in config_dict.items() 
        if k in NeRFConfig.__dataclass_fields__
    })


def create_weighted_nerf_config(config_dict=None):
    """
    Create a WeightedNeRFConfig instance from a dictionary.
    
    Args:
        config_dict (dict, optional): Dictionary of configuration parameters
        
    Returns:
        WeightedNeRFConfig: Configuration object
    """
    if config_dict is None:
        return WeightedNeRFConfig()
    
    return WeightedNeRFConfig(**{
        k: v for k, v in config_dict.items() 
        if k in WeightedNeRFConfig.__dataclass_fields__
    })


def create_joint_training_config(config_dict=None):
    """
    Create a JointTrainingConfig instance from a dictionary.
    
    Args:
        config_dict (dict, optional): Dictionary of configuration parameters
        
    Returns:
        JointTrainingConfig: Configuration object
    """
    if config_dict is None:
        return JointTrainingConfig()
    
    return JointTrainingConfig(**{
        k: v for k, v in config_dict.items() 
        if k in JointTrainingConfig.__dataclass_fields__
    })