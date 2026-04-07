"""
Face Landmark to Pose Estimation
--------------------------------
This script processes images of celebrities, detects facial landmarks,
and estimates 3D camera poses using PnP algorithm.

Usage:
    python face_landmarks_to_pose.py --input_dir "data2/raw" --output_dir "data2/poses"

Dependencies:
    pip install opencv-python mediapipe numpy scipy tqdm
"""

import os
import cv2
import numpy as np
import mediapipe as mp
import argparse
from scipy.spatial.transform import Rotation
from tqdm import tqdm
import multiprocessing
from pathlib import Path
import logging

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

# MediaPipe Face Mesh initialization
mp_face_mesh = mp.solutions.face_mesh
mp_drawing = mp.solutions.drawing_utils

# 3D facial model - key landmark points in a frontal pose (in mm)
# These are approximations based on an average face model
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

def estimate_pose(image_path):
    """
    Detect landmarks and estimate 3D pose from a single image.
    
    Args:
        image_path: Path to the input image
        
    Returns:
        numpy.ndarray: 7D pose vector [tx, ty, tz, qx, qy, qz, qw] or None if estimation fails
    """
    # Read image
    image = cv2.imread(image_path)
    if image is None:
        logger.warning(f"Failed to read image: {image_path}")
        return None
        
    # Get image dimensions
    h, w, _ = image.shape
    
    # Convert to RGB for MediaPipe
    rgb = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
    
    # Process with MediaPipe
    results = face_mesh.process(rgb)
    
    # Check if face was detected
    if not results.multi_face_landmarks:
        logger.warning(f"No face detected in {image_path}")
        return None
        
    # Get landmarks
    landmarks = results.multi_face_landmarks[0]
    
    # Extract the specific landmarks needed for PnP
    image_points = np.array([
        (landmarks.landmark[MP_INDEXES[k]].x * w, landmarks.landmark[MP_INDEXES[k]].y * h)
        for k in MP_INDEXES
    ], dtype=np.float32)
    
    # Create 3D model points array
    model_points = np.array([FACE_3D_MODEL[k] for k in MP_INDEXES], dtype=np.float32)
    
    # Camera intrinsics
    focal_length = w  # A reasonable approximation
    center = (w / 2, h / 2)
    camera_matrix = np.array([
        [focal_length, 0, center[0]],
        [0, focal_length, center[1]],
        [0, 0, 1]
    ], dtype=np.float32)
    
    # Assume no lens distortion
    dist_coeffs = np.zeros((4, 1))
    
    # Solve for pose
    try:
        success, rvec, tvec = cv2.solvePnP(
            model_points, image_points, camera_matrix, dist_coeffs
        )
        
        if not success:
            logger.warning(f"PnP solver failed for {image_path}")
            return None
            
        # Convert rotation vector to quaternion
        rot_matrix, _ = cv2.Rodrigues(rvec)
        quat = Rotation.from_matrix(rot_matrix).as_quat()  # x, y, z, w
        
        # Create 7D pose vector [tx, ty, tz, qx, qy, qz, qw]
        pose = np.concatenate([tvec.flatten(), quat])
        
        return pose
    except Exception as e:
        logger.error(f"Error during pose estimation for {image_path}: {str(e)}")
        return None

# These functions are now integrated into the estimate_pose function

def visualize_pose(image_path, pose, output_path=None):
    """
    Visualize the estimated pose by projecting 3D axes onto the image.
    
    Args:
        image_path: Path to the input image
        pose: 7D pose vector [tx, ty, tz, qx, qy, qz, qw]
        output_path: Path to save the visualized image
    """
    img = cv2.imread(image_path)
    if img is None:
        return None
        
    h, w, _ = img.shape
    
    # Extract translation and rotation from pose
    tvec = pose[:3].reshape(3, 1)
    quat = pose[3:]
    
    # Convert quaternion back to rotation vector
    rot = Rotation.from_quat(quat)
    rot_matrix = rot.as_matrix()
    rvec, _ = cv2.Rodrigues(rot_matrix)
    
    # Camera matrix
    focal_length = w
    camera_matrix = np.array([
        [focal_length, 0, w / 2],
        [0, focal_length, h / 2],
        [0, 0, 1]
    ], dtype=np.float64)
    
    dist_coeffs = np.zeros((4, 1))
    
    # Draw face landmarks
    with mp_face_mesh.FaceMesh(static_image_mode=True, max_num_faces=1) as face_mesh:
        rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        results = face_mesh.process(rgb)
        if results.multi_face_landmarks:
            landmarks = results.multi_face_landmarks[0]
            for key in MP_INDEXES:
                idx = MP_INDEXES[key]
                px = int(landmarks.landmark[idx].x * w)
                py = int(landmarks.landmark[idx].y * h)
                cv2.circle(img, (px, py), 3, (0, 255, 0), -1)
    
    # Project 3D axes to image plane
    axis_length = 50.0  # length of axis in mm
    axes_3d = np.array([
        [0, 0, 0],
        [axis_length, 0, 0],  # X-axis (red)
        [0, axis_length, 0],  # Y-axis (green)
        [0, 0, axis_length]   # Z-axis (blue)
    ], dtype=np.float64)
    
    # Project 3D points to the image plane
    axes_2d, _ = cv2.projectPoints(
        axes_3d, rvec, tvec, camera_matrix, dist_coeffs
    )
    
    # Draw the axes
    origin = tuple(map(int, axes_2d[0].ravel()))
    x_point = tuple(map(int, axes_2d[1].ravel()))
    y_point = tuple(map(int, axes_2d[2].ravel()))
    z_point = tuple(map(int, axes_2d[3].ravel()))
    
    cv2.line(img, origin, x_point, (0, 0, 255), 3)  # X-axis: red
    cv2.line(img, origin, y_point, (0, 255, 0), 3)  # Y-axis: green
    cv2.line(img, origin, z_point, (255, 0, 0), 3)  # Z-axis: blue
    
    # Add pose information text
    info_text = [
        f"Tx: {tvec[0][0]:.2f}",
        f"Ty: {tvec[1][0]:.2f}",
        f"Tz: {tvec[2][0]:.2f}",
        f"Qx: {quat[0]:.2f}",
        f"Qy: {quat[1]:.2f}",
        f"Qz: {quat[2]:.2f}",
        f"Qw: {quat[3]:.2f}"
    ]
    
    for i, txt in enumerate(info_text):
        cv2.putText(img, txt, (10, 30 + i * 30), 
                   cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
    
    if output_path:
        os.makedirs(os.path.dirname(output_path), exist_ok=True)
        cv2.imwrite(output_path, img)
        return True
    else:
        return img

def process_directory(input_dir, output_dir, visualize=True):
    """
    Process all images in the input directory structure.
    
    Args:
        input_dir: Root directory containing celebrity subdirectories
        output_dir: Output directory for pose vectors
        visualize: Whether to create visualization images
    """
    # Make sure output directory exists
    os.makedirs(output_dir, exist_ok=True)
    
    # Tracking failed images
    failed = []
    
    # For each identity (subdirectory)
    for identity in sorted(os.listdir(input_dir)):
        identity_dir = os.path.join(input_dir, identity)
        
        # Skip if not a directory
        if not os.path.isdir(identity_dir):
            continue
            
        # Create output directory for this identity
        identity_output_dir = os.path.join(output_dir, identity)
        os.makedirs(identity_output_dir, exist_ok=True)
        
        # Create visualization directory if needed
        if visualize:
            vis_dir = os.path.join(output_dir + "_viz", identity)
            os.makedirs(vis_dir, exist_ok=True)
        
        # Process each image in the identity directory
        image_files = [f for f in sorted(os.listdir(identity_dir)) 
                      if f.lower().endswith(('.jpg', '.jpeg', '.png'))]
        
        for fname in tqdm(image_files, desc=f"Processing {identity}"):
            image_path = os.path.join(identity_dir, fname)
            
            # Estimate pose
            pose = estimate_pose(image_path)
            
            if pose is not None:
                # Save pose as .npy file
                out_name = os.path.splitext(fname)[0] + ".npy"
                np.save(os.path.join(identity_output_dir, out_name), pose)
                
                # Create visualization if requested
                if visualize:
                    vis_path = os.path.join(vis_dir, os.path.splitext(fname)[0] + "_pose.jpg")
                    visualize_pose(image_path, pose, vis_path)
            else:
                failed.append((identity, fname))
    
    # Report results
    logger.info(f"✅ Processing completed. Failed: {len(failed)} images.")
    if failed:
        logger.info("❌ Examples of failed images:")
        for entry in failed[:5]:
            logger.info(f"    {entry[0]}/{entry[1]}")
            
    return failed
def main():
    parser = argparse.ArgumentParser(description="Estimate 3D face pose from images using facial landmarks")
    parser.add_argument(
        "--input_dir", 
        default="data/raw", 
        help="Input directory containing celebrity subdirectories (default: data2/raw)"
    )
    parser.add_argument(
        "--output_dir", 
        default="data/poses", 
        help="Output directory for pose vectors (default: data2/poses)"
    )
    parser.add_argument("--visualize", action="store_true", help="Create visualization images")
    args = parser.parse_args()

    # Resolve full paths for robustness
    RAW_DATA_DIR = os.path.abspath(args.input_dir)
    POSE_OUTPUT_DIR = os.path.abspath(args.output_dir)

    # Make sure output directory exists
    os.makedirs(POSE_OUTPUT_DIR, exist_ok=True)

    logger.info(f"📸 Starting pose estimation for images in: {RAW_DATA_DIR}")
    logger.info(f"📤 Output will be saved to: {POSE_OUTPUT_DIR}")

    # Initialize MediaPipe Face Mesh
    with mp_face_mesh.FaceMesh(static_image_mode=True, max_num_faces=1, refine_landmarks=True) as face_mesh_instance:
        global face_mesh
        face_mesh = face_mesh_instance

        # Process all images
        failed = process_directory(RAW_DATA_DIR, POSE_OUTPUT_DIR, args.visualize)

    # Print summary
    print(f"✅ Done. Failed: {len(failed)} images.")
    if failed:
        print("❌ Examples of failures:")
        for entry in failed[:5]:
            print("   ", entry)

    logger.info("✅ Pose estimation completed")


if __name__ == "__main__":
    main()