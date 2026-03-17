#!/usr/bin/env python3
"""
Joint training script for the MTCM-NeRF pipeline.

This script implements the end-to-end training of:
1. Transformer model for view selection and pose regression
2. Neural Radiance Field (NeRF) model for novel view synthesis
3. Joint optimization with NeRF reconstruction loss supervising the Transformer

Author: Your Name
Date: 2025
"""

import json
import os
import sys
import argparse
import torch
import torch.nn as nn
import torch.nn.functional as F
import numpy as np
import time
from pathlib import Path
from typing import Dict, List, Tuple, Optional, Union, Any
import contextlib
import matplotlib.pyplot as plt
from torch.utils.tensorboard import SummaryWriter

from torch.amp import GradScaler, autocast
# Import Transformer model
from mtcm_mae.model import MTCM_MAE
from mtcm_mae.config import MTCMConfig

# Import NeRF model
from nerf.tiny_nerf import TinyNeRF
from nerf.weighted_tiny_nerf import WeightedTinyNeRF
from nerf.nerf_config import (
    NeRFConfig, 
    WeightedNeRFConfig, 
    JointTrainingConfig,
    create_nerf_config,
    create_weighted_nerf_config,
    create_joint_training_config
)

# Import dataset and utilities
from dataset_joint_mtcm_nerf import JointMTCMNeRFDataset, ViewSelectionDataModule
from data_visualization import (
    visualize_token_embeddings,
    visualize_image_grid,
    visualize_pose_distribution,
    visualize_nerf_results,
    visualize_selection_weights
)
from training_utils import (
    AverageMeter,
    ProgressMeter,
    save_checkpoint,
    adjust_learning_rate,
    compute_pose_loss,
    compute_selection_entropy_loss,
    compute_diversity_loss,
    setup_training,
    create_experiment_dir,
    setup_logging,
    get_selected_views,
    calculate_selection_temperature,
    SelectionMetrics,
    write_metrics_to_tensorboard,
    get_learning_rates
)
def safe_log_image(tag, img_tensor, writer, global_step=None):
    """
    Safely logs an image tensor to TensorBoard with proper shape handling.
    """
    if torch.is_tensor(img_tensor):
        img_tensor = img_tensor.detach().cpu()

    if img_tensor.ndim == 4 and img_tensor.shape[0] == 1:
        img_tensor = img_tensor[0]  # [H, W, C]

    if img_tensor.ndim == 3 and img_tensor.shape[-1] == 3:
        img_tensor = img_tensor.permute(2, 0, 1)  # [3, H, W]

    img_tensor = torch.clamp(img_tensor.float(), 0, 1)

    if img_tensor.shape[0] != 3:
        print(f"❌ Skipping log: unexpected shape for {tag}: {img_tensor.shape}")
        return

    writer.add_image(tag, img_tensor, global_step=global_step)
def visualize_epoch(transformer_model, nerf_model, val_loader, device, epoch, writer, vis_dir, temperature, config):
    transformer_model.eval()
    nerf_model.eval()

    epoch_vis_dir = os.path.join(vis_dir, f"epoch_{epoch:04d}")
    os.makedirs(epoch_vis_dir, exist_ok=True)

    with torch.no_grad():
        batch = next(iter(val_loader))
        input_tokens = batch["input_tokens"].to(device)
        target_tokens = batch["target_tokens"].to(device)
        input_indices = batch["input_indices"].to(device)
        input_images = batch["input_images"].to(device)
        target_images = batch["target_images"].to(device)

        for b in range(min(3, input_tokens.size(0))):
            identity = batch["identity"][b]
            identity_dir = os.path.join(epoch_vis_dir, identity)
            os.makedirs(identity_dir, exist_ok=True)

            tokens_b = input_tokens[b:b+1]
            target_b = target_tokens[b:b+1]
            images_b = input_images[b:b+1]
            target_img_b = target_images[b:b+1, 0]  # [1, H, W, C]

            transformer_outputs = transformer_model(tokens_b)
            selection_weights = transformer_outputs["selection_weights"][0]
            pose_predictions = transformer_outputs["pose_predictions"][0]
            target_pose = target_b[0, 0, 384:391]

            # Visuals
            fig = visualize_token_embeddings(tokens_b[0], selection_weights, 2, f"{identity} - Token Embeddings", os.path.join(identity_dir, "token_embeddings_pca.png"))
            plt.close(fig)
            fig = visualize_token_embeddings(tokens_b[0], selection_weights, 3, f"{identity} - Token Embeddings 3D", os.path.join(identity_dir, "token_embeddings_pca_3d.png"))
            plt.close(fig)
            fig = visualize_image_grid(
    images=images_b[0],
    selection_weights=selection_weights,
    indices=list(range(images_b[0].shape[0])),  # Add this to specify valid indices
    title=f"{identity} - Input Images (Epoch {epoch})",
    save_path=os.path.join(identity_dir, "input_images.png")
)

            fig = visualize_image_grid(target_images[b, 0], title=f"{identity} - Target Image", save_path=os.path.join(identity_dir, "target_image.png"))
            plt.close(fig)
            fig = visualize_pose_distribution(pose_predictions.view(-1, 7), selection_weights, target_pose, f"{identity} - Pose Distribution", os.path.join(identity_dir, "pose_distribution.png"))
            plt.close(fig)
            fig = visualize_selection_weights(
    selection_weights=selection_weights,
    title=f"{identity} - Selection Weights",
    save_path=os.path.join(identity_dir, "selection_weights.png")
)


            # NeRF rendering
            nerf_outputs = nerf_model(
                selection_weights=selection_weights.unsqueeze(0),
                images=images_b,
                poses=pose_predictions.view(1, -1, 7),
                target_pose=target_pose.unsqueeze(0),
                target_image=target_img_b,
                num_rays=config.nerf_ray_batch_size
            )

            if "rgb" in nerf_outputs:
                rendered_rgb = nerf_outputs["rgb"]  # [1, N, 3]
                num_rays = rendered_rgb.shape[1]
                batch_size = rendered_rgb.shape[0]

                if num_rays == config.image_height * config.image_width:
                    rendered_rgb = rendered_rgb.view(batch_size, config.image_height, config.image_width, 3)
                else:
                    grid_size = int(np.sqrt(num_rays))
                    if grid_size * grid_size != num_rays:
                        print(f"⚠️ Warning: {num_rays} rays not square. Truncating to {grid_size * grid_size}")
                    rendered_rgb = rendered_rgb[:, :grid_size * grid_size].view(batch_size, grid_size, grid_size, 3)

                # Save visualization
                fig = visualize_nerf_results(
                    rendered_images=rendered_rgb,
                    target_images=target_img_b,
                    metrics={"PSNR": nerf_outputs.get("psnr", torch.tensor(0.0)).item()},
                    title=f"{identity} - NeRF Results (Epoch {epoch})",
                    save_path=os.path.join(identity_dir, "nerf_results.png")
                )
                plt.close(fig)

                # TensorBoard logs
                safe_log_image(f"{identity}/target_image", target_img_b, writer, epoch)
                safe_log_image(f"{identity}/rendered_image", rendered_rgb, writer, epoch)

                if target_img_b.shape[1:] == rendered_rgb.shape[1:]:
                    comparison = torch.cat([target_img_b[0], rendered_rgb[0]], dim=1)  # [H, W*2, 3]
                    safe_log_image(f"{identity}/comparison", comparison, writer, epoch)

            # Log input grid
            safe_log_image(
                f"{identity}/input_images",
                torch.cat([img for img in images_b[0][:min(9, images_b[0].size(0))]], dim=1),
                writer, epoch
            )

            writer.add_histogram(f"{identity}/selection_weights", F.softmax(selection_weights, dim=0), epoch)


# def visualize_epoch(
#     transformer_model: nn.Module,
#     nerf_model: nn.Module,
#     val_loader: torch.utils.data.DataLoader,
#     device: torch.device,
#     epoch: int,
#     writer: SummaryWriter,
#     vis_dir: str,
#     temperature: float,
#     config: JointTrainingConfig
# ):
#     """
#     Create visualizations for the current epoch.
    
#     Args:
#         transformer_model: Transformer model
#         nerf_model: NeRF model
#         val_loader: Validation data loader
#         device: Device to use
#         epoch: Current epoch
#         writer: TensorBoard writer
#         vis_dir: Visualization directory
#         temperature: Temperature for selection softmax
#         config: Training configuration
#     """
#     # Set models to evaluation mode
#     transformer_model.eval()
#     nerf_model.eval()
    
#     # Create directory for this epoch
#     epoch_vis_dir = os.path.join(vis_dir, f"epoch_{epoch:04d}")
#     os.makedirs(epoch_vis_dir, exist_ok=True)
    
#     with torch.no_grad():
#         # Get a single batch
#         batch = next(iter(val_loader))
        
#         # Move data to device
#         input_tokens = batch["input_tokens"].to(device)
#         target_tokens = batch["target_tokens"].to(device)
#         input_indices = batch["input_indices"].to(device)
        
#         if "input_images" in batch and "target_images" in batch:
#             input_images = batch["input_images"].to(device)
#             target_images = batch["target_images"].to(device)
#         else:
#             raise ValueError("Input and target images are required for visualization")
        
#         # Process each item in the batch
#         for b in range(min(3, input_tokens.size(0))):
#             # Get identity
#             identity = batch["identity"][b]
            
#             # Create directory for this identity
#             identity_dir = os.path.join(epoch_vis_dir, identity)
#             os.makedirs(identity_dir, exist_ok=True)
            
#             # Extract single-item batch
#             tokens_b = input_tokens[b:b+1]
#             target_b = target_tokens[b:b+1]
#             images_b = input_images[b:b+1]
#             target_img_b = target_images[b:b+1, 0]  # First target image
            
#             # Forward pass through transformer
#             transformer_outputs = transformer_model(tokens_b)
            
#             # Get selection weights and pose predictions
#             selection_weights = transformer_outputs["selection_weights"][0]
#             pose_predictions = transformer_outputs["pose_predictions"][0]
            
#             # Extract target pose
#             target_pose = target_b[0, 0, 384:391]  # Position + Quaternion
            
#             # Visualize tokens embeddings
#             fig = visualize_token_embeddings(
#                 tokens=tokens_b[0],
#                 selection_weights=selection_weights,
#                 pca_components=2,
#                 title=f"{identity} - Token Embeddings (Epoch {epoch})",
#                 save_path=os.path.join(identity_dir, "token_embeddings_pca.png")
#             )
#             plt.close(fig)
            
#             # Visualize 3D PCA as well
#             fig = visualize_token_embeddings(
#                 tokens=tokens_b[0],
#                 selection_weights=selection_weights,
#                 pca_components=3,
#                 title=f"{identity} - Token Embeddings 3D (Epoch {epoch})",
#                 save_path=os.path.join(identity_dir, "token_embeddings_pca_3d.png")
#             )
#             plt.close(fig)
            
#             # Visualize input images and selection weights
#             fig = visualize_image_grid(
#                 images=images_b[0],
#                 selection_weights=selection_weights,
#                 title=f"{identity} - Input Images (Epoch {epoch})",
#                 save_path=os.path.join(identity_dir, "input_images.png")
#             )
#             plt.close(fig)
            
#             # Visualize target image
#             fig = visualize_image_grid(
#                 images=target_images[b, 0],
#                 title=f"{identity} - Target Image (Epoch {epoch})",
#                 save_path=os.path.join(identity_dir, "target_image.png")
#             )
#             plt.close(fig)
            
#             # Visualize pose distribution
#             fig = visualize_pose_distribution(
#                 poses=pose_predictions.view(-1, 7),
#                 selection_weights=selection_weights,
#                 target_pose=target_pose,
#                 title=f"{identity} - Pose Distribution (Epoch {epoch})",
#                 save_path=os.path.join(identity_dir, "pose_distribution.png")
#             )
#             plt.close(fig)
            
#             # Visualize selection weights
#             fig = visualize_selection_weights(
#                 selection_weights=selection_weights,
#                 title=f"{identity} - Selection Weights (Epoch {epoch})",
#                 save_path=os.path.join(identity_dir, "selection_weights.png")
#             )
#             plt.close(fig)
            
#             # Forward pass through NeRF for this sample
#             nerf_outputs = nerf_model(
#                 selection_weights=selection_weights.unsqueeze(0),
#                 images=images_b,
#                 poses=pose_predictions.view(1, -1, 7),
#                 target_pose=target_pose.unsqueeze(0),
#                 target_image=target_img_b,
#                 num_rays=config.nerf_ray_batch_size
#             )
            
#             # Visualize NeRF output
#             if "rgb" in nerf_outputs:
#                 # Get the number of rays rendered
#                 num_rays = nerf_outputs["rgb"].shape[1]
#                 grid_size = int(np.sqrt(num_rays))

#                 # Dynamically reshape the rendered RGB
#                 rendered_rgb = nerf_outputs["rgb"]  # Shape: [1, N, 3]
#                 batch_size, num_rays, _ = rendered_rgb.shape  # [1, N, 3]

#                 # CASE 1: Full-resolution rendering
#                 if num_rays == config.image_height * config.image_width:
#                     rendered_rgb = rendered_rgb.view(batch_size, config.image_height, config.image_width, 3)

#                 # CASE 2: Partial ray sampling (e.g., 1024 rays during training)
#                 else:
#                     grid_size = int(num_rays ** 0.5)  # e.g., 1024 -> 32x32
#                     if grid_size * grid_size != num_rays:
#                         print(f"⚠️ Warning: {num_rays} rays is not a perfect square. Truncating to {grid_size}x{grid_size}.")
                    
#                     # Truncate and reshape into image-like grid
#                     rendered_rgb = rendered_rgb[:, :grid_size * grid_size]
#                     rendered_rgb = rendered_rgb.view(batch_size, grid_size, grid_size, 3)


#                 # Reshape to image
                
                
#                 fig = visualize_nerf_results(
#                     rendered_images=rendered_rgb,
#                     target_images=target_img_b,
#                     metrics={"PSNR": nerf_outputs.get("psnr", torch.tensor(0.0)).item()},
#                     title=f"{identity} - NeRF Results (Epoch {epoch})",
#                     save_path=os.path.join(identity_dir, "nerf_results.png")
#                 )
#                 plt.close(fig)
                
#             # Add images to TensorBoard
#             writer.add_image(
#                 f"{identity}/input_images",
#                 torch.cat([img.permute(2, 0, 1) for img in images_b[0][:min(9, images_b[0].size(0))]], dim=2),
#                 epoch
#             )
            
#             if "rgb" in nerf_outputs:
#                 writer.add_image(
#                     f"{identity}/nerf_comparison",
#                     torch.cat([
#                         target_img_b[0].permute(2, 0, 1),
#                         rendered_rgb[0].permute(2, 0, 1)
#                     ], dim=2),
#                     epoch
#                 )
                
#             # Add selection weights histogram to TensorBoard
#             writer.add_histogram(
#                 f"{identity}/selection_weights",
#                 F.softmax(selection_weights, dim=0),
#                 epoch
#             )


def generate_selection_outputs(
    transformer_model: nn.Module,
    val_loader: torch.utils.data.DataLoader,
    device: torch.device,
    output_dir: str,
    temperature: float,
    top_k: int
):
    """
    Generate output files with selected views and poses.
    
    Args:
        transformer_model: Transformer model
        val_loader: Validation data loader
        device: Device to use
        output_dir: Output directory
        temperature: Temperature for selection softmax
        top_k: Number of top views to select
    """
    # Set model to evaluation mode
    transformer_model.eval()
    
    # Create output directory
    os.makedirs(output_dir, exist_ok=True)
    
    # Results dictionary
    results = {}
    
    with torch.no_grad():
        for batch in val_loader:
            # Move data to device
            input_tokens = batch["input_tokens"].to(device)
            input_indices = batch["input_indices"].to(device)
            
            # Process each item in the batch
            for b in range(input_tokens.size(0)):
                # Get identity
                identity = batch["identity"][b]
                
                # Skip if already processed
                if identity in results:
                    continue
                
                # Extract single-item batch
                tokens_b = input_tokens[b:b+1]
                indices_b = input_indices[b].cpu().numpy().tolist()
                
                # Forward pass through transformer
                transformer_outputs = transformer_model(tokens_b)
                
                # Get selection weights and pose predictions
                selection_weights = transformer_outputs["selection_weights"][0]
                pose_predictions = transformer_outputs["pose_predictions"][0]
                
                # Get top-k indices
                selection_probs = F.softmax(selection_weights / temperature, dim=0)
                top_values, top_indices = torch.topk(selection_probs, min(top_k, len(selection_probs)))
                
                # Convert to list
                top_indices = top_indices.cpu().numpy().tolist()
                top_values = top_values.cpu().numpy().tolist()
                
                # Map to original indices
                original_indices = [indices_b[i] for i in top_indices]
                
                # Extract poses for selected views
                selected_poses = pose_predictions[top_indices].cpu().numpy().tolist()
                
                # Create result dictionary
                results[identity] = {
                    "selected_views": original_indices,
                    "selection_weights": top_values,
                    "predicted_poses": selected_poses
                }
    
    # Save results
    with open(os.path.join(output_dir, "selected_views.json"), "w") as f:
        json.dump(results, f, indent=2)
    
    print(f"Selection outputs saved to {output_dir}/selected_views.json")

def parse_args():
    """Parse command line arguments"""
    parser = argparse.ArgumentParser(description="Joint MTCM-NeRF Training")
    
    # Data arguments
    parser.add_argument("--data-dir", type=str, required=True, help="Path to data directory")
    parser.add_argument("--output-dir", type=str, default="outputs", help="Path to output directory")
    parser.add_argument("--experiment-name", type=str, default=None, help="Experiment name")
    parser.add_argument("--identity", type=str, default=None, help="Specific identity to train on")
    
    # Model arguments
    parser.add_argument("--transformer-checkpoint", type=str, default=None, help="Path to transformer checkpoint")
    parser.add_argument("--nerf-checkpoint", type=str, default=None, help="Path to NeRF checkpoint")
    parser.add_argument("--joint-checkpoint", type=str, default=None, help="Path to joint model checkpoint")
    
    # Transformer arguments
    parser.add_argument("--input-dim", type=int, default=394, help="Transformer input dimension")
    parser.add_argument("--model-dim", type=int, default=128, help="Transformer model dimension")
    parser.add_argument("--depth", type=int, default=4, help="Transformer depth")
    parser.add_argument("--heads", type=int, default=8, help="Number of attention heads")
    parser.add_argument("--drop-path", type=float, default=0.1, help="Drop path rate")
    
    # NeRF arguments
    parser.add_argument("--nerf-hidden-dim", type=int, default=128, help="NeRF hidden dimension")
    parser.add_argument("--nerf-num-layers", type=int, default=4, help="NeRF number of layers")
    parser.add_argument("--nerf-encoding-functions", type=int, default=10, help="NeRF encoding functions")
    parser.add_argument("--image-height", type=int, default=256, help="Image height")
    parser.add_argument("--image-width", type=int, default=256, help="Image width")
    parser.add_argument("--num-samples", type=int, default=64, help="Number of samples per ray")
    
    # Training arguments
    parser.add_argument("--batch-size", type=int, default=8, help="Batch size")
    parser.add_argument("--num-workers", type=int, default=4, help="Number of workers")
    parser.add_argument("--num-epochs", type=int, default=50, help="Number of epochs")
    parser.add_argument("--lr", type=float, default=1e-4, help="Learning rate")
    parser.add_argument("--weight-decay", type=float, default=1e-5, help="Weight decay")
    parser.add_argument("--warmup-epochs", type=int, default=5, help="Number of warmup epochs")
    parser.add_argument("--num-rays", type=int, default=1024, help="Number of rays to sample for NeRF")
    parser.add_argument("--num-selected-views", type=int, default=5, help="Number of views to select")
    parser.add_argument("--max-views", type=int, default=64, help="Maximum number of views per identity")
    
    # Loss weights
    parser.add_argument("--pose-loss-weight", type=float, default=1.0, help="Weight for pose regression loss")
    parser.add_argument("--nerf-loss-weight", type=float, default=10.0, help="Weight for NeRF reconstruction loss")
    parser.add_argument("--selection-entropy-weight", type=float, default=0.01, help="Weight for selection entropy regularization")
    parser.add_argument("--diversity-loss-weight", type=float, default=0.1, help="Weight for view diversity loss")
    
    # Training options
    parser.add_argument("--mixed-precision", action="store_true", help="Use mixed precision training")
    parser.add_argument("--pose-supervision", action="store_true", help="Use ground truth pose supervision")
    parser.add_argument("--render-interval", type=int, default=5, help="Render visualization every N epochs")
    parser.add_argument("--checkpoint-interval", type=int, default=5, help="Save checkpoint every N epochs")
    parser.add_argument("--validation-interval", type=int, default=2, help="Run validation every N epochs")
    
    # Temperature annealing
    parser.add_argument("--min-selection-temp", type=float, default=0.5, help="Minimum temperature for selection softmax")
    parser.add_argument("--max-selection-temp", type=float, default=5.0, help="Maximum temperature for selection softmax")
    parser.add_argument("--selection-temp-decay", type=float, default=0.9, help="Decay rate for selection temperature")
    
    # Misc
    parser.add_argument("--seed", type=int, default=42, help="Random seed")
    parser.add_argument("--debug", action="store_true", help="Enable debug mode")
    
    args = parser.parse_args()
    return args


def main():
    """Main training function"""
    print("Starting joint training of MTCM and NeRF...")
    args = parse_args()
    print("Parsed arguments:")
    
    # Set random seed
    torch.manual_seed(args.seed)
    np.random.seed(args.seed)
    if torch.cuda.is_available():
        torch.cuda.manual_seed(args.seed)
        torch.backends.cudnn.deterministic = True
        torch.backends.cudnn.benchmark = False
    
    # Set device
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    print(f"Using device: {device}")
    
    # Create experiment directory
    exp_dir = create_experiment_dir(args.output_dir, args.experiment_name)
    print(f"Experiment directory: {exp_dir}")
    
    # Create configurations
    transformer_config = MTCMConfig(
    input_dim=394,
    model_dim=128,
    mlp_dim=256,
    depth=4,
    heads=8,
    drop_path=0.1,
    predict_weights=True,
    predict_poses=True
)

    
    nerf_config = create_weighted_nerf_config({
        "num_encoding_functions": args.nerf_encoding_functions,
        "hidden_dim": args.nerf_hidden_dim,
        "num_layers": args.nerf_num_layers,
        "image_height": args.image_height,
        "image_width": args.image_width,
        "top_k": args.num_selected_views
    })
    
    joint_config = create_joint_training_config({
        "batch_size": args.batch_size,
        "num_epochs": args.num_epochs,
        "learning_rate": args.lr,
        "weight_decay": args.weight_decay,
        "checkpoint_interval": args.checkpoint_interval,
        "validation_interval": args.validation_interval,
        "pose_loss_weight": args.pose_loss_weight,
        "nerf_loss_weight": args.nerf_loss_weight,
        "selection_entropy_weight": args.selection_entropy_weight,
        "pose_supervision": args.pose_supervision,
        "mixed_precision": args.mixed_precision,
        "render_interval": args.render_interval,
        "warmup_epochs": args.warmup_epochs,
        "nerf_ray_batch_size": args.num_rays,
        "min_selection_temp": args.min_selection_temp,
        "max_selection_temp": args.max_selection_temp,
        "selection_temp_decay": args.selection_temp_decay,
        "top_k": args.num_selected_views,
    })
    
    # Save configurations
    with open(os.path.join(exp_dir, "transformer_config.txt"), "w") as f:
        f.write(str(transformer_config))
    with open(os.path.join(exp_dir, "nerf_config.txt"), "w") as f:
        f.write(str(nerf_config))
    with open(os.path.join(exp_dir, "joint_config.txt"), "w") as f:
        f.write(str(joint_config))
    
    # Create data module
    data_module = ViewSelectionDataModule(
        root_dir=args.data_dir,
        batch_size=args.batch_size,
        num_workers=args.num_workers,
        max_views_per_identity=args.max_views,
        num_selected_views=args.num_selected_views,
        target_views_per_identity=1,  # 1 target view per identity for supervision
        val_identities=[args.identity] if args.identity else None,
        debug=args.debug
    )
    
    # Set up the datasets
    data_module.setup(mode="joint")
    
    # Create models
    transformer_model = MTCM_MAE(
        input_dim=transformer_config.input_dim,
        model_dim=transformer_config.model_dim,
        depth=transformer_config.depth,
        heads=transformer_config.heads,
        drop_path=transformer_config.drop_path,
        predict_weights=transformer_config.predict_weights,
        predict_poses=transformer_config.predict_poses
    ).to(device)
    
    nerf_model = WeightedTinyNeRF(
        num_encoding_functions=nerf_config.num_encoding_functions,
        hidden_dim=nerf_config.hidden_dim,
        num_layers=nerf_config.num_layers,
        image_height=nerf_config.image_height,
        image_width=nerf_config.image_width,
        top_k=nerf_config.top_k
    ).to(device)
    
    # Print model information
    transformer_params = sum(p.numel() for p in transformer_model.parameters())
    nerf_params = sum(p.numel() for p in nerf_model.parameters())
    print(f"Transformer model parameters: {transformer_params:,}")
    print(f"NeRF model parameters: {nerf_params:,}")
    print(f"Total parameters: {transformer_params + nerf_params:,}")
    
    # Set up training
    optimizer, scheduler, start_epoch, best_metric = setup_training(
        transformer_model=transformer_model,
        nerf_model=nerf_model,
        config=joint_config,
        resume_path=args.joint_checkpoint
    )
    
    # Set up logging
    writer = setup_logging(exp_dir, joint_config)
    
    # Create selection metrics tracker
    selection_metrics = SelectionMetrics(
        num_identities=len(data_module.train_dataset.identity_list),
        num_views=args.max_views
    )
    
    # Set up mixed precision training
    scaler = GradScaler('cuda') if args.mixed_precision else None
    
    # Get data loaders
    train_loader = data_module.train_dataloader()
    val_loader = data_module.val_dataloader()
    
    # Create visualization directory
    vis_dir = os.path.join(exp_dir, "visualizations")
    os.makedirs(vis_dir, exist_ok=True)
    
    # Training loop
    for epoch in range(start_epoch, args.num_epochs):
        # Calculate temperature for this epoch
        temperature = calculate_selection_temperature(
            epoch=epoch,
            max_temp=args.max_selection_temp,
            min_temp=args.min_selection_temp,
            decay_factor=args.selection_temp_decay,
            total_epochs=args.num_epochs
        )
        
        # Train for one epoch
        train_metrics = train_epoch(
            transformer_model=transformer_model,
            nerf_model=nerf_model,
            train_loader=train_loader,
            optimizer=optimizer,
            device=device,
            epoch=epoch,
            writer=writer,
            selection_metrics=selection_metrics,
            temperature=temperature,
            config=joint_config,
            scaler=scaler
        )
        
        # Validate model
        if epoch % args.validation_interval == 0:
            val_metrics = validate(
                transformer_model=transformer_model,
                nerf_model=nerf_model,
                val_loader=val_loader,
                device=device,
                epoch=epoch,
                writer=writer,
                vis_dir=vis_dir,
                temperature=temperature,
                config=joint_config
            )
            
            # Update best model
            is_best = val_metrics['val_loss'] < best_metric
            best_metric = min(val_metrics['val_loss'], best_metric)
        else:
            is_best = False
        
        # Visualize selection
        if epoch % args.render_interval == 0:
            visualize_epoch(
                transformer_model=transformer_model,
                nerf_model=nerf_model,
                val_loader=val_loader,
                device=device,
                epoch=epoch,
                writer=writer,
                vis_dir=vis_dir,
                temperature=temperature,
                config=joint_config
            )
        
        # Update scheduler
        if scheduler is not None:
            scheduler.step()
        
        # Save checkpoint
        if (epoch + 1) % args.checkpoint_interval == 0 or is_best:
            save_checkpoint(
                state={
                    'epoch': epoch,
                    'transformer_state_dict': transformer_model.state_dict(),
                    'nerf_state_dict': nerf_model.state_dict(),
                    'best_metric': best_metric,
                    'optimizer': optimizer.state_dict(),
                    'scheduler': scheduler.state_dict() if scheduler else None,
                },
                is_best=is_best,
                filename=os.path.join(exp_dir, f'checkpoint_epoch_{epoch}.pth'),
                best_filename=os.path.join(exp_dir, 'model_best.pth')
            )
    
    # Final evaluation
    print("Training completed, running final evaluation...")
    val_metrics = validate(
        transformer_model=transformer_model,
        nerf_model=nerf_model,
        val_loader=val_loader,
        device=device,
        epoch=args.num_epochs,
        writer=writer,
        vis_dir=vis_dir,
        temperature=args.min_selection_temp,  # Use minimum temperature for final evaluation
        config=joint_config
    )
    
    # Visualize final selection
    visualize_epoch(
        transformer_model=transformer_model,
        nerf_model=nerf_model,
        val_loader=val_loader,
        device=device,
        epoch=args.num_epochs,
        writer=writer,
        vis_dir=os.path.join(exp_dir, "final_visualizations"),
        temperature=args.min_selection_temp,
        config=joint_config
    )
    
    # Save final model
    save_checkpoint(
        state={
            'epoch': args.num_epochs,
            'transformer_state_dict': transformer_model.state_dict(),
            'nerf_state_dict': nerf_model.state_dict(),
            'best_metric': best_metric,
            'optimizer': optimizer.state_dict(),
            'scheduler': scheduler.state_dict() if scheduler else None,
        },
        is_best=False,
        filename=os.path.join(exp_dir, 'model_final.pth')
    )
    
    # Generate output files with selected views
    generate_selection_outputs(
        transformer_model=transformer_model,
        val_loader=val_loader,
        device=device,
        output_dir=os.path.join(exp_dir, "selections"),
        temperature=args.min_selection_temp,
        top_k=args.num_selected_views
    )
    
    print(f"Final validation loss: {val_metrics['val_loss']:.4f}")
    print(f"Best validation loss: {best_metric:.4f}")
    print(f"Training completed. Results saved to {exp_dir}")


def train_epoch(
    transformer_model: nn.Module,
    nerf_model: nn.Module,
    train_loader: torch.utils.data.DataLoader,
    optimizer: torch.optim.Optimizer,
    device: torch.device,
    epoch: int,
    writer: SummaryWriter,
    selection_metrics: SelectionMetrics,
    temperature: float,
    config: JointTrainingConfig,
    scaler: Optional[GradScaler] = None
) -> Dict[str, float]:
    """
    Train the model for one epoch.
    
    Args:
        transformer_model: Transformer model
        nerf_model: NeRF model
        train_loader: Training data loader
        optimizer: Optimizer
        device: Device to use
        epoch: Current epoch
        writer: TensorBoard writer
        selection_metrics: Selection metrics tracker
        temperature: Temperature for selection softmax
        config: Training configuration
        scaler: GradScaler for mixed precision training
        
    Returns:
        Dictionary of training metrics
    """
    # Set models to training mode
    transformer_model.train()
    nerf_model.train()
    
    # Initialize meters
    batch_time = AverageMeter('Time', ':6.3f')
    data_time = AverageMeter('Data', ':6.3f')
    losses = AverageMeter('Loss', ':.4e')
    pose_losses = AverageMeter('PoseLoss', ':.4e')
    nerf_losses = AverageMeter('NeRFLoss', ':.4e')
    entropy_losses = AverageMeter('EntropyLoss', ':.4e')
    psnrs = AverageMeter('PSNR', ':.2f')
    
    # Create progress display
    progress = ProgressMeter(
        num_batches=len(train_loader),
        meters=[batch_time, data_time, losses, pose_losses, nerf_losses, entropy_losses, psnrs],
        prefix=f"Epoch: [{epoch}]"
    )
    
    # Reset selection metrics
    selection_metrics.reset()
    
    end = time.time()
    
    # Iterate over batches
    for i, batch in enumerate(train_loader):
        # Measure data loading time
        data_time.update(time.time() - end)
        
        # Move data to device
        input_tokens = batch["input_tokens"].to(device)
        target_tokens = batch["target_tokens"].to(device)
        input_indices = batch["input_indices"].to(device)
        target_indices = batch["target_indices"].to(device)
        
        if "input_images" in batch and "target_images" in batch:
            input_images = batch["input_images"].to(device)
            target_images = batch["target_images"].to(device)
        else:
            raise ValueError("Input and target images are required for joint training")
        
        # Get batch size
        batch_size = input_tokens.size(0)
        
        # Mixed precision context
        with autocast('cuda') if config.mixed_precision else contextlib.nullcontext():
            # Forward pass through transformer
            transformer_outputs = transformer_model(input_tokens)
            
            # Get selection weights and pose predictions
            selection_weights = transformer_outputs["selection_weights"]
            pose_predictions = transformer_outputs["pose_predictions"]
            
            # Extract target poses for supervision
            target_poses = target_tokens[:, 0, 384:391]  # Position + Quaternion
            
            # Get target pose for NeRF rendering (first pose in target_tokens)
            target_pose = target_poses
            
            # Select views based on selection weights
            selected_tokens, selection_probs = get_selected_views(
                selection_weights=selection_weights,
                views=input_tokens,
                num_views=config.top_k,
                temperature=temperature,
                training=True
            )
            
            # Extract poses from selected tokens
            selected_poses = selected_tokens[:, :, 384:391]  # Position + Quaternion
            
            # Select images for NeRF rendering
            selected_images, _ = get_selected_views(
                selection_weights=selection_weights,
                views=input_images,
                num_views=config.top_k,
                temperature=temperature,
                training=True
            )
            
            # Forward pass through NeRF
            nerf_outputs = nerf_model(
                selection_weights=selection_weights,
                images=input_images,
                poses=pose_predictions,
                target_pose=target_pose,
                target_image=target_images[:, 0],  # First target image
                num_rays=config.nerf_ray_batch_size
            )
            
            # # Calculate losses
            # # 1. Pose regression loss
            # if config.pose_supervision:
            #     pose_loss = compute_pose_loss(pose_predictions.view(-1, 7), target_poses, pose_loss_type='separate')
            # else:
            #     pose_loss = torch.tensor(0.0, device=device)
            # Calculate losses
            # 1. Pose regression loss
            if config.pose_supervision:
                # Get selection probabilities to weight the loss
                selection_probs = F.softmax(selection_weights / temperature, dim=1)
                
                pose_loss = compute_pose_loss(
                    pose_predictions,  # [B, N, 7]
                    target_poses,      # [B, 7]
                    pose_loss_type='separate',
                    weights=selection_probs  # Use selection probabilities as weights
                )
            else:
                pose_loss = torch.tensor(0.0, device=device)
            
            # 2. NeRF reconstruction loss
            nerf_loss = nerf_outputs["loss"]
            
            # 3. Selection entropy regularization (optional)
            entropy_loss = compute_selection_entropy_loss(selection_weights, temperature)
            
            # 4. View diversity loss (optional)
            diversity_loss = compute_diversity_loss(selected_poses)
            
            # Combined loss
            loss = (
                config.pose_loss_weight * pose_loss +
                config.nerf_loss_weight * nerf_loss +
                config.selection_entropy_weight * entropy_loss +
                config.diversity_loss_weight * diversity_loss
            )
        
        # Compute gradients and update parameters
        optimizer.zero_grad()
        
        if scaler is not None:
            # Mixed precision training
            scaler.scale(loss).backward()
            scaler.step(optimizer)
            scaler.update()
        else:
            # Standard training
            loss.backward()
            optimizer.step()
        
        # Calculate PSNR
        psnr_val = nerf_outputs.get("psnr", torch.tensor(0.0))
        
        # Update metrics
        losses.update(loss.item(), batch_size)
        pose_losses.update(pose_loss.item(), batch_size)
        nerf_losses.update(nerf_loss.item(), batch_size)
        entropy_losses.update(entropy_loss.item(), batch_size)
        psnrs.update(psnr_val.item(), batch_size)
        
        # Update selection metrics
        for b in range(batch_size):
            selection_metrics.update_selection(
                identity=batch["identity"][b],
                view_indices=input_indices[b],
                selection_probs=selection_probs[b]
            )
        
        # Measure elapsed time
        batch_time.update(time.time() - end)
        end = time.time()
        
        # Print progress
        if i % 10 == 0:
            progress.display(i)
        
        # Write metrics to TensorBoard
        step = epoch * len(train_loader) + i
        if i % 10 == 0:
            metrics = {
                'loss': loss.item(),
                'pose_loss': pose_loss.item(),
                'nerf_loss': nerf_loss.item(),
                'entropy_loss': entropy_loss.item(),
                'diversity_loss': diversity_loss.item(),
                'psnr': psnr_val.item(),
                'temperature': temperature
            }
            
            # Add learning rates
            metrics.update(get_learning_rates(optimizer))
            
            # Write metrics
            write_metrics_to_tensorboard(writer, metrics, step, prefix='train')
    
    # Write epoch summary
    epoch_metrics = {
        'loss': losses.avg,
        'pose_loss': pose_losses.avg,
        'nerf_loss': nerf_losses.avg,
        'entropy_loss': entropy_losses.avg,
        'psnr': psnrs.avg,
        'temperature': temperature
    }
    write_metrics_to_tensorboard(writer, epoch_metrics, epoch, prefix='train_epoch')
    
    # Return metrics
    return epoch_metrics


def validate(
    transformer_model: nn.Module,
    nerf_model: nn.Module,
    val_loader: torch.utils.data.DataLoader,
    device: torch.device,
    epoch: int,
    writer: SummaryWriter,
    vis_dir: str,
    temperature: float,
    config: JointTrainingConfig
) -> Dict[str, float]:
    """
    Validate the model.
    
    Args:
        transformer_model: Transformer model
        nerf_model: NeRF model
        val_loader: Validation data loader
        device: Device to use
        epoch: Current epoch
        writer: TensorBoard writer
        vis_dir: Visualization directory
        temperature: Temperature for selection softmax
        config: Training configuration
        
    Returns:
        Dictionary of validation metrics
    """
    # Set models to evaluation mode
    transformer_model.eval()
    nerf_model.eval()
    
    # Initialize meters
    batch_time = AverageMeter('Time', ':6.3f')
    losses = AverageMeter('Loss', ':.4e')
    pose_losses = AverageMeter('PoseLoss', ':.4e')
    nerf_losses = AverageMeter('NeRFLoss', ':.4e')
    psnrs = AverageMeter('PSNR', ':.2f')
    
    end = time.time()
    
    # Create validation metrics
    val_metrics = {}
    
    with torch.no_grad():
        for i, batch in enumerate(val_loader):
            # Move data to device
            input_tokens = batch["input_tokens"].to(device)
            target_tokens = batch["target_tokens"].to(device)
            
            if "input_images" in batch and "target_images" in batch:
                input_images = batch["input_images"].to(device)
                target_images = batch["target_images"].to(device)
            else:
                raise ValueError("Input and target images are required for validation")
            
            # Get batch size
            batch_size = input_tokens.size(0)
            
            # Forward pass through transformer
            transformer_outputs = transformer_model(input_tokens)
            
            # Get selection weights and pose predictions
            selection_weights = transformer_outputs["selection_weights"]
            pose_predictions = transformer_outputs["pose_predictions"]
            
            # Extract target poses for supervision
            target_poses = target_tokens[:, 0, 384:391]  # Position + Quaternion
            
            # Get target pose for NeRF rendering (first pose in target_tokens)
            target_pose = target_poses
            
            # Forward pass through NeRF
            nerf_outputs = nerf_model(
                selection_weights=selection_weights,
                images=input_images,
                poses=pose_predictions,
                target_pose=target_pose,
                target_image=target_images[:, 0],  # First target image
                num_rays=config.nerf_ray_batch_size
            )
            
            # Calculate losses
            # 1. Pose regression loss
            if config.pose_supervision:
                pose_loss = compute_pose_loss(pose_predictions.view(-1, 7), target_poses, pose_loss_type='separate')
            else:
                pose_loss = torch.tensor(0.0, device=device)
            
            # 2. NeRF reconstruction loss
            nerf_loss = nerf_outputs["loss"]
            
            # Combined loss
            loss = (
                config.pose_loss_weight * pose_loss +
                config.nerf_loss_weight * nerf_loss
            )
            
            # Calculate PSNR
            psnr_val = nerf_outputs.get("psnr", torch.tensor(0.0))
            
            # Update metrics
            losses.update(loss.item(), batch_size)
            pose_losses.update(pose_loss.item(), batch_size)
            nerf_losses.update(nerf_loss.item(), batch_size)
            psnrs.update(psnr_val.item(), batch_size)
            
            # Measure elapsed time
            batch_time.update(time.time() - end)
            end = time.time()
            
            # Save visualization for first batch
            if i == 0 and epoch % config.render_interval == 0:
                val_vis_dir = os.path.join(vis_dir, f"epoch_{epoch:04d}", "validation")
                os.makedirs(val_vis_dir, exist_ok=True)
                
                # Visualize input images and selection weights
                fig = visualize_image_grid(
                    images=input_images[0],
                    selection_weights=selection_weights[0],
                    title=f"Validation Input Images (Epoch {epoch})",
                    save_path=os.path.join(val_vis_dir, "input_images.png")
                )
                plt.close(fig)
                
                # Visualize target image
                fig = visualize_image_grid(
                    images=target_images[0],
                    title=f"Validation Target Image (Epoch {epoch})",
                    save_path=os.path.join(val_vis_dir, "target_image.png")
                )
                plt.close(fig)
                
                # Process rendered images for visualization
                num_rays_rendered = nerf_outputs["rgb"].shape[1]
                
                # Determine appropriate visualization format based on number of rays
                if num_rays_rendered == nerf_model.image_height * nerf_model.image_width:
                    # Full image rendering
                    rendered_images = nerf_outputs["rgb"].view(batch_size, nerf_model.image_height, nerf_model.image_width, 3)
                else:
                    # Partial rendering - create a square grid from available rays
                    grid_size = int(np.sqrt(num_rays_rendered))
                    rendered_images = nerf_outputs["rgb"][:, :grid_size*grid_size].view(batch_size, grid_size, grid_size, 3)
                
                # Visualize NeRF output
                fig = visualize_nerf_results(
                    rendered_images=rendered_images,
                    target_images=target_images[:, 0],
                    metrics={"PSNR": psnr_val.item()},
                    title=f"Validation NeRF Results (Epoch {epoch})",
                    save_path=os.path.join(val_vis_dir, "nerf_results.png")
                )
                plt.close(fig)
    
    # Write validation metrics
    val_metrics = {
        'val_loss': losses.avg,
        'val_pose_loss': pose_losses.avg,
        'val_nerf_loss': nerf_losses.avg,
        'val_psnr': psnrs.avg
    }
    write_metrics_to_tensorboard(writer, val_metrics, epoch, prefix='validation')
    
    # Print validation summary
    print(f"Validation Epoch: {epoch}")
    print(f"  Loss: {losses.avg:.4f}")
    print(f"  Pose Loss: {pose_losses.avg:.4f}")
    print(f"  NeRF Loss: {nerf_losses.avg:.4f}")
    print(f"  PSNR: {psnrs.avg:.2f}")
    
    return val_metrics

if __name__ == "__main__":
    main()
