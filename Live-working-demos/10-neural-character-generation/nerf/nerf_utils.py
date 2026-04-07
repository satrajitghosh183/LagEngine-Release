import torch
import torch.nn.functional as F
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import LinearSegmentedColormap
import os
from PIL import Image
import torchvision.transforms as transforms


def create_camera_frustum(pose, scale=0.1, focal_length=1.0):
    """
    Create a camera frustum visualization for a given pose.
    
    Args:
        pose (torch.Tensor): Camera pose tensor [7] (position + quaternion)
        scale (float): Scale factor for frustum size
        focal_length (float): Focal length for frustum shape
        
    Returns:
        tuple: (vertices, faces) for the camera frustum
    """
    # Extract position and create rotation matrix from quaternion
    position = pose[:3].cpu().numpy()
    quaternion = pose[3:].cpu().numpy()
    rotation_matrix = quaternion_to_rotation_matrix_numpy(quaternion)
    
    # Create camera frustum vertices in camera space
    vertices = np.array([
        [0, 0, 0],  # Camera center
        [-scale, -scale, scale * focal_length],  # Bottom-left
        [scale, -scale, scale * focal_length],   # Bottom-right
        [scale, scale, scale * focal_length],    # Top-right
        [-scale, scale, scale * focal_length]    # Top-left
    ])
    
    # Transform vertices to world space
    vertices_world = []
    for v in vertices:
        # Rotate and translate
        v_rotated = rotation_matrix.dot(v)
        v_world = v_rotated + position
        vertices_world.append(v_world)
    
    # Define faces (for visualization)
    faces = [
        [0, 1, 2],  # Bottom triangle
        [0, 2, 3],  # Right triangle
        [0, 3, 4],  # Top triangle
        [0, 4, 1],  # Left triangle
        [1, 2, 3, 4]  # Back face (quad)
    ]
    
    return np.array(vertices_world), faces


def visualize_camera_poses(poses, selection_weights=None, title="Camera Poses", figsize=(10, 10)):
    """
    Visualize camera poses and optionally their selection weights.
    
    Args:
        poses (torch.Tensor): Camera poses [N, 7]
        selection_weights (torch.Tensor, optional): Selection weights [N]
        title (str): Plot title
        figsize (tuple): Figure size
        
    Returns:
        matplotlib.figure.Figure: The figure with camera poses visualization
    """
    fig = plt.figure(figsize=figsize)
    ax = fig.add_subplot(111, projection='3d')
    
    # Create colormap for selection weights if provided
    if selection_weights is not None:
        # Normalize weights to [0, 1]
        weights = F.softmax(selection_weights, dim=0).cpu().numpy()
        
        # Create a custom colormap from blue to red
        cmap = LinearSegmentedColormap.from_list(
            'blue_to_red', 
            [(0, 'blue'), (0.5, 'purple'), (1, 'red')]
        )
        
        # Get colors for each camera
        colors = cmap(weights)
    else:
        # Default color
        colors = ['blue'] * len(poses)
    
    # Plot camera positions and frustums
    for i, pose in enumerate(poses):
        position = pose[:3].cpu().numpy()
        
        # Plot camera position
        ax.scatter(
            position[0], position[1], position[2], 
            color=colors[i] if isinstance(colors, list) else colors[i,:3],
            s=50
        )
        
        # Add camera ID
        ax.text(position[0], position[1], position[2], str(i), fontsize=8)
        
        # Create and plot camera frustum
        vertices, faces = create_camera_frustum(pose)
        
        # Plot frustum edges
        for face in faces:
            if len(face) == 3:  # Triangle
                for i1, i2 in [(0, 1), (1, 2), (2, 0)]:
                    ax.plot(
                        [vertices[face[i1]][0], vertices[face[i2]][0]],
                        [vertices[face[i1]][1], vertices[face[i2]][1]],
                        [vertices[face[i1]][2], vertices[face[i2]][2]],
                        color=colors[i] if isinstance(colors, list) else colors[i,:3],
                        alpha=0.7
                    )
            elif len(face) == 4:  # Quad
                for i1, i2 in [(0, 1), (1, 2), (2, 3), (3, 0)]:
                    ax.plot(
                        [vertices[face[i1]][0], vertices[face[i2]][0]],
                        [vertices[face[i1]][1], vertices[face[i2]][1]],
                        [vertices[face[i1]][2], vertices[face[i2]][2]],
                        color=colors[i] if isinstance(colors, list) else colors[i,:3],
                        alpha=0.7
                    )
    
    # Add colorbar if selection weights are provided
    if selection_weights is not None:
        sm = plt.cm.ScalarMappable(cmap=cmap)
        sm.set_array(weights)
        cbar = plt.colorbar(sm, ax=ax)
        cbar.set_label('Selection Weight')
    
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
    return fig


def save_view_selection(selection_weights, image_paths, output_path, top_k=5):
    """
    Save visualization of view selection results.
    
    Args:
        selection_weights (torch.Tensor): View selection scores [N]
        image_paths (list): List of image file paths
        output_path (str): Output directory path
        top_k (int): Number of top views to highlight
        
    Returns:
        str: Path to saved visualization
    """
    # Create output directory if it doesn't exist
    os.makedirs(output_path, exist_ok=True)
    
    # Convert to probabilities
    selection_probs = F.softmax(selection_weights, dim=0).cpu().numpy()
    
    # Get top-k indices
    top_k_values, top_k_indices = torch.topk(
        torch.tensor(selection_probs), k=min(top_k, len(selection_probs))
    )
    top_k_indices = top_k_indices.numpy()
    
    # Create grid of images
    num_images = len(image_paths)
    grid_size = int(np.ceil(np.sqrt(num_images)))
    
    # Create figure
    fig, axes = plt.subplots(grid_size, grid_size, figsize=(20, 20))
    fig.suptitle("View Selection Results", fontsize=24)
    
    # Flatten axes for easier indexing
    axes = axes.flatten()
    
    # Process each image
    for i, img_path in enumerate(image_paths):
        if i >= len(axes):
            break
            
        # Load and display image
        img = Image.open(img_path).convert('RGB')
        axes[i].imshow(img)
        
        # Determine border color based on selection
        is_selected = i in top_k_indices
        border_color = 'red' if is_selected else 'black'
        border_width = 5 if is_selected else 1
        
        # Add border
        for spine in axes[i].spines.values():
            spine.set_edgecolor(border_color)
            spine.set_linewidth(border_width)
        
        # Determine rank if selected
        rank = "N/A"
        if is_selected:
            rank = f"Rank: {np.where(top_k_indices == i)[0][0] + 1}"
        
        # Add weight and selection info
        axes[i].set_title(
            f"Image {i}\nWeight: {selection_probs[i]:.4f}\n{rank}",
            color=border_color,
            fontweight='bold' if is_selected else 'normal'
        )
        
        # Remove axis ticks
        axes[i].set_xticks([])
        axes[i].set_yticks([])
    
    # Hide unused axes
    for i in range(len(image_paths), len(axes)):
        axes[i].axis('off')
    
    # Save and close figure
    plt.tight_layout()
    save_path = os.path.join(output_path, "view_selection.png")
    plt.savefig(save_path, dpi=100)
    plt.close(fig)
    
    return save_path


def visualize_nerf_results(nerf_output, target_image=None, save_path=None):
    """
    Visualize NeRF rendering results compared to the target image.
    
    Args:
        nerf_output (dict): Output from TinyNeRF or WeightedTinyNeRF
        target_image (torch.Tensor, optional): Target image [H, W, 3]
        save_path (str, optional): Path to save visualization
        
    Returns:
        matplotlib.figure.Figure: Visualization figure
    """
    # Extract rendered image and depth
    rgb = nerf_output['rgb'].detach().cpu().numpy()
    depth = nerf_output['depth'].detach().cpu().numpy()
    
    # Create figure based on available data
    if target_image is not None:
        # Comparison visualization (side-by-side)
        target = target_image.detach().cpu().numpy()
        
        fig, axes = plt.subplots(1, 3, figsize=(18, 6))
        
        # Plot target image
        axes[0].imshow(target)
        axes[0].set_title("Target Image")
        axes[0].axis('off')
        
        # Plot rendered image
        axes[1].imshow(np.clip(rgb, 0, 1))
        axes[1].set_title("Rendered Image")
        axes[1].axis('off')
        
        # Plot depth map
        depth_vis = axes[2].imshow(depth, cmap='viridis')
        axes[2].set_title("Depth Map")
        axes[2].axis('off')
        fig.colorbar(depth_vis, ax=axes[2], shrink=0.6)
        
        # Add PSNR if available
        if 'psnr' in nerf_output:
            psnr = nerf_output['psnr'].item()
            fig.suptitle(f"NeRF Results - PSNR: {psnr:.2f} dB", fontsize=16)
        else:
            fig.suptitle("NeRF Results", fontsize=16)
    else:
        # Only rendering visualization
        fig, axes = plt.subplots(1, 2, figsize=(12, 6))
        
        # Plot rendered image
        axes[0].imshow(np.clip(rgb, 0, 1))
        axes[0].set_title("Rendered Image")
        axes[0].axis('off')
        
        # Plot depth map
        depth_vis = axes[1].imshow(depth, cmap='viridis')
        axes[1].set_title("Depth Map")
        axes[1].axis('off')
        fig.colorbar(depth_vis, ax=axes[1], shrink=0.6)
        
        fig.suptitle("NeRF Rendering Results", fontsize=16)
    
    plt.tight_layout()
    
    # Save if path is provided
    if save_path is not None:
        os.makedirs(os.path.dirname(save_path), exist_ok=True)
        plt.savefig(save_path, dpi=150)
    
    return fig


def quaternion_to_rotation_matrix_numpy(q):
    """
    Convert quaternion to rotation matrix (numpy version).
    
    Args:
        q (numpy.ndarray): Quaternion [4] (qx, qy, qz, qw)
        
    Returns:
        numpy.ndarray: Rotation matrix [3, 3]
    """
    # Normalize quaternion
    q = q / np.linalg.norm(q)
    qx, qy, qz, qw = q
    
    # Compute rotation matrix elements
    R00 = 1 - 2 * (qy**2 + qz**2)
    R01 = 2 * (qx * qy - qw * qz)
    R02 = 2 * (qx * qz + qw * qy)
    
    R10 = 2 * (qx * qy + qw * qz)
    R11 = 1 - 2 * (qx**2 + qz**2)
    R12 = 2 * (qy * qz - qw * qx)
    
    R20 = 2 * (qx * qz - qw * qy)
    R21 = 2 * (qy * qz + qw * qx)
    R22 = 1 - 2 * (qx**2 + qy**2)
    
    return np.array([
        [R00, R01, R02],
        [R10, R11, R12],
        [R20, R21, R22]
    ])