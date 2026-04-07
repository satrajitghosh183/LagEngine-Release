#!/usr/bin/env python3
"""
Test script for the joint MTCM-NeRF dataset.
This validates the dataset implementation for the joint training pipeline.
"""

import os
import sys
import argparse
import torch
import numpy as np
import matplotlib.pyplot as plt
from PIL import Image
from dataset_joint_mtcm_nerf import JointMTCMNeRFDataset, ViewSelectionDataModule
from data_visualization import (
    visualize_token_embeddings, 
    visualize_image_grid, 
    visualize_pose_distribution,
    visualize_selection_weights
)


def test_dataset_loading(args):
    """Test dataset loading and basic functionality"""
    
    print(f"Testing dataset loading from {args.data_dir}")
    
    # Create dataset with selected identity if specified
    identity_list = [args.identity] if args.identity else None
    dataset = JointMTCMNeRFDataset(
        root_dir=args.data_dir,
        mode=args.mode,
        identity_list=identity_list,
        max_views_per_identity=args.max_views,
        num_selected_views=args.num_selected,
        target_views_per_identity=args.target_views,
        debug=True
    )
    
    print(f"Dataset size: {len(dataset)}")
    
    if len(dataset) == 0:
        print("ERROR: Dataset is empty. Please check the data directory.")
        return False
    
    # Test getting an item
    print("\nTesting getting first item from dataset...")
    try:
        sample = dataset[0]
        
        print(f"Sample keys: {sample.keys()}")
        
        if 'identity' in sample:
            print(f"Identity: {sample['identity']}")
        
        if 'input_tokens' in sample:
            print(f"Input tokens shape: {sample['input_tokens'].shape}")
            
            # Check token dimensions
            token_dim = sample['input_tokens'].shape[-1]
            if token_dim != 394:
                print(f"WARNING: Expected token dimension 394, got {token_dim}")
            else:
                print("Token dimensions are correct (394D)")
        
        if 'target_tokens' in sample:
            print(f"Target tokens shape: {sample['target_tokens'].shape}")
        
        if 'input_images' in sample:
            print(f"Input images shape: {sample['input_images'].shape}")
        
        if 'target_images' in sample:
            print(f"Target images shape: {sample['target_images'].shape}")
            
        print("Successfully loaded sample from dataset!")
        return True
        
    except Exception as e:
        print(f"Error loading sample: {e}")
        import traceback
        traceback.print_exc()
        return False


def test_dataloader(args):
    """Test dataloader functionality"""
    
    print("\nTesting dataloader...")
    
    # Create data module
    data_module = ViewSelectionDataModule(
        root_dir=args.data_dir,
        batch_size=args.batch_size,
        num_workers=args.num_workers,
        max_views_per_identity=args.max_views,
        num_selected_views=args.num_selected,
        target_views_per_identity=args.target_views,
        val_identities=[args.identity] if args.identity else None,
        debug=True
    )
    
    # Set up the dataset
    data_module.setup(mode=args.mode)
    
    # Get train and validation loaders
    train_loader = data_module.train_dataloader()
    val_loader = data_module.val_dataloader()
    
    print(f"Train dataloader size: {len(train_loader)}")
    print(f"Validation dataloader size: {len(val_loader)}")
    
    # Test iterating through dataloader
    print("\nTesting iteration through train dataloader...")
    try:
        for i, batch in enumerate(train_loader):
            print(f"Batch {i}:")
            print(f"  Batch keys: {batch.keys()}")
            
            if 'input_tokens' in batch:
                print(f"  Input tokens shape: {batch['input_tokens'].shape}")
            
            if 'target_tokens' in batch:
                print(f"  Target tokens shape: {batch['target_tokens'].shape}")
            
            if 'input_images' in batch:
                print(f"  Input images shape: {batch['input_images'].shape}")
            
            if 'target_images' in batch:
                print(f"  Target images shape: {batch['target_images'].shape}")
            
            # Test just the first few batches
            if i >= 2:
                break
                
        print("Dataloader iteration successful!")
        return True
        
    except Exception as e:
        print(f"Error during dataloader iteration: {e}")
        import traceback
        traceback.print_exc()
        return False


def visualize_dataset_samples(args):
    """Create and save visualizations of dataset samples"""
    
    print("\nCreating visualizations of dataset samples...")
    
    # Create dataset
    identity_list = [args.identity] if args.identity else None
    dataset = JointMTCMNeRFDataset(
        root_dir=args.data_dir,
        mode='joint',  # Need joint mode for images
        identity_list=identity_list,
        max_views_per_identity=args.max_views,
        num_selected_views=args.num_selected,
        target_views_per_identity=args.target_views,
        debug=True
    )
    
    if len(dataset) == 0:
        print("ERROR: Dataset is empty. Cannot create visualizations.")
        return False
    
    # Create output directory
    vis_dir = os.path.join(args.output_dir, "visualizations")
    os.makedirs(vis_dir, exist_ok=True)
    
    # Process a few samples
    for idx in range(min(3, len(dataset))):
        sample = dataset[idx]
        identity = sample['identity']
        
        # Create identity-specific directory
        identity_dir = os.path.join(vis_dir, identity)
        os.makedirs(identity_dir, exist_ok=True)
        
        # Extract data
        input_tokens = sample['input_tokens']
        target_tokens = sample['target_tokens']
        
        # 1. Visualize token embeddings
        if 'input_tokens' in sample:
            # Create random selection weights for visualization
            random_weights = torch.randn(input_tokens.shape[0])
            
            # Visualize token embeddings
            fig = visualize_token_embeddings(
                tokens=input_tokens,
                selection_weights=random_weights,
                title=f"{identity} - Token Embeddings PCA",
                save_path=os.path.join(identity_dir, "token_embeddings_pca.png")
            )
            plt.close(fig)
            
            # Also create 3D visualization
            fig = visualize_token_embeddings(
                tokens=input_tokens,
                selection_weights=random_weights,
                pca_components=3,
                title=f"{identity} - Token Embeddings 3D PCA",
                save_path=os.path.join(identity_dir, "token_embeddings_pca_3d.png")
            )
            plt.close(fig)
        
        # 2. Visualize poses
        if 'input_tokens' in sample:
            # Extract poses from tokens
            input_poses = input_tokens[:, 384:391]  # Positions (3) + Quaternions (4)
            
            # Target poses
            target_pose = None
            if 'target_tokens' in sample and len(target_tokens) > 0:
                target_pose = target_tokens[0, 384:391]
            
            # Visualize pose distribution
            fig = visualize_pose_distribution(
                poses=input_poses,
                selection_weights=random_weights,
                target_pose=target_pose,
                title=f"{identity} - Camera Pose Distribution",
                save_path=os.path.join(identity_dir, "pose_distribution.png")
            )
            plt.close(fig)
            
        # 3. Visualize input images
        if 'input_images' in sample:
            # Visualize image grid
            fig = visualize_image_grid(
                images=sample['input_images'],
                selection_weights=random_weights,
                title=f"{identity} - Input Images",
                save_path=os.path.join(identity_dir, "input_images_grid.png")
            )
            plt.close(fig)
            
        # 4. Visualize target images
        if 'target_images' in sample:
            # Visualize target images
            fig = visualize_image_grid(
                images=sample['target_images'],
                title=f"{identity} - Target Images",
                save_path=os.path.join(identity_dir, "target_images_grid.png")
            )
            plt.close(fig)
            
        # 5. Visualize selection weights
        if 'input_tokens' in sample:
            # Create random mock timestamps for visualization
            timestamps = torch.linspace(0, 1, input_tokens.shape[0])
            
            # Mock selection weights
            mock_weights = torch.randn(input_tokens.shape[0])
            
            # Visualize selection weights
            fig = visualize_selection_weights(
                selection_weights=mock_weights,
                timestamps=timestamps,
                title=f"{identity} - View Selection Weights",
                save_path=os.path.join(identity_dir, "selection_weights.png")
            )
            plt.close(fig)
        
        print(f"Created visualizations for sample {idx} (identity: {identity})")
    
    print(f"Visualizations saved to {vis_dir}")
    return True


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Test the joint MTCM-NeRF dataset")
    parser.add_argument("--data-dir", type=str, required=True, help="Path to data directory")
    parser.add_argument("--output-dir", type=str, default="outputs", help="Directory to save output files")
    parser.add_argument("--mode", type=str, default="joint",
                       choices=["transformer", "nerf", "joint"], help="Dataset mode")
    parser.add_argument("--identity", type=str, help="Process only a specific identity")
    parser.add_argument("--max-views", type=int, default=64, help="Maximum views per identity")
    parser.add_argument("--num-selected", type=int, default=5, help="Number of views to select")
    parser.add_argument("--target-views", type=int, default=1, help="Number of target views per identity")
    parser.add_argument("--batch-size", type=int, default=1, help="Batch size")
    parser.add_argument("--num-workers", type=int, default=4, help="Number of dataloader workers")
    parser.add_argument("--skip-visualizations", action="store_true", help="Skip dataset visualization")
    
    args = parser.parse_args()
    
    # Create output directory
    os.makedirs(args.output_dir, exist_ok=True)
    
    # Test dataset loading
    if not test_dataset_loading(args):
        print("Dataset loading failed.")
        sys.exit(1)
        
    # Test dataloader
    if not test_dataloader(args):
        print("Dataloader testing failed.")
        sys.exit(1)
        
    # Create visualizations
    if not args.skip_visualizations:
        if not visualize_dataset_samples(args):
            print("Dataset visualization failed.")
            sys.exit(1)
    
    print("\nAll tests passed successfully!")