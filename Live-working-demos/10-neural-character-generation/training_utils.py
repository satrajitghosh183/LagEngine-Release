import os
import torch
import torch.nn as nn
import torch.nn.functional as F
import time
import numpy as np
import math
import contextlib
from datetime import datetime
from typing import Dict, List, Tuple, Optional, Union, Any, Callable
from torch.utils.tensorboard import SummaryWriter


class AverageMeter:
    """Computes and stores the average and current value"""
    def __init__(self, name: str, fmt: str = ':f'):
        self.name = name
        self.fmt = fmt
        self.reset()

    def reset(self):
        self.val = 0
        self.avg = 0
        self.sum = 0
        self.count = 0

    def update(self, val, n=1):
        self.val = val
        self.sum += val * n
        self.count += n
        self.avg = self.sum / self.count

    def __str__(self):
        fmtstr = '{name} {val' + self.fmt + '} ({avg' + self.fmt + '})'
        return fmtstr.format(**self.__dict__)


class ProgressMeter:
    """Display training progress"""
    def __init__(self, num_batches, meters, prefix=""):
        self.batch_fmtstr = self._get_batch_fmtstr(num_batches)
        self.meters = meters
        self.prefix = prefix

    def display(self, batch):
        entries = [self.prefix + self.batch_fmtstr.format(batch)]
        entries += [str(meter) for meter in self.meters]
        print('\t'.join(entries))

    def _get_batch_fmtstr(self, num_batches):
        num_digits = len(str(num_batches // 1))
        fmt = '{:' + str(num_digits) + 'd}'
        return '[' + fmt + '/' + fmt.format(num_batches) + ']'


def save_checkpoint(
    state: Dict[str, Any],
    is_best: bool,
    filename: str,
    best_filename: Optional[str] = None
):
    """
    Save training checkpoint.
    
    Args:
        state: Checkpoint state with model weights and metadata
        is_best: Whether this is the best model so far
        filename: Path to save the checkpoint
        best_filename: Path to save the best checkpoint (if different)
    """
    # Create directory if it doesn't exist
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    
    # Save current checkpoint
    torch.save(state, filename)
    
    # Save as best if applicable
    if is_best:
        if best_filename is None:
            best_filename = os.path.join(
                os.path.dirname(filename),
                f"best_{os.path.basename(filename)}"
            )
        
        import shutil
        shutil.copyfile(filename, best_filename)
        print(f"Saved best checkpoint to {best_filename}")


def adjust_learning_rate(
    optimizer: torch.optim.Optimizer,
    epoch: int,
    args: Any
) -> float:
    """
    Adjust learning rate with cosine annealing.
    
    Args:
        optimizer: Optimizer to adjust learning rate for
        epoch: Current epoch
        args: Arguments containing lr_schedule info
        
    Returns:
        Current learning rate
    """
    lr = args.lr
    
    if hasattr(args, 'lr_schedule') and args.lr_schedule == 'cosine':
        # Cosine annealing
        eta_min = args.lr * 0.1
        lr = eta_min + (args.lr - eta_min) * (
            1 + np.cos(np.pi * epoch / args.epochs)
        ) / 2
    elif hasattr(args, 'lr_schedule') and args.lr_schedule == 'step':
        # Step decay
        lr_decay_epochs = args.lr_decay_epochs if hasattr(args, 'lr_decay_epochs') else [30, 60, 90]
        lr_decay_rate = args.lr_decay_rate if hasattr(args, 'lr_decay_rate') else 0.1
        
        for milestone in lr_decay_epochs:
            lr *= lr_decay_rate if epoch >= milestone else 1.0
    
    # Set the new learning rate
    for param_group in optimizer.param_groups:
        param_group['lr'] = lr
        
    return lr
def compute_pose_loss(pred_poses, target_poses, pose_loss_type='combined', weights=None):
    """
    Compute loss between predicted and target poses.
    
    Args:
        pred_poses (torch.Tensor): Predicted poses [B*N, 7] or [B, N, 7] 
        target_poses (torch.Tensor): Target poses [B, 7]
        pose_loss_type (str): Type of loss calculation
        weights (torch.Tensor, optional): Weights for each prediction
        
    Returns:
        torch.Tensor: Pose loss
    """
    # Handle different input formats
    if len(pred_poses.shape) == 3:  # [B, N, 7]
        batch_size, num_views, _ = pred_poses.shape
        pred_poses_flat = pred_poses.reshape(batch_size * num_views, -1)
    else:  # [B*N, 7]
        batch_size = target_poses.shape[0]
        num_views = pred_poses.shape[0] // batch_size
        pred_poses_flat = pred_poses
    
    # Expand target poses to match predicted poses shape
    target_poses_expanded = target_poses.unsqueeze(1).expand(-1, num_views, -1).reshape(batch_size * num_views, -1)
    
    # Now both have shape [B*N, 7] and we can compute the loss
    position_loss = F.mse_loss(pred_poses_flat[:, :3], target_poses_expanded[:, :3], reduction='none').sum(dim=1)
    rotation_loss = F.mse_loss(pred_poses_flat[:, 3:], target_poses_expanded[:, 3:], reduction='none').sum(dim=1)
    
    # Reshape to [B, N] for weighting
    position_loss = position_loss.view(batch_size, num_views)
    rotation_loss = rotation_loss.view(batch_size, num_views)
    
    # Apply weights if provided
    if weights is not None:
        position_loss = (position_loss * weights).sum(dim=1).mean()  # [B]
        rotation_loss = (rotation_loss * weights).sum(dim=1).mean()  # [B]
    else:
        position_loss = position_loss.mean()
        rotation_loss = rotation_loss.mean()
    
    if pose_loss_type == 'separate':
        return position_loss + rotation_loss
    elif pose_loss_type == 'position_only':
        return position_loss
    else:  # combined
        return position_loss + rotation_loss

# def compute_pose_loss(
#     pred_poses: torch.Tensor,
#     target_poses: torch.Tensor,
#     pose_loss_type: str = 'l2'
# ) -> torch.Tensor:
#     """
#     Compute loss between predicted and target poses.
    
#     Args:
#         pred_poses: Predicted poses [B, 7] (position + quaternion)
#         target_poses: Target poses [B, 7]
#         pose_loss_type: Type of loss to use ('l2', 'l1', 'huber', or 'separate')
        
#     Returns:
#         Pose loss
#     """
#     if pose_loss_type == 'separate':
#         # Separate position and orientation losses
#         position_loss = F.mse_loss(pred_poses[:, :3], target_poses[:, :3])
        
#         # Normalize quaternions before computing loss
#         pred_quats = F.normalize(pred_poses[:, 3:], dim=1)
#         target_quats = F.normalize(target_poses[:, 3:], dim=1)
        
#         # Handle antipodal quaternions (q and -q represent the same rotation)
#         # Take the minimum of the distance between q and q' and q and -q'
#         quat_dist = torch.sum((pred_quats - target_quats) ** 2, dim=1)
#         quat_dist_neg = torch.sum((pred_quats + target_quats) ** 2, dim=1)
#         quat_loss = torch.mean(torch.min(quat_dist, quat_dist_neg))
        
#         # Combine with higher weight on position
#         return position_loss + 0.2 * quat_loss
    
#     elif pose_loss_type == 'l1':
#         return F.l1_loss(pred_poses, target_poses)
    
#     elif pose_loss_type == 'huber':
#         return F.smooth_l1_loss(pred_poses, target_poses)
    
#     else:  # Default: L2 loss
#         return F.mse_loss(pred_poses, target_poses)
# def compute_pose_loss(predicted_poses, target_poses, pose_loss_type='combined', weights=None):
#     """
#     Compute loss between predicted and target poses.
    
#     Args:
#         predicted_poses (torch.Tensor): Predicted poses [B*N, 7] or [B, N, 7] (position + quaternion)
#         target_poses (torch.Tensor): Target poses [B, 7] (position + quaternion)
#         pose_loss_type (str): Type of pose loss ('combined', 'separate', 'weighted')
#         weights (torch.Tensor, optional): Weights for each pose prediction [B, N]
        
#     Returns:
#         torch.Tensor: Pose loss
#     """
#     # Check if predicted_poses is [B, N, 7] and reshape if needed
#     if len(predicted_poses.shape) == 3:  # [B, N, 7]
#         batch_size, num_views, _ = predicted_poses.shape
#         predicted_poses_flat = predicted_poses.reshape(batch_size * num_views, -1)
#     else:
#         batch_size = target_poses.shape[0]
#         num_views = predicted_poses.shape[0] // batch_size
#         predicted_poses_flat = predicted_poses  # Already [B*N, 7]
    
#     # Expand target_poses to match predicted_poses dimensions
#     target_poses_expanded = target_poses.unsqueeze(1).expand(-1, num_views, -1).reshape(batch_size * num_views, -1)
    
#     # Extract position and quaternion components
#     pred_position = predicted_poses_flat[:, :3]
#     pred_quaternion = predicted_poses_flat[:, 3:7]
#     target_position = target_poses_expanded[:, :3]
#     target_quaternion = target_poses_expanded[:, 3:7]
    
#     # Normalize quaternions (important for valid rotations)
#     pred_quaternion = F.normalize(pred_quaternion, dim=1)
#     target_quaternion = F.normalize(target_quaternion, dim=1)
    
#     # Position loss (MSE)
#     position_loss = torch.sum((pred_position - target_position) ** 2, dim=1)  # [B*N]
    
#     # Quaternion loss - use absolute dot product as quaternions q and -q represent same rotation
#     quat_dot = torch.sum(pred_quaternion * target_quaternion, dim=1).abs()  # [B*N]
#     quaternion_loss = 1.0 - quat_dot  # [B*N]
    
#     # Combine losses
#     if pose_loss_type == 'separate':
#         # Keep position and quaternion loss separate but weighted equally
#         combined_loss = position_loss + quaternion_loss  # [B*N]
#     elif pose_loss_type == 'weighted':
#         # Custom weights for position and quaternion components
#         pos_weight = 1.0
#         quat_weight = 0.5
#         combined_loss = pos_weight * position_loss + quat_weight * quaternion_loss  # [B*N]
#     else:  # 'combined'
#         combined_loss = position_loss + quaternion_loss  # [B*N]
    
#     # Reshape to [B, N] for applying selection weights
#     combined_loss = combined_loss.reshape(batch_size, num_views)
    
#     # Apply weights if provided
#     if weights is not None:
#         combined_loss = combined_loss * weights  # weighted by selection probability
#         return combined_loss.sum() / batch_size  # sum over views, mean over batch
#     else:
#         return combined_loss.mean()  # mean over all predictions

def compute_selection_entropy_loss(
    selection_weights: torch.Tensor,
    temperature: float = 1.0
) -> torch.Tensor:
    """
    Compute entropy regularization loss for selection weights.
    Encourages diversity in view selection.
    
    Args:
        selection_weights: Raw selection weights [B, N]
        temperature: Temperature for softmax
        
    Returns:
        Entropy loss (negated entropy, so minimizing loss increases entropy)
    """
    # Apply softmax with temperature
    probs = F.softmax(selection_weights / temperature, dim=1)
    
    # Compute entropy: -sum(p * log(p))
    eps = 1e-8  # Small epsilon to avoid log(0)
    entropy = -torch.sum(probs * torch.log(probs + eps), dim=1).mean()
    
    # Return negative entropy (we want to maximize entropy)
    return -entropy


def compute_diversity_loss(
    selected_poses: torch.Tensor,
    temperature: float = 1.0
) -> torch.Tensor:
    """
    Compute diversity loss for selected poses.
    Encourages diverse camera viewpoints.
    
    Args:
        selected_poses: Selected camera poses [B, K, 7]
        temperature: Temperature parameter
        
    Returns:
        Diversity loss
    """
    # Extract positions
    positions = selected_poses[:, :, :3]  # [B, K, 3]
    
    # Compute pairwise distances between positions
    B, K, _ = positions.shape
    
    # Reshape to [B, K, 1, 3] and [B, 1, K, 3]
    pos1 = positions.unsqueeze(2)  # [B, K, 1, 3]
    pos2 = positions.unsqueeze(1)  # [B, 1, K, 3]
    
    # Compute squared distances
    distances = torch.sum((pos1 - pos2) ** 2, dim=3)  # [B, K, K]
    
    # Create mask to exclude self distances
    mask = 1.0 - torch.eye(K, device=positions.device).unsqueeze(0)  # [1, K, K]
    
    # Apply mask and compute average negative distance (we want to maximize distances)
    masked_distances = distances * mask
    
    # Use softmin to focus on the closest pairs (most problematic for diversity)
    weights = F.softmax(-masked_distances / temperature, dim=2)
    weighted_distances = torch.sum(weights * masked_distances, dim=(1, 2))
    
    return weighted_distances.mean()


def calculate_selection_temperature(
    epoch: int,
    max_temp: float = 5.0,
    min_temp: float = 0.5,
    decay_factor: float = 0.9,
    total_epochs: int = 50
) -> float:
    """
    Calculate temperature for selection weights with annealing.
    
    Args:
        epoch: Current epoch
        max_temp: Maximum temperature
        min_temp: Minimum temperature
        decay_factor: Temperature decay factor
        total_epochs: Total number of training epochs
        
    Returns:
        Current temperature
    """
    # Linear annealing
    progress = min(1.0, epoch / (total_epochs * 0.8))
    temperature = max_temp - (max_temp - min_temp) * progress
    
    # Ensure temperature doesn't go below minimum
    return max(temperature, min_temp)


def setup_training(
    transformer_model: nn.Module,
    nerf_model: nn.Module,
    config: Any,
    resume_path: Optional[str] = None
) -> Tuple[
    torch.optim.Optimizer,
    Optional[torch.optim.lr_scheduler._LRScheduler],
    int,
    float
]:
    """
    Set up training components.
    
    Args:
        transformer_model: Transformer model
        nerf_model: NeRF model
        config: Training configuration
        resume_path: Path to resume from checkpoint
        
    Returns:
        Tuple of (optimizer, scheduler, start_epoch, best_metric)
    """
    # Create optimizer
    # Combine parameters from both models
    params = list(transformer_model.parameters()) + list(nerf_model.parameters())
    
    if hasattr(config, 'optimizer') and config.optimizer == 'adamw':
        optimizer = torch.optim.AdamW(
            params,
            lr=config.lr if hasattr(config, 'lr') else config.learning_rate,
            weight_decay=config.weight_decay if hasattr(config, 'weight_decay') else 1e-4
        )
    else:
        optimizer = torch.optim.Adam(
            params,
            lr=config.lr if hasattr(config, 'lr') else config.learning_rate,
            weight_decay=config.weight_decay if hasattr(config, 'weight_decay') else 0
        )
    
    # Create scheduler
    if hasattr(config, 'scheduler') and config.scheduler == 'cosine':
        scheduler = torch.optim.lr_scheduler.CosineAnnealingLR(
            optimizer,
            T_max=config.epochs if hasattr(config, 'epochs') else config.num_epochs,
            eta_min=config.lr_min if hasattr(config, 'lr_min') else (config.lr * 0.1 if hasattr(config, 'lr') else config.learning_rate * 0.1)
        )
    elif hasattr(config, 'scheduler') and config.scheduler == 'step':
        scheduler = torch.optim.lr_scheduler.StepLR(
            optimizer,
            step_size=config.lr_step_size if hasattr(config, 'lr_step_size') else 30,
            gamma=config.lr_gamma if hasattr(config, 'lr_gamma') else 0.1
        )
    else:
        scheduler = None
    
    # Initialize training state
    start_epoch = 0
    best_metric = float('inf')  # Lower is better for loss
    
    # Resume from checkpoint if specified
    if resume_path and os.path.exists(resume_path):
        print(f"Loading checkpoint from {resume_path}")
        checkpoint = torch.load(resume_path, map_location='cpu')
        
        # Load model weights
        transformer_model.load_state_dict(checkpoint['transformer_state_dict'])
        nerf_model.load_state_dict(checkpoint['nerf_state_dict'])
        
        # Load optimizer and scheduler states
        optimizer.load_state_dict(checkpoint['optimizer'])
        if scheduler is not None and 'scheduler' in checkpoint:
            scheduler.load_state_dict(checkpoint['scheduler'])
        
        # Get training state
        start_epoch = checkpoint['epoch'] + 1
        best_metric = checkpoint['best_metric']
        
        print(f"Resumed from epoch {start_epoch} with best metric {best_metric:.4f}")
    
    return optimizer, scheduler, start_epoch, best_metric


def create_experiment_dir(
    base_dir: str,
    experiment_name: Optional[str] = None
) -> str:
    """
    Create a directory for the experiment.
    
    Args:
        base_dir: Base directory for experiments
        experiment_name: Optional experiment name (uses timestamp if None)
        
    Returns:
        Path to the experiment directory
    """
    if experiment_name is None:
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        experiment_name = f"experiment_{timestamp}"
    
    experiment_dir = os.path.join(base_dir, experiment_name)
    os.makedirs(experiment_dir, exist_ok=True)
    
    # Create subdirectories
    os.makedirs(os.path.join(experiment_dir, "checkpoints"), exist_ok=True)
    os.makedirs(os.path.join(experiment_dir, "logs"), exist_ok=True)
    os.makedirs(os.path.join(experiment_dir, "visualizations"), exist_ok=True)
    
    print(f"Created experiment directory: {experiment_dir}")
    return experiment_dir


def setup_logging(
    experiment_dir: str,
    config: Any
) -> SummaryWriter:
    """
    Set up logging for the experiment.
    
    Args:
        experiment_dir: Path to the experiment directory
        config: Training configuration
        
    Returns:
        TensorBoard SummaryWriter
    """
    # Create TensorBoard writer
    log_dir = os.path.join(experiment_dir, "logs")
    writer = SummaryWriter(log_dir=log_dir)
    
    # Save configuration
    if config is not None:
        import json
        
        # Try to convert config to dict if it's a dataclass
        try:
            from dataclasses import asdict
            config_dict = asdict(config)
        except:
            # If not a dataclass, convert to dict if possible
            config_dict = config.__dict__ if hasattr(config, '__dict__') else {}
        
        # Save as JSON
        with open(os.path.join(experiment_dir, "config.json"), 'w') as f:
            json.dump(config_dict, f, indent=2)
    
    return writer


def get_selected_views(
    selection_weights: torch.Tensor,
    views: torch.Tensor,
    num_views: int,
    temperature: float = 1.0,
    training: bool = True
) -> Tuple[torch.Tensor, torch.Tensor]:
    """
    Get selected views based on selection weights.
    
    Args:
        selection_weights: Selection weights [B, N]
        views: Input views (tokens or images) [B, N, ...]
        num_views: Number of views to select
        temperature: Temperature for softmax
        training: Whether in training mode (soft selection) or not (hard selection)
        
    Returns:
        Tuple of (selected_views, selection_probabilities)
    """
    B, N = selection_weights.shape
    
    # Apply softmax with temperature
    selection_probs = F.softmax(selection_weights / temperature, dim=1)
    
    if training:
        # Soft selection (differentiable)
        # During training, we use the top-k weights but normalize them
        # This allows gradient flow through the selection process
        
        # Get top-k indices
        top_values, top_indices = torch.topk(selection_probs, num_views, dim=1)
        
        # Create normalized weights for the top-k views
        top_probs = top_values / top_values.sum(dim=1, keepdim=True)
        
        # Create batch indices for gather
        batch_indices = torch.arange(B, device=selection_weights.device).unsqueeze(1)
        batch_indices = batch_indices.expand(-1, num_views)
        
        # Gather top-k views
        selected_views = views[batch_indices, top_indices]
        
        # Create a tensor of selection probabilities (for analysis)
        selection_prob_tensor = torch.zeros_like(selection_probs)
        for b in range(B):
            selection_prob_tensor[b, top_indices[b]] = top_probs[b]
        
        return selected_views, selection_prob_tensor
        
    else:
        # Hard selection (during inference)
        # Simply take the top-k views without normalization
        _, top_indices = torch.topk(selection_probs, num_views, dim=1)
        
        # Create batch indices for gather
        batch_indices = torch.arange(B, device=selection_weights.device).unsqueeze(1)
        batch_indices = batch_indices.expand(-1, num_views)
        
        # Gather top-k views
        selected_views = views[batch_indices, top_indices]
        
        # Create a tensor of selection probabilities (one-hot for top-k)
        selection_prob_tensor = torch.zeros_like(selection_probs)
        for b in range(B):
            selection_prob_tensor[b, top_indices[b]] = 1.0 / num_views
        
        return selected_views, selection_prob_tensor


class SelectionMetrics:
    """Track and analyze view selection metrics during training"""
    
    def __init__(self, num_identities: int, num_views: int):
        """
        Initialize selection metrics tracker.
        
        Args:
            num_identities: Number of identities in the dataset
            num_views: Number of views per identity
        """
        self.num_identities = num_identities
        self.num_views = num_views
        
        # Initialize selection counters
        self.selection_counts = torch.zeros(num_identities, num_views)
        self.identity_indices = {}  # Map identity name to index
        
        # Performance metrics per view
        self.view_losses = torch.zeros(num_identities, num_views)
        self.view_counts = torch.zeros(num_identities, num_views)
    
    def update_selection(
        self,
        identity: str,
        view_indices: torch.Tensor,
        selection_probs: torch.Tensor
    ):
        """
        Update selection counts.
        
        Args:
            identity: Identity name
            view_indices: Indices of available views
            selection_probs: Selection probabilities for each view
        """
        # Get or create identity index
        if identity not in self.identity_indices:
            if len(self.identity_indices) >= self.num_identities:
                # If full, skip this update
                return
            self.identity_indices[identity] = len(self.identity_indices)
        
        identity_idx = self.identity_indices[identity]
        
        # Update selection counts for this identity
        for i, idx in enumerate(view_indices):
            if idx >= self.num_views:
                continue
            
            # Add selection probability to count
            self.selection_counts[identity_idx, idx] += selection_probs[i].item()
    
    def update_performance(
        self,
        identity: str,
        view_indices: torch.Tensor,
        view_losses: torch.Tensor
    ):
        """
        Update performance metrics for views.
        
        Args:
            identity: Identity name
            view_indices: Indices of views
            view_losses: Loss value for each view
        """
        # Skip if identity not registered
        if identity not in self.identity_indices:
            return
            
        identity_idx = self.identity_indices[identity]
        
        # Update losses for each view
        for i, idx in enumerate(view_indices):
            if idx >= self.num_views:
                continue
                
            self.view_losses[identity_idx, idx] += view_losses[i].item()
            self.view_counts[identity_idx, idx] += 1
    
    def get_most_selected_views(
        self,
        identity: Optional[str] = None,
        top_k: int = 5
    ) -> Tuple[torch.Tensor, torch.Tensor]:
        """
        Get the most frequently selected views.
        
        Args:
            identity: Optional identity to filter by
            top_k: Number of top views to return
            
        Returns:
            Tuple of (view_indices, selection_counts)
        """
        if identity is not None and identity in self.identity_indices:
            # Get for specific identity
            identity_idx = self.identity_indices[identity]
            counts = self.selection_counts[identity_idx]
        else:
            # Average across all identities
            counts = self.selection_counts.mean(dim=0)
        
        # Get top-k
        top_values, top_indices = torch.topk(counts, min(top_k, self.num_views))
        
        return top_indices, top_values
    
    def get_best_performing_views(
        self,
        identity: Optional[str] = None,
        top_k: int = 5
    ) -> Tuple[torch.Tensor, torch.Tensor]:
        """
        Get the best performing views (lowest loss).
        
        Args:
            identity: Optional identity to filter by
            top_k: Number of top views to return
            
        Returns:
            Tuple of (view_indices, average_losses)
        """
        if identity is not None and identity in self.identity_indices:
            # Get for specific identity
            identity_idx = self.identity_indices[identity]
            losses = self.view_losses[identity_idx]
            counts = self.view_counts[identity_idx]
        else:
            # Average across all identities
            losses = self.view_losses.sum(dim=0)
            counts = self.view_counts.sum(dim=0)
        
        # Calculate average loss
        avg_losses = torch.zeros_like(losses)
        mask = counts > 0
        avg_losses[mask] = losses[mask] / counts[mask]
        
        # Set unused views to infinity
        avg_losses[~mask] = float('inf')
        
        # Get top-k (lowest loss)
        negative_losses = -avg_losses  # Negate for top-k
        top_values, top_indices = torch.topk(
            negative_losses, 
            min(top_k, mask.sum().item())
        )
        
        return top_indices, -top_values  # Negate back to get actual losses
    
    def reset(self):
        """Reset all metrics."""
        self.selection_counts.zero_()
        self.view_losses.zero_()
        self.view_counts.zero_()


def write_metrics_to_tensorboard(
    writer: SummaryWriter,
    metrics: Dict[str, float],
    step: int,
    prefix: str = ''
):
    """
    Write metrics to TensorBoard.
    
    Args:
        writer: TensorBoard writer
        metrics: Dictionary of metrics
        step: Current step
        prefix: Optional prefix for metric names
    """
    for name, value in metrics.items():
        if prefix:
            name = f"{prefix}/{name}"
        writer.add_scalar(name, value, step)


def get_learning_rates(optimizer: torch.optim.Optimizer) -> Dict[str, float]:
    """
    Get current learning rates from optimizer.
    
    Args:
        optimizer: Optimizer
        
    Returns:
        Dictionary of learning rates
    """
    lrs = {}
    for i, param_group in enumerate(optimizer.param_groups):
        lrs[f'lr_group_{i}'] = param_group['lr']
    return lrs