#!/usr/bin/env python3
# Preprocessing code for multiface dataset
import os
import random
import shutil
from pathlib import Path

def sample_multiface_dataset(source_root, target_root, num_samples=3):
    """
    Sample images from the multiface dataset and organize them into a new directory structure.
    
    Args:
        source_root (str): Path to the root of the source dataset
        target_root (str): Path to the root where processed images will be stored
        num_samples (int): Number of images to sample from each lowest-level directory
    """
    # Create the processed root directory if it doesn't exist
    processed_dir = os.path.join(target_root, "processed")
    os.makedirs(processed_dir, exist_ok=True)
    
    # Iterate through each subject directory (m--...)
    for subject_dir in os.listdir(source_root):
        if not subject_dir.startswith("m--"):
            continue
        
        subject_path = os.path.join(source_root, subject_dir)
        if not os.path.isdir(subject_path):
            continue
        
        # Create corresponding subject directory in processed folder
        processed_subject_dir = os.path.join(processed_dir, subject_dir)
        os.makedirs(processed_subject_dir, exist_ok=True)
        
        # Find the images directory
        images_dir = os.path.join(subject_path, "images")
        if not os.path.exists(images_dir):
            print(f"Warning: No images directory found in {subject_path}")
            continue
        
        # Iterate through expression directories (E00x_...)
        for expr_dir in os.listdir(images_dir):
            if not expr_dir.startswith("E00"):
                continue
            
            expr_path = os.path.join(images_dir, expr_dir)
            if not os.path.isdir(expr_path):
                continue
            
            # Create corresponding expression directory in processed folder
            processed_expr_dir = os.path.join(processed_subject_dir, expr_dir)
            os.makedirs(processed_expr_dir, exist_ok=True)
            
            # Iterate through view directories (400xxx)
            for view_dir in os.listdir(expr_path):
                view_path = os.path.join(expr_path, view_dir)
                if not os.path.isdir(view_path):
                    continue
                
                # Get all image files in this directory
                image_files = [f for f in os.listdir(view_path) if f.lower().endswith(('.png', '.jpg', '.jpeg'))]
                
                if not image_files:
                    print(f"Warning: No image files found in {view_path}")
                    continue
                
                # Sample num_samples images (or all if fewer are available)
                num_to_sample = min(num_samples, len(image_files))
                sampled_images = random.sample(image_files, num_to_sample)
                
                # Copy sampled images to the processed directory
                for img in sampled_images:
                    src_path = os.path.join(view_path, img)
                    dst_path = os.path.join(processed_expr_dir, f"{view_dir}_{img}")
                    shutil.copy2(src_path, dst_path)
                    print(f"Copied {src_path} to {dst_path}")

if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser(description="Sample images from multiface dataset")
    parser.add_argument("--source", default="/scratch/network/hy4522/FDNeRF_data/multiface", help="Path to the root of the multiface dataset")
    parser.add_argument("--target", default="/scratch/network/hy4522/FDNeRF_data/multiface", help="Path where processed directory will be created")
    parser.add_argument("--samples", type=int, default=3, help="Number of images to sample from each view directory")
    
    args = parser.parse_args()
    
    print(f"Sampling {args.samples} images from each view directory in {args.source}")
    print(f"Saving processed images to {os.path.join(args.target, 'processed')}")
    
    sample_multiface_dataset(args.source, args.target, args.samples)
    
    print("Sampling complete!")