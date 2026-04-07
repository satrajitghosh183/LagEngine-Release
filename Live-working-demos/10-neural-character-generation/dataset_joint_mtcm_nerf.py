import os
import torch
import json
import numpy as np
from torch.utils.data import Dataset
from PIL import Image
import random
from typing import List, Dict, Tuple, Optional, Union


class JointMTCMNeRFDataset(Dataset):
    """
    Dataset for joint training of MTCM and NeRF with view selection and pose regression.
    
    This dataset:
    1. Loads tokens for the transformer: [DINOv2 | pose | focal | mask area | resolution]
    2. Supports filtering by identity
    3. Prepares data for the joint training pipeline including target views
    4. Handles the creation of training triplets (input views, selected views, target view)
    
    The dataset can be used in different modes:
    - 'transformer': For pretraining the transformer (MAE style)
    - 'nerf': For pretraining the NeRF model
    - 'joint': For end-to-end joint training
    """
    def __init__(
        self, 
        root_dir: str,
        mode: str = 'joint',
        identity_list: Optional[List[str]] = None,
        max_views_per_identity: int = 64,
        num_selected_views: int = 5,
        target_views_per_identity: int = 1,
        transform = None,
        debug: bool = False
    ):
        """
        Initialize the dataset.
        
        Args:
            root_dir: Root directory containing the data
            mode: Dataset mode ('transformer', 'nerf', or 'joint')
            identity_list: List of identities to use (use all if None)
            max_views_per_identity: Maximum number of views per identity
            num_selected_views: Number of views to select for NeRF
            target_views_per_identity: Number of target views per identity for supervision
            transform: Optional transform to apply to images
            debug: Whether to print debug information
        """
        self.root_dir = root_dir
        self.mode = mode
        self.max_views_per_identity = max_views_per_identity
        self.num_selected_views = num_selected_views
        self.target_views_per_identity = target_views_per_identity
        self.transform = transform
        self.debug = debug
        
        # Check if root directory exists
        if not os.path.exists(root_dir):
            raise ValueError(f"Root directory '{root_dir}' does not exist")
            
        # Check if embeddings directory exists
        embeddings_dir = os.path.join(root_dir, "embeddings")
        if not os.path.exists(embeddings_dir):
            raise ValueError(f"Embeddings directory '{embeddings_dir}' does not exist")

        # Set identity list
        self.identity_list = identity_list or sorted(os.listdir(embeddings_dir))
        
        if self.debug:
            print(f"Root directory: {root_dir}")
            print(f"Found {len(self.identity_list)} identities")
        
        # Load all identities data
        self.identity_data = {}
        self.samples = []  # Will contain (identity, input_indices, target_indices)
        
        # Dictionary to track skipped matches for debugging
        skipped_matches = {}
        
        valid_identities = 0
        
        for identity in self.identity_list:
            if self.debug:
                print(f"Processing identity: {identity}")
                
            # Path setup
            emb_dir = os.path.join(root_dir, "embeddings", identity)
            images_dir = os.path.join(root_dir, "raw", identity)
            exif_path = os.path.join(root_dir, "metadata", identity, "exif.json")
            mask_dir = os.path.join(root_dir, "masks", identity)
            
            # Check if required directories exist
            if not all(os.path.exists(p) for p in [emb_dir, exif_path]):
                if self.debug:
                    print(f"  Skipping identity {identity} due to missing data")
                continue
                
            # Load EXIF data
            try:
                with open(exif_path, "r") as f:
                    exif = json.load(f)
            except Exception as e:
                if self.debug:
                    print(f"  Error loading EXIF data for {identity}: {e}")
                continue
                
            # Find all embedding files
            emb_files = [f for f in os.listdir(emb_dir) if f.endswith(".npy")]
            if not emb_files:
                if self.debug:
                    print(f"  No embedding files found for {identity}")
                continue
                
            if self.debug:
                print(f"  Found {len(emb_files)} embedding files")
                
            # Process each image and collect tokens
            token_data = []
            image_paths = []
            successful_matches = 0
            skipped = []
            
            # for fname in sorted(emb_files):
            #     # Skip special files
            #     if fname in ['cameras.npy', 'images.npy']:
            #         continue
                    
            #     stem = os.path.splitext(fname)[0]
            #     emb_path = os.path.join(emb_dir, fname)
                
            #     # Try to load embedding
            #     try:
            #         emb = np.load(emb_path)
            #     except Exception as e:
            #         if self.debug:
            #             print(f"  Error loading embedding {fname}: {e}")
            #         skipped.append(f"Failed to load {fname}: {str(e)}")
            #         continue
                
            #     # Extract the numerical part from the filename
            #     image_id = stem  # We'll use the stem directly as ID
                
            #     # Get EXIF data
            #     exif_entry = None
            #     for key, value in exif.items():
            #         if image_id in key:
            #             exif_entry = value
            #             break
                        
            #     if not exif_entry:
            #         if self.debug and successful_matches < 3:
            #             print(f"  No EXIF data found for {stem}")
            #         skipped.append(f"No EXIF match for {stem}")
            #         continue
                
            #     # Get or create pose (use placeholder or identity if missing)
            #     # For development, we'll use a random placeholder pose
            #     # In production, this could be loaded from saved pose data
            #     pose = None
                
            #     # Try to find a pre-existing pose
            #     pose_path = os.path.join(root_dir, "poses", identity, f"{image_id}.npy")
            #     if os.path.exists(pose_path):
            #         try:
            #             pose = np.load(pose_path)
            #         except Exception as e:
            #             if self.debug:
            #                 print(f"  Error loading pose for {image_id}: {e}")
                
            #     # If no pose found, create a placeholder
            #     if pose is None:
            #         # Random position in a sphere around the origin
            #         phi = np.random.uniform(0, 2 * np.pi)
            #         theta = np.random.uniform(0, np.pi)
            #         radius = np.random.uniform(3.5, 4.5)
                    
            #         x = radius * np.sin(theta) * np.cos(phi)
            #         y = radius * np.sin(theta) * np.sin(phi)
            #         z = radius * np.cos(theta)
                    
            #         # Random quaternion (normalized)
            #         quat = np.random.normal(0, 1, 4)
            #         quat = quat / np.linalg.norm(quat)
                    
            #         pose = np.array([x, y, z, quat[0], quat[1], quat[2], quat[3]])
                
            #     # Extract metadata from EXIF
            #     timestamp = exif_entry.get("timestamp", 0)
            #     focal = exif_entry.get("focal", 1.0)
                
            #     # Get mask area if available
            #     mask_area = 0.0
            #     mask_files = [
            #         f"{stem}.png",
            #         f"{image_id}.png",
            #         f"{image_id.split('_')[0]}.png" if '_' in image_id else None
            #     ]
                
            #     mask_files = [f for f in mask_files if f]  # Remove None entries
                
            #     for mask_file in mask_files:
            #         mask_path = os.path.join(mask_dir, mask_file)
            #         if os.path.exists(mask_path):
            #             try:
            #                 mask = np.array(Image.open(mask_path).convert("L")) > 0
            #                 mask_area = mask.mean()
            #                 break
            #             except Exception as e:
            #                 if self.debug:
            #                     print(f"  Error loading mask {mask_file}: {e}")
                
            #     # Get image resolution (placeholder if not available)
            #     resolution = exif_entry.get("resolution", 1.0)
                
            #     # # Create token by concatenating embedding, pose, and metadata
            #     # try:
            #     #     token = np.concatenate([
            #     #         emb,                               # DINOv2 embedding (384D)
            #     #         pose,                              # Position + quaternion (7D)
            #     #         np.array([timestamp, mask_area, focal])  # Metadata (3D)
            #     #     ])  # Total: 394D
                    
            #     #     # Add to our collection
            #     #     token_data.append(token)
            #     #     image_path = os.path.join(images_dir, f"{image_id}.jpg")
            #     #     image_paths.append(image_path)
            #     #     successful_matches += 1
                    
            #     #     if self.debug and successful_matches <= 2:
            #     #         print(f"  Successfully processed {stem}")
            #     # except Exception as e:
            #     #     if self.debug:
            #     #         print(f"  Error creating token for {fname}: {e}")
            #     #     skipped.append(f"Token creation failed for {fname}: {str(e)}")
            #     #     continue

            # # Defensive checks
            # if emb is None or pose is None:
            #     if self.debug:
            #         print(f"  Skipping {fname} due to missing embedding or pose")
            #     skipped.append(f"Missing emb/pose for {fname}")
            #     continue

            # if None in [timestamp, focal]:
            #     if self.debug:
            #         print(f"  Skipping {fname} due to missing metadata")
            #     skipped.append(f"Missing timestamp/focal for {fname}")
            #     continue

            # try:
            #     token = np.concatenate([
            #         emb,
            #         pose,
            #         np.array([timestamp, mask_area, focal])
            #     ])
            #     if np.any(np.isnan(token)) or np.any(np.isinf(token)):
            #         raise ValueError("NaN or Inf detected in token")

            #     token_data.append(token)
            #     image_path = os.path.join(images_dir, f"{image_id}.jpg")
            #     image_paths.append(image_path)
            #     successful_matches += 1

            #     if self.debug and successful_matches <= 2:
            #         print(f"  Successfully processed {stem}")
            # except Exception as e:
            #     if self.debug:
            #         print(f"  Error creating token for {fname}: {e}")
            #     skipped.append(f"Token creation failed for {fname}: {str(e)}")
            #     continue
                
            for fname in sorted(emb_files):
                if fname in ['cameras.npy', 'images.npy']:
                    continue

                stem = os.path.splitext(fname)[0]
                emb_path = os.path.join(emb_dir, fname)

                try:
                    emb = np.load(emb_path)
                except Exception as e:
                    if self.debug:
                        print(f"  Error loading embedding {fname}: {e}")
                    skipped.append(f"Failed to load {fname}: {str(e)}")
                    continue

                image_id = stem
                exif_entry = None
                for key, value in exif.items():
                    if image_id in key:
                        exif_entry = value
                        break

                if not exif_entry:
                    if self.debug and successful_matches < 3:
                        print(f"  No EXIF data found for {stem}")
                    skipped.append(f"No EXIF match for {stem}")
                    continue

                pose = None
                pose_path = os.path.join(root_dir, "poses", identity, f"{image_id}.npy")
                if os.path.exists(pose_path):
                    try:
                        pose = np.load(pose_path)
                    except Exception as e:
                        if self.debug:
                            print(f"  Error loading pose for {image_id}: {e}")

                if pose is None:
                    phi = np.random.uniform(0, 2 * np.pi)
                    theta = np.random.uniform(0, np.pi)
                    radius = np.random.uniform(3.5, 4.5)
                    x = radius * np.sin(theta) * np.cos(phi)
                    y = radius * np.sin(theta) * np.sin(phi)
                    z = radius * np.cos(theta)
                    quat = np.random.normal(0, 1, 4)
                    quat = quat / np.linalg.norm(quat)
                    pose = np.array([x, y, z, quat[0], quat[1], quat[2], quat[3]])

                timestamp = exif_entry.get("timestamp", 0)
                if timestamp is None:
                    timestamp = 0.0  # Default fallback value

                focal = exif_entry.get("focal")
                if focal is None:
                    if self.debug:
                        print(f"  Skipping {fname} due to missing focal length")
                    skipped.append(f"Missing focal for {fname}")
                    continue


                mask_area = 0.0
                mask_files = [f"{stem}.png", f"{image_id}.png", f"{image_id.split('_')[0]}.png" if '_' in image_id else None]
                mask_files = [f for f in mask_files if f]

                for mask_file in mask_files:
                    mask_path = os.path.join(mask_dir, mask_file)
                    if os.path.exists(mask_path):
                        try:
                            mask = np.array(Image.open(mask_path).convert("L")) > 0
                            mask_area = mask.mean()
                            break
                        except Exception as e:
                            if self.debug:
                                print(f"  Error loading mask {mask_file}: {e}")

                resolution = exif_entry.get("resolution", 1.0)

                if emb is None or pose is None:
                    if self.debug:
                        print(f"  Skipping {fname} due to missing embedding or pose")
                    skipped.append(f"Missing emb/pose for {fname}")
                    continue

                if None in [timestamp, focal]:
                    if self.debug:
                        print(f"  Skipping {fname} due to missing metadata")
                    skipped.append(f"Missing timestamp/focal for {fname}")
                    continue

                try:
                    token = np.concatenate([
                        emb,
                        pose,
                        np.array([timestamp, mask_area, focal])
                    ])
                    if np.any(np.isnan(token)) or np.any(np.isinf(token)):
                        raise ValueError("NaN or Inf detected in token")

                    token_data.append(token)
                    image_paths.append(os.path.join(images_dir, f"{image_id}.jpg"))
                    successful_matches += 1

                    if self.debug and successful_matches <= 2:
                        print(f"  Successfully processed {stem}")
                except Exception as e:
                    if self.debug:
                        print(f"  Error creating token for {fname}: {e}")
                    skipped.append(f"Token creation failed for {fname}: {str(e)}")
                    continue

            # If we have enough data for this identity, add it
            if len(token_data) >= self.num_selected_views + self.target_views_per_identity:
                # Convert to tensors
                tokens_tensor = torch.from_numpy(np.stack(token_data)).float()

                
                # Store the data for this identity
                self.identity_data[identity] = {
                    "tokens": tokens_tensor,
                    "image_paths": image_paths
                }
                
                # Create training samples
                self._create_samples_for_identity(identity, len(token_data))
                
                valid_identities += 1
                
                if self.debug:
                    print(f"  Added {identity} with {len(token_data)} tokens")
                    print(f"  Match rate: {successful_matches}/{len(emb_files)} ({successful_matches/len(emb_files)*100:.1f}%)")
            elif self.debug:
                print(f"  Not enough tokens for {identity}, found {len(token_data)}")
            
            # Store skipped entries for debugging
            skipped_matches[identity] = {
                "total": len(emb_files),
                "matched": successful_matches,
                "skipped_reasons": skipped[:10]  # Store first 10 reasons only
            }
        
        if self.debug:
            print(f"Created dataset with {len(self.samples)} samples from {valid_identities}/{len(self.identity_list)} identities")
            
            # Save skipped matches report
            if len(skipped_matches) > 0:
                try:
                    with open(os.path.join(self.root_dir, "skipped_matches_joint.json"), "w") as f:
                        json.dump(skipped_matches, f, indent=2)
                    print(f"Saved skipped matches report to {os.path.join(self.root_dir, 'skipped_matches_joint.json')}")
                except Exception as e:
                    print(f"Error saving skipped matches report: {e}")
            
        if len(self.samples) == 0:
            raise ValueError("No valid samples found. Check your data directory structure.")
    
    def _create_samples_for_identity(self, identity: str, num_tokens: int) -> None:
        """
        Create training samples for an identity.
        
        Args:
            identity: Identity name
            num_tokens: Number of tokens available for this identity
        """
        if self.mode == 'transformer':
            # For transformer pretraining, just use all tokens
            print(f"🧪 Creating samples for identity: {identity}, tokens: {num_tokens}")

            self.samples.append((identity, list(range(min(num_tokens, self.max_views_per_identity))), []))
        else:
            # For joint training or NeRF mode, create training triplets
            # We'll create multiple samples per identity with different target views
            
            # Limit to max views per identity
            num_tokens = min(num_tokens, self.max_views_per_identity)
            
            for _ in range(self.target_views_per_identity):
                # Randomly select target view indices
                all_indices = list(range(num_tokens))
                target_indices = sorted(random.sample(all_indices, self.target_views_per_identity))
                
                # Input indices are all other indices
                input_indices = [i for i in all_indices if i not in target_indices]
                
                # Add the sample
                self.samples.append((identity, input_indices, target_indices))
                print(f"✅ Sample created with input: {input_indices}, target: {target_indices}")

    
    def __len__(self) -> int:
        """Get the number of samples in the dataset."""
        return len(self.samples)
    
    def _load_image(self, path: str) -> torch.Tensor:
        """
        Load and preprocess an image.
        
        Args:
            path: Path to the image file
            
        Returns:
            Preprocessed image tensor
        """
        try:
            image = Image.open(path).convert('RGB')
            
            # Apply transform if provided
            if self.transform:
                image = self.transform(image)
            else:
                # Basic preprocessing: resize and normalize
                image = image.resize((256, 256))
                image = torch.tensor(np.array(image)).float() / 255.0
                # Change from [H, W, C] to [C, H, W] if needed
                if image.shape[-1] == 3:
                    image = image.permute(2, 0, 1)
            
            return image
        except Exception as e:
            if self.debug:
                print(f"Error loading image {path}: {e}")
            # Return a placeholder image (black)
            return torch.zeros(3, 256, 256)
    
    def __getitem__(self, idx: int) -> Dict:
        """
        Get a sample by index.
        
        Args:
            idx: Sample index
            
        Returns:
            Sample dictionary with tokens, images, and metadata
        """
        identity, input_indices, target_indices = self.samples[idx]
        identity_data = self.identity_data[identity]
        
        # Get tokens for this identity
        all_tokens = identity_data["tokens"]
        
        # For transformer pretraining mode
        if self.mode == 'transformer':
            return {
                "tokens": all_tokens[input_indices],
                "identity": identity
            }
        
        # For joint or NeRF mode - prepare input views, target views
        
        # Get input tokens for the transformer
        input_tokens = all_tokens[input_indices]
        
        # Get target views for supervision
        target_tokens = all_tokens[target_indices]
        
        result = {
            "identity": identity,
            "input_tokens": input_tokens,
            "target_tokens": target_tokens,
            "input_indices": torch.tensor(input_indices),
            "target_indices": torch.tensor(target_indices)
        }
        
        # Only load images for NeRF mode (to save memory in transformer mode)
        if self.mode == 'nerf' or self.mode == 'joint':
            # Load images for input and target views
            input_image_paths = [identity_data["image_paths"][i] for i in input_indices]
            target_image_paths = [identity_data["image_paths"][i] for i in target_indices]
            
            # Load input images
            input_images = torch.stack([self._load_image(path) for path in input_image_paths])
            
            # Load target images
            target_images = torch.stack([self._load_image(path) for path in target_image_paths])
            
            # Add to result
            result["input_images"] = input_images
            result["target_images"] = target_images
        
        return result


class ViewSelectionDataModule:
    """
    Data module for managing datasets for joint MTCM-NeRF training.
    
    This handles the creation of train/validation/test splits,
    and provides convenient access to dataloaders for each.
    """
    def __init__(
        self,
        root_dir: str,
        batch_size: int = 1,
        num_workers: int = 4,
        max_views_per_identity: int = 64,
        num_selected_views: int = 5,
        target_views_per_identity: int = 1,
        val_identities: Optional[List[str]] = None,
        train_transform = None,
        val_transform = None,
        debug: bool = False
    ):
        """
        Initialize the data module.
        
        Args:
            root_dir: Root directory containing the data
            batch_size: Batch size for dataloaders
            num_workers: Number of workers for dataloaders
            max_views_per_identity: Maximum number of views per identity
            num_selected_views: Number of views to select for NeRF
            target_views_per_identity: Number of target views per identity
            val_identities: List of identities to use for validation (random 20% if None)
            train_transform: Transform to apply to training images
            val_transform: Transform to apply to validation images
            debug: Whether to print debug information
        """
        self.root_dir = root_dir
        self.batch_size = batch_size
        self.num_workers = num_workers
        self.max_views_per_identity = max_views_per_identity
        self.num_selected_views = num_selected_views
        self.target_views_per_identity = target_views_per_identity
        self.train_transform = train_transform
        self.val_transform = val_transform
        self.debug = debug
        
        # Get list of all identities
        embeddings_dir = os.path.join(root_dir, "embeddings")
        if not os.path.exists(embeddings_dir):
            raise ValueError(f"Embeddings directory '{embeddings_dir}' does not exist")
        
        all_identities = sorted(os.listdir(embeddings_dir))
        
        # Split identities into train and validation
        if val_identities:
            self.val_identities = [id for id in val_identities if id in all_identities]
            self.train_identities = [id for id in all_identities if id not in self.val_identities]
        else:
            # Randomly select 20% for validation
            n_val = max(1, int(0.2 * len(all_identities)))
            self.val_identities = sorted(random.sample(all_identities, n_val))
            self.train_identities = [id for id in all_identities if id not in self.val_identities]
        
        if self.debug:
            print(f"Training identities: {len(self.train_identities)}")
            print(f"Validation identities: {len(self.val_identities)}")
    
    def setup(self, mode: str = 'joint'):
        """
        Set up datasets for the different splits.
        
        Args:
            mode: Dataset mode ('transformer', 'nerf', or 'joint')
        """
        self.mode = mode
        
        # Create training dataset
        self.train_dataset = JointMTCMNeRFDataset(
            root_dir=self.root_dir,
            mode=mode,
            identity_list=self.train_identities,
            max_views_per_identity=self.max_views_per_identity,
            num_selected_views=self.num_selected_views,
            target_views_per_identity=self.target_views_per_identity,
            transform=self.train_transform,
            debug=self.debug
        )
        
        # Create validation dataset
        self.val_dataset = JointMTCMNeRFDataset(
            root_dir=self.root_dir,
            mode=mode,
            identity_list=self.val_identities,
            max_views_per_identity=self.max_views_per_identity,
            num_selected_views=self.num_selected_views,
            target_views_per_identity=self.target_views_per_identity,
            transform=self.val_transform,
            debug=self.debug
        )
    
    def train_dataloader(self):
        """Get the training dataloader."""
        return torch.utils.data.DataLoader(
            self.train_dataset,
            batch_size=self.batch_size,
            shuffle=True,
            num_workers=self.num_workers,
            pin_memory=True
        )
    
    def val_dataloader(self):
        """Get the validation dataloader."""
        return torch.utils.data.DataLoader(
            self.val_dataset,
            batch_size=self.batch_size,
            shuffle=False,
            num_workers=self.num_workers,
            pin_memory=True
        )
    
    def test_dataloader(self):
        """Get the test dataloader (same as validation for now)."""
        return self.val_dataloader()


if __name__ == "__main__":
    # Example usage and testing
    import argparse
    
    parser = argparse.ArgumentParser(description="Test the JointMTCMNeRFDataset")
    parser.add_argument("--data-dir", type=str, required=True, help="Path to data directory")
    parser.add_argument("--mode", type=str, default="joint", choices=["transformer", "nerf", "joint"], 
                        help="Dataset mode")
    parser.add_argument("--identity", type=str, help="Specific identity to process")
    parser.add_argument("--batch-size", type=int, default=1, help="Batch size")
    args = parser.parse_args()
    
    # Create a dataset module
    data_module = ViewSelectionDataModule(
        root_dir=args.data_dir,
        batch_size=args.batch_size,
        val_identities=[args.identity] if args.identity else None,
        debug=True
    )
    
    # Set up the datasets
    data_module.setup(mode=args.mode)
    
    # Get dataloaders
    train_loader = data_module.train_dataloader()
    val_loader = data_module.val_dataloader()
    
    print("\nTesting train dataloader:")
    batch = next(iter(train_loader))
    print(f"Batch keys: {batch.keys()}")
    
    if "input_tokens" in batch:
        print(f"Input tokens shape: {batch['input_tokens'].shape}")
        
    if "target_tokens" in batch:
        print(f"Target tokens shape: {batch['target_tokens'].shape}")
        
    if "input_images" in batch:
        print(f"Input images shape: {batch['input_images'].shape}")
        
    if "target_images" in batch:
        print(f"Target images shape: {batch['target_images'].shape}")
        
    print("\nTesting validation dataloader:")
    try:
        val_batch = next(iter(val_loader))
        print(f"Validation batch keys: {val_batch.keys()}")
    except Exception as e:
        print(f"Error testing validation loader: {e}")
    
    print("\nDataset module test complete!")