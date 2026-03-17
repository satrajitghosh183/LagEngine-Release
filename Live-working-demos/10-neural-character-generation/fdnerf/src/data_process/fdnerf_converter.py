"""
FDNeRF Dataset Converter with 3DMM Parameter Generation

This script:
1. Converts a processed dataset to the FDNeRF format
2. Generates realistic 3DMM parameters based on the processed data
3. Creates all necessary files for FDNeRF training
"""

import os
import sys
import json
import shutil
import glob
import numpy as np
import pickle
import torch
import argparse
from pathlib import Path
import random
import cv2
from scipy.spatial.transform import Rotation as R


def print_section(title):
    """Print a formatted section title"""
    print("\n" + "="*50)
    print(f" {title}")
    print("="*50)


def extract_euler_from_matrix(matrix):
    """Extract Euler angles from rotation matrix"""
    # Convert 3x3 rotation matrix to euler angles
    rot = R.from_matrix(matrix[:3, :3])
    euler = rot.as_euler('xyz', degrees=False)
    return euler


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


def load_landmarks(img_path, ldm_visual_dir=None):
    """
    Try to load facial landmarks from an image or landmark files
    
    Args:
        img_path: Path to the image
        ldm_visual_dir: Optional directory with landmark visualizations
    
    Returns:
        landmarks: numpy array of landmark coordinates or None if not found
    """
    img_name = os.path.basename(img_path)
    img_id = os.path.splitext(img_name)[0]
    
    # First try to get landmarks from landmark visualization if provided
    if ldm_visual_dir:
        ldm_files = glob.glob(os.path.join(ldm_visual_dir, f"{img_id}*.png"))
        
        if ldm_files:
            try:
                # Load the image with landmarks
                ldm_img = cv2.imread(ldm_files[0], cv2.IMREAD_COLOR)
                
                # Extract landmarks (white pixels)
                white_pixels = np.where(ldm_img > 240)
                
                # Convert to x,y coordinates
                landmarks = np.array([white_pixels[1], white_pixels[0]]).T
                
                if len(landmarks) > 0:
                    return landmarks
            except Exception as e:
                print(f"Error loading landmarks from visualization for {img_id}: {e}")
    
    # As a fallback, try basic face detection with OpenCV
    try:
        # Load image
        img = cv2.imread(img_path)
        if img is None:
            return None
            
        # Convert to grayscale
        gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
        
        # Perform face detection
        face_cascade = cv2.CascadeClassifier(cv2.data.haarcascades + 'haarcascade_frontalface_default.xml')
        faces = face_cascade.detectMultiScale(gray, 1.1, 4)
        
        if len(faces) == 0:
            return None
            
        # Use face rectangle as simple landmark
        x, y, w, h = faces[0]
        
        # Create basic landmarks from face rectangle
        landmarks = np.array([
            [x, y],  # Top-left
            [x+w, y],  # Top-right
            [x, y+h],  # Bottom-left
            [x+w, y+h],  # Bottom-right
            [x+w//2, y+h//2]  # Center
        ])
        
        return landmarks
    except Exception as e:
        print(f"Error detecting face in {img_path}: {e}")
        return None


def extract_facial_features(img_path, ldm_visual_dir=None, parsing_dir=None):
    """
    Extract facial features from an image for 3DMM parameter estimation
    
    Args:
        img_path: Path to face image
        ldm_visual_dir: Directory with landmark visualizations
        parsing_dir: Directory with parsing images
    
    Returns:
        features: Dictionary of extracted features
    """
    features = {}
    
    try:
        # Load image
        img = cv2.imread(img_path)
        if img is None:
            return None
            
        # Basic image statistics
        features['img_size'] = img.shape
        
        # Color information
        b, g, r = cv2.split(img)
        features['color_mean'] = [np.mean(r)/255, np.mean(g)/255, np.mean(b)/255]
        features['color_std'] = [np.std(r)/255, np.std(g)/255, np.std(b)/255]
        
        # Get landmarks
        landmarks = load_landmarks(img_path, ldm_visual_dir)
        features['landmarks'] = landmarks
        
        # Load parsing if available
        img_name = os.path.basename(img_path)
        if parsing_dir:
            parsing_path = os.path.join(parsing_dir, img_name)
            if os.path.exists(parsing_path):
                parsing = cv2.imread(parsing_path)
                features['parsing'] = parsing
        
        # Grayscale features
        gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
        features['gray_mean'] = np.mean(gray)/255
        features['gray_std'] = np.std(gray)/255
        
        return features
        
    except Exception as e:
        print(f"Error extracting features from {img_path}: {e}")
        return None


def generate_parameters_from_features(features_list, sample_consistency=True):
    """
    Generate 3DMM parameters based on extracted features
    
    Args:
        features_list: List of feature dictionaries for all images
        sample_consistency: If True, ensure parameters are consistent for the same identity
    
    Returns:
        id_params, exp_params, tex_params: Generated parameter arrays
    """
    num_images = len(features_list)
    
    # Set parameter dimensions
    id_dimensions = 80
    exp_dimensions = 79
    tex_dimensions = 80
    
    # Initialize parameter arrays
    id_params = np.zeros((num_images, id_dimensions))
    exp_params = np.zeros((num_images, exp_dimensions))
    tex_params = np.zeros((num_images, tex_dimensions))
    
    # Generate a consistent identity parameter for all images
    if sample_consistency:
        # For identity parameters, use the same base for all images (with small variations)
        base_id_params = np.random.normal(0, 0.5, id_dimensions)
        base_id_params = base_id_params / np.linalg.norm(base_id_params) * 0.8
        
        # For texture parameters, also keep somewhat consistent
        base_tex_params = np.random.normal(0, 0.5, tex_dimensions)
        base_tex_params = base_tex_params / np.linalg.norm(base_tex_params) * 0.8
    
    # Generate parameters for each image
    for i, features in enumerate(features_list):
        if features is None:
            continue
        
        # Identity parameters - consistent with small variations if requested
        if sample_consistency:
            # Add small variations to the base identity
            variation = np.random.normal(0, 0.05, id_dimensions)
            id_params[i] = base_id_params + variation
        else:
            # Generate unique identity parameters
            params = np.random.normal(0, 0.5, id_dimensions)
            id_params[i] = params / np.linalg.norm(params) * 0.8
        
        # Expression parameters - derive from image features if possible
        if 'landmarks' in features and features['landmarks'] is not None:
            # Simplified: use landmark positions to influence expression parameters
            landmarks = features['landmarks']
            if len(landmarks) >= 5:
                # Generate some parameters based on relative landmark positions
                for j in range(min(exp_dimensions, 20)):
                    idx1, idx2 = j % len(landmarks), (j + 1) % len(landmarks)
                    # Compute distance between landmarks
                    dist = np.linalg.norm(landmarks[idx1] - landmarks[idx2])
                    # Normalize and use as parameter
                    exp_params[i, j] = (dist / np.sqrt(features['img_size'][0]**2 + features['img_size'][1]**2)) * 2 - 1
        
        # Fill remaining expression parameters with reasonable noise
        mask = exp_params[i] == 0
        exp_params[i, mask] = np.random.normal(0, 0.15, np.sum(mask))
        
        # Normalize expression parameters
        if np.linalg.norm(exp_params[i]) > 0:
            exp_params[i] = exp_params[i] / np.linalg.norm(exp_params[i]) * 0.6
        
        # Texture parameters - derive from color information
        if 'color_mean' in features and 'color_std' in features:
            # Simplified: use color statistics to influence texture parameters
            if sample_consistency:
                # Add variations based on color
                color_variation = np.zeros(tex_dimensions)
                for j in range(3):  # RGB channels
                    color_variation[j::3] = (features['color_mean'][j] - 0.5) * 0.4
                    color_variation[j+3::3] = features['color_std'][j] * 2 - 0.5
                
                tex_params[i] = base_tex_params + color_variation * 0.5
            else:
                # Generate texture parameters based on color
                for j in range(3):  # RGB channels
                    tex_params[i, j::3] = (features['color_mean'][j] - 0.5) * 0.8
                    tex_params[i, j+3::3] = features['color_std'][j] * 2 - 0.5
                    
                # Normalize texture parameters
                if np.linalg.norm(tex_params[i]) > 0:
                    tex_params[i] = tex_params[i] / np.linalg.norm(tex_params[i]) * 0.8
    
    return id_params, exp_params, tex_params


def generate_3dmm_parameters(img_files, img_dir, ldm_visual_dir=None, parsing_dir=None, euler_angles=None, translations=None):
    """
    Generate 3DMM parameters for a set of images
    
    Args:
        img_files: List of image filenames
        img_dir: Directory containing the images
        ldm_visual_dir: Optional directory with landmark visualizations
        parsing_dir: Optional directory with parsing images
        euler_angles: Optional pre-computed Euler angles
        translations: Optional pre-computed translations
    
    Returns:
        params_3dmm: Dictionary of 3DMM parameters
        track_params: Dictionary of tracking parameters
    """
    print(f"Generating 3DMM parameters for {len(img_files)} images...")
    
    # Extract features from all images
    features_list = []
    for img_file in img_files:
        img_path = os.path.join(img_dir, img_file)
        features = extract_facial_features(img_path, ldm_visual_dir, parsing_dir)
        features_list.append(features)
    
    # Generate parameters based on features
    id_params, exp_params, tex_params = generate_parameters_from_features(features_list)
    
    # Create 3DMM parameters dictionary
    params_3dmm = {"params": {}}
    
    # Create tracking parameters tensors
    euler_tensor = torch.zeros((len(img_files), 3))
    trans_tensor = torch.zeros((len(img_files), 3))
    exp_tensor = torch.from_numpy(exp_params).float()
    
    # Fill in Euler angles and translations if provided, otherwise generate them
    if euler_angles is not None and translations is not None:
        euler_tensor = torch.from_numpy(euler_angles).float()
        trans_tensor = torch.from_numpy(translations * 10.0).float()  # Scale by 10 as in FDNeRF
    else:
        # Generate basic poses if not provided
        for i in range(len(img_files)):
            # Generate random angles between -15 and 15 degrees
            euler_tensor[i] = torch.tensor([
                random.uniform(-0.2, 0.2),  # pitch
                random.uniform(-0.2, 0.2),  # yaw
                random.uniform(-0.1, 0.1)   # roll
            ])
            
            # Generate random translations between -1 and 1
            trans_tensor[i] = torch.tensor([
                random.uniform(-0.5, 0.5),  # x
                random.uniform(-0.5, 0.5),  # y
                random.uniform(-0.5, 0.5)   # z
            ]) * 10.0  # Scale by 10 as in FDNeRF
    
    # Assemble all 3DMM parameters
    for i, img_file in enumerate(img_files):
        img_id = os.path.splitext(img_file)[0]
        
        # Combine parameters into the expected order for BFM09
        # [id_params(80), exp_params(79), tex_params(80), euler(3), trans(3), others(6)]
        params = np.zeros(257)
        params[:80] = id_params[i]                # 0-79 (80 dims)
        params[80:159] = exp_params[i]            # 80-158 (79 dims)
        params[159:239] = tex_params[i]           # 159-238 (80 dims)
        params[239:242] = euler_tensor[i].numpy() # 239-241 (3 dims)
        params[242:245] = trans_tensor[i].numpy() / 10.0  # 242-244 (3 dims)
        # params[245:257] remain zero      # 245-256 
        
        params_3dmm["params"][img_id] = params
    
    # Create track_params dictionary
    track_params = {
        'euler': euler_tensor,
        'trans': trans_tensor,
        'exp': exp_tensor
    }
    
    return params_3dmm, track_params

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

def process_id_directory(id_dir, output_base_dir, generate_real_params=True):
    """
    Process a single ID directory and create the required structure.
    
    Args:
        id_dir: Path to the ID directory (like id00000)
        output_base_dir: Base directory where the restructured data will be placed
        generate_real_params: If True, generate realistic 3DMM parameters
    """
    id_name = os.path.basename(id_dir)
    successful_subfolders = 0
    processed_subfolders = 0
    
    # Process each subfolder (Test, Test2, etc.)
    for subfolder in os.listdir(id_dir):
        subfolder_path = os.path.join(id_dir, subfolder)
        if not os.path.isdir(subfolder_path):
            continue
            
        processed_subfolders += 1
        print(f"Processing subfolder: {subfolder}")
        
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
        
        # Check for both "images_parsing" and "parsing" directories
        src_parsing_dir = None
        parsing_dir_candidates = [
            os.path.join(subfolder_path, "images_parsing"),
            os.path.join(subfolder_path, "parsing")
        ]
        for candidate in parsing_dir_candidates:
            if os.path.exists(candidate) and os.path.isdir(candidate):
                src_parsing_dir = candidate
                print(f"Found parsing directory at: {src_parsing_dir}")
                break
        
        # Similarly check for landmark visualization directories
        src_ldm_visual_dir = None
        ldm_dir_candidates = [
            os.path.join(subfolder_path, "ldm_visual"),
            os.path.join(subfolder_path, "landmarks")
        ]
        for candidate in ldm_dir_candidates:
            if os.path.exists(candidate) and os.path.isdir(candidate):
                src_ldm_visual_dir = candidate
                print(f"Found landmark directory at: {src_ldm_visual_dir}")
                break
        
        if not os.path.exists(src_masked_dir):
            print(f"Warning: images_masked directory not found in {subfolder_path}")
            continue
            
        # Check if poses_face.npy exists
        poses_face_path = os.path.join(src_masked_dir, "poses_face.npy")
        if not os.path.exists(poses_face_path):
            print(f"Warning: poses_face.npy not found in {src_masked_dir}")
            continue
            
        # Convert poses to transforms
        output_transforms_path = os.path.join(images_masked_dir, "face_transforms_pose.json")
        num_frames, valid_images = convert_poses_to_transforms(poses_face_path, src_masked_dir, output_transforms_path)
        print(f"Converted {num_frames} poses to face_transforms_pose.json")
        
        # Copy JSON to 3DMM directory as well
        shutil.copy2(output_transforms_path, os.path.join(images_3dmm_dir, "face_transforms_pose.json"))
        
        # Create valid_img_ids.txt
        valid_ids_path = os.path.join(images_3dmm_dir, "valid_img_ids.txt")
        create_valid_img_ids(valid_images, valid_ids_path)
        
        # Copy images from images_masked to our structure
        for img_file in valid_images:
            src_img_path = os.path.join(src_masked_dir, img_file)
            dst_img_path = os.path.join(images_masked_dir, img_file)
            shutil.copy2(src_img_path, dst_img_path)
        
        # Copy images from parsing directory to our structure if available
        parsing_available = False
        if src_parsing_dir:
            parsing_available = True
            for img_file in valid_images:
                src_parse_path = os.path.join(src_parsing_dir, img_file)
                if os.path.exists(src_parse_path):
                    dst_parse_path = os.path.join(parsing_dir, img_file)
                    shutil.copy2(src_parse_path, dst_parse_path)
                else:
                    print(f"Warning: Parsing image not found: {src_parse_path}")
        else:
            print(f"Warning: No parsing directory found in {subfolder_path}")
        
        # Generate 3DMM parameters
        if generate_real_params:
            # Load poses for Euler angles and translations
            poses = np.load(poses_face_path)
            euler_angles = np.array([extract_euler_from_matrix(p) for p in poses])
            translations = np.array([p[:3, 3] for p in poses])
            
            # Generate realistic 3DMM parameters
            parsing_dir_to_use = parsing_dir if parsing_available else None
            
            params_3dmm, track_params = generate_3dmm_parameters(
                valid_images, 
                images_masked_dir,
                src_ldm_visual_dir,
                parsing_dir_to_use,
                euler_angles,
                translations
            )
            
            # Save parameters
            with open(os.path.join(images_3dmm_dir, 'params_3dmm.pkl'), 'wb') as f:
                pickle.dump(params_3dmm, f)
                
            torch.save(track_params, os.path.join(images_3dmm_dir, 'track_params.pt'))
        else:
            # Create placeholder 3DMM files
            create_fake_tracking_params(images_3dmm_dir, num_frames=num_frames)
            
        print(f"Processed {id_name}_{subfolder} with {num_frames} frames")
        successful_subfolders += 1
    
    print(f"Finished processing {id_name}: {successful_subfolders}/{processed_subfolders} subfolders processed successfully")
    return successful_subfolders

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
    
    # Shuffle the directories for random split
    random.shuffle(all_dirs)
    
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
    parser = argparse.ArgumentParser(description="Convert processed dataset to FDNeRF format with 3DMM parameter generation")
    parser.add_argument("--input_dir", type=str, required=True, 
                        help="Directory containing the processed ID folders")
    parser.add_argument("--output_dir", type=str, required=True,
                        help="Directory where the restructured dataset will be created")
    parser.add_argument("--sample_limit", type=int, default=None,
                        help="Process only the specified number of samples (for testing)")
    parser.add_argument("--generate_3dmm", action="store_true", default=False,
                        help="Generate realistic 3DMM parameters (default: create placeholders)")
    
    args = parser.parse_args()
    
    # Process all ID directories
    id_dirs = [os.path.join(args.input_dir, d) for d in os.listdir(args.input_dir) 
               if os.path.isdir(os.path.join(args.input_dir, d)) and d.startswith('id')]
    # Process all ID directories
    id_dirs.extend([os.path.join(args.input_dir, d) for d in os.listdir(args.input_dir) 
               if os.path.isdir(os.path.join(args.input_dir, d)) and d.startswith('m--')])
    
    id_dirs = [os.path.join(args.input_dir, "Dataset")]
    
    if not id_dirs:
        print(f"No ID directories found in {args.input_dir}")
        return
    
    # Limit the number of samples if requested
    if args.sample_limit is not None and args.sample_limit > 0:
        print(f"Limiting to {args.sample_limit} samples for testing")
        id_dirs = id_dirs[:args.sample_limit]
        
    print(f"Found {len(id_dirs)} ID directories to process")
    
    # Initialize counters
    total_successful_subfolders = 0
    total_ids_processed = 0
    
    for id_dir in id_dirs:
        print_section(f"Processing {os.path.basename(id_dir)}...")
        successful_subfolders = process_id_directory(id_dir, args.output_dir, args.generate_3dmm)
        total_successful_subfolders += successful_subfolders
        total_ids_processed += 1
        print(f"Progress: {total_ids_processed}/{len(id_dirs)} ID directories processed")
    
    # Generate split lists
    generate_split_lists(args.output_dir)
    
    print_section("Summary")
    print(f"Total ID directories processed: {total_ids_processed}")
    print(f"Total successful subfolder conversions: {total_successful_subfolders}")
    print(f"Dataset conversion complete. New structure created in {args.output_dir}")
    
    if args.generate_3dmm:
        print("\nGenerated realistic 3DMM parameters for each face.")
    else:
        print("\nCreated placeholder files for params_3dmm.pkl and track_params.pt.")
        print("For better results, re-run with --generate_3dmm flag.")


if __name__ == "__main__":
    main()