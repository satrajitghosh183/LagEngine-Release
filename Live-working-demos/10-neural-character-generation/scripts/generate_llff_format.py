# generate_llff_format.py - Fixed version with proper initialization
import os
import sys
import argparse
import logging
import cv2
import numpy as np
import mediapipe as mp
from scipy.spatial.transform import Rotation
from tqdm import tqdm

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler("llff_generation.log"),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger("LLFFGenerator")

# Parse command-line arguments
parser = argparse.ArgumentParser(description="Generate LLFF format data from processed images")
parser.add_argument(
    "--subject_id", type=str, required=True,
    help="Subject ID to process (e.g., subject_01)"
)
parser.add_argument(
    "--data_folder", type=str, default="data",
    help="Base data folder (contains preprocessed/, llff/, etc.)"
)
parser.add_argument(
    "--near", type=float, default=0.1,
    help="Near bound for LLFF rendering"
)
parser.add_argument(
    "--far", type=float, default=1.0,
    help="Far bound for LLFF rendering"
)
parser.add_argument(
    "--pose", type=str, default="front",
    help="Pose to process (default: front)"
)
args = parser.parse_args()

# 3D facial model points for PnP pose estimation
FACE_3D_MODEL = {
    "nose_tip": [0.0, 0.0, 0.0],
    "chin": [0.0, -63.6, -12.5],
    "left_eye_left_corner": [-42.0, 32.0, -26.0],
    "right_eye_right_corner": [42.0, 32.0, -26.0],
    "left_mouth_corner": [-28.0, -28.9, -20.0],
    "right_mouth_corner": [28.0, -28.9, -20.0]
}

# MediaPipe indices for the corresponding 3D model points
MP_INDEXES = {
    "nose_tip": 1,
    "chin": 152,
    "left_eye_left_corner": 263,
    "right_eye_right_corner": 33,
    "left_mouth_corner": 61,
    "right_mouth_corner": 291
}

def estimate_pose(image_path, face_mesh):
    """Estimate camera pose from facial landmarks."""
    try:
        # Load image
        image = cv2.imread(image_path)
        if image is None:
            logger.warning(f"Failed to load image: {image_path}")
            return None
        
        h, w = image.shape[:2]
        
        # Convert to RGB for MediaPipe
        rgb_image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
        
        # Process with MediaPipe
        results = face_mesh.process(rgb_image)
        
        # Check if face was detected
        if not results.multi_face_landmarks:
            logger.warning(f"No face detected in {image_path}")
            return None
        
        # Get landmarks
        landmarks = results.multi_face_landmarks[0]
        
        # Extract 2D points for PnP
        image_points = np.array([
            (landmarks.landmark[MP_INDEXES[k]].x * w, landmarks.landmark[MP_INDEXES[k]].y * h)
            for k in MP_INDEXES
        ], dtype=np.float32)
        
        # Get 3D model points
        model_points = np.array([FACE_3D_MODEL[k] for k in MP_INDEXES], dtype=np.float32)
        
        # Camera matrix
        focal_length = w  # Approximate focal length
        center = (w / 2, h / 2)
        camera_matrix = np.array([
            [focal_length, 0, center[0]],
            [0, focal_length, center[1]],
            [0, 0, 1]
        ], dtype=np.float32)
        
        # Solve PnP
        dist_coeffs = np.zeros((4, 1))  # Assume no lens distortion
        success, rotation_vector, translation_vector = cv2.solvePnP(
            model_points, image_points, camera_matrix, dist_coeffs
        )
        
        if not success:
            logger.warning(f"PnP solver failed for {image_path}")
            return None
        
        # Convert rotation vector to matrix
        rotation_matrix, _ = cv2.Rodrigues(rotation_vector)
        
        # Create pose matrix
        pose = np.eye(4)
        pose[:3, :3] = rotation_matrix
        pose[:3, 3] = translation_vector.reshape(-1)
        
        # Camera parameters
        hwf = np.array([h, w, focal_length])
        
        return pose, hwf
    
    except Exception as e:
        logger.error(f"Error estimating pose from {image_path}: {e}")
        return None

def main():
    try:
        # Setup directories
        pose = args.pose
        input_dir = os.path.join(args.data_folder, "preprocessed", args.subject_id, "poses", pose)
        output_dir = os.path.join(args.data_folder, "llff", args.subject_id)
        
        # Check if input directory exists
        if not os.path.exists(input_dir):
            logger.error(f"Input directory does not exist: {input_dir}")
            return 1
            
        # Create output directory
        os.makedirs(output_dir, exist_ok=True)
        
        # Initialize MediaPipe Face Mesh
        face_mesh = mp.solutions.face_mesh.FaceMesh(
            static_image_mode=True,
            max_num_faces=1,
            refine_landmarks=True,
            min_detection_confidence=0.5
        )
        
        # Get all image files
        image_files = [f for f in os.listdir(input_dir) 
                     if f.lower().endswith(('.png', '.jpg', '.jpeg'))]
        
        if not image_files:
            logger.warning(f"No image files found in {input_dir}")
            face_mesh.close()
            return 1
            
        # Sort files for consistency
        image_files = sorted(image_files)
        logger.info(f"Found {len(image_files)} images to process")
        
        # Process each image
        all_poses = []
        
        for img_file in tqdm(image_files, desc=f"Processing {args.subject_id} - {pose}"):
            img_path = os.path.join(input_dir, img_file)
            
            # Estimate pose
            result = estimate_pose(img_path, face_mesh)
            if result is None:
                logger.warning(f"Failed to estimate pose for {img_file}")
                continue
            
            pose_matrix, hwf = result
            
            # Create LLFF format pose
            pose_3x5 = np.concatenate([pose_matrix[:3, :4].reshape(-1), hwf])
            pose_bound = np.concatenate([pose_3x5, [args.near, args.far]])
            all_poses.append(pose_bound)
        
        # Release MediaPipe resources
        face_mesh.close()
        
        if not all_poses:
            logger.error(f"No valid poses estimated for {args.subject_id} - {pose}")
            return 1
        
        # Stack and save poses_bounds.npy
        all_poses = np.stack(all_poses, axis=0)
        poses_path = os.path.join(output_dir, "poses_bounds.npy")
        np.save(poses_path, all_poses)
        logger.info(f"Saved poses_bounds.npy with {len(all_poses)} entries")
        
        # Create train/val split
        indices = np.arange(len(all_poses))
        np.random.seed(42)  # For reproducibility
        np.random.shuffle(indices)
        
        split = max(1, int(len(all_poses) * 0.875))  # 87.5% for training
        train_indices = indices[:split]
        val_indices = indices[split:]
        
        # Save to txt files
        np.savetxt(os.path.join(output_dir, "train_ids.txt"), train_indices, fmt='%d')
        np.savetxt(os.path.join(output_dir, "val_ids.txt"), val_indices, fmt='%d')
        
        logger.info(f"Created train split with {len(train_indices)} images")
        logger.info(f"Created val split with {len(val_indices)} images")
        
        logger.info(f"LLFF data generation completed for {args.subject_id}")
        return 0
    
    except Exception as e:
        logger.error(f"Unhandled exception: {e}")
        return 1

if __name__ == "__main__":
    sys.exit(main())