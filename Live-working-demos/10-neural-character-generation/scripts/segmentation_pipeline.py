# # Modified segmentation_pipeline.py
# import os
# import cv2
# import torch
# import torchvision
# import numpy as np
# from PIL import Image
# from tqdm import tqdm

# # Load DeepLabV3+ model
# model = torchvision.models.segmentation.deeplabv3_resnet101(weights=torchvision.models.segmentation.DeepLabV3_ResNet101_Weights.DEFAULT)
# model.eval()

# PERSON_CLASS = 15  # COCO person class

# def segment_and_save(image_np, save_path, preview=False):
#     # Convert to PIL Image
#     image = Image.fromarray(cv2.cvtColor(image_np, cv2.COLOR_BGR2RGB))
#     orig_w, orig_h = image.size

#     preprocess = torchvision.transforms.Compose([
#         torchvision.transforms.ToTensor(),
#         torchvision.transforms.Normalize(mean=[0.485, 0.456, 0.406],
#                                          std=[0.229, 0.224, 0.225])
#     ])
#     input_tensor = preprocess(image).unsqueeze(0)

#     with torch.no_grad():
#         output = model(input_tensor)["out"][0]
#         output_predictions = output.argmax(0).byte().cpu().numpy()

#     mask = (output_predictions == PERSON_CLASS).astype(np.uint8) * 255
#     mask = cv2.dilate(mask, np.ones((5, 5), np.uint8), iterations=1)
#     mask = cv2.GaussianBlur(mask, (7, 7), 0)

#     # Add alpha to original
#     rgba = cv2.cvtColor(image_np, cv2.COLOR_BGR2BGRA)
#     rgba[:, :, 3] = mask
    
#     # Ensure output directory exists
#     os.makedirs(os.path.dirname(save_path), exist_ok=True)
#     cv2.imwrite(save_path, rgba)

#     print(f"Saved segmented image to {save_path}")

#     if preview:
#         cv2.imshow("Segmented", rgba)
#         cv2.waitKey(50)


# def process_directory(input_dir, output_dir, preview=False):
#     """Process all JPG files in a directory"""
#     if not os.path.exists(input_dir):
#         print(f"Input directory does not exist: {input_dir}")
#         return 0
        
#     os.makedirs(output_dir, exist_ok=True)
    
#     # Get all JPG files
#     image_files = [f for f in os.listdir(input_dir) if f.lower().endswith(('.jpg', '.jpeg'))]
#     if not image_files:
#         print(f"No JPG files found in {input_dir}")
#         return 0
    
#     print(f"Found {len(image_files)} JPG files in {input_dir}")
#     processed = 0
    
#     for img_file in tqdm(image_files, desc=f"Processing images"):
#         img_path = os.path.join(input_dir, img_file)
#         save_path = os.path.join(output_dir, img_file.replace('.jpg', '.png').replace('.jpeg', '.png'))
        
#         # Skip if already processed
#         if os.path.exists(save_path):
#             processed += 1
#             continue
        
#         try:
#             image_np = cv2.imread(img_path)
#             if image_np is None:
#                 print(f"Failed to load image: {img_path}")
#                 continue
                
#             segment_and_save(image_np, save_path, preview)
#             processed += 1
#         except Exception as e:
#             print(f"Error processing {img_path}: {e}")
    
#     return processed


# def process_subject(subject_id, input_base="data/dataset", output_base="data/preprocessed", preview=False):
#     """Process images for a subject, handling both pose directories and direct files"""
#     subject_input = os.path.join(input_base, subject_id)
#     subject_output = os.path.join(output_base, subject_id)
    
#     print(f"Processing subject: {subject_id}")
#     print(f"Input directory: {subject_input}")
#     print(f"Output directory: {subject_output}")
    
#     # Create output directory
#     os.makedirs(subject_output, exist_ok=True)
    
#     # Check if we have a poses directory
#     poses_dir = os.path.join(subject_input, "poses")
#     if os.path.exists(poses_dir) and os.path.isdir(poses_dir):
#         print(f"Found poses directory: {poses_dir}")
#         # Process each pose directory
#         for pose in os.listdir(poses_dir):
#             pose_input = os.path.join(poses_dir, pose)
#             if os.path.isdir(pose_input):
#                 pose_output = os.path.join(subject_output, "poses", pose)
#                 print(f"Processing pose directory: {pose}")
#                 processed = process_directory(pose_input, pose_output, preview)
#                 print(f"Processed {processed} images for pose: {pose}")
#     else:
#         # Check if there are JPG files directly in the subject directory
#         direct_files = [f for f in os.listdir(subject_input) if f.lower().endswith(('.jpg', '.jpeg'))]
#         if direct_files:
#             print(f"Found {len(direct_files)} JPG files directly in subject directory")
#             # Create a "front" pose directory for these files
#             front_dir = os.path.join(subject_output, "poses", "front")
#             os.makedirs(front_dir, exist_ok=True)
            
#             # Process the files
#             for i, fname in enumerate(direct_files):
#                 img_path = os.path.join(subject_input, fname)
#                 save_path = os.path.join(front_dir, f"{i:03d}.png")
                
#                 try:
#                     image_np = cv2.imread(img_path)
#                     if image_np is None:
#                         print(f"Failed to load image: {img_path}")
#                         continue
                        
#                     segment_and_save(image_np, save_path, preview)
#                 except Exception as e:
#                     print(f"Error processing {img_path}: {e}")
    
#     # Process action videos if present
#     actions_dir = os.path.join(subject_input, "actions")
#     if os.path.exists(actions_dir) and os.path.isdir(actions_dir):
#         print(f"Found actions directory: {actions_dir}")
#         # Process each video
#         for action_video in [f for f in os.listdir(actions_dir) if f.endswith('.mp4')]:
#             video_path = os.path.join(actions_dir, action_video)
#             action_name = action_video.replace('.mp4', '')
#             action_output = os.path.join(subject_output, "actions", action_name)
#             os.makedirs(action_output, exist_ok=True)
            
#             # Process the video
#             cap = cv2.VideoCapture(video_path)
#             frame_idx = 0
#             print(f"Processing video: {action_video}")
            
#             while True:
#                 ret, frame = cap.read()
#                 if not ret:
#                     break
                    
#                 save_path = os.path.join(action_output, f"{frame_idx:04d}.png")
#                 segment_and_save(frame, save_path, preview)
#                 frame_idx += 1
            
#             cap.release()
#             print(f"Processed {frame_idx} frames for action: {action_name}")
    
#     print(f"Subject processing complete: {subject_id}")


# if __name__ == "__main__":
#     import argparse
#     parser = argparse.ArgumentParser(description="DeepLabV3+ Segmentation for images and videos")
#     parser.add_argument('--subject_id', type=str, required=True, help="Subject ID (e.g., subject_01)")
#     parser.add_argument('--input_dir', type=str, default="data/dataset", help="Input base directory")
#     parser.add_argument('--output_dir', type=str, default="data/preprocessed", help="Output base directory")
#     parser.add_argument('--preview', action='store_true', help="Enable preview window (optional)")
#     args = parser.parse_args()

#     process_subject(args.subject_id, args.input_dir, args.output_dir, args.preview)


import os
import cv2
import torch
import torchvision
import numpy as np
from PIL import Image
from tqdm import tqdm
import logging

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler("segmentation.log"),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger("SegmentationPipeline")

# Load DeepLabV3+ model (loaded only once when the module is imported)
try:
    model = torchvision.models.segmentation.deeplabv3_resnet101(weights=torchvision.models.segmentation.DeepLabV3_ResNet101_Weights.DEFAULT)
    model.eval()
    logger.info("Successfully loaded DeepLabV3+ model")
except Exception as e:
    logger.error(f"Error loading DeepLabV3+ model: {e}")
    model = None

# Constants
PERSON_CLASS = 15  # COCO person class

def segment_and_save(image_np, save_path, preview=False):
    """Segment a person from an image and save as RGBA with transparency."""
    try:
        # Check if model is loaded
        if model is None:
            raise RuntimeError("DeepLabV3+ model not loaded")
        
        # Convert to PIL Image
        image = Image.fromarray(cv2.cvtColor(image_np, cv2.COLOR_BGR2RGB))
        orig_w, orig_h = image.size

        # Preprocess image
        preprocess = torchvision.transforms.Compose([
            torchvision.transforms.ToTensor(),
            torchvision.transforms.Normalize(mean=[0.485, 0.456, 0.406],
                                            std=[0.229, 0.224, 0.225])
        ])
        input_tensor = preprocess(image).unsqueeze(0)

        # Run inference
        with torch.no_grad():
            output = model(input_tensor)["out"][0]
            output_predictions = output.argmax(0).byte().cpu().numpy()

        # Create and improve mask
        mask = (output_predictions == PERSON_CLASS).astype(np.uint8) * 255
        mask = cv2.dilate(mask, np.ones((5, 5), np.uint8), iterations=1)
        mask = cv2.GaussianBlur(mask, (7, 7), 0)

        # Add alpha to original
        rgba = cv2.cvtColor(image_np, cv2.COLOR_BGR2BGRA)
        rgba[:, :, 3] = mask
        
        # Ensure output directory exists
        os.makedirs(os.path.dirname(save_path), exist_ok=True)
        
        # Save result
        cv2.imwrite(save_path, rgba)
        logger.info(f"Segmented image saved to {save_path}")

        # Show preview if requested
        if preview:
            cv2.imshow("Segmented", rgba)
            cv2.waitKey(50)
        
        return True
    except Exception as e:
        logger.error(f"Error in segment_and_save: {e}")
        return False


def process_directory(input_dir, output_dir, preview=False):
    """Process all image files in a directory."""
    try:
        if not os.path.exists(input_dir):
            logger.error(f"Input directory does not exist: {input_dir}")
            return 0
            
        os.makedirs(output_dir, exist_ok=True)
        
        # Find all image files
        image_files = [f for f in os.listdir(input_dir) if f.lower().endswith(('.jpg', '.jpeg', '.png'))]
        if not image_files:
            logger.warning(f"No image files found in {input_dir}")
            return 0
        
        logger.info(f"Found {len(image_files)} image files in {input_dir}")
        successful = 0
        
        # Process each image
        for i, image_file in enumerate(image_files):
            logger.info(f"Processing image {i+1}/{len(image_files)}: {image_file}")
            
            img_path = os.path.join(input_dir, image_file)
            save_path = os.path.join(output_dir, os.path.splitext(image_file)[0] + ".png")
            
            # Skip if already processed
            if os.path.exists(save_path):
                logger.info(f"Skipping already processed image: {image_file}")
                successful += 1
                continue
            
            # Read and process image
            try:
                image_np = cv2.imread(img_path)
                if image_np is None:
                    logger.error(f"Failed to read image: {img_path}")
                    continue
                
                if segment_and_save(image_np, save_path, preview):
                    successful += 1
            except Exception as e:
                logger.error(f"Error processing {img_path}: {e}")
        
        logger.info(f"Successfully processed {successful} out of {len(image_files)} images")
        return successful
    except Exception as e:
        logger.error(f"Error in process_directory: {e}")
        return 0


def batch_process(subject_id="subject_01", input_base="data/dataset", output_base="data/preprocessed", preview=False):
    """Process all poses and actions for a subject."""
    try:
        logger.info(f"Starting batch processing for subject: {subject_id}")
        
        subject_dir = os.path.join(input_base, subject_id)
        subject_output_dir = os.path.join(output_base, subject_id)
        
        if not os.path.exists(subject_dir):
            logger.error(f"Subject directory not found: {subject_dir}")
            return
        
        # Create output directories
        os.makedirs(os.path.join(subject_output_dir, "poses"), exist_ok=True)
        os.makedirs(os.path.join(subject_output_dir, "actions"), exist_ok=True)
        
        # Process poses directory if it exists
        poses_dir = os.path.join(subject_dir, "poses")
        if os.path.exists(poses_dir) and os.path.isdir(poses_dir):
            logger.info(f"Processing poses directory: {poses_dir}")
            
            # Process each pose directory
            for pose in os.listdir(poses_dir):
                pose_dir = os.path.join(poses_dir, pose)
                if os.path.isdir(pose_dir):
                    logger.info(f"Processing pose: {pose}")
                    output_pose_dir = os.path.join(subject_output_dir, "poses", pose)
                    processed = process_directory(pose_dir, output_pose_dir, preview)
                    logger.info(f"Processed {processed} images for pose: {pose}")
        else:
            logger.warning(f"Poses directory not found: {poses_dir}")
            
            # Check if there are image files directly in the subject directory
            image_files = [f for f in os.listdir(subject_dir) if f.lower().endswith(('.jpg', '.jpeg', '.png'))]
            if image_files:
                logger.info(f"Found {len(image_files)} image files directly in subject directory")
                # Create front directory for processing
                front_dir = os.path.join(subject_output_dir, "poses", "front")
                os.makedirs(front_dir, exist_ok=True)
                
                # Process each image
                for i, img_file in enumerate(image_files):
                    logger.info(f"Processing image {i+1}/{len(image_files)}: {img_file}")
                    
                    img_path = os.path.join(subject_dir, img_file)
                    save_path = os.path.join(front_dir, f"{i:03d}.png")
                    
                    try:
                        image_np = cv2.imread(img_path)
                        if image_np is None:
                            logger.error(f"Failed to read image: {img_path}")
                            continue
                        
                        segment_and_save(image_np, save_path, preview)
                    except Exception as e:
                        logger.error(f"Error processing {img_path}: {e}")
        
        # Process actions directory if it exists
        actions_dir = os.path.join(subject_dir, "actions")
        if os.path.exists(actions_dir) and os.path.isdir(actions_dir):
            logger.info(f"Processing actions directory: {actions_dir}")
            
            # Find all video files
            video_files = [f for f in os.listdir(actions_dir) if f.lower().endswith('.mp4')]
            
            # Process each video
            for video_file in video_files:
                logger.info(f"Processing video: {video_file}")
                
                video_path = os.path.join(actions_dir, video_file)
                action_name = os.path.splitext(video_file)[0]
                action_output_dir = os.path.join(subject_output_dir, "actions", action_name)
                os.makedirs(action_output_dir, exist_ok=True)
                
                try:
                    # Open video
                    cap = cv2.VideoCapture(video_path)
                    if not cap.isOpened():
                        logger.error(f"Failed to open video: {video_path}")
                        continue
                    
                    # Process each frame
                    frame_idx = 0
                    while True:
                        ret, frame = cap.read()
                        if not ret:
                            break
                        
                        save_path = os.path.join(action_output_dir, f"{frame_idx:04d}.png")
                        segment_and_save(frame, save_path, preview)
                        frame_idx += 1
                    
                    cap.release()
                    logger.info(f"Processed {frame_idx} frames for action: {action_name}")
                except Exception as e:
                    logger.error(f"Error processing video {video_path}: {e}")
        else:
            logger.warning(f"Actions directory not found: {actions_dir}")
        
        logger.info(f"Completed batch processing for subject: {subject_id}")
    except Exception as e:
        logger.error(f"Error in batch_process: {e}")


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="DeepLabV3+ Segmentation for both poses and action videos")
    parser.add_argument('--subject_id', type=str, required=True, help="Subject ID (e.g., subject_01)")
    parser.add_argument('--input_dir', type=str, default="data/dataset", help="Input base directory")
    parser.add_argument('--output_dir', type=str, default="data/preprocessed", help="Output base directory")
    parser.add_argument('--preview', action='store_true', help="Enable preview window (optional)")
    args = parser.parse_args()

    batch_process(
        subject_id=args.subject_id,
        input_base=args.input_dir,
        output_base=args.output_dir,
        preview=args.preview
    )