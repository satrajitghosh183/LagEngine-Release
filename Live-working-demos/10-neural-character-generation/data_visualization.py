import os
import torch
import numpy as np
import matplotlib.pyplot as plt
from PIL import Image
import torch.nn.functional as F
from matplotlib.colors import LinearSegmentedColormap
from typing import Dict, List, Tuple, Optional, Union, Any
import math
from torchvision.transforms.functional import resize
import torch

def visualize_token_embeddings(
    tokens: torch.Tensor,
    selection_weights: Optional[torch.Tensor] = None,
    pca_components: int = 2,
    title: str = "Token Embeddings",
    save_path: Optional[str] = None
) -> plt.Figure:
    """
    Visualize token embeddings using PCA.
    
    Args:
        tokens: Token embeddings [N, D]
        selection_weights: Optional selection weights for coloring [N]
        pca_components: Number of PCA components (2 or 3)
        title: Plot title
        save_path: Path to save the figure
        
    Returns:
        Figure object
    """
    # Extract DINO embeddings from tokens
    tokens_np = tokens.detach().cpu().numpy()
    embeddings = tokens_np[:, :384]  # First 384 dimensions are DINO embeddings
    
    # Apply PCA
    from sklearn.decomposition import PCA
    pca = PCA(n_components=pca_components)
    embeddings_pca = pca.fit_transform(embeddings)
    
    # Create figure
    if pca_components == 3:
        fig = plt.figure(figsize=(10, 8))
        ax = fig.add_subplot(111, projection='3d')
    else:
        fig, ax = plt.subplots(figsize=(10, 8))
    
    # Color by selection weights if provided
    if selection_weights is not None:
        weights = F.softmax(selection_weights, dim=0).detach().cpu().numpy()
        cmap = plt.cm.viridis
        colors = cmap(weights)
        
        # Create scatter plot
        if pca_components == 3:
            scatter = ax.scatter(
                embeddings_pca[:, 0],
                embeddings_pca[:, 1],
                embeddings_pca[:, 2],
                c=weights,
                cmap=cmap,
                s=100,
                alpha=0.8
            )
        else:
            scatter = ax.scatter(
                embeddings_pca[:, 0],
                embeddings_pca[:, 1],
                c=weights,
                cmap=cmap,
                s=100,
                alpha=0.8
            )
            
        # Add colorbar
        cbar = plt.colorbar(scatter, ax=ax)
        cbar.set_label('Selection Weight')
    else:
        # Create scatter plot without weights
        if pca_components == 3:
            ax.scatter(
                embeddings_pca[:, 0],
                embeddings_pca[:, 1],
                embeddings_pca[:, 2],
                s=100,
                alpha=0.8
            )
        else:
            ax.scatter(
                embeddings_pca[:, 0],
                embeddings_pca[:, 1],
                s=100,
                alpha=0.8
            )
    
    # Add labels
    ax.set_xlabel(f'PC1 ({pca.explained_variance_ratio_[0]:.2%})')
    ax.set_ylabel(f'PC2 ({pca.explained_variance_ratio_[1]:.2%})')
    if pca_components == 3:
        ax.set_zlabel(f'PC3 ({pca.explained_variance_ratio_[2]:.2%})')
    
    # Add title and annotations
    plt.title(title)
    
    # Add index annotations
    for i in range(len(embeddings_pca)):
        if pca_components == 3:
            ax.text(
                embeddings_pca[i, 0],
                embeddings_pca[i, 1],
                embeddings_pca[i, 2],
                str(i),
                fontsize=8
            )
        else:
            ax.text(
                embeddings_pca[i, 0],
                embeddings_pca[i, 1],
                str(i),
                fontsize=8
            )
    
    plt.tight_layout()
    
    # Save figure if path provided
    if save_path:
        os.makedirs(os.path.dirname(save_path), exist_ok=True)
        plt.savefig(save_path, dpi=150)
    
    return fig


def visualize_pose_distribution(
    poses: torch.Tensor,
    selection_weights: Optional[torch.Tensor] = None,
    target_pose: Optional[torch.Tensor] = None,
    title: str = "Camera Pose Distribution",
    save_path: Optional[str] = None,
    figsize: Tuple[int, int] = (10, 8)
) -> plt.Figure:
    """
    Visualize camera pose distribution in 3D space.
    
    Args:
        poses: Camera poses [N, 7] (position + quaternion)
        selection_weights: Optional selection weights [N]
        target_pose: Optional target pose [7]
        title: Plot title
        save_path: Path to save the figure
        figsize: Figure size
        
    Returns:
        Figure object
    """
    # Extract positions from poses
    poses_np = poses.detach().cpu().numpy()
    positions = poses_np[:, :3]  # First 3 dimensions are XYZ positions
    
    # Create figure
    fig = plt.figure(figsize=figsize)
    ax = fig.add_subplot(111, projection='3d')
    
    # Color by selection weights if provided
    if selection_weights is not None:
        weights = F.softmax(selection_weights, dim=0).detach().cpu().numpy()
        cmap = LinearSegmentedColormap.from_list(
            'selection', 
            [(0, 'blue'), (0.5, 'purple'), (1, 'red')]
        )
        colors = cmap(weights)
        
        # Create scatter plot
        scatter = ax.scatter(
            positions[:, 0],
            positions[:, 1],
            positions[:, 2],
            c=weights,
            cmap=cmap,
            s=100,
            alpha=0.8
        )
        
        # Add colorbar
        cbar = plt.colorbar(scatter, ax=ax)
        cbar.set_label('Selection Weight')
    else:
        # Create scatter plot without weights
        ax.scatter(
            positions[:, 0],
            positions[:, 1],
            positions[:, 2],
            s=100,
            alpha=0.8
        )
    
    # Add target pose if provided
    if target_pose is not None:
        target_pos = target_pose.detach().cpu().numpy()[:3]
        ax.scatter(
            target_pos[0],
            target_pos[1],
            target_pos[2],
            color='green',
            s=200,
            alpha=1.0,
            marker='*',
            label='Target'
        )
        
        # Add legend
        ax.legend()
    
    # Add direction vectors (using quaternions)
    for i in range(len(poses_np)):
        pos = positions[i]
        quat = poses_np[i, 3:7]  # Quaternion components
        
        # Convert quaternion to forward direction vector (using simplified approach)
        # This is a basic approximation - a more accurate version would convert quat to rotation matrix
        w, x, y, z = quat
        forward_vec = np.array([
            2 * (x * z + w * y),
            2 * (y * z - w * x),
            1 - 2 * (x * x + y * y)
        ])
        forward_vec = forward_vec / np.linalg.norm(forward_vec)
        
        # Scale the direction vector
        scale = 0.5
        end_point = pos + scale * forward_vec
        
        # Draw direction arrow
        color = colors[i] if selection_weights is not None else 'blue'
        ax.plot(
            [pos[0], end_point[0]],
            [pos[1], end_point[1]],
            [pos[2], end_point[2]],
            color=color if isinstance(color, str) else color[:3],
            alpha=0.6,
            linewidth=2
        )
        
        # Add index text
        ax.text(
            pos[0],
            pos[1],
            pos[2],
            str(i),
            fontsize=8
        )
    
    # Set labels and title
    ax.set_xlabel('X')
    ax.set_ylabel('Y')
    ax.set_zlabel('Z')
    ax.set_title(title)
    
    # Make axes equal for better visualization
    max_range = np.array([
        ax.get_xlim(), ax.get_ylim(), ax.get_zlim()
    ]).T.reshape(-1, 3).max(axis=0) - np.array([
        ax.get_xlim(), ax.get_ylim(), ax.get_zlim()
    ]).T.reshape(-1, 3).min(axis=0)
    
    mid_x = np.mean(ax.get_xlim())
    mid_y = np.mean(ax.get_ylim())
    mid_z = np.mean(ax.get_zlim())
    
    ax.set_xlim(mid_x - max_range.max() / 2, mid_x + max_range.max() / 2)
    ax.set_ylim(mid_y - max_range.max() / 2, mid_y + max_range.max() / 2)
    ax.set_zlim(mid_z - max_range.max() / 2, mid_z + max_range.max() / 2)
    
    plt.tight_layout()
    
    # Save figure if path provided
    if save_path:
        os.makedirs(os.path.dirname(save_path), exist_ok=True)
        plt.savefig(save_path, dpi=150)
    
    return fig


def visualize_nerf_results(
    rendered_images: torch.Tensor,
    target_images: torch.Tensor,
    metrics: Optional[Dict[str, float]] = None,
    title: str = "NeRF Reconstruction Results",
    save_path: Optional[str] = None
) -> plt.Figure:
    """
    Visualize NeRF reconstruction results.
    
    Args:
        rendered_images: Rendered images [B, C, H, W]
        target_images: Target images [B, C, H, W]
        metrics: Optional dictionary of metrics to display
        title: Plot title
        save_path: Path to save the figure
        
    Returns:
        Figure object
    """
    # Ensure tensors are on CPU
    rendered_images = rendered_images.detach().cpu()
    target_images = target_images.detach().cpu()
    
    # Get batch size
    batch_size = rendered_images.shape[0]
    
    # Create figure
    fig, axes = plt.subplots(batch_size, 2, figsize=(10, 5 * batch_size))
    
    # Handle single batch case
    if batch_size == 1:
        axes = axes.reshape(1, 2)
    
    # Metrics string
    metrics_str = ""
    if metrics:
        for name, value in metrics.items():
            metrics_str += f"{name}: {value:.4f} | "
        metrics_str = metrics_str[:-3]  # Remove trailing separator
    
    # Add title
    full_title = title
    if metrics_str:
        full_title += f"\n{metrics_str}"
    fig.suptitle(full_title, fontsize=16)
    
    # Plot each image pair
    # for b in range(batch_size):
    #     # Convert tensors to numpy
    #     rendered = rendered_images[b].permute(1, 2, 0).numpy()
    #     target = target_images[b].permute(1, 2, 0).numpy()
        
    #     # Clip to valid range
    #     rendered = np.clip(rendered, 0, 1)
    #     target = np.clip(target, 0, 1)
        
    #     # Plot target image
    #     axes[b, 0].imshow(target)
    #     axes[b, 0].set_title(f"Target Image #{b}")
    #     axes[b, 0].axis('off')
    for b in range(batch_size):
        # ---- Target Image ----
        target = target_images[b]
        if target.shape[-1] != 3:
            # Handle channels-first format [3, H, W] → [H, W, 3]
            target = np.transpose(target, (1, 2, 0)) if target.shape[0] == 3 else target
        axes[b, 0].imshow(np.clip(target, 0, 1))
        axes[b, 0].set_title("Target Image")
        axes[b, 0].axis('off')

        # ---- Rendered Image ----
        rendered = rendered_images[b]
        if rendered.shape[-1] != 3:
            # Handle channels-first format [3, H, W] or [H, 3, W]
            if rendered.shape[0] == 3:
                rendered = np.transpose(rendered, (1, 2, 0))  # [3, H, W] → [H, W, 3]
            elif rendered.shape[1] == 3:
                rendered = np.transpose(rendered, (0, 2, 1))  # [H, 3, W] → [H, W, 3]
        axes[b, 1].imshow(np.clip(rendered, 0, 1))
        axes[b, 1].set_title("Rendered Image")
        axes[b, 1].axis('off')

        
        # Plot rendered image
        # axes[b, 1].imshow(rendered)
        # axes[b, 1].set_title(f"Rendered Image #{b}")
        rendered = rendered_images[b]
        if rendered.shape[-1] != 3:
                # Transpose if channels are not in the last dim
            rendered = np.transpose(rendered, (1, 2, 0)) if rendered.shape[1] == 3 else np.transpose(rendered, (0, 2, 1))

            axes[b, 1].imshow(np.clip(rendered, 0, 1))

        axes[b, 1].axis('off')
        if rendered.shape[:2] != target.shape[:2]:
    # Convert to tensor if needed
            if not torch.is_tensor(target):
                target = torch.from_numpy(target)

            if not torch.is_tensor(rendered):
                rendered = torch.from_numpy(rendered)

            # Transpose to CHW format for resize, then back to HWC
            target_resized = resize(target.permute(2, 0, 1), size=rendered.shape[:2], antialias=True).permute(1, 2, 0)
            target = target_resized.numpy()
        # Calculate PSNR and add to title
        mse = ((rendered - target) ** 2).mean()
        psnr = -10 * math.log10(mse) if mse > 0 else 100
        axes[b, 1].set_title(f"Rendered Image #{b}\nPSNR: {psnr:.2f} dB")
    
    plt.tight_layout()
    
    # Save figure if path provided
    if save_path:
        os.makedirs(os.path.dirname(save_path), exist_ok=True)
        plt.savefig(save_path, dpi=150)
    
    return fig
def safe_log_image(tag, img_tensor, writer, global_step=None):
    """
    Safely log an image tensor to TensorBoard with proper shape handling.
    """
    if torch.is_tensor(img_tensor):
        img_tensor = img_tensor.detach().cpu()
    
    # If batch dimension exists
    if img_tensor.ndim == 4:
        img_tensor = img_tensor[0]

    # Convert [H, W, 3] to [3, H, W]
    if img_tensor.shape[-1] == 3:
        img_tensor = img_tensor.permute(2, 0, 1)

    img_tensor = torch.clamp(img_tensor, 0, 1)

    writer.add_image(tag, img_tensor, global_step=global_step)


def visualize_selection_weights(
    selection_weights: torch.Tensor,
    timestamps: Optional[torch.Tensor] = None,
    title: str = "View Selection Weights",
    save_path: Optional[str] = None
) -> plt.Figure:
    """
    Visualize view selection weights, optionally over time.
    
    Args:
        selection_weights: Selection weights [N]
        timestamps: Optional timestamps for each view [N]
        title: Plot title
        save_path: Path to save the figure
        
    Returns:
        Figure object
    """
    # Ensure tensors are on CPU
    weights = F.softmax(selection_weights, dim=0).detach().cpu().numpy()
    
    # Create figure
    fig, ax = plt.subplots(figsize=(10, 6))
    
    # Plot weights
    if timestamps is not None:
        # Sort by timestamps
        timestamps_np = timestamps.detach().cpu().numpy()
        sorted_indices = np.argsort(timestamps_np)
        sorted_weights = weights[sorted_indices]
        sorted_timestamps = timestamps_np[sorted_indices]
        
        # Plot weights over time
        ax.plot(sorted_timestamps, sorted_weights, marker='o', linestyle='-', linewidth=2)
        ax.set_xlabel('Timestamp')
    else:
        # Just plot weights as a bar chart
        indices = np.arange(len(weights))
        ax.bar(indices, weights, alpha=0.7)
        ax.set_xlabel('View Index')
        
        # Add text annotations with weight values
        for i, w in enumerate(weights):
            ax.text(i, w + 0.01, f"{w:.3f}", ha='center', va='bottom', fontsize=8)
    
    # Set labels and title
    ax.set_ylabel('Selection Weight')
    ax.set_title(title)
    
    # Highlight top-k views
    top_k = 5  # Number of views to highlight
    top_indices = np.argsort(weights)[-top_k:]
    top_weights = weights[top_indices]
    
    # Plot top-k as red markers
    if timestamps is not None and len(top_indices) > 0:
        top_timestamps = timestamps_np[top_indices]
        ax.scatter(top_timestamps, top_weights, color='red', s=100, zorder=10, label=f'Top {top_k}')
        ax.legend()
    else:
        for idx in top_indices:
            ax.patches[idx].set_facecolor('red')
        
        # Add "Top-K" text in legend
        import matplotlib.patches as mpatches
        red_patch = mpatches.Patch(color='red', label=f'Top {top_k}')
        ax.legend(handles=[red_patch])
    
    plt.tight_layout()
    
    # Save figure if path provided
    if save_path:
        os.makedirs(os.path.dirname(save_path), exist_ok=True)
        plt.savefig(save_path, dpi=150)
    
    return fig


def visualize_image_grid(
    images: Union[torch.Tensor, List[Image.Image]],
    selection_weights: Optional[torch.Tensor] = None,
    indices: Optional[List[int]] = None,
    ncols: int = 4,
    title: str = "Image Grid",
    save_path: Optional[str] = None
) -> plt.Figure:
    """
    Visualize a grid of images with optional selection weights.
    
    Args:
        images: List of PIL images or tensor of shape [N, C, H, W] or [N, H, W, C]
        selection_weights: Optional selection weights [N]
        indices: Optional list of indices to display
        ncols: Number of columns in the grid
        title: Plot title
        save_path: Path to save the figure
        
    Returns:
        Figure object
    """
    # Convert tensor to list of PIL images
    if isinstance(images, torch.Tensor):
        # Ensure images are on CPU
        images_np = images.detach().cpu().numpy()
        
        # Handle different tensor formats
        if images.shape[1] == 3 and len(images.shape) == 4:
            # [N, C, H, W] -> [N, H, W, C]
            images_np = np.transpose(images_np, (0, 2, 3, 1))
            
        # Convert to list of PIL images
        images_pil = []
        for img in images_np:
            # Scale to 0-255 and convert to uint8
            img = (img * 255).clip(0, 255).astype(np.uint8)
            images_pil.append(Image.fromarray(img))
        
        images = images_pil
    
    # Apply indices filter if provided
    # if indices is not None:
    #     images = [images[i] for i in indices]
    #     if selection_weights is not None:
    #         selection_weights = selection_weights[indices]
    if indices is not None:
        images = [images[i] for i in indices]
        if selection_weights is not None:
            selection_weights = selection_weights[torch.tensor(indices, dtype=torch.long, device=selection_weights.device)]

    
    # Calculate layout
    n_images = len(images)
    nrows = (n_images + ncols - 1) // ncols
    
    # Create figure
    fig, axes = plt.subplots(nrows, ncols, figsize=(ncols * 3, nrows * 3))
    
    # Flatten axes for easier indexing
    if nrows == 1 and ncols == 1:
        axes = np.array([axes])
    axes = axes.flatten()
    
    # Create a colormap for selection weights if provided
    if selection_weights is not None:
        weights = F.softmax(selection_weights, dim=0).detach().cpu().numpy()
        cmap = LinearSegmentedColormap.from_list(
            'selection', 
            [(0, 'blue'), (0.5, 'purple'), (1, 'red')]
        )
    
    # Plot each image
    for i in range(n_images):
        ax = axes[i]
        ax.imshow(images[i])
        
        # Add border based on selection weight
        if selection_weights is not None:
            weight = weights[i]
            color = cmap(weight)
            
            # Add colored border
            for spine in ax.spines.values():
                spine.set_edgecolor(color)
                spine.set_linewidth(3)
            
            # Add weight as text
            ax.set_title(f"Weight: {weight:.4f}", color=color)
        
        # Remove ticks
        ax.set_xticks([])
        ax.set_yticks([])
        
        # Add index in corner
        ax.text(
            0.05, 0.95, f"#{i}", 
            transform=ax.transAxes,
            fontsize=12, fontweight='bold',
            verticalalignment='top', 
            bbox=dict(facecolor='white', alpha=0.7)
        )
    
    # Hide unused axes
    for i in range(n_images, len(axes)):
        axes[i].axis('off')
    
    # Add overall title
    fig.suptitle(title, fontsize=16)
    plt.tight_layout()
    
    # Save figure if path provided
    if save_path:
        os.makedirs(os.path.dirname(save_path), exist_ok=True)
        plt.savefig(save_path, dpi=150)
    
    return fig