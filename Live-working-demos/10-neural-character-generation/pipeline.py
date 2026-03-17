#!/usr/bin/env python3
"""
Neural Character Generation Pipeline -- end-to-end orchestration.

Connects every stage of the pipeline:
  1. Image preprocessing  -- segmentation (DeepLabV3), landmark extraction
                             (MediaPipe), embedding extraction (DINOv2)
  2. Pose estimation      -- MediaPipe landmarks + OpenCV solvePnP to estimate
                             camera extrinsics (rotation + translation -> quaternion)
  3. Token construction   -- 394D MTCM tokens (384D DINOv2 + 7D pose + 3D metadata)
  4. MTCM view selection  -- MTCM_MAE transformer to select optimal views
  5. NeRF training        -- WeightedTinyNeRF with selected views
  6. Mesh extraction      -- marching cubes to extract 3D mesh from trained NeRF
  7. Post-processing      -- relighting, rigging, animation

Usage as library:
    from pipeline import AvatarPipeline
    pipe = AvatarPipeline()
    results = pipe.run_full_pipeline(image_paths, "subject_01")

Usage as CLI:
    python pipeline.py --images img1.jpg img2.jpg ... --subject-id subject_01
"""

import argparse
import json
import logging
import os
import sys
import time
import traceback
from pathlib import Path
from typing import Any, Callable, Dict, List, Optional, Tuple, Union

import cv2
import numpy as np
import torch
import torch.nn as nn
import torch.nn.functional as F

# ---------------------------------------------------------------------------
# Local project imports (lazy-loaded in methods where possible)
# ---------------------------------------------------------------------------
from nerf.weighted_tiny_nerf import WeightedTinyNeRF
from nerf.nerf_config import JointTrainingConfig
from mesh_extraction import extract_mesh as mc_extract_mesh, export_obj, export_glb

logger = logging.getLogger(__name__)

# ============================================================================
# Constants
# ============================================================================

# Canonical 3D face model for solvePnP -- key MediaPipe landmark indices.
# Coordinates are approximate, in centimetres, right-handed system with
# the nose tip at the origin.
#
#   Index 1   -> Nose tip
#   Index 152 -> Chin
#   Index 263 -> Left eye outer corner
#   Index 33  -> Right eye outer corner
#   Index 287 -> Left mouth corner
#   Index 57  -> Right mouth corner
CANONICAL_3D_FACE_POINTS = np.array([
    [ 0.0,   0.0,   0.0],   # 1   nose tip
    [ 0.0,  -3.3,  -0.6],   # 152 chin
    [-3.3,   0.5,  -1.3],   # 263 left eye outer corner
    [ 3.3,   0.5,  -1.3],   # 33  right eye outer corner
    [-2.5,  -1.7,  -1.0],   # 287 left mouth corner
    [ 2.5,  -1.7,  -1.0],   # 57  right mouth corner
], dtype=np.float64)

CANONICAL_LANDMARK_INDICES = [1, 152, 263, 33, 287, 57]

# DINOv2 small produces 384D embeddings.
DINO_EMBEDDING_DIM = 384

# Token layout: DINOv2(384) + pose(7) + metadata(3) = 394
#   pose     = [x, y, z, qx, qy, qz, qw]
#   metadata = [image_index, total_images, face_coverage_ratio]
TOKEN_DIM = 394


# ============================================================================
# Default pipeline configuration
# ============================================================================

DEFAULT_CONFIG = {
    # Preprocessing
    "segmentation_model": "deeplabv3_resnet101",
    "dino_model": "dinov2_vits14",
    "face_mesh_confidence": 0.5,

    # MTCM
    "mtcm_input_dim": TOKEN_DIM,
    "mtcm_model_dim": 128,
    "mtcm_depth": 4,
    "mtcm_heads": 8,
    "mtcm_drop_path": 0.1,

    # NeRF
    "nerf_encoding_functions": 10,
    "nerf_hidden_dim": 128,
    "nerf_num_layers": 4,
    "nerf_image_height": 256,
    "nerf_image_width": 256,
    "nerf_top_k": 5,
    "nerf_learning_rate": 1e-4,

    # Mesh extraction
    "mesh_resolution": 128,
    "mesh_density_threshold": 50.0,
    "mesh_bounds": (-1.5, 1.5),

    # Output
    "output_dir": "output",
}


# ============================================================================
# Pipeline
# ============================================================================

class AvatarPipeline:
    """Orchestrates the full Neural Character Generation pipeline.

    All heavy models (DeepLabV3, DINOv2, MediaPipe FaceMesh, MTCM, NeRF) are
    loaded lazily on first use to keep the constructor lightweight.
    """

    def __init__(
        self,
        config: Optional[dict] = None,
        device: Optional[str] = None,
    ):
        """
        Args:
            config: Optional dict of configuration overrides. Keys that are
                    not provided fall back to DEFAULT_CONFIG.
            device: Compute device ('cuda', 'cpu', or None for auto-detect).
        """
        self.config = {**DEFAULT_CONFIG, **(config or {})}

        if device is None:
            self.device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
        else:
            self.device = torch.device(device)

        # Lazy-loaded heavy resources
        self._segmentation_model = None
        self._seg_transforms = None
        self._dino_model = None
        self._dino_transform = None
        self._face_mesh = None
        self._mtcm_model = None
        self._nerf_model = None

        # State from the most recent training run
        self._trained_nerf = None
        self._last_checkpoint_path = None

        logger.info("AvatarPipeline initialised (device=%s)", self.device)

    # ------------------------------------------------------------------
    # Lazy model loaders
    # ------------------------------------------------------------------

    def _get_segmentation_model(self):
        """Load DeepLabV3 (ResNet-101 backbone) for person segmentation."""
        if self._segmentation_model is None:
            from torchvision.models.segmentation import (
                deeplabv3_resnet101,
                DeepLabV3_ResNet101_Weights,
            )
            weights = DeepLabV3_ResNet101_Weights.DEFAULT
            self._segmentation_model = deeplabv3_resnet101(weights=weights).to(
                self.device
            )
            self._segmentation_model.eval()
            self._seg_transforms = weights.transforms()
            logger.info("DeepLabV3 segmentation model loaded")
        return self._segmentation_model

    def _get_dino_model(self):
        """Load DINOv2-small for 384D patch embeddings."""
        if self._dino_model is None:
            self._dino_model = torch.hub.load(
                "facebookresearch/dinov2", self.config["dino_model"]
            ).to(self.device)
            self._dino_model.eval()

            from torchvision import transforms as T
            self._dino_transform = T.Compose([
                T.Resize(256),
                T.CenterCrop(224),
                T.ToTensor(),
                T.Normalize(mean=[0.485, 0.456, 0.406],
                            std=[0.229, 0.224, 0.225]),
            ])
            logger.info("DINOv2 embedding model loaded")
        return self._dino_model

    def _get_face_mesh(self):
        """Initialise a MediaPipe FaceMesh instance (cached)."""
        if self._face_mesh is None:
            import mediapipe as mp
            self._face_mesh = mp.solutions.face_mesh.FaceMesh(
                static_image_mode=True,
                max_num_faces=1,
                refine_landmarks=True,
                min_detection_confidence=self.config["face_mesh_confidence"],
            )
            logger.info("MediaPipe FaceMesh initialised")
        return self._face_mesh

    # ------------------------------------------------------------------
    # 1. Image Preprocessing
    # ------------------------------------------------------------------

    def preprocess_images(
        self,
        image_paths: List[str],
        subject_id: str,
    ) -> dict:
        """Run segmentation, landmark extraction, and DINOv2 embedding
        extraction on all input images.

        Args:
            image_paths: List of file paths to input images.
            subject_id:  Unique identifier for this subject.

        Returns:
            dict with keys:
                'segmented_paths' -- list of paths to saved segmentation masks
                'landmarks'       -- list of np.ndarray [468, 3] per image
                'embeddings'      -- np.ndarray [N, 384]
                'poses'           -- np.ndarray [N, 7] (estimated camera poses)
        """
        from PIL import Image as PILImage

        seg_model = self._get_segmentation_model()
        dino_model = self._get_dino_model()
        face_mesh = self._get_face_mesh()

        output_dir = Path(self.config["output_dir"]) / subject_id / "preprocess"
        output_dir.mkdir(parents=True, exist_ok=True)

        segmented_paths: List[str] = []
        landmarks_list: List[np.ndarray] = []
        embeddings_list: List[np.ndarray] = []
        image_sizes: List[Tuple[int, int]] = []

        N = len(image_paths)

        for idx, img_path in enumerate(image_paths):
            logger.info("Preprocessing image %d/%d: %s", idx + 1, N,
                        Path(img_path).name)

            try:
                pil_img = PILImage.open(img_path).convert("RGB")
            except Exception as e:
                logger.error("Failed to open image %s: %s", img_path, e)
                # Append placeholder data so indices stay aligned.
                segmented_paths.append("")
                landmarks_list.append(np.zeros((468, 3), dtype=np.float32))
                embeddings_list.append(np.zeros(DINO_EMBEDDING_DIM, dtype=np.float32))
                image_sizes.append((256, 256))
                continue

            img_np = np.array(pil_img)
            h, w = img_np.shape[:2]
            image_sizes.append((w, h))

            # -- Segmentation (DeepLabV3) --
            try:
                seg_input = self._seg_transforms(pil_img).unsqueeze(0).to(self.device)
                with torch.no_grad():
                    seg_out = seg_model(seg_input)["out"]
                seg_pred = seg_out.argmax(1).squeeze().cpu().numpy()
                mask = (seg_pred == 15).astype(np.uint8)  # COCO class 15 = person

                mask_path = str(output_dir / f"mask_{idx:04d}.png")
                cv2.imwrite(mask_path, mask * 255)
                segmented_paths.append(mask_path)
            except Exception as e:
                logger.warning("Segmentation failed for %s: %s", img_path, e)
                segmented_paths.append("")
                mask = np.zeros((h, w), dtype=np.uint8)

            # -- Landmarks (MediaPipe) --
            try:
                img_bgr = cv2.cvtColor(img_np, cv2.COLOR_RGB2BGR)
                mp_result = face_mesh.process(img_bgr)
                if mp_result.multi_face_landmarks:
                    face_lms = mp_result.multi_face_landmarks[0]
                    landmarks = np.array(
                        [[lm.x, lm.y, lm.z] for lm in face_lms.landmark],
                        dtype=np.float32,
                    )
                else:
                    landmarks = np.zeros((468, 3), dtype=np.float32)
                    logger.warning("No face detected in %s", img_path)
            except Exception as e:
                logger.warning("Landmark extraction failed for %s: %s", img_path, e)
                landmarks = np.zeros((468, 3), dtype=np.float32)

            landmarks_list.append(landmarks)

            # -- DINOv2 embedding --
            try:
                dino_input = self._dino_transform(pil_img).unsqueeze(0).to(self.device)
                with torch.no_grad():
                    embedding = dino_model(dino_input)  # [1, 384]
                embedding = embedding.squeeze(0).cpu().numpy()
            except Exception as e:
                logger.warning("DINOv2 embedding failed for %s: %s", img_path, e)
                embedding = np.zeros(DINO_EMBEDDING_DIM, dtype=np.float32)

            embeddings_list.append(embedding)

        embeddings = np.stack(embeddings_list)  # [N, 384]

        # Estimate poses from landmarks
        poses = self.estimate_poses(landmarks_list, image_sizes)

        # Persist artifacts
        np.save(str(output_dir / "embeddings.npy"), embeddings)
        np.save(str(output_dir / "poses.npy"), poses)

        logger.info("Preprocessing complete for %d images (subject=%s)",
                     N, subject_id)

        return {
            "segmented_paths": segmented_paths,
            "landmarks": landmarks_list,
            "embeddings": embeddings,
            "poses": poses,
        }

    # ------------------------------------------------------------------
    # 2. Pose Estimation
    # ------------------------------------------------------------------

    def estimate_poses(
        self,
        landmarks_list: List[dict],
        image_sizes: List[tuple],
    ) -> np.ndarray:
        """Estimate camera poses from MediaPipe landmarks using solvePnP.

        Uses a canonical 3D face model and the 2D landmark projections to
        recover camera extrinsics via ``cv2.solvePnP``.  The camera matrix
        uses focal_length = image_width as a reasonable default.

        Args:
            landmarks_list: List of [468, 3] arrays (MediaPipe normalised
                            coords) or dicts with landmark data.  Each entry
                            can be an np.ndarray of shape (468, 3).
            image_sizes:    List of (width, height) tuples.

        Returns:
            np.ndarray of shape (N, 7) where each row is
            [x, y, z, qx, qy, qz, qw].
        """
        N = len(landmarks_list)
        poses = np.zeros((N, 7), dtype=np.float64)

        for idx in range(N):
            lms = landmarks_list[idx]

            # Accept either np.ndarray or dict with a 'landmarks' key.
            if isinstance(lms, dict):
                lms = lms.get("landmarks", np.zeros((468, 3), dtype=np.float32))
            lms = np.asarray(lms, dtype=np.float64)

            w, h = image_sizes[idx]

            # Extract the 2D pixel positions for our canonical landmark subset.
            pts_2d = np.array(
                [[lms[i, 0] * w, lms[i, 1] * h]
                 for i in CANONICAL_LANDMARK_INDICES],
                dtype=np.float64,
            )

            # Build camera intrinsic matrix.
            # focal length = image width is a reasonable default for webcam/phone.
            focal = float(w)
            cx, cy = w / 2.0, h / 2.0
            camera_matrix = np.array([
                [focal, 0.0,   cx],
                [0.0,   focal, cy],
                [0.0,   0.0,   1.0],
            ], dtype=np.float64)

            dist_coeffs = np.zeros((4, 1), dtype=np.float64)

            # Check whether landmarks look valid (non-zero).
            if np.allclose(pts_2d, 0.0):
                # No valid landmarks -- use an identity-like default pose
                # (camera at z=4 looking towards origin).
                poses[idx] = [0.0, 0.0, 4.0, 0.0, 0.0, 0.0, 1.0]
                logger.warning("Landmarks are all zero for image %d; "
                               "using default pose", idx)
                continue

            try:
                success, rvec, tvec = cv2.solvePnP(
                    CANONICAL_3D_FACE_POINTS,
                    pts_2d,
                    camera_matrix,
                    dist_coeffs,
                    flags=cv2.SOLVEPNP_ITERATIVE,
                )
            except cv2.error as e:
                logger.warning("solvePnP raised cv2.error for image %d: %s",
                               idx, e)
                poses[idx] = [0.0, 0.0, 4.0, 0.0, 0.0, 0.0, 1.0]
                continue

            if not success:
                logger.warning("solvePnP failed for image %d; using default pose",
                               idx)
                poses[idx] = [0.0, 0.0, 4.0, 0.0, 0.0, 0.0, 1.0]
                continue

            # Convert Rodrigues rotation vector to quaternion.
            quat = self._rvec_to_quaternion(rvec.flatten())
            poses[idx] = np.concatenate([tvec.flatten(), quat])

        logger.info("Pose estimation complete for %d images", N)
        return poses

    @staticmethod
    def _rvec_to_quaternion(rvec: np.ndarray) -> np.ndarray:
        """Convert a Rodrigues rotation vector to a unit quaternion
        (qx, qy, qz, qw).

        Uses cv2.Rodrigues to get the rotation matrix, then applies
        Shepperd's method for robust matrix-to-quaternion conversion.
        """
        R, _ = cv2.Rodrigues(rvec)

        # Shepperd's method for rotation matrix -> quaternion.
        trace = np.trace(R)
        if trace > 0:
            s = 0.5 / np.sqrt(trace + 1.0)
            qw = 0.25 / s
            qx = (R[2, 1] - R[1, 2]) * s
            qy = (R[0, 2] - R[2, 0]) * s
            qz = (R[1, 0] - R[0, 1]) * s
        elif R[0, 0] > R[1, 1] and R[0, 0] > R[2, 2]:
            s = 2.0 * np.sqrt(1.0 + R[0, 0] - R[1, 1] - R[2, 2])
            qw = (R[2, 1] - R[1, 2]) / s
            qx = 0.25 * s
            qy = (R[0, 1] + R[1, 0]) / s
            qz = (R[0, 2] + R[2, 0]) / s
        elif R[1, 1] > R[2, 2]:
            s = 2.0 * np.sqrt(1.0 + R[1, 1] - R[0, 0] - R[2, 2])
            qw = (R[0, 2] - R[2, 0]) / s
            qx = (R[0, 1] + R[1, 0]) / s
            qy = 0.25 * s
            qz = (R[1, 2] + R[2, 1]) / s
        else:
            s = 2.0 * np.sqrt(1.0 + R[2, 2] - R[0, 0] - R[1, 1])
            qw = (R[1, 0] - R[0, 1]) / s
            qx = (R[0, 2] + R[2, 0]) / s
            qy = (R[1, 2] + R[2, 1]) / s
            qz = 0.25 * s

        q = np.array([qx, qy, qz, qw], dtype=np.float64)
        q /= np.linalg.norm(q) + 1e-12  # enforce unit quaternion
        return q

    # ------------------------------------------------------------------
    # 3. Token Construction
    # ------------------------------------------------------------------

    def build_tokens(
        self,
        embeddings: np.ndarray,
        poses: np.ndarray,
        metadata: np.ndarray,
    ) -> torch.Tensor:
        """Construct 394D MTCM tokens from embeddings, poses, and metadata.

        Token layout (per token):
            [0:384]   -- DINOv2 visual embedding
            [384:391] -- pose [x, y, z, qx, qy, qz, qw]
            [391:394] -- metadata [image_index, total_images, face_coverage_ratio]

        Args:
            embeddings: np.ndarray of shape (N, 384) -- DINOv2 embeddings.
            poses:      np.ndarray of shape (N, 7) -- camera poses.
            metadata:   np.ndarray of shape (N, 3) -- per-image metadata.

        Returns:
            torch.Tensor of shape (N, 394).
        """
        N = embeddings.shape[0]
        assert embeddings.shape == (N, DINO_EMBEDDING_DIM), (
            f"Expected embeddings shape (N, {DINO_EMBEDDING_DIM}), "
            f"got {embeddings.shape}"
        )
        assert poses.shape == (N, 7), (
            f"Expected poses shape (N, 7), got {poses.shape}"
        )
        assert metadata.shape == (N, 3), (
            f"Expected metadata shape (N, 3), got {metadata.shape}"
        )

        tokens = np.zeros((N, TOKEN_DIM), dtype=np.float32)

        tokens[:, :DINO_EMBEDDING_DIM] = embeddings.astype(np.float32)
        tokens[:, DINO_EMBEDDING_DIM:DINO_EMBEDDING_DIM + 7] = poses.astype(np.float32)
        tokens[:, DINO_EMBEDDING_DIM + 7:TOKEN_DIM] = metadata.astype(np.float32)

        tokens_tensor = torch.from_numpy(tokens).float()

        logger.info("Built %d tokens of dimension %d", N, TOKEN_DIM)
        return tokens_tensor

    # ------------------------------------------------------------------
    # 4. MTCM View Selection
    # ------------------------------------------------------------------

    def select_views(
        self,
        tokens: torch.Tensor,
        images: torch.Tensor,
        poses: torch.Tensor,
        k: int = 5,
        checkpoint_path: Optional[str] = None,
    ) -> dict:
        """Run the MTCM transformer for view selection.

        Args:
            tokens: (N, 394) multimodal tokens.
            images: (N, H, W, 3) input images as float tensors in [0, 1].
            poses:  (N, 7) camera poses.
            k:      Number of views to select.
            checkpoint_path: Optional path to a trained MTCM checkpoint.

        Returns:
            dict with keys:
                'selected_indices'  -- np.ndarray [k] of selected image indices
                'selection_weights' -- np.ndarray [N] softmax probabilities
                'selected_images'   -- torch.Tensor [k, H, W, 3]
                'selected_poses'    -- torch.Tensor [k, 7]
        """
        from mtcm_mae.model import MTCM_MAE
        from mtcm_mae.config import MTCMConfig

        cfg = self.config

        model = MTCM_MAE(
            input_dim=cfg["mtcm_input_dim"],
            model_dim=cfg["mtcm_model_dim"],
            depth=cfg["mtcm_depth"],
            heads=cfg["mtcm_heads"],
            drop_path=cfg["mtcm_drop_path"],
            predict_weights=True,
            predict_poses=True,
        ).to(self.device)

        if checkpoint_path and os.path.isfile(checkpoint_path):
            ckpt = torch.load(checkpoint_path, map_location=self.device,
                              weights_only=False)
            state = ckpt.get("transformer_state_dict", ckpt)
            model.load_state_dict(state, strict=False)
            logger.info("Loaded MTCM checkpoint from %s", checkpoint_path)

        model.eval()

        # Add batch dimension: [1, N, 394]
        tokens_batch = tokens.unsqueeze(0).to(self.device)

        with torch.no_grad():
            outputs = model(tokens_batch)

        selection_weights_raw = outputs["selection_weights"].squeeze(0)  # [N]
        selection_probs = F.softmax(selection_weights_raw, dim=0)

        effective_k = min(k, selection_probs.shape[0])
        top_values, top_indices = torch.topk(selection_probs, k=effective_k)

        top_indices_np = top_indices.cpu().numpy()
        selection_probs_np = selection_probs.cpu().numpy()

        # Gather selected images and poses
        selected_images = images[top_indices_np]  # [k, H, W, 3]
        selected_poses = poses[top_indices_np]    # [k, 7]

        logger.info("Selected %d views: indices=%s, weights=%s",
                     effective_k,
                     top_indices_np.tolist(),
                     top_values.cpu().numpy().round(4).tolist())

        return {
            "selected_indices": top_indices_np,
            "selection_weights": selection_probs_np,
            "selected_images": selected_images,
            "selected_poses": selected_poses,
        }

    # ------------------------------------------------------------------
    # 5. NeRF Training
    # ------------------------------------------------------------------

    def train_nerf(
        self,
        images: torch.Tensor,
        poses: torch.Tensor,
        num_epochs: int = 50,
        num_rays: int = 1024,
        callback: Optional[Callable] = None,
        checkpoint_path: Optional[str] = None,
    ) -> dict:
        """Train WeightedTinyNeRF on selected views.

        Args:
            images:     (N, H, W, 3) float tensor of input images in [0, 1].
            poses:      (N, 7) float tensor of camera poses.
            num_epochs: Number of training epochs.
            num_rays:   Number of rays sampled per training step.
            callback:   Optional ``callback(epoch, metrics)`` called each epoch
                        for progress updates.  ``metrics`` is a dict with keys
                        like 'loss', 'psnr', 'epoch'.
            checkpoint_path: Optional path to a pre-trained NeRF checkpoint
                             to resume from.

        Returns:
            dict with keys:
                'checkpoint_path'   -- str, path to saved checkpoint
                'final_psnr'        -- float, PSNR from the last epoch
                'training_history'  -- list of dicts (one per epoch)
        """
        from nerf.tiny_nerf import TinyNeRF

        cfg = self.config
        H = cfg["nerf_image_height"]
        W = cfg["nerf_image_width"]

        # Ensure images are the right resolution and on device
        if images.shape[1] != H or images.shape[2] != W:
            # Resize: [N, H_in, W_in, 3] -> [N, 3, H_in, W_in] -> resize -> back
            imgs_perm = images.permute(0, 3, 1, 2).float()
            imgs_resized = F.interpolate(imgs_perm, size=(H, W), mode="bilinear",
                                         align_corners=False)
            images = imgs_resized.permute(0, 2, 3, 1).contiguous()

        images = images.to(self.device)
        poses = poses.to(self.device).float()
        N = images.shape[0]

        # Build model
        nerf = WeightedTinyNeRF(
            num_encoding_functions=cfg["nerf_encoding_functions"],
            hidden_dim=cfg["nerf_hidden_dim"],
            num_layers=cfg["nerf_num_layers"],
            image_height=H,
            image_width=W,
            top_k=min(cfg["nerf_top_k"], N),
        ).to(self.device)

        if checkpoint_path and os.path.isfile(checkpoint_path):
            ckpt = torch.load(checkpoint_path, map_location=self.device,
                              weights_only=False)
            state = ckpt.get("nerf_state_dict", ckpt)
            nerf.load_state_dict(state, strict=False)
            logger.info("Loaded NeRF checkpoint from %s", checkpoint_path)

        optimizer = torch.optim.Adam(nerf.parameters(),
                                      lr=cfg["nerf_learning_rate"])
        nerf.train()

        save_dir = Path(self.config["output_dir"]) / "nerf_checkpoints"
        save_dir.mkdir(parents=True, exist_ok=True)

        training_history: List[dict] = []
        best_loss = float("inf")
        final_psnr = 0.0

        for epoch in range(num_epochs):
            epoch_loss = 0.0
            epoch_psnr = 0.0
            num_steps = 0

            for v in range(N):
                target_pose = poses[v].unsqueeze(0)        # [1, 7]
                target_image = images[v].unsqueeze(0)      # [1, H, W, 3]

                # Use all views as input (the model handles selection internally
                # via its WeightedTinyNeRF selection mechanism).
                input_images = images.unsqueeze(0)   # [1, N, H, W, 3]
                input_poses = poses.unsqueeze(0)     # [1, N, 7]

                # Create uniform selection weights (equal weight for all views)
                selection_weights = torch.ones(1, N, device=self.device)

                try:
                    output = nerf(
                        selection_weights=selection_weights,
                        images=input_images,
                        poses=input_poses,
                        target_pose=target_pose,
                        target_image=target_image.permute(0, 3, 1, 2),
                        num_rays=num_rays,
                        train=True,
                    )
                except Exception as e:
                    logger.warning("NeRF forward pass failed for view %d: %s",
                                   v, e)
                    continue

                loss = output.get("loss", torch.tensor(0.0, device=self.device))

                optimizer.zero_grad()
                if loss.requires_grad:
                    loss.backward()
                    # Gradient clipping for stability
                    torch.nn.utils.clip_grad_norm_(nerf.parameters(), max_norm=1.0)
                    optimizer.step()

                epoch_loss += loss.item()
                epoch_psnr += output.get("psnr", torch.tensor(0.0)).item()
                num_steps += 1

            # Compute epoch averages
            avg_loss = epoch_loss / max(num_steps, 1)
            avg_psnr = epoch_psnr / max(num_steps, 1)
            final_psnr = avg_psnr

            epoch_metrics = {
                "epoch": epoch + 1,
                "loss": avg_loss,
                "psnr": avg_psnr,
            }
            training_history.append(epoch_metrics)

            logger.info("Epoch %d/%d  loss=%.6f  psnr=%.2f",
                        epoch + 1, num_epochs, avg_loss, avg_psnr)

            if callback is not None:
                try:
                    callback(epoch + 1, epoch_metrics)
                except Exception as e:
                    logger.warning("Training callback raised: %s", e)

            # Save best checkpoint
            if avg_loss < best_loss:
                best_loss = avg_loss
                best_path = str(save_dir / "nerf_best.pth")
                torch.save({
                    "epoch": epoch + 1,
                    "nerf_state_dict": nerf.state_dict(),
                    "optimizer_state_dict": optimizer.state_dict(),
                    "best_loss": best_loss,
                    "best_psnr": avg_psnr,
                }, best_path)

        # Save final checkpoint
        final_path = str(save_dir / "nerf_final.pth")
        torch.save({
            "epoch": num_epochs,
            "nerf_state_dict": nerf.state_dict(),
            "optimizer_state_dict": optimizer.state_dict(),
            "final_loss": avg_loss if num_steps > 0 else float("inf"),
            "final_psnr": final_psnr,
        }, final_path)

        self._trained_nerf = nerf
        self._last_checkpoint_path = final_path

        logger.info("NeRF training complete. Best loss: %.6f, Final PSNR: %.2f",
                     best_loss, final_psnr)

        return {
            "checkpoint_path": final_path,
            "final_psnr": final_psnr,
            "training_history": training_history,
        }

    # ------------------------------------------------------------------
    # 6. Mesh Extraction
    # ------------------------------------------------------------------

    def extract_mesh(
        self,
        checkpoint_path: Optional[str] = None,
        resolution: int = 128,
        threshold: float = 50.0,
    ) -> dict:
        """Extract a 3D mesh from a trained NeRF using marching cubes.

        If no ``checkpoint_path`` is given, uses the model from the most
        recent ``train_nerf()`` call, or falls back to the last saved
        checkpoint path.

        Args:
            checkpoint_path: Path to the NeRF checkpoint file.  If None,
                             uses the internally cached trained model.
            resolution:      Voxel grid resolution per axis.
            threshold:       Iso-value for marching cubes.

        Returns:
            dict with keys:
                'obj_path'  -- str, path to exported OBJ file
                'glb_path'  -- str, path to exported GLB file
                'vertices'  -- np.ndarray [V, 3]
                'faces'     -- np.ndarray [F, 3]
        """
        from nerf.tiny_nerf import TinyNeRF

        cfg = self.config
        bounds = cfg.get("mesh_bounds", (-1.5, 1.5))

        mesh_dir = Path(cfg["output_dir"]) / "meshes"
        mesh_dir.mkdir(parents=True, exist_ok=True)

        # Determine which NeRF model to use
        nerf_model = None

        if checkpoint_path and os.path.isfile(checkpoint_path):
            # Load from checkpoint
            logger.info("Loading NeRF from checkpoint: %s", checkpoint_path)
            ckpt = torch.load(checkpoint_path, map_location=self.device,
                              weights_only=False)
            state = ckpt.get("nerf_state_dict", ckpt)

            # Detect whether this is a WeightedTinyNeRF or bare TinyNeRF
            is_weighted = any(k.startswith("nerf.") for k in state.keys())

            if is_weighted:
                weighted = WeightedTinyNeRF(
                    num_encoding_functions=cfg["nerf_encoding_functions"],
                    hidden_dim=cfg["nerf_hidden_dim"],
                    num_layers=cfg["nerf_num_layers"],
                    image_height=cfg["nerf_image_height"],
                    image_width=cfg["nerf_image_width"],
                    top_k=cfg["nerf_top_k"],
                ).to(self.device)
                weighted.load_state_dict(state, strict=False)
                weighted.eval()
                nerf_model = weighted.nerf
            else:
                nerf_model = TinyNeRF(
                    num_encoding_functions=cfg["nerf_encoding_functions"],
                    hidden_dim=cfg["nerf_hidden_dim"],
                    num_layers=cfg["nerf_num_layers"],
                    image_height=cfg["nerf_image_height"],
                    image_width=cfg["nerf_image_width"],
                ).to(self.device)
                nerf_model.load_state_dict(state, strict=False)
                nerf_model.eval()
        elif self._trained_nerf is not None:
            # Use the model from the most recent train_nerf() call
            logger.info("Using cached trained NeRF model")
            if hasattr(self._trained_nerf, "nerf"):
                nerf_model = self._trained_nerf.nerf
            else:
                nerf_model = self._trained_nerf
            nerf_model.eval()
        elif self._last_checkpoint_path and os.path.isfile(self._last_checkpoint_path):
            # Recurse with the last checkpoint path
            return self.extract_mesh(
                checkpoint_path=self._last_checkpoint_path,
                resolution=resolution,
                threshold=threshold,
            )
        else:
            raise RuntimeError(
                "No NeRF model available for mesh extraction. "
                "Either provide a checkpoint_path, or call train_nerf() first."
            )

        # Run mesh extraction using the shared module
        logger.info("Extracting mesh at resolution %d, threshold %.1f",
                     resolution, threshold)

        try:
            vertices, faces, colors = mc_extract_mesh(
                nerf_model,
                resolution=resolution,
                density_threshold=threshold,
                bounds=bounds,
            )
        except RuntimeError as e:
            logger.error("Mesh extraction failed: %s", e)
            return {
                "obj_path": "",
                "glb_path": "",
                "vertices": np.array([]),
                "faces": np.array([]),
                "error": str(e),
            }

        # Export OBJ
        obj_path = str(mesh_dir / "avatar.obj")
        try:
            export_obj(vertices, faces, colors, obj_path)
        except Exception as e:
            logger.error("OBJ export failed: %s", e)
            obj_path = ""

        # Export GLB
        glb_path = str(mesh_dir / "avatar.glb")
        try:
            export_glb(vertices, faces, colors, glb_path)
        except (ImportError, Exception) as e:
            logger.warning("GLB export failed: %s", e)
            glb_path = ""

        logger.info("Mesh extraction complete: %d vertices, %d faces",
                     vertices.shape[0], faces.shape[0])

        return {
            "obj_path": obj_path,
            "glb_path": glb_path,
            "vertices": vertices,
            "faces": faces,
        }

    # ------------------------------------------------------------------
    # 7. Post-processing (relighting, rigging, animation)
    # ------------------------------------------------------------------

    def postprocess(
        self,
        mesh_result: dict,
        subject_id: str,
        enable_relighting: bool = True,
        enable_rigging: bool = True,
        enable_animation: bool = False,
    ) -> dict:
        """Run post-processing steps on the extracted mesh.

        This is a placeholder integration point for the Blender-based
        relighting pipeline and any rigging/animation steps.

        Args:
            mesh_result: Output dict from ``extract_mesh()``.
            subject_id:  Subject identifier.
            enable_relighting: Whether to run the relighting pass.
            enable_rigging:    Whether to auto-rig the mesh.
            enable_animation:  Whether to generate a turntable animation.

        Returns:
            dict with post-processing output paths.
        """
        output_dir = Path(self.config["output_dir"]) / subject_id / "postprocess"
        output_dir.mkdir(parents=True, exist_ok=True)

        result: dict = {"status": "complete"}

        obj_path = mesh_result.get("obj_path", "")

        if enable_relighting and obj_path and os.path.isfile(obj_path):
            logger.info("Relighting step: lighting scripts available in "
                        "BlenderRelighting/")
            result["relighting_note"] = (
                "Use BlenderRelighting/lighting.py with Blender to apply "
                "cinematic lighting to the exported mesh."
            )

        if enable_rigging and obj_path and os.path.isfile(obj_path):
            logger.info("Rigging: auto-rigging is available via external tools "
                        "(e.g. Mixamo, Rigify)")
            result["rigging_note"] = (
                "Upload the OBJ/GLB to Mixamo or use Blender Rigify for "
                "automatic skeletal rigging."
            )

        if enable_animation:
            logger.info("Animation generation is a future extension")
            result["animation_note"] = "Animation support is planned."

        return result

    # ------------------------------------------------------------------
    # Full Pipeline
    # ------------------------------------------------------------------

    def run_full_pipeline(
        self,
        image_paths: List[str],
        subject_id: str,
        progress_callback: Optional[Callable] = None,
        top_k: int = 5,
        nerf_epochs: int = 50,
        nerf_rays: int = 1024,
        mesh_resolution: int = 128,
        mesh_threshold: float = 50.0,
        mtcm_checkpoint: Optional[str] = None,
        nerf_checkpoint: Optional[str] = None,
    ) -> dict:
        """Run the complete end-to-end pipeline.

        Args:
            image_paths:       Paths to input images of the subject.
            subject_id:        Unique identifier for this subject.
            progress_callback: Optional ``progress_callback(stage, progress_pct,
                               message)`` for UI updates.
            top_k:             Number of views to select.
            nerf_epochs:       NeRF training epochs.
            nerf_rays:         Rays per training step.
            mesh_resolution:   Marching-cubes grid resolution.
            mesh_threshold:    Marching-cubes density iso-value.
            mtcm_checkpoint:   Optional pre-trained MTCM weights.
            nerf_checkpoint:   Optional pre-trained NeRF weights.

        Returns:
            dict with all outputs from each stage, including:
                subject_id, preprocess, poses, tokens_shape, view_selection,
                nerf_training, mesh_extraction, postprocess, timings.
        """
        from PIL import Image as PILImage

        def _notify(stage: str, pct: float, msg: str) -> None:
            if progress_callback is not None:
                try:
                    progress_callback(stage, pct, msg)
                except Exception:
                    pass
            logger.info("[%s] %.0f%% -- %s", stage, pct, msg)

        timings: Dict[str, float] = {}
        results: Dict[str, Any] = {"subject_id": subject_id}

        N = len(image_paths)
        if N == 0:
            logger.error("No images provided")
            return {"error": "No images provided"}

        # ==================================================================
        # Stage 1: Preprocessing
        # ==================================================================
        _notify("preprocessing", 0, f"Starting preprocessing of {N} images")
        t0 = time.time()

        try:
            preprocess_result = self.preprocess_images(image_paths, subject_id)
        except Exception as e:
            logger.error("Preprocessing failed: %s\n%s", e, traceback.format_exc())
            return {"error": f"Preprocessing failed: {e}"}

        timings["preprocessing"] = time.time() - t0
        results["preprocess"] = {
            "num_images": N,
            "segmented_paths": preprocess_result["segmented_paths"],
            "num_landmarks_detected": sum(
                1 for lm in preprocess_result["landmarks"]
                if not np.allclose(np.asarray(lm), 0.0)
            ),
        }
        _notify("preprocessing", 100, "Preprocessing complete")

        embeddings = preprocess_result["embeddings"]    # [N, 384]
        poses_np = preprocess_result["poses"]           # [N, 7]
        landmarks_list = preprocess_result["landmarks"]

        # ==================================================================
        # Stage 2: Pose estimation (already done inside preprocess_images)
        # ==================================================================
        results["poses"] = poses_np.tolist()

        # ==================================================================
        # Stage 3: Token construction
        # ==================================================================
        _notify("token_construction", 0, "Building MTCM tokens")
        t0 = time.time()

        # Build metadata: [image_index, total_images, face_coverage_ratio]
        metadata = np.zeros((N, 3), dtype=np.float32)
        for i in range(N):
            metadata[i, 0] = float(i)          # image_index
            metadata[i, 1] = float(N)          # total_images
            # face_coverage_ratio: fraction of image with detected face area
            lm = np.asarray(landmarks_list[i])
            if not np.allclose(lm, 0.0):
                # Estimate face bounding box from landmarks
                x_coords = lm[:, 0]
                y_coords = lm[:, 1]
                face_w = x_coords.max() - x_coords.min()
                face_h = y_coords.max() - y_coords.min()
                metadata[i, 2] = face_w * face_h  # normalised area
            else:
                metadata[i, 2] = 0.0

        tokens = self.build_tokens(embeddings, poses_np, metadata)
        timings["token_construction"] = time.time() - t0
        results["tokens_shape"] = list(tokens.shape)
        _notify("token_construction", 100, f"Built {N} tokens")

        # ==================================================================
        # Stage 4: MTCM view selection
        # ==================================================================
        _notify("view_selection", 0, "Running MTCM view selection")
        t0 = time.time()

        # Load images as tensors for the view selection and NeRF stages
        images_list = []
        for img_path in image_paths:
            try:
                pil_img = PILImage.open(img_path).convert("RGB")
                img_np = np.array(pil_img, dtype=np.float32) / 255.0
                images_list.append(torch.from_numpy(img_np))
            except Exception:
                images_list.append(torch.zeros(256, 256, 3))

        images_tensor = torch.stack(images_list)   # [N, H, W, 3]
        poses_tensor = torch.from_numpy(poses_np).float()   # [N, 7]

        try:
            view_selection = self.select_views(
                tokens=tokens,
                images=images_tensor,
                poses=poses_tensor,
                k=top_k,
                checkpoint_path=mtcm_checkpoint,
            )
        except Exception as e:
            logger.warning("MTCM view selection failed: %s; falling back to "
                           "first %d views", e, top_k)
            k_eff = min(top_k, N)
            view_selection = {
                "selected_indices": np.arange(k_eff),
                "selection_weights": np.ones(N) / N,
                "selected_images": images_tensor[:k_eff],
                "selected_poses": poses_tensor[:k_eff],
            }

        timings["view_selection"] = time.time() - t0
        results["view_selection"] = {
            "selected_indices": view_selection["selected_indices"].tolist(),
            "selection_weights": view_selection["selection_weights"].tolist(),
        }
        _notify("view_selection", 100,
                f"Selected views: {view_selection['selected_indices'].tolist()}")

        # ==================================================================
        # Stage 5: NeRF training
        # ==================================================================
        _notify("nerf_training", 0, "Starting NeRF training")
        t0 = time.time()

        def nerf_progress_callback(epoch: int, metrics: dict) -> None:
            pct = epoch / max(nerf_epochs, 1) * 100
            _notify("nerf_training", pct,
                    f"Epoch {epoch}/{nerf_epochs} "
                    f"loss={metrics.get('loss', 0):.6f} "
                    f"psnr={metrics.get('psnr', 0):.2f}")

        selected_images = view_selection["selected_images"]
        selected_poses = view_selection["selected_poses"]

        # Ensure tensors
        if isinstance(selected_images, np.ndarray):
            selected_images = torch.from_numpy(selected_images).float()
        if isinstance(selected_poses, np.ndarray):
            selected_poses = torch.from_numpy(selected_poses).float()

        try:
            nerf_result = self.train_nerf(
                images=selected_images,
                poses=selected_poses,
                num_epochs=nerf_epochs,
                num_rays=nerf_rays,
                callback=nerf_progress_callback,
                checkpoint_path=nerf_checkpoint,
            )
        except Exception as e:
            logger.error("NeRF training failed: %s\n%s", e, traceback.format_exc())
            nerf_result = {
                "checkpoint_path": "",
                "final_psnr": 0.0,
                "training_history": [],
                "error": str(e),
            }

        timings["nerf_training"] = time.time() - t0
        results["nerf_training"] = {
            "checkpoint_path": nerf_result["checkpoint_path"],
            "final_psnr": nerf_result["final_psnr"],
            "num_epochs": len(nerf_result["training_history"]),
        }
        _notify("nerf_training", 100,
                f"NeRF training complete. PSNR={nerf_result['final_psnr']:.2f}")

        # ==================================================================
        # Stage 6: Mesh extraction
        # ==================================================================
        _notify("mesh_extraction", 0, "Extracting 3D mesh")
        t0 = time.time()

        try:
            mesh_result = self.extract_mesh(
                checkpoint_path=nerf_result["checkpoint_path"],
                resolution=mesh_resolution,
                threshold=mesh_threshold,
            )
        except Exception as e:
            logger.error("Mesh extraction failed: %s\n%s", e,
                         traceback.format_exc())
            mesh_result = {
                "obj_path": "",
                "glb_path": "",
                "vertices": np.array([]),
                "faces": np.array([]),
                "error": str(e),
            }

        timings["mesh_extraction"] = time.time() - t0
        results["mesh_extraction"] = {
            "obj_path": mesh_result.get("obj_path", ""),
            "glb_path": mesh_result.get("glb_path", ""),
            "num_vertices": (mesh_result["vertices"].shape[0]
                             if len(mesh_result["vertices"]) > 0 else 0),
            "num_faces": (mesh_result["faces"].shape[0]
                          if len(mesh_result["faces"]) > 0 else 0),
        }
        _notify("mesh_extraction", 100, "Mesh extraction complete")

        # ==================================================================
        # Stage 7: Post-processing
        # ==================================================================
        _notify("postprocessing", 0, "Running post-processing")
        t0 = time.time()

        try:
            postprocess_result = self.postprocess(mesh_result, subject_id)
        except Exception as e:
            logger.warning("Post-processing failed: %s", e)
            postprocess_result = {"status": "skipped", "error": str(e)}

        timings["postprocessing"] = time.time() - t0
        results["postprocess"] = postprocess_result

        # ==================================================================
        # Summary
        # ==================================================================
        results["timings"] = timings
        total = sum(timings.values())
        results["total_time_seconds"] = total

        # Persist run manifest
        manifest_dir = Path(self.config["output_dir"]) / subject_id
        manifest_dir.mkdir(parents=True, exist_ok=True)
        manifest_path = manifest_dir / "pipeline_manifest.json"

        # Make the results JSON-serialisable
        serialisable = _make_json_serialisable(results)
        with open(str(manifest_path), "w") as f:
            json.dump(serialisable, f, indent=2, default=str)

        _notify("complete", 100,
                f"Pipeline complete in {total:.1f}s")

        logger.info("Pipeline complete for subject '%s'. Total time: %.1fs",
                     subject_id, total)
        return results


# ============================================================================
# JSON serialisation helper
# ============================================================================

def _make_json_serialisable(obj: Any) -> Any:
    """Recursively convert numpy/torch types to JSON-friendly Python types."""
    if isinstance(obj, dict):
        return {k: _make_json_serialisable(v) for k, v in obj.items()}
    elif isinstance(obj, (list, tuple)):
        return [_make_json_serialisable(v) for v in obj]
    elif isinstance(obj, np.ndarray):
        return obj.tolist()
    elif isinstance(obj, (np.integer,)):
        return int(obj)
    elif isinstance(obj, (np.floating,)):
        return float(obj)
    elif isinstance(obj, torch.Tensor):
        return obj.detach().cpu().numpy().tolist()
    elif isinstance(obj, Path):
        return str(obj)
    return obj


# ============================================================================
# CLI entry point
# ============================================================================

def _build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="Neural Character Generation Pipeline -- end-to-end",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    p.add_argument("--images", nargs="+", required=True,
                    help="Input image paths for the subject")
    p.add_argument("--subject-id", type=str, default="subject_01",
                    help="Subject identifier")
    p.add_argument("--output-dir", type=str, default="output",
                    help="Where to write results")
    p.add_argument("--device", type=str, default=None,
                    choices=["cuda", "cpu"],
                    help="Compute device (auto-detect if omitted)")
    p.add_argument("--top-k", type=int, default=5,
                    help="Number of views to select")
    p.add_argument("--nerf-epochs", type=int, default=50,
                    help="NeRF training epochs")
    p.add_argument("--nerf-rays", type=int, default=1024,
                    help="Rays per NeRF training step")
    p.add_argument("--mesh-resolution", type=int, default=128,
                    help="Marching-cubes grid resolution")
    p.add_argument("--mesh-threshold", type=float, default=50.0,
                    help="Marching-cubes density iso-value")
    p.add_argument("--mtcm-checkpoint", type=str, default=None,
                    help="Pre-trained MTCM weights (.pth)")
    p.add_argument("--nerf-checkpoint", type=str, default=None,
                    help="Pre-trained NeRF weights (.pth)")
    p.add_argument("--verbose", action="store_true",
                    help="Enable verbose (DEBUG-level) logging")
    return p


def main() -> None:
    args = _build_parser().parse_args()

    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
    )

    config = {**DEFAULT_CONFIG, "output_dir": args.output_dir}

    pipeline = AvatarPipeline(config=config, device=args.device)

    def cli_progress(stage: str, pct: float, msg: str) -> None:
        print(f"  [{stage}] {pct:5.1f}%  {msg}")

    results = pipeline.run_full_pipeline(
        image_paths=args.images,
        subject_id=args.subject_id,
        progress_callback=cli_progress,
        top_k=args.top_k,
        nerf_epochs=args.nerf_epochs,
        nerf_rays=args.nerf_rays,
        mesh_resolution=args.mesh_resolution,
        mesh_threshold=args.mesh_threshold,
        mtcm_checkpoint=args.mtcm_checkpoint,
        nerf_checkpoint=args.nerf_checkpoint,
    )

    print("\n=== Pipeline Results ===")
    print(f"Subject:           {results['subject_id']}")
    vs = results.get("view_selection", {})
    print(f"Views selected:    {vs.get('selected_indices', [])}")
    nr = results.get("nerf_training", {})
    print(f"NeRF checkpoint:   {nr.get('checkpoint_path', 'N/A')}")
    print(f"Final PSNR:        {nr.get('final_psnr', 0):.2f}")
    me = results.get("mesh_extraction", {})
    print(f"Mesh (OBJ):        {me.get('obj_path', 'N/A')}")
    print(f"Mesh (GLB):        {me.get('glb_path', 'N/A')}")
    print(f"Mesh vertices:     {me.get('num_vertices', 0)}")
    print(f"Mesh faces:        {me.get('num_faces', 0)}")
    print(f"Total time:        {results.get('total_time_seconds', 0):.1f}s")
    print("\nTiming breakdown:")
    for stage, t in results.get("timings", {}).items():
        print(f"  {stage:20s}  {t:8.2f}s")


if __name__ == "__main__":
    main()
