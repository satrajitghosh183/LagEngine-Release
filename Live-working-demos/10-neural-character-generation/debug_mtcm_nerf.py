#!/usr/bin/env python3
"""
Minimal version of the joint training script to debug crashes
"""

import os
import sys
import argparse
import torch
import contextlib
import numpy as np
import matplotlib.pyplot as plt

# Import only the main model classes
from mtcm_mae.model import MTCM_MAE
from mtcm_mae.config import MTCMConfig
from nerf.tiny_nerf import TinyNeRF
from nerf.weighted_tiny_nerf import WeightedTinyNeRF
from nerf.nerf_config import (
    create_nerf_config,
    create_weighted_nerf_config,
    create_joint_training_config
)

# Import minimal dataset and utilities
from dataset_joint_mtcm_nerf import ViewSelectionDataModule
from training_utils import (
    create_experiment_dir,
    setup_logging,
    setup_training,
    SelectionMetrics
)

def parse_args():
    """Parse command line arguments"""
    parser = argparse.ArgumentParser(description="Joint MTCM-NeRF Training")
    
    # Add just the essential arguments
    parser.add_argument("--data-dir", type=str, required=True, help="Path to data directory")
    parser.add_argument("--output-dir", type=str, default="outputs", help="Path to output directory")
    parser.add_argument("--identity", type=str, default=None, help="Specific identity to train on")
    parser.add_argument("--batch-size", type=int, default=1, help="Batch size")
    parser.add_argument("--num-epochs", type=int, default=5, help="Number of epochs")
    parser.add_argument("--max-views", type=int, default=32, help="Maximum number of views per identity")
    parser.add_argument("--num-selected-views", type=int, default=5, help="Number of views to select")
    parser.add_argument("--debug", action="store_true", help="Enable debug mode")
    
    args = parser.parse_args()
    return args

def main():
    """Minimal main function for testing"""
    print("Starting minimal test script...")
    
    try:
        args = parse_args()
        print("Arguments parsed successfully")
        
        # Set random seed
        torch.manual_seed(42)
        np.random.seed(42)
        print("Random seeds set")
        
        # Set device
        device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
        print(f"Using device: {device}")
        
        # Create experiment directory
        exp_dir = create_experiment_dir(args.output_dir, "test_run")
        print(f"Created experiment directory: {exp_dir}")
        
        # Create minimal configurations


        transformer_config = MTCMConfig(
        model_dim=64,
        depth=2,
        heads=4,
        drop_path=0.1,
        predict_weights=True,
        predict_poses=True
    )

       
        print("Created transformer config")
        
        nerf_config = create_weighted_nerf_config({
            "num_encoding_functions": 6,  # Reduced for testing
            "hidden_dim": 64,            # Reduced for testing
            "num_layers": 2,             # Reduced for testing
            "image_height": 256,
            "image_width": 256,
            "top_k": args.num_selected_views
        })
        print("Created NeRF config")
        
        joint_config = create_joint_training_config({
            "batch_size": args.batch_size,
            "num_epochs": args.num_epochs,
            "learning_rate": 1e-4,
            "weight_decay": 1e-5,
            "top_k": args.num_selected_views
        })
        print("Created joint config")
        
        # Try creating data module
        print("Creating data module...")
        data_module = ViewSelectionDataModule(
            root_dir=args.data_dir,
            batch_size=args.batch_size,
            num_workers=0,  # Use 0 workers for debugging
            max_views_per_identity=args.max_views,
            num_selected_views=args.num_selected_views,
            target_views_per_identity=1,
            val_identities=[args.identity] if args.identity else None,
            debug=args.debug
        )
        print("Data module created successfully")
        
        # Set up the datasets
        print("Setting up datasets...")
        data_module.setup(mode="joint")
        print("Datasets set up successfully")
        
        # Create models
        print("Creating models...")
        transformer_model = MTCM_MAE(
            input_dim=transformer_config.input_dim,
            model_dim=transformer_config.model_dim,
            depth=transformer_config.depth,
            heads=transformer_config.heads,
            drop_path=transformer_config.drop_path,
            predict_weights=transformer_config.predict_weights,
            predict_poses=transformer_config.predict_poses
        ).to(device)
        print("Transformer model created")
        
        nerf_model = WeightedTinyNeRF(
            num_encoding_functions=nerf_config.num_encoding_functions,
            hidden_dim=nerf_config.hidden_dim,
            num_layers=nerf_config.num_layers,
            image_height=nerf_config.image_height,
            image_width=nerf_config.image_width,
            top_k=nerf_config.top_k
        ).to(device)
        print("NeRF model created")
        
        # Set up training components
        print("Setting up training components...")
        optimizer, scheduler, start_epoch, best_metric = setup_training(
            transformer_model=transformer_model,
            nerf_model=nerf_model,
            config=joint_config,
            resume_path=None
        )
        print("Training components set up successfully")
        
        # Set up logging
        print("Setting up logging...")
        writer = setup_logging(exp_dir, joint_config)
        print("Logging set up successfully")
        
        # Get data loaders
        print("Getting data loaders...")
        train_loader = data_module.train_dataloader()
        val_loader = data_module.val_dataloader()
        print(f"Data loaders created. Train loader has {len(train_loader)} batches")
        
        # Just print that everything worked
        print("All setup steps completed successfully!")
        print("You can now continue with the full training script.")
        
    except Exception as e:
        import traceback
        print(f"Error in main function: {e}")
        traceback.print_exc()

if __name__ == "__main__":
    main()