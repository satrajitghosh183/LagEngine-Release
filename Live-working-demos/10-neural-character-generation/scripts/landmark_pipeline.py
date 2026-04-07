# # import os
# # import cv2
# # import mediapipe as mp
# # import numpy as np
# # import json
# # from tqdm import tqdm
# # import traceback
# # import sys

# # # Force CPU processing for MediaPipe
# # os.environ["CUDA_VISIBLE_DEVICES"] = "-1"
# # os.environ["TF_FORCE_GPU_ALLOW_GROWTH"] = "true"

# # def extract_landmarks(image_path):
# #     """Extract face and body landmarks from an image using MediaPipe."""
# #     try:
# #         # Initialize MediaPipe solutions for each image
# #         mp_pose = mp.solutions.pose
# #         mp_face_mesh = mp.solutions.face_mesh
        
# #         # Read the image
# #         image = cv2.imread(image_path)
# #         if image is None:
# #             print(f"Failed to load image: {image_path}")
# #             return None
        
# #         # Check if the image has an alpha channel (transparency)
# #         has_alpha = image.shape[2] == 4 if len(image.shape) == 3 else False
        
# #         # For transparent images, create a mask and handle background
# #         if has_alpha:
# #             # Split the image into color and alpha channels
# #             bgr = image[:, :, :3]
# #             alpha = image[:, :, 3]
            
# #             # Create a white background
# #             white_background = np.ones_like(bgr) * 255
            
# #             # Blend the image with the white background based on alpha
# #             alpha_factor = alpha[:, :, np.newaxis].astype(np.float32) / 255.0
# #             image_processed = (bgr * alpha_factor + white_background * (1 - alpha_factor)).astype(np.uint8)
# #         else:
# #             image_processed = image
        
# #         # Convert to RGB for MediaPipe
# #         image_rgb = cv2.cvtColor(image_processed, cv2.COLOR_BGR2RGB)
        
# #         landmarks = {}
        
# #         # Use separate instances for each detection to avoid handle issues
# #         pose = mp_pose.Pose(
# #             static_image_mode=True,
# #             model_complexity=1,  # Lower complexity
# #             enable_segmentation=False,
# #             min_detection_confidence=0.5
# #         )
        
# #         pose_results = pose.process(image_rgb)
        
# #         if pose_results.pose_landmarks:
# #             pose_landmarks = []
# #             for idx, landmark in enumerate(pose_results.pose_landmarks.landmark):
# #                 pose_landmarks.append({
# #                     'idx': idx,
# #                     'x': landmark.x,
# #                     'y': landmark.y,
# #                     'z': landmark.z,
# #                     'visibility': landmark.visibility
# #                 })
# #             landmarks['pose'] = pose_landmarks
# #         else:
# #             landmarks['pose'] = []
        
# #         # Release resources
# #         pose.close()
        
# #         # Extract face landmarks with separate instance
# #         face_mesh = mp_face_mesh.FaceMesh(
# #             static_image_mode=True,
# #             max_num_faces=1,
# #             refine_landmarks=True,
# #             min_detection_confidence=0.5
# #         )
        
# #         face_results = face_mesh.process(image_rgb)
        
# #         if face_results.multi_face_landmarks:
# #             face_landmarks = []
# #             for idx, landmark in enumerate(face_results.multi_face_landmarks[0].landmark):
# #                 face_landmarks.append({
# #                     'idx': idx,
# #                     'x': landmark.x,
# #                     'y': landmark.y,
# #                     'z': landmark.z
# #                 })
# #             landmarks['face'] = face_landmarks
# #         else:
# #             landmarks['face'] = []
        
# #         # Release resources
# #         face_mesh.close()
        
# #         # Add image dimensions for reference
# #         landmarks['image_info'] = {
# #             'width': image.shape[1],
# #             'height': image.shape[0],
# #             'channels': image.shape[2],
# #             'has_alpha': has_alpha
# #         }
        
# #         return landmarks
# #     except Exception as e:
# #         print(f"Error in extract_landmarks for {image_path}: {e}")
# #         traceback.print_exc()
# #         return None

# # def process_folder(input_folder, output_folder):
# #     """Process all PNG files in a folder and extract landmarks."""
# #     try:
# #         if not os.path.exists(input_folder):
# #             print(f"Input folder does not exist: {input_folder}")
# #             return 0
            
# #         os.makedirs(output_folder, exist_ok=True)
        
# #         # Get all PNG files
# #         image_files = [f for f in os.listdir(input_folder) if f.lower().endswith('.png')]
# #         if not image_files:
# #             print(f"No PNG files found in {input_folder}")
# #             return 0
        
# #         print(f"Found {len(image_files)} PNG files in {input_folder}")
# #         successful = 0
        
# #         # Process each image one at a time (no tqdm to avoid handle issues)
# #         for i, image_file in enumerate(image_files):
# #             print(f"Processing image {i+1}/{len(image_files)}: {image_file}")
# #             image_path = os.path.join(input_folder, image_file)
# #             json_path = os.path.join(output_folder, image_file.replace('.png', '.json'))
            
# #             # Skip if already processed
# #             if os.path.exists(json_path):
# #                 successful += 1
# #                 continue
            
# #             # Extract landmarks
# #             landmarks = extract_landmarks(image_path)
# #             if landmarks:
# #                 # Save to JSON
# #                 with open(json_path, 'w') as f:
# #                     json.dump(landmarks, f, indent=2)
# #                 successful += 1
# #                 print(f"Successfully processed {image_file}")
            
# #             # Force garbage collection to prevent handle leaks
# #             import gc
# #             gc.collect()
        
# #         return successful
# #     except Exception as e:
# #         print(f"Error in process_folder for {input_folder}: {e}")
# #         traceback.print_exc()
# #         return 0

# # def process_subject(subject_id, base_input="data/preprocessed", base_output="data/landmarks"):
# #     """Process all images for a subject."""
# #     try:
# #         # Print current working directory for debugging
# #         print(f"Current working directory: {os.getcwd()}")
        
# #         subject_input = os.path.join(base_input, subject_id)
# #         subject_output = os.path.join(base_output, subject_id)
        
# #         print(f"Looking for subject in: {subject_input}")
        
# #         if not os.path.exists(subject_input):
# #             print(f"Subject folder not found: {subject_input}")
# #             return
        
# #         os.makedirs(subject_output, exist_ok=True)
# #         total_processed = 0
        
# #         # Process actions folders
# #         actions_dir = os.path.join(subject_input, "actions")
# #         if os.path.exists(actions_dir):
# #             print(f"Found actions directory: {actions_dir}")
# #             for action in os.listdir(actions_dir):
# #                 action_input = os.path.join(actions_dir, action)
# #                 if os.path.isdir(action_input):
# #                     action_output = os.path.join(subject_output, "actions", action)
# #                     print(f"Processing action: {action}")
# #                     processed = process_folder(action_input, action_output)
# #                     total_processed += processed
# #                     print(f"Processed {processed} images for action: {action}")
# #         else:
# #             print(f"No actions directory found at: {actions_dir}")
        
# #         # Process poses folders (front, left, right, etc.)
# #         pose_dirs = ["front", "left", "right", "up", "down"]
# #         for pose in pose_dirs:
# #             pose_input = os.path.join(subject_input, pose)
# #             if os.path.exists(pose_input) and os.path.isdir(pose_input):
# #                 pose_output = os.path.join(subject_output, "poses", pose)
# #                 print(f"Processing pose: {pose}")
# #                 processed = process_folder(pose_input, pose_output)
# #                 total_processed += processed
# #                 print(f"Processed {processed} images for pose: {pose}")
# #             else:
# #                 print(f"Pose directory not found: {pose_input}")
        
# #         print(f"Total images processed: {total_processed}")
# #     except Exception as e:
# #         print(f"Error in process_subject for {subject_id}: {e}")
# #         traceback.print_exc()

# # if __name__ == "__main__":
# #     try:
# #         import argparse
        
# #         parser = argparse.ArgumentParser(description="Extract landmarks from image sequences")
# #         parser.add_argument("--subject_id", type=str, required=True, help="Subject ID to process (e.g., subject_01)")
# #         parser.add_argument("--input_dir", type=str, default="data/preprocessed", help="Base input directory")
# #         parser.add_argument("--output_dir", type=str, default="data/landmarks", help="Base output directory")
        
# #         args = parser.parse_args()
        
# #         print(f"Starting landmark extraction for subject: {args.subject_id}")
# #         process_subject(args.subject_id, args.input_dir, args.output_dir)
# #         print("Done extracting landmarks!")
# #     except Exception as e:
# #         print(f"Fatal error: {e}")
# #         traceback.print_exc()



# import os, sys, argparse, json, traceback
# import cv2, mediapipe as mp
# from tqdm import tqdm

# def extract_landmarks(image_path):
#     img = cv2.imread(image_path)
#     if img is None:
#         return None
#     rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
#     mp_fm = mp.solutions.face_mesh.FaceMesh(
#         static_image_mode=True, max_num_faces=1, refine_landmarks=True, min_detection_confidence=0.5)
#     res = mp_fm.process(rgb)
#     mp_fm.close()
#     if not res.multi_face_landmarks:
#         return None
#     face = res.multi_face_landmarks[0]
#     data = []
#     for lm in face.landmark:
#         data.append({'x':lm.x,'y':lm.y,'z':lm.z})
#     return {'face':data,'image_info':{'h':img.shape[0],'w':img.shape[1]}}

# def process_folder(inp, out):
#     os.makedirs(out,exist_ok=True)
#     files = [f for f in os.listdir(inp) if f.lower().endswith('.png')]
#     for fn in tqdm(files, desc="Landmarks"):
#         ip = os.path.join(inp,fn)
#         op = os.path.join(out,fn.rsplit('.',1)[0]+'.json')
#         if os.path.exists(op): continue
#         lm = extract_landmarks(ip)
#         if lm:
#             with open(op,'w') as f: json.dump(lm,f,indent=2)
#     return

# if __name__=='__main__':
#     p = argparse.ArgumentParser()
#     p.add_argument('--subject_id', required=True)
#     p.add_argument('--data_folder', default='data')
#     args = p.parse_args()

#     inp  = os.path.join(args.data_folder,'preprocessed', args.subject_id,'poses','front')
#     outp = os.path.join(args.data_folder,'landmarks',    args.subject_id,'poses','front')
#     if not os.path.isdir(inp):
#         print("No input:",inp); sys.exit(1)
#     process_folder(inp,outp)
#     print("Landmarks done.")


# landmark_pipeline.py - Modified version
import os
import sys
import argparse
import json
import logging
import cv2
import mediapipe as mp
from tqdm import tqdm

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler("landmark_extraction.log"),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger("LandmarkExtractor")

# Parse command-line arguments
parser = argparse.ArgumentParser(description="Extract facial landmarks from processed images")
parser.add_argument(
    "--subject_id", type=str, required=True,
    help="Subject ID to process (e.g., subject_01)"
)
parser.add_argument(
    "--input_dir", type=str, default="data/preprocessed",
    help="Base input directory containing preprocessed images"
)
parser.add_argument(
    "--output_dir", type=str, default="data/landmarks",
    help="Base output directory for landmark data"
)
parser.add_argument(
    "--pose", type=str, default="front",
    help="Pose to process (e.g., front, left, right, up, down)"
)
args = parser.parse_args()


class LandmarkExtractor:
    def __init__(self, subject_id, input_base, output_base, pose="front"):
        self.subject_id = subject_id
        self.pose = pose
        
        # Setup paths for the specific pose
        self.input_dir = os.path.join(input_base, subject_id, "poses", pose)
        self.output_dir = os.path.join(output_base, subject_id, "poses", pose)
        
        # Verify input path exists
        if not os.path.exists(self.input_dir):
            logger.error(f"Input directory does not exist: {self.input_dir}")
            raise FileNotFoundError(f"Could not find preprocessed images for pose {pose}: {self.input_dir}")
        
        # Create output directory
        os.makedirs(self.output_dir, exist_ok=True)
        
        # Initialize MediaPipe Face Mesh
        self.mp_face_mesh = mp.solutions.face_mesh.FaceMesh(
            static_image_mode=True,
            max_num_faces=1,
            refine_landmarks=True,
            min_detection_confidence=0.5
        )
    
    def extract_landmarks(self, image_path):
        try:
            # Load image
            image = cv2.imread(image_path)
            if image is None:
                logger.error(f"Failed to load image: {image_path}")
                return None
            
            # Convert to RGB for MediaPipe
            rgb_image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
            
            # Process with MediaPipe
            results = self.mp_face_mesh.process(rgb_image)
            
            # Check if face was detected
            if not results.multi_face_landmarks:
                logger.warning(f"No face detected in {image_path}")
                return None
            
            # Extract landmarks
            face_landmarks = []
            for idx, landmark in enumerate(results.multi_face_landmarks[0].landmark):
                face_landmarks.append({
                    'idx': idx,
                    'x': landmark.x,
                    'y': landmark.y,
                    'z': landmark.z
                })
            
            # Create landmark data structure
            landmark_data = {
                'face': face_landmarks,
                'image_info': {
                    'height': image.shape[0],
                    'width': image.shape[1],
                    'channels': image.shape[2]
                }
            }
            
            return landmark_data
        
        except Exception as e:
            logger.error(f"Error extracting landmarks from {image_path}: {e}")
            return None
    
    def process_all_images(self):
        # Get all image files
        image_files = [f for f in os.listdir(self.input_dir) 
                     if f.lower().endswith(('.png', '.jpg', '.jpeg'))]
        
        if not image_files:
            logger.warning(f"No image files found in {self.input_dir}")
            return False
        
        logger.info(f"Found {len(image_files)} images to process for pose {self.pose}")
        
        # Process each image
        success_count = 0
        for img_file in tqdm(image_files, desc=f"Processing {self.subject_id} - {self.pose}"):
            img_path = os.path.join(self.input_dir, img_file)
            json_path = os.path.join(self.output_dir, os.path.splitext(img_file)[0] + '.json')
            
            # Skip if already processed
            if os.path.exists(json_path):
                logger.info(f"Skipping already processed: {img_file}")
                success_count += 1
                continue
            
            # Extract landmarks
            landmarks = self.extract_landmarks(img_path)
            if landmarks:
                # Save to JSON
                with open(json_path, 'w') as f:
                    json.dump(landmarks, f, indent=2)
                success_count += 1
                logger.info(f"Extracted landmarks from {img_file}")
            else:
                logger.warning(f"Failed to extract landmarks from {img_file}")
        
        logger.info(f"Successfully processed {success_count}/{len(image_files)} images for pose {self.pose}")
        return success_count > 0
    
    def cleanup(self):
        # Release MediaPipe resources
        self.mp_face_mesh.close()


def main():
    try:
        # Create landmark extractor for the specified pose
        extractor = LandmarkExtractor(
            args.subject_id,
            args.input_dir,
            args.output_dir,
            args.pose
        )
        
        # Process all images for this pose
        success = extractor.process_all_images()
        
        # Cleanup
        extractor.cleanup()
        
        if success:
            logger.info(f"Landmark extraction completed for {args.subject_id} - {args.pose}")
            return 0
        else:
            logger.error(f"Failed to extract landmarks for {args.subject_id} - {args.pose}")
            return 1
            
    except Exception as e:
        logger.error(f"Unhandled exception: {e}")
        return 1


if __name__ == "__main__":
    sys.exit(main())