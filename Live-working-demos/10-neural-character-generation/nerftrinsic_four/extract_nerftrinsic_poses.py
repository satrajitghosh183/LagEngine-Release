#!/usr/bin/env python3
"""
Extract poses from NeRFtrinsic model checkpoints for integration with MTCM pipeline.

This version fixes the focal model loading issue by handling different model structures.
"""

import os
import sys
import json
import argparse
import numpy as np
import torch
from tqdm import tqdm
from scipy.spatial.transform import Rotation as R
from pathlib import Path

# Add models directory to path if needed
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# Import NeRFtrinsic-specific modules 
try:
    from models.poses import LearnPoseGF
    from models.intrinsics import LearnFocalCamDependent
    from dataloader.with_colmap_numnorm import DataloaderParameterLearning
except ImportError as e:
    print(f"Error importing NeRFtrinsic modules: {e}")
    print("Make sure you're running this script from the NeRFtrinsic root directory or adjust the import paths.")
    sys.exit(1)

def setup_args():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(description="Extract camera poses from NeRFtrinsic model")
    parser.add_argument("--data-root", type=str, required=True, 
                        help="Root directory containing the LLFF-formatted data")
    parser.add_argument("--ckpt-dir", type=str, required=True,
                        help="Directory containing the NeRFtrinsic checkpoints")
    parser.add_argument("--output-dir", type=str, default="nerftrinsic_outputs",
                        help="Directory to save the extracted poses")
    parser.add_argument("--scene", type=str, default=None,
                        help="Process only a specific scene/identity (default: all scenes)")
    parser.add_argument("--use-confidence-scores", action="store_true",
                        help="Compute and include confidence scores for each view")
    parser.add_argument("--image-dir-name", type=str, default="images",
                        help="Name of the directory containing images (default: 'images')")
    parser.add_argument("--resolution", type=int, default=512,
                        help="Resolution to use for focal length computation (default: 512)")
    parser.add_argument("--res-ratio", type=int, default=4,
                        help="Resolution ratio for the dataloader (default: 4)")
    parser.add_argument("--verbose", action="store_true",
                        help="Print additional debugging information")
    parser.add_argument("--fixed-focal", type=float, default=None,
                        help="Use fixed focal length value instead of loading from checkpoint")
    
    return parser.parse_args()

def pose_matrix_to_quat(pose_matrix):
    """Convert a 4x4 pose matrix to a position and quaternion [x, y, z, qx, qy, qz, qw]."""
    R_mat = pose_matrix[:3, :3]
    t_vec = pose_matrix[:3, 3]
    quat = R.from_matrix(R_mat).as_quat()  # [x, y, z, w]
    return np.concatenate([t_vec, quat])

def compute_view_confidence(pose_model, img_idx):
    """Compute a confidence score for the view (0-1)."""
    # Placeholder implementation
    return 1.0

def check_data_path(path):
    """Check if a path exists and print status."""
    exists = os.path.exists(path)
    if exists:
        print(f"✓ Found: {path}")
    else:
        print(f"✗ Missing: {path}")
    return exists

def get_latest_checkpoint(ckpt_dir, identity):
    """Find the latest checkpoint directory for a given identity."""
    identity_dir = os.path.join(ckpt_dir, identity)
    
    if not os.path.exists(identity_dir):
        print(f"Warning: No checkpoint directory found for {identity} at {identity_dir}")
        return None
        
    # Find all potential checkpoint subdirectories
    subdirs = [d for d in os.listdir(identity_dir) 
               if os.path.isdir(os.path.join(identity_dir, d))]
    
    if not subdirs:
        print(f"Warning: No checkpoint subdirectories found in {identity_dir}")
        return None
        
    # Sort directories (assuming they have timestamps or other sortable format)
    subdirs = sorted(subdirs)
    latest_dir = os.path.join(identity_dir, subdirs[-1])
    
    # Check if required checkpoint files exist
    pose_ckpt = os.path.join(latest_dir, "latest_pose.pth")
    
    if not os.path.exists(pose_ckpt):
        print(f"Warning: No pose checkpoint found at {pose_ckpt}")
        return None
    
    return latest_dir

def get_image_filenames(data_root, scene_name, img_dir_name="images"):
    """Get the list of image filenames for a given scene."""
    scene_root = os.path.join(data_root, "LLFF", scene_name)
    img_dir = os.path.join(scene_root, img_dir_name)
    
    if not os.path.exists(img_dir):
        print(f"Warning: Image directory not found at {img_dir}")
        # Try alternate paths
        if os.path.exists(os.path.join(scene_root, "imgs")):
            img_dir = os.path.join(scene_root, "imgs")
        else:
            # Try to find any directory that might contain images
            for item in os.listdir(scene_root):
                if os.path.isdir(os.path.join(scene_root, item)) and item != "sparse":
                    potential_img_dir = os.path.join(scene_root, item)
                    if any(f.lower().endswith(('.png', '.jpg', '.jpeg')) 
                           for f in os.listdir(potential_img_dir)):
                        img_dir = potential_img_dir
                        break
    
    # Get all image files
    image_files = []
    if os.path.exists(img_dir):
        image_files = sorted([f for f in os.listdir(img_dir) 
                             if f.lower().endswith(('.png', '.jpg', '.jpeg'))])
    
    return image_files, img_dir

def load_focal_model(focal_ckpt_path, num_cams=1, device="cpu"):
    """
    Attempt to load the focal model with proper error handling for different model versions.
    Returns the focal model or None if loading fails, along with a constant focal length value.
    """
    try:
        # Check if checkpoint exists
        if not os.path.exists(focal_ckpt_path):
            print(f"Warning: Focal checkpoint not found at {focal_ckpt_path}")
            return None, 500.0  # Default focal length
        
        # Try to load checkpoint first to examine its structure
        checkpoint = torch.load(focal_ckpt_path, map_location=device)
        model_state = checkpoint["model_state_dict"]
        
        # Check for model structure - old version vs new version
        if "fy" not in model_state:
            print(f"Detected older focal model format (no fy parameter)")
            # This is likely an older model with just fx parameter
            # Extract a single focal value from the tensor
            if "fx" in model_state:
                fx_shape = model_state["fx"].shape
                print(f"Found fx parameter with shape {fx_shape}")
                
                # If the fx shape doesn't match [1, 1], we can't load directly
                # Extract a representative value instead
                if fx_shape != torch.Size([1, 1]):
                    # Get the median value as a reasonable focal length
                    focal_value = float(model_state["fx"].median().item())
                    print(f"Using median focal value: {focal_value}")
                    return None, focal_value
            
            # If we couldn't extract a focal value, use a default
            return None, 500.0
        
        # If it looks like a compatible model, try loading it
        focal_model = LearnFocalCamDependent(num_cams=num_cams).to(device)
        focal_model.load_state_dict(model_state)
        focal_model.eval()
        return focal_model, None
        
    except Exception as e:
        print(f"Warning: Failed to load focal model: {e}")
        return None, 500.0  # Default focal length

def process_scene(args, scene_name):
    """Process a single scene/identity and extract the camera poses."""
    print(f"\n[INFO] Processing scene: {scene_name}")
    
    # Handle path variations (spaces vs underscores)
    scene_variations = [scene_name, scene_name.replace(" ", "_"), scene_name.replace("_", " ")]
    
    # Find the data directory
    data_path = None
    llff_folder = os.path.join(args.data_root, "LLFF")
    
    for scene_var in scene_variations:
        path = os.path.join(llff_folder, scene_var)
        if os.path.exists(path):
            data_path = path
            break
    
    if not data_path:
        print(f"[ERROR] Could not find data directory for {scene_name}")
        return False
    
    # Check for poses_bounds.npy
    poses_bounds_path = os.path.join(data_path, "poses_bounds.npy")
    if not os.path.exists(poses_bounds_path):
        print(f"[ERROR] poses_bounds.npy not found at {poses_bounds_path}")
        return False
        
    # Find the latest checkpoint
    ckpt_dir = get_latest_checkpoint(args.ckpt_dir, scene_name)
    if not ckpt_dir:
        print(f"[ERROR] No valid checkpoint found for {scene_name}")
        return False
        
    print(f"[INFO] Using checkpoint directory: {ckpt_dir}")
    
    # Load the pose and focal models
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    
    # Initialize DataLoader to get number of images and other information
    try:
        scene_data = DataloaderParameterLearning(
            base_dir=args.data_root,
            scene_name=os.path.join("LLFF", scene_name),
            data_type="train",
            res_ratio=args.res_ratio,
            num_img_to_load=-1,
            skip=1,
            use_ndc=True
        )
    except Exception as e:
        print(f"[ERROR] Failed to load scene data: {e}")
        return False
        
    # Load pose model
    pose_model = LearnPoseGF(scene_data.N_imgs).to(device)
    pose_ckpt_path = os.path.join(ckpt_dir, "latest_pose.pth")
    
    try:
        pose_checkpoint = torch.load(pose_ckpt_path, map_location=device)
        pose_model.load_state_dict(pose_checkpoint["model_state_dict"])
        pose_model.eval()
    except Exception as e:
        print(f"[ERROR] Failed to load pose model: {e}")
        return False
    
    # Get focal values - either from model or fixed value
    focal_value = None
    if args.fixed_focal is not None:
        print(f"[INFO] Using fixed focal length: {args.fixed_focal}")
        focal_value = args.fixed_focal
    else:
        # Try to load the focal model
        focal_ckpt_path = os.path.join(ckpt_dir, "latest_focal.pth")
        focal_model, focal_value = load_focal_model(focal_ckpt_path, num_cams=1, device=device)
        
        if focal_model is None and focal_value is None:
            print(f"[ERROR] Failed to get focal information. Using default 500.0")
            focal_value = 500.0
    
    # Get image filenames
    image_files, img_dir = get_image_filenames(
        args.data_root, scene_name, args.image_dir_name
    )
    
    if not image_files:
        print(f"[WARNING] No image files found for {scene_name}")
        # Fall back to using indices as keys
        image_files = [f"{i:05d}.jpg" for i in range(scene_data.N_imgs)]
    elif len(image_files) != scene_data.N_imgs:
        print(f"[WARNING] Number of image files ({len(image_files)}) doesn't match "
              f"scene data ({scene_data.N_imgs})")
        # Truncate or extend the list to match
        if len(image_files) > scene_data.N_imgs:
            image_files = image_files[:scene_data.N_imgs]
        else:
            # Fill in the missing entries with indices
            image_files.extend([f"{i:05d}.jpg" for i in range(len(image_files), scene_data.N_imgs)])
    
    # Extract poses
    pose_dict = {}
    
    with torch.no_grad():
        for i in tqdm(range(scene_data.N_imgs), desc=f"Extracting poses for {scene_name}"):
            # Get camera-to-world matrix
            c2w = pose_model(i).cpu().numpy()
            
            # Convert to 7D pose representation [x, y, z, qx, qy, qz, qw]
            pose_7d = pose_matrix_to_quat(c2w)
            
            # Get focal length
            fx = fy = focal_value
            if focal_model is not None:
                try:
                    fx, fy = focal_model(0, H=args.resolution, W=args.resolution)
                    fx = fx.item()
                    fy = fy.item()
                except Exception as e:
                    print(f"[WARNING] Error computing focal length: {e}. Using default.")
            
            # Initialize entry
            filename = image_files[i] if i < len(image_files) else f"{i:05d}.jpg"
            pose_dict[filename] = {
                "pose": pose_7d.tolist(),
                "focal": [fx, fy]
            }
            
            # Add confidence score if requested
            if args.use_confidence_scores:
                confidence = compute_view_confidence(pose_model, i)
                pose_dict[filename]["score"] = confidence
    
    # Create output directory
    os.makedirs(args.output_dir, exist_ok=True)
    
    # Save to JSON
    output_path = os.path.join(args.output_dir, f"{scene_name}.json")
    with open(output_path, "w") as f:
        json.dump(pose_dict, f, indent=2)
        
    print(f"[SUCCESS] Saved poses for {scene_name} to {output_path}")
    return True

def main():
    """Main function."""
    args = setup_args()
    
    # Create output directory
    os.makedirs(args.output_dir, exist_ok=True)
    
    # Process specific scene or all scenes
    if args.scene:
        success = process_scene(args, args.scene)
        if not success:
            print(f"[ERROR] Failed to process scene: {args.scene}")
    else:
        # Process all scenes in the checkpoint directory
        scenes = [d for d in os.listdir(args.ckpt_dir) 
                  if os.path.isdir(os.path.join(args.ckpt_dir, d))]
        
        if not scenes:
            print(f"[ERROR] No scenes found in checkpoint directory: {args.ckpt_dir}")
            return
            
        print(f"[INFO] Found {len(scenes)} scenes to process: {', '.join(scenes)}")
        
        success_count = 0
        for scene in scenes:
            # Skip the LLFF directory if it exists at top level
            if scene.lower() == "llff":
                continue
                
            if process_scene(args, scene):
                success_count += 1
                
        print(f"\n[COMPLETE] Successfully processed {success_count}/{len(scenes)} scenes")

if __name__ == "__main__":
    main()