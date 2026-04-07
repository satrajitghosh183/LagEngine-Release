# generate_deep_masks.py - Modified version
import os
import sys
import argparse
import logging
import time
import torch
from torchvision import models, transforms
from PIL import Image
import numpy as np
from tqdm import tqdm

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler("mask_generation.log"),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger("DeepLabV3MaskGenerator")

# Parse command-line arguments
parser = argparse.ArgumentParser(
    description="Generate DeepLabV3 masks for subject under Flask app's data/ structure."
)
parser.add_argument(
    "subject_id", type=str, help="Process data/dataset/<subject_id>/poses/front"
)
parser.add_argument(
    "--data_folder", default="data",
    help="Base data folder (contains dataset/, masks/, etc.)"
)
args = parser.parse_args()


class MaskGenerator:
    def __init__(self, subject_id, data_folder="data"):
        self.subject_id = subject_id
        self.data_folder = data_folder
        
        # Setup paths
        self.input_dir = os.path.join(data_folder, "dataset", subject_id, "poses", "front")
        self.output_dir = os.path.join(data_folder, "masks", subject_id)
        self.preprocessed_dir = os.path.join(data_folder, "preprocessed", subject_id, "poses", "front")
        
        # Verify input path exists
        if not os.path.exists(self.input_dir):
            logger.error(f"Input directory does not exist: {self.input_dir}")
            raise FileNotFoundError(f"Could not find images directory: {self.input_dir}")
        
        # Create output directories
        os.makedirs(self.output_dir, exist_ok=True)
        os.makedirs(self.preprocessed_dir, exist_ok=True)
        
        # Configure model
        self.device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
        logger.info(f"Using device: {self.device}")
        
        # Load model
        self.model = models.segmentation.deeplabv3_resnet101(pretrained=True)
        self.model.to(self.device).eval()
        
        # Image transformations
        self.preprocess = transforms.Compose([
            transforms.Resize((480, 480)),
            transforms.ToTensor(),
            transforms.Normalize(mean=[0.485, 0.456, 0.406], std=[0.229, 0.224, 0.225])
        ])
        
        # Constants
        self.PERSON_CLASS = 15  # COCO dataset person class
    
    def process_image(self, img_path):
        try:
            # Load image
            img = Image.open(img_path).convert("RGB")
            w, h = img.size
            
            # Preprocess for model
            input_tensor = self.preprocess(img).unsqueeze(0).to(self.device)
            
            # Get segmentation mask
            with torch.no_grad():
                output = self.model(input_tensor)['out'][0]
            
            # Extract person mask
            mask = output.argmax(0).byte().cpu().numpy()
            binary_mask = (mask == self.PERSON_CLASS).astype(np.uint8) * 255
            
            # Post-processing to improve mask quality
            import cv2
            kernel = np.ones((5, 5), np.uint8)
            binary_mask = cv2.dilate(binary_mask, kernel, iterations=1)
            binary_mask = cv2.morphologyEx(binary_mask, cv2.MORPH_CLOSE, kernel)
            binary_mask = cv2.GaussianBlur(binary_mask, (7, 7), 0)
            
            # Resize back to original size
            binary_mask = cv2.resize(binary_mask, (w, h), interpolation=cv2.INTER_NEAREST)
            
            # Create RGBA image with alpha channel as mask
            img_np = np.array(img)
            rgba = cv2.cvtColor(img_np, cv2.COLOR_RGB2BGRA)
            rgba[:, :, 3] = binary_mask
            
            # Save mask to masks directory
            mask_filename = os.path.basename(img_path).replace('.jpg', '.png')
            mask_path = os.path.join(self.output_dir, mask_filename)
            cv2.imwrite(mask_path, binary_mask)
            
            # Save RGBA to preprocessed directory
            rgba_path = os.path.join(self.preprocessed_dir, mask_filename)
            cv2.imwrite(rgba_path, rgba)
            
            logger.info(f"Processed {img_path} -> {rgba_path}")
            return True
            
        except Exception as e:
            logger.error(f"Error processing {img_path}: {e}")
            return False
    
    def process_all_images(self):
        # Get all jpg files
        image_files = [f for f in os.listdir(self.input_dir) 
                      if f.lower().endswith(('.jpg', '.jpeg'))]
        
        if not image_files:
            logger.warning(f"No images found in {self.input_dir}")
            return False
        
        logger.info(f"Found {len(image_files)} images to process")
        
        # Process each image
        success_count = 0
        for img_file in tqdm(image_files, desc=f"Processing {self.subject_id}"):
            img_path = os.path.join(self.input_dir, img_file)
            if self.process_image(img_path):
                success_count += 1
        
        logger.info(f"Successfully processed {success_count}/{len(image_files)} images")
        return success_count > 0


def main():
    try:
        # Create mask generator
        generator = MaskGenerator(args.subject_id, args.data_folder)
        
        # Process all images
        success = generator.process_all_images()
        
        if success:
            logger.info(f"✅ Mask generation completed for {args.subject_id}")
            return 0
        else:
            logger.error(f"❌ Failed to generate masks for {args.subject_id}")
            return 1
            
    except Exception as e:
        logger.error(f"Unhandled exception: {e}")
        return 1


if __name__ == "__main__":
    sys.exit(main())