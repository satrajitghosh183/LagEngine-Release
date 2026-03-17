import os
import json
import shutil
import numpy as np
import pickle
import torch
import argparse
from pathlib import Path
import random

def convert_poses_to_transforms(poses_face_path, image_dir, output_json_path):
    """
    Convert poses_face.npy to face_transforms_pose.json format.
    
    Args:
        poses_face_path: Path to the poses_face.npy file
        image_dir: Directory containing the images
        output_json_path: Path where the face_transforms_pose.json will be saved
    """
    # Load the poses matrix
    poses = np.load(poses_face_path)
    
    # Create the basic structure for the JSON file
    transform_data = {
        "focal_len": 1000,  # Default values from your example
        "cx": 128,
        "cy": 128,
        "near": 8,
        "far": 26,
        "frames": []
    }
    
    # Get image filenames
    image_files = [f for f in os.listdir(image_dir) if f.endswith(('.png', '.jpg')) 
                  and not f == 'poses_face.npy']
    image_files.sort()  # Ensure consistent ordering
    
    # Verify we have at least as many poses as images
    if len(image_files) > len(poses):
        print(f"Warning: More images ({len(image_files)}) than poses ({len(poses)}). Only processing first {len(poses)} images.")
        image_files = image_files[:len(poses)]
    
    # Convert each pose to the required format
    for i, image_file in enumerate(image_files):
        # Get base name without extension
        img_id = os.path.splitext(image_file)[0]
        
        # Extract transformation matrix (ensure it's 4x4)
        pose = poses[i]
        if pose.shape != (4, 4):
            # If not 4x4, try to reshape or pad it
            if pose.size == 16:  # If it's flattened
                pose = pose.reshape(4, 4)
            else:
                print(f"Warning: Pose matrix for {img_id} has unexpected shape {pose.shape}")
                # Create a default pose if necessary
                pose = np.eye(4)
        
        # Create frame entry
        frame = {
            "img_id": img_id,
            "transform_matrix": pose.tolist()
        }
        
        transform_data["frames"].append(frame)
    
    # Save to JSON file
    os.makedirs(os.path.dirname(output_json_path), exist_ok=True)
    with open(output_json_path, 'w') as f:
        json.dump(transform_data, f, indent=2)
    
    return len(transform_data["frames"]), image_files[:len(poses)]

def create_valid_img_ids(image_files, output_path):
    """
    Create valid_img_ids.txt file.
    
    Args:
        image_files: List of image filenames
        output_path: Path to save valid_img_ids.txt
    """
    with open(output_path, 'w') as f:
        for img_file in image_files:
            img_id = os.path.splitext(img_file)[0]
            f.write(f"{img_id}\n")
    
    print(f"Created valid_img_ids.txt with {len(image_files)} entries")
def create_fake_tracking_params(output_dir, num_frames):
    """Create fake tracking parameters for testing"""
    # Creating simple tensors for demonstration
    # In a real scenario, these would contain meaningful expression parameters
    euler = torch.zeros((num_frames, 3))  # Rotation angles (pitch, yaw, roll)
    trans = torch.zeros((num_frames, 3))  # Translation (x, y, z)
    exp = torch.zeros((num_frames, 79))   # Expression parameters (can adjust dimension as needed)
    
    # Add some variation to make it look like different expressions
    for i in range(num_frames):
        # Random angles between -15 and 15 degrees
        euler[i] = torch.tensor([
            random.uniform(-0.2, 0.2),  # pitch
            random.uniform(-0.2, 0.2),  # yaw
            random.uniform(-0.1, 0.1)   # roll
        ])
        
        # Random translations between -1 and 1
        trans[i] = torch.tensor([
            random.uniform(-0.5, 0.5),  # x
            random.uniform(-0.5, 0.5),  # y
            random.uniform(-0.5, 0.5)   # z
        ])
        
        # Random expression parameters (simplified)
        exp[i, :] = torch.randn(79) * 0.1
    
    # Multiply by 10 as the code divides by 10 later
    trans = trans * 10.0
    
    # Create the dict and save as torch.pt file
    tracking_params = {
        'euler': euler,
        'trans': trans,
        'exp': exp
    }
    
    torch.save(tracking_params, os.path.join(output_dir, 'track_params.pt'))

    # Also create a dummy 3DMM parameters file
    params_3dmm = {
        'params': {}
    }
    
    for i, frame_idx in enumerate(range(num_frames)):
        # Create a parameter vector that has enough dimensions
        # Typical 3DMM parameters include identity, expression, texture, etc.
        param_vector = np.zeros(257)  # Arbitrary size based on code
        
        # Set different parts of the vector for different meanings
        # 80:144 - expression parameters
        # 224:227 - angle parameters
        # 254:257 - translation parameters
        param_vector[80:144] = np.random.randn(64) * 0.1  # Expression
        param_vector[224:227] = np.random.randn(3) * 0.1  # Angle
        param_vector[254:257] = np.random.randn(3) * 0.1  # Translation
        
        params_3dmm['params'][i] = param_vector
    
    # Save as pickle
    with open(os.path.join(output_dir, 'params_3dmm.pkl'), 'wb') as f:
        pickle.dump(params_3dmm, f)
        
# def create_placeholder_3dmm_files(output_dir):
#     """
#     Create placeholder 3DMM parameter files.
    
#     Args:
#         output_dir: Directory to save the placeholder files
#     """
#     # Create a simple placeholder pickle file
#     params_3dmm_path = os.path.join(output_dir, "params_3dmm.pkl")
#     with open(params_3dmm_path, 'wb') as f:
#         pickle.dump({"placeholder": True}, f)
    
#     # Create a placeholder PyTorch tensor file
#     track_params_path = os.path.join(output_dir, "track_params.pt")
#     dummy_tensor = torch.ones(1, 1)  # Simple 1x1 tensor
#     torch.save(dummy_tensor, track_params_path)
    
#     print(f"Created placeholder 3DMM files in {output_dir}")
#     print("Warning: These are placeholder files and should be replaced with actual 3DMM parameters.")

def process_id_directory(id_dir, output_base_dir):
    """
    Process a single ID directory and create the required structure.
    
    Args:
        id_dir: Path to the ID directory (like id00000)
        output_base_dir: Base directory where the restructured data will be placed
    """
    id_name = os.path.basename(id_dir)
    
    # Process each subfolder (Test, Test2, etc.)
    for subfolder in os.listdir(id_dir):
        subfolder_path = os.path.join(id_dir, subfolder)
        if not os.path.isdir(subfolder_path):
            continue
            
        # Create output directory structure
        output_dir = os.path.join(output_base_dir, f"{id_name}_{subfolder}")
        mixexp_dir = os.path.join(output_dir, "mixexp")
        images_3dmm_dir = os.path.join(mixexp_dir, "images_3dmm")
        images_masked_dir = os.path.join(mixexp_dir, "images_masked")
        parsing_dir = os.path.join(mixexp_dir, "parsing")
        
        os.makedirs(images_3dmm_dir, exist_ok=True)
        os.makedirs(images_masked_dir, exist_ok=True)
        os.makedirs(parsing_dir, exist_ok=True)
        
        # Check if required directories exist in the source
        src_masked_dir = os.path.join(subfolder_path, "images_masked")
        src_parsing_dir = os.path.join(subfolder_path, "images_parsing")
        
        if not os.path.exists(src_masked_dir):
            print(f"Warning: images_masked directory not found in {subfolder_path}")
            continue
            
        # Check if poses_face.npy exists
        poses_face_path = os.path.join(src_masked_dir, "poses_face.npy")
        if not os.path.exists(poses_face_path):
            print(f"Warning: poses_face.npy not found in {src_masked_dir}")
            continue
            
        # Convert poses to transforms
        output_transforms_path = os.path.join(images_3dmm_dir, "face_transforms_pose.json")
        num_frames, valid_images = convert_poses_to_transforms(poses_face_path, src_masked_dir, output_transforms_path)
        print(f"Converted {num_frames} poses to face_transforms_pose.json")
        
        # Create valid_img_ids.txt
        valid_ids_path = os.path.join(images_3dmm_dir, "valid_img_ids.txt")
        create_valid_img_ids(valid_images, valid_ids_path)
        
        # Create placeholder 3DMM files
        create_fake_tracking_params(images_3dmm_dir, num_frames=num_frames)
        
        # Copy images from images_masked to our structure
        for img_file in valid_images:
            src_img_path = os.path.join(src_masked_dir, img_file)
            dst_img_path = os.path.join(images_masked_dir, img_file)
            shutil.copy2(src_img_path, dst_img_path)
        
        # Copy images from images_parsing to our structure if available
        if os.path.exists(src_parsing_dir):
            for img_file in valid_images:
                src_parse_path = os.path.join(src_parsing_dir, img_file)
                if os.path.exists(src_parse_path):
                    dst_parse_path = os.path.join(parsing_dir, img_file)
                    shutil.copy2(src_parse_path, dst_parse_path)
                else:
                    print(f"Warning: Parsing image not found: {src_parse_path}")
        else:
            print(f"Warning: images_parsing directory not found in {subfolder_path}")
            
        print(f"Processed {id_name}_{subfolder} with {num_frames} frames")

def generate_split_lists(output_base_dir):
    """
    Generate train/val/test split lists based on the restructured directories.
    
    Args:
        output_base_dir: Base directory containing the restructured data
    """
    all_dirs = [d for d in os.listdir(output_base_dir) if os.path.isdir(os.path.join(output_base_dir, d))]
    
    # Create an 80/10/10 split for train/val/test
    num_dirs = len(all_dirs)
    num_train = int(num_dirs * 0.8)
    num_val = int(num_dirs * 0.1)
    
    train_dirs = all_dirs[:num_train]
    val_dirs = all_dirs[num_train:num_train+num_val]
    test_dirs = all_dirs[num_train+num_val:]
    
    # Write the lists
    with open(os.path.join(output_base_dir, "mixwild_train.lst"), 'w') as f:
        for d in train_dirs:
            f.write(f"{d}\n")
            
    with open(os.path.join(output_base_dir, "mixwild_val.lst"), 'w') as f:
        for d in val_dirs:
            f.write(f"{d}\n")
            
    with open(os.path.join(output_base_dir, "mixwild_test.lst"), 'w') as f:
        for d in test_dirs:
            f.write(f"{d}\n")
    
    print(f"Generated split lists: train={len(train_dirs)}, val={len(val_dirs)}, test={len(test_dirs)}")

def main():
    parser = argparse.ArgumentParser(description="Convert processed dataset to FDNeRF format")
    parser.add_argument("--input_dir", type=str, required=True, 
                        help="Directory containing the processed ID folders")
    parser.add_argument("--output_dir", type=str, required=True,
                        help="Directory where the restructured dataset will be created")
    
    args = parser.parse_args()
    
    # Process all ID directories
    id_dirs = [os.path.join(args.input_dir, d) for d in os.listdir(args.input_dir) 
               if os.path.isdir(os.path.join(args.input_dir, d)) and d.startswith('id')]
    
    if not id_dirs:
        print(f"No ID directories found in {args.input_dir}")
        return
        
    print(f"Found {len(id_dirs)} ID directories to process")
    
    for id_dir in id_dirs:
        print(f"Processing {os.path.basename(id_dir)}...")
        process_id_directory(id_dir, args.output_dir)
    
    # Generate split lists
    generate_split_lists(args.output_dir)
    
    print(f"Dataset conversion complete. New structure created in {args.output_dir}")
    print("\nImportant note: The script created placeholder files for params_3dmm.pkl and track_params.pt.")
    print("These should be replaced with proper 3DMM parameters for the NeRF training to work correctly.")

if __name__ == "__main__":
    main()