
# # import os
# # import torch
# # import torchvision.transforms as transforms
# # from PIL import Image
# # import numpy as np
# # from tqdm import tqdm
# # import argparse
# # from pathlib import Path
# # import glob

# # def extract_dino_embeddings(model, image_path, transform):
# #     """Extract DINO embeddings for a single image."""
# #     try:
# #         # Load and preprocess image
# #         img = Image.open(image_path).convert('RGB')
# #         img_tensor = transform(img).unsqueeze(0)
        
# #         # Move to device
# #         if torch.cuda.is_available():
# #             img_tensor = img_tensor.cuda()
        
# #         # Generate embedding
# #         with torch.no_grad():
# #             embedding = model(img_tensor).cpu().numpy()[0]
        
# #         return embedding
# #     except Exception as e:
# #         print(f"Error processing {image_path}: {e}")
# #         return None

# # def main():
# #     parser = argparse.ArgumentParser(description="Generate embeddings for all images")
# #     parser.add_argument('--images_dir', type=str, default='data/raw_images',
# #                         help='Directory containing images')
# #     parser.add_argument('--output_dir', type=str, default='data/embeddings',
# #                         help='Directory to save embeddings')
# #     parser.add_argument('--model', type=str, default='dinov2',
# #                         choices=['dinov2', 'resnet50', 'clip'],
# #                         help='Feature extraction model')
# #     args = parser.parse_args()
    
# #     # Create output directory
# #     os.makedirs(args.output_dir, exist_ok=True)
    
# #     # Set device
# #     device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
# #     print(f"Using device: {device}")
    
# #     # Load model
# #     if args.model == 'dinov2':
# #         try:
# #             print("Loading DINOv2 model...")
# #             model = torch.hub.load('facebookresearch/dinov2', 'dinov2_vits14')
# #             transform = transforms.Compose([
# #                 transforms.Resize(256),
# #                 transforms.CenterCrop(224),
# #                 transforms.ToTensor(),
# #                 transforms.Normalize(mean=[0.485, 0.456, 0.406], std=[0.229, 0.224, 0.225])
# #             ])
# #         except Exception as e:
# #             print(f"Error loading DINOv2: {e}")
# #             print("Falling back to ResNet50...")
# #             args.model = 'resnet50'
    
# #     if args.model == 'resnet50':
# #         print("Loading ResNet50 model...")
# #         model = torch.hub.load('pytorch/vision:v0.10.0', 'resnet50', pretrained=True)
# #         # Remove the final classification layer
# #         model = torch.nn.Sequential(*list(model.children())[:-1])
# #         transform = transforms.Compose([
# #             transforms.Resize(256),
# #             transforms.CenterCrop(224),
# #             transforms.ToTensor(),
# #             transforms.Normalize(mean=[0.485, 0.456, 0.406], std=[0.229, 0.224, 0.225])
# #         ])
    
# #     elif args.model == 'clip':
# #         print("Loading CLIP model...")
# #         try:
# #             import clip
# #             model, transform = clip.load('ViT-B/32', device=device)
# #         except ImportError:
# #             print("CLIP not installed. Please install with: pip install clip")
# #             print("Falling back to ResNet50...")
# #             model = torch.hub.load('pytorch/vision:v0.10.0', 'resnet50', pretrained=True)
# #             # Remove the final classification layer
# #             model = torch.nn.Sequential(*list(model.children())[:-1])
# #             transform = transforms.Compose([
# #                 transforms.Resize(256),
# #                 transforms.CenterCrop(224),
# #                 transforms.ToTensor(),
# #                 transforms.Normalize(mean=[0.485, 0.456, 0.406], std=[0.229, 0.224, 0.225])
# #             ])
    
# #     # Move model to device
# #     model = model.to(device)
# #     model.eval()
    
# #     # Find all image files
# #     all_image_files = []
    
# #     # Check if the folder structure includes person subfolders
# #     has_subfolders = False
# #     for item in os.listdir(args.images_dir):
# #         if os.path.isdir(os.path.join(args.images_dir, item)):
# #             has_subfolders = True
# #             break
    
# #     if has_subfolders:
# #         # Process images in subfolders
# #         for person_dir in os.listdir(args.images_dir):
# #             person_path = os.path.join(args.images_dir, person_dir)
# #             if os.path.isdir(person_path):
# #                 for img_file in glob.glob(os.path.join(person_path, "*.jpg")):
# #                     all_image_files.append(img_file)
# #     else:
# #         # Process images directly in the images directory
# #         all_image_files = glob.glob(os.path.join(args.images_dir, "*.jpg"))
    
# #     print(f"Found {len(all_image_files)} image files")
    
# #     # Process all images
# #     for img_path in tqdm(all_image_files, desc="Generating embeddings"):
# #         # Extract image ID from path
# #         img_id = os.path.splitext(os.path.basename(img_path))[0]
        
# #         # Skip if embedding already exists
# #         # Extract identity name from parent folder
# #         identity = os.path.basename(os.path.dirname(img_path))
# #         identity_outdir = os.path.join(args.output_dir, identity)
# #         os.makedirs(identity_outdir, exist_ok=True)

# #         output_path = os.path.join(identity_outdir, f"{img_id}.npy")

# #         if os.path.exists(output_path):
# #             continue
        
# #         # Generate embedding
# #         if args.model == 'clip':
# #             # Special handling for CLIP
# #             img = Image.open(img_path).convert('RGB')
# #             img_tensor = transform(img).unsqueeze(0).to(device)
# #             with torch.no_grad():
# #                 embedding = model.encode_image(img_tensor).cpu().numpy()[0]
# #         else:
# #             # For DINOv2 or ResNet
# #             embedding = extract_dino_embeddings(model, img_path, transform)
        
# #         if embedding is not None:
# #             # Save embedding
# #             np.save(output_path, embedding)
    
# #     print(f"Embeddings saved to {args.output_dir}")

# # if __name__ == "__main__":
# #     main()



# import os, sys, argparse, glob
# import torch, numpy as np
# from torchvision import transforms
# from PIL import Image
# from tqdm import tqdm

# def get_model():
#     try:
#         model = torch.hub.load('facebookresearch/dinov2','dinov2_vits14')
#         return model
#     except:
#         model = torch.hub.load('pytorch/vision:v0.10.0','resnet50',pretrained=True)
#         return torch.nn.Sequential(*list(model.children())[:-1])

# def main():
#     p=argparse.ArgumentParser()
#     p.add_argument('--subject_id', required=True)
#     p.add_argument('--data_folder', default='data')
#     args = p.parse_args()

#     inp  = os.path.join(args.data_folder,'preprocessed', args.subject_id,'poses','front')
#     outp = os.path.join(args.data_folder,'embeddings', args.subject_id)
#     if not os.path.isdir(inp):
#         print("No input:",inp); sys.exit(1)
#     os.makedirs(outp,exist_ok=True)

#     device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
#     print("Device:",device)
#     model = get_model().to(device).eval()
#     tf = transforms.Compose([
#         transforms.Resize(256), transforms.CenterCrop(224),
#         transforms.ToTensor(),
#         transforms.Normalize([0.485,0.456,0.406],[0.229,0.224,0.225])
#     ])

#     files = glob.glob(os.path.join(inp,'*.png'))
#     for f in tqdm(files, desc="Embeddings"):
#         img = Image.open(f).convert('RGB')
#         t = tf(img).unsqueeze(0).to(device)
#         with torch.no_grad(): emb = model(t).cpu().numpy()[0]
#         np.save(os.path.join(outp, os.path.basename(f).rsplit('.',1)[0]+'.npy'), emb)

# if __name__=='__main__':
#     main()


# extract_embeddings_dinov2.py - Modified version
import os
import sys
import argparse
import logging
import torch
from torchvision import transforms
from PIL import Image
import numpy as np
from tqdm import tqdm

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler("embedding_extraction.log"),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger("EmbeddingExtractor")

# Parse command-line arguments
parser = argparse.ArgumentParser(description="Extract DINOv2 visual embeddings from processed images")
parser.add_argument(
    "--subject_id", type=str, required=True,
    help="Subject ID to process (e.g., subject_01)"
)
parser.add_argument(
    "--data_folder", type=str, default="data",
    help="Base data folder (contains preprocessed/, embeddings/, etc.)"
)
parser.add_argument(
    "--pose", type=str, default="front",
    help="Pose to process (e.g., front, left, right, up, down)"
)
args = parser.parse_args()


class EmbeddingExtractor:
    def __init__(self, subject_id, data_folder, pose="front"):
        self.subject_id = subject_id
        self.data_folder = data_folder
        self.pose = pose
        
        # Setup paths for the specific pose
        self.input_dir = os.path.join(data_folder, "preprocessed", subject_id, "poses", pose)
        self.output_dir = os.path.join(data_folder, "embeddings", subject_id, "poses", pose)
        
        # Verify input path exists
        if not os.path.exists(self.input_dir):
            logger.error(f"Input directory does not exist: {self.input_dir}")
            raise FileNotFoundError(f"Could not find preprocessed images for pose {pose}: {self.input_dir}")
        
        # Create output directory
        os.makedirs(self.output_dir, exist_ok=True)
        
        # Device configuration
        self.device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
        logger.info(f"Using device: {self.device}")
        
        # Load model
        try:
            logger.info("Loading DINOv2 model...")
            self.model = torch.hub.load('facebookresearch/dinov2', 'dinov2_vits14')
            self.model.to(self.device).eval()
            logger.info("Successfully loaded DINOv2 model")
        except Exception as e:
            logger.error(f"Error loading DINOv2: {e}")
            logger.info("Falling back to ResNet50...")
            self.model = torch.hub.load('pytorch/vision:v0.10.0', 'resnet50', pretrained=True)
            # Remove the final classification layer
            self.model = torch.nn.Sequential(*list(self.model.children())[:-1])
            self.model.to(self.device).eval()
        
        # Image transformations
        self.transform = transforms.Compose([
            transforms.Resize(256),
            transforms.CenterCrop(224),
            transforms.ToTensor(),
            transforms.Normalize(mean=[0.485, 0.456, 0.406], std=[0.229, 0.224, 0.225])
        ])
    
    def extract_embedding(self, image_path):
        try:
            # Load image
            img = Image.open(image_path).convert('RGB')
            
            # Preprocess for model
            img_tensor = self.transform(img).unsqueeze(0).to(self.device)
            
            # Get embedding
            with torch.no_grad():
                embedding = self.model(img_tensor).cpu().numpy()[0]
            
            return embedding
        
        except Exception as e:
            logger.error(f"Error extracting embedding from {image_path}: {e}")
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
            npy_path = os.path.join(self.output_dir, os.path.splitext(img_file)[0] + '.npy')
            
            # Skip if already processed
            if os.path.exists(npy_path):
                logger.info(f"Skipping already processed: {img_file}")
                success_count += 1
                continue
            
            # Extract embedding
            embedding = self.extract_embedding(img_path)
            if embedding is not None:
                # Save to NPY file
                np.save(npy_path, embedding)
                success_count += 1
                logger.info(f"Extracted embedding from {img_file}")
            else:
                logger.warning(f"Failed to extract embedding from {img_file}")
        
        logger.info(f"Successfully processed {success_count}/{len(image_files)} images for pose {self.pose}")
        return success_count > 0


def main():
    try:
        # Create embedding extractor for the specific pose
        extractor = EmbeddingExtractor(
            args.subject_id, 
            args.data_folder,
            args.pose
        )
        
        # Process all images for this pose
        success = extractor.process_all_images()
        
        if success:
            logger.info(f"Embedding extraction completed for {args.subject_id} - {args.pose}")
            return 0
        else:
            logger.error(f"Failed to extract embeddings for {args.subject_id} - {args.pose}")
            return 1
            
    except Exception as e:
        logger.error(f"Unhandled exception: {e}")
        return 1


if __name__ == "__main__":
    sys.exit(main())