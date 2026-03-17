"""
Neural Character Generation -- Flask Application
==================================================

Full-pipeline Flask app for the neural character generation demo.

Working pipeline stages:
  1. Upload & file management
  2. Segmentation (DeepLabV3+)
  3. Facial landmarks (MediaPipe Face Mesh)
  4. Visual embeddings (DINOv2 / ResNet50 fallback)
  5. Camera pose estimation (solvePnP from landmarks)
  6. NeRF training (background thread)
  7. Mesh extraction (marching cubes)
  8. Relighting presets
  9. Auto-rigging (skeleton + blend shapes)
 10. Export (GLB / OBJ)
 11. 3D viewer (Three.js)
"""

from flask import (
    Flask, render_template, request, redirect, url_for,
    jsonify, send_from_directory,
)
import os
import sys
import time
import uuid
import json
import logging
import traceback
import functools
import threading
from werkzeug.utils import secure_filename

import cv2
import numpy as np
import torch
import torchvision
from torchvision import transforms
from PIL import Image
import mediapipe as mp

# ---------------------------------------------------------------------------
# Pipeline module imports
# ---------------------------------------------------------------------------
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))
from mesh_extraction import extract_mesh, export_obj, export_glb
from postprocessing import MeshRelighter, SimpleRigger, FaceAnimator

# ---------------------------------------------------------------------------
# Logging
# ---------------------------------------------------------------------------
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler("avatar_app.log"),
        logging.StreamHandler(),
    ],
)
logger = logging.getLogger("AvatarApp")

# ---------------------------------------------------------------------------
# Flask app
# ---------------------------------------------------------------------------
app = Flask(__name__)

app.config['UPLOAD_FOLDER'] = 'uploads'
app.config['DATA_FOLDER'] = 'data'
app.config['ALLOWED_EXTENSIONS'] = {'jpg', 'jpeg', 'png', 'mp4'}
app.config['MAX_CONTENT_LENGTH'] = 32 * 1024 * 1024  # 32 MB

# Create all required directories
for _subdir in [
    'dataset', 'preprocessed', 'landmarks', 'embeddings', 'masks',
    'llff', 'outputs', 'checkpoints', 'meshes',
]:
    os.makedirs(os.path.join(app.config['DATA_FOLDER'], _subdir), exist_ok=True)
os.makedirs(app.config['UPLOAD_FOLDER'], exist_ok=True)

# ---------------------------------------------------------------------------
# Training job registry  (simple dict, no Celery)
# ---------------------------------------------------------------------------
training_jobs = {}  # training_id -> {status, epoch, total_epochs, psnr, loss, subject_id}
_training_lock = threading.Lock()

# ---------------------------------------------------------------------------
# Canonical 3D face model for solvePnP (millimetres, frontal pose)
# ---------------------------------------------------------------------------
_FACE_3D_MODEL_POINTS = np.array([
    [  0.0,    0.0,    0.0],    # nose tip
    [  0.0,  -63.6,  -12.5],   # chin
    [-42.0,   32.0,  -26.0],   # left eye outer corner
    [ 42.0,   32.0,  -26.0],   # right eye outer corner
    [-28.0,  -28.9,  -20.0],   # left mouth corner
    [ 28.0,  -28.9,  -20.0],   # right mouth corner
], dtype=np.float32)

# MediaPipe landmark indices matching the rows above
_MP_LANDMARK_INDICES = [1, 152, 263, 33, 287, 57]

# ---------------------------------------------------------------------------
# Global model instances (lazy-loaded)
# ---------------------------------------------------------------------------
segmentation_model = None
embedding_model = None
mp_face_mesh = None


def load_segmentation_model():
    """Load DeepLabV3+ model for segmentation."""
    global segmentation_model
    if segmentation_model is None:
        try:
            logger.info("Loading DeepLabV3+ segmentation model...")
            segmentation_model = torchvision.models.segmentation.deeplabv3_resnet101(
                pretrained=True,
            )
            device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
            segmentation_model.to(device).eval()
            logger.info("Segmentation model loaded (device: %s)", device)
        except Exception as e:
            logger.error("Error loading segmentation model: %s", e)
            raise
    return segmentation_model


def load_embedding_model():
    """Load DINOv2 (or ResNet50 fallback) for embedding extraction."""
    global embedding_model
    if embedding_model is None:
        try:
            logger.info("Loading DINOv2 model for embeddings...")
            try:
                embedding_model = torch.hub.load(
                    'facebookresearch/dinov2', 'dinov2_vits14',
                )
            except Exception:
                logger.info("DINOv2 unavailable -- falling back to ResNet50")
                embedding_model = torch.hub.load(
                    'pytorch/vision:v0.10.0', 'resnet50', pretrained=True,
                )
                embedding_model = torch.nn.Sequential(
                    *list(embedding_model.children())[:-1]
                )
            device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
            embedding_model.to(device).eval()
            logger.info("Embedding model loaded (device: %s)", device)
        except Exception as e:
            logger.error("Error loading embedding model: %s", e)
            raise
    return embedding_model


def load_face_mesh():
    """Load MediaPipe Face Mesh model."""
    global mp_face_mesh
    if mp_face_mesh is None:
        try:
            logger.info("Initialising MediaPipe Face Mesh...")
            mp_face_mesh = mp.solutions.face_mesh.FaceMesh(
                static_image_mode=True,
                max_num_faces=1,
                refine_landmarks=True,
                min_detection_confidence=0.5,
            )
            logger.info("MediaPipe Face Mesh initialised")
        except Exception as e:
            logger.error("Error initialising MediaPipe Face Mesh: %s", e)
            raise
    return mp_face_mesh


# ---------------------------------------------------------------------------
# Utility decorators / helpers
# ---------------------------------------------------------------------------

def handle_exceptions(route_function):
    """Decorator that catches unhandled exceptions and renders error.html."""
    @functools.wraps(route_function)
    def wrapper(*args, **kwargs):
        try:
            return route_function(*args, **kwargs)
        except Exception as e:
            logger.error("Error in %s: %s", route_function.__name__, e)
            logger.error(traceback.format_exc())
            return render_template('error.html', message=f"An error occurred: {e}"), 500
    return wrapper


def allowed_file(filename):
    """Return True if *filename* has an allowed extension."""
    return '.' in filename and \
           filename.rsplit('.', 1)[1].lower() in app.config['ALLOWED_EXTENSIONS']


# ===================================================================
#  STAGE 1 -- Image segmentation (DeepLabV3+)
# ===================================================================

def process_image_segmentation(img_path, output_path):
    """Segment a person from a single image and save an RGBA PNG."""
    try:
        img = cv2.imread(img_path)
        if img is None:
            logger.error("Failed to load image: %s", img_path)
            return False

        pil_img = Image.fromarray(cv2.cvtColor(img, cv2.COLOR_BGR2RGB))

        model = load_segmentation_model()
        device = next(model.parameters()).device

        preprocess = transforms.Compose([
            transforms.Resize((480, 480)),
            transforms.ToTensor(),
            transforms.Normalize(
                mean=[0.485, 0.456, 0.406],
                std=[0.229, 0.224, 0.225],
            ),
        ])
        input_tensor = preprocess(pil_img).unsqueeze(0).to(device)

        with torch.no_grad():
            output = model(input_tensor)['out'][0]
            output_predictions = output.argmax(0).byte().cpu().numpy()

        PERSON_CLASS = 15
        binary_mask = (output_predictions == PERSON_CLASS).astype(np.uint8) * 255

        kernel = np.ones((5, 5), np.uint8)
        binary_mask = cv2.dilate(binary_mask, kernel, iterations=1)
        binary_mask = cv2.morphologyEx(binary_mask, cv2.MORPH_CLOSE, kernel)
        binary_mask = cv2.GaussianBlur(binary_mask, (7, 7), 0)

        h, w = img.shape[:2]
        binary_mask = cv2.resize(binary_mask, (w, h), interpolation=cv2.INTER_NEAREST)

        rgba = cv2.cvtColor(img, cv2.COLOR_BGR2BGRA)
        rgba[:, :, 3] = binary_mask

        os.makedirs(os.path.dirname(output_path), exist_ok=True)
        cv2.imwrite(output_path, rgba)
        logger.info("Segmentation OK: %s -> %s", img_path, output_path)
        return True

    except Exception as e:
        logger.error("Error segmenting %s: %s", img_path, e)
        logger.error(traceback.format_exc())
        return False


# ===================================================================
#  STAGE 2 -- Facial landmark extraction (MediaPipe)
# ===================================================================

def extract_landmarks(img_path, json_path):
    """Extract facial landmarks from an image and save as JSON."""
    try:
        img = cv2.imread(img_path)
        if img is None:
            logger.error("Failed to load image: %s", img_path)
            return False

        rgb_img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        face_mesh = load_face_mesh()
        results = face_mesh.process(rgb_img)

        if not results.multi_face_landmarks:
            logger.warning("No face detected in %s", img_path)
            return False

        face_landmarks = [
            {'idx': idx, 'x': lm.x, 'y': lm.y, 'z': lm.z}
            for idx, lm in enumerate(results.multi_face_landmarks[0].landmark)
        ]

        landmark_data = {
            'face': face_landmarks,
            'image_info': {
                'height': img.shape[0],
                'width': img.shape[1],
                'channels': img.shape[2],
            },
        }

        os.makedirs(os.path.dirname(json_path), exist_ok=True)
        with open(json_path, 'w') as f:
            json.dump(landmark_data, f, indent=2)

        logger.info("Landmarks extracted: %s -> %s", img_path, json_path)
        return True

    except Exception as e:
        logger.error("Error extracting landmarks from %s: %s", img_path, e)
        logger.error(traceback.format_exc())
        return False


# ===================================================================
#  STAGE 3 -- Visual embedding extraction (DINOv2 / ResNet50)
# ===================================================================

def extract_embedding(img_path, npy_path):
    """Extract a visual embedding from an image and save as .npy."""
    try:
        img = Image.open(img_path).convert('RGB')

        model = load_embedding_model()
        device = next(model.parameters()).device

        transform = transforms.Compose([
            transforms.Resize(256),
            transforms.CenterCrop(224),
            transforms.ToTensor(),
            transforms.Normalize(
                mean=[0.485, 0.456, 0.406],
                std=[0.229, 0.224, 0.225],
            ),
        ])
        img_tensor = transform(img).unsqueeze(0).to(device)

        with torch.no_grad():
            embedding = model(img_tensor).cpu().numpy()[0]

        os.makedirs(os.path.dirname(npy_path), exist_ok=True)
        np.save(npy_path, embedding)
        logger.info("Embedding extracted: %s -> %s", img_path, npy_path)
        return True

    except Exception as e:
        logger.error("Error extracting embedding from %s: %s", img_path, e)
        logger.error(traceback.format_exc())
        return False


# ===================================================================
#  STAGE 4 -- Camera pose estimation (solvePnP from landmarks)
# ===================================================================

def estimate_camera_poses(subject_id):
    """Estimate camera poses from landmarks using solvePnP.

    For every image that has a landmark JSON, this function:
      1. Reads the stored MediaPipe landmarks.
      2. Picks the six canonical face-model correspondences.
      3. Builds a pinhole camera matrix (focal_length = image_width).
      4. Calls ``cv2.solvePnP`` to recover rotation + translation.
      5. Assembles a 3x5 LLFF-style camera matrix row (R|t columns,
         then height, width, focal) and a near/far depth bound.

    The results are concatenated into ``poses_bounds.npy`` with shape
    ``(N, 17)`` -- the standard LLFF format expected by NeRF loaders.

    Returns:
        True on success, False if no valid poses could be estimated.
    """
    try:
        logger.info("Estimating camera poses for subject %s", subject_id)

        landmarks_base = os.path.join(
            app.config['DATA_FOLDER'], 'landmarks', subject_id,
        )
        llff_dir = os.path.join(app.config['DATA_FOLDER'], 'llff', subject_id)
        os.makedirs(llff_dir, exist_ok=True)

        # Collect all landmark JSONs (recurse into pose sub-dirs)
        landmark_files = []
        for root, _dirs, files in os.walk(landmarks_base):
            for fname in sorted(files):
                if fname.endswith('.json'):
                    landmark_files.append(os.path.join(root, fname))

        if not landmark_files:
            logger.warning("No landmark files found for subject %s", subject_id)
            return False

        all_poses_bounds = []
        valid_indices = []

        for file_idx, lm_path in enumerate(landmark_files):
            with open(lm_path, 'r') as f:
                lm_data = json.load(f)

            face_lms = lm_data.get('face', [])
            img_info = lm_data.get('image_info', {})
            h = img_info.get('height', 480)
            w = img_info.get('width', 640)

            # Build a lookup by landmark index
            lm_by_idx = {lm['idx']: lm for lm in face_lms}

            # Check that all required indices are present
            if not all(idx in lm_by_idx for idx in _MP_LANDMARK_INDICES):
                logger.warning(
                    "Landmark file %s missing required indices -- skipping",
                    lm_path,
                )
                continue

            # 2D image points (pixel coords)
            image_points = np.array([
                [lm_by_idx[idx]['x'] * w, lm_by_idx[idx]['y'] * h]
                for idx in _MP_LANDMARK_INDICES
            ], dtype=np.float64)

            # Camera intrinsics
            focal_length = float(w)
            cx, cy = w / 2.0, h / 2.0
            camera_matrix = np.array([
                [focal_length, 0.0, cx],
                [0.0, focal_length, cy],
                [0.0, 0.0, 1.0],
            ], dtype=np.float64)
            dist_coeffs = np.zeros((4, 1), dtype=np.float64)

            # solvePnP
            success, rvec, tvec = cv2.solvePnP(
                _FACE_3D_MODEL_POINTS.astype(np.float64),
                image_points,
                camera_matrix,
                dist_coeffs,
                flags=cv2.SOLVEPNP_ITERATIVE,
            )
            if not success:
                logger.warning("solvePnP failed for %s", lm_path)
                continue

            # Rotation matrix from Rodrigues vector
            R, _ = cv2.Rodrigues(rvec)

            # Build a 3x5 matrix:  [R | t | hwf]
            # Column 0-2: rotation, column 3: translation, column 4: [h, w, f]
            pose_34 = np.concatenate([R, tvec.reshape(3, 1)], axis=1)  # 3x4
            hwf_col = np.array([[h], [w], [focal_length]], dtype=np.float64)
            pose_35 = np.concatenate([pose_34, hwf_col], axis=1)  # 3x5

            # Flatten to 15 values, then append near/far bounds
            flat = pose_35.flatten()  # 15
            t_norm = np.linalg.norm(tvec)
            near = max(0.1, t_norm - 200.0)
            far = t_norm + 200.0
            row = np.concatenate([flat, [near, far]])  # 17

            all_poses_bounds.append(row)
            valid_indices.append(file_idx)

        if not all_poses_bounds:
            logger.error(
                "No valid camera poses could be estimated for subject %s",
                subject_id,
            )
            return False

        poses_bounds = np.array(all_poses_bounds, dtype=np.float64)
        np.save(os.path.join(llff_dir, 'poses_bounds.npy'), poses_bounds)

        # Write train/val split (80/20)
        n = len(valid_indices)
        split = max(1, int(n * 0.8))
        train_ids = valid_indices[:split]
        val_ids = valid_indices[split:] if split < n else []

        with open(os.path.join(llff_dir, 'train_ids.txt'), 'w') as f:
            f.write('\n'.join(str(i) for i in train_ids) + '\n')

        with open(os.path.join(llff_dir, 'val_ids.txt'), 'w') as f:
            f.write('\n'.join(str(i) for i in val_ids) + '\n')

        logger.info(
            "Camera pose estimation complete for %s: %d poses "
            "(%d train, %d val)",
            subject_id, n, len(train_ids), len(val_ids),
        )
        return True

    except Exception as e:
        logger.error("Error estimating poses for %s: %s", subject_id, e)
        logger.error(traceback.format_exc())
        return False


# ===================================================================
#  Subject processing  (segmentation + landmarks + embeddings + poses)
# ===================================================================

def process_subject(subject_id):
    """Run the full preprocessing pipeline for a subject."""
    try:
        logger.info("Starting processing for subject %s", subject_id)

        subject_dataset_dir = os.path.join(
            app.config['DATA_FOLDER'], 'dataset', subject_id,
        )
        subject_preprocessed_dir = os.path.join(
            app.config['DATA_FOLDER'], 'preprocessed', subject_id,
        )
        subject_landmarks_dir = os.path.join(
            app.config['DATA_FOLDER'], 'landmarks', subject_id,
        )
        subject_embeddings_dir = os.path.join(
            app.config['DATA_FOLDER'], 'embeddings', subject_id,
        )

        poses_dir = os.path.join(subject_dataset_dir, 'poses')
        if os.path.exists(poses_dir):
            for pose in os.listdir(poses_dir):
                pose_dir = os.path.join(poses_dir, pose)
                if not os.path.isdir(pose_dir):
                    continue

                preprocessed_pose_dir = os.path.join(
                    subject_preprocessed_dir, 'poses', pose,
                )
                landmarks_pose_dir = os.path.join(
                    subject_landmarks_dir, 'poses', pose,
                )
                embeddings_pose_dir = os.path.join(
                    subject_embeddings_dir, 'poses', pose,
                )
                os.makedirs(preprocessed_pose_dir, exist_ok=True)
                os.makedirs(landmarks_pose_dir, exist_ok=True)
                os.makedirs(embeddings_pose_dir, exist_ok=True)

                image_files = [
                    f for f in os.listdir(pose_dir)
                    if f.lower().endswith(('.jpg', '.jpeg', '.png'))
                ]
                logger.info(
                    "Processing %d images for %s pose", len(image_files), pose,
                )

                for img_file in image_files:
                    img_path = os.path.join(pose_dir, img_file)
                    base_name = os.path.splitext(img_file)[0]

                    preprocessed_path = os.path.join(
                        preprocessed_pose_dir, f"{base_name}.png",
                    )
                    landmarks_path = os.path.join(
                        landmarks_pose_dir, f"{base_name}.json",
                    )
                    embeddings_path = os.path.join(
                        embeddings_pose_dir, f"{base_name}.npy",
                    )

                    # Skip already-processed images
                    if (os.path.exists(preprocessed_path)
                            and os.path.exists(landmarks_path)
                            and os.path.exists(embeddings_path)):
                        logger.info("Skipping already processed: %s", img_file)
                        continue

                    # Step 1 -- Segmentation
                    if not os.path.exists(preprocessed_path):
                        if not process_image_segmentation(img_path, preprocessed_path):
                            logger.error(
                                "Segmentation failed for %s -- skipping", img_file,
                            )
                            continue

                    # Step 2 -- Landmarks
                    if not os.path.exists(landmarks_path):
                        extract_landmarks(preprocessed_path, landmarks_path)

                    # Step 3 -- Embeddings
                    if not os.path.exists(embeddings_path):
                        extract_embedding(preprocessed_path, embeddings_path)

        # Step 4 -- Camera pose estimation (replaces old LLFF placeholder)
        estimate_camera_poses(subject_id)

        logger.info("Processing completed for subject %s", subject_id)
        return True

    except Exception as e:
        logger.error("Error processing subject %s: %s", subject_id, e)
        logger.error(traceback.format_exc())
        return False


# ===================================================================
#  File upload helpers
# ===================================================================

def save_uploaded_files(subject_id, uploaded_files=None):
    """Save uploaded files to the dataset directory.

    Supports both pose-specific uploads (``pose_front``, ``pose_left``, etc.)
    and a generic ``image_files`` upload that goes into the *front* bucket.
    """
    try:
        logger.info("Saving uploaded files for subject %s", subject_id)

        poses = ['front', 'left', 'right', 'up', 'down']

        subject_dir = os.path.join(
            app.config['DATA_FOLDER'], 'dataset', subject_id,
        )
        poses_dir = os.path.join(subject_dir, 'poses')
        os.makedirs(poses_dir, exist_ok=True)

        # Detect pose-specific uploads
        pose_specific = any(
            request.files.getlist(f'pose_{p}')
            and request.files.getlist(f'pose_{p}')[0].filename
            for p in poses
        )

        processed_poses = []

        if pose_specific:
            for pose in poses:
                pose_files = request.files.getlist(f'pose_{pose}')
                if not (pose_files and pose_files[0].filename):
                    continue
                pose_dir = os.path.join(poses_dir, pose)
                os.makedirs(pose_dir, exist_ok=True)
                existing = [
                    f for f in os.listdir(pose_dir)
                    if f.lower().endswith(('.jpg', '.jpeg', '.png'))
                ]
                start_index = len(existing)
                for i, file in enumerate(pose_files):
                    if file and file.filename:
                        filename = f"{start_index + i:03d}.jpg"
                        file.save(os.path.join(pose_dir, filename))
                processed_poses.append(pose)
        else:
            files_to_process = (
                uploaded_files
                if uploaded_files
                else request.files.getlist('image_files')
            )
            if files_to_process and files_to_process[0].filename:
                front_dir = os.path.join(poses_dir, 'front')
                os.makedirs(front_dir, exist_ok=True)
                existing = [
                    f for f in os.listdir(front_dir)
                    if f.lower().endswith(('.jpg', '.jpeg', '.png'))
                ]
                start_index = len(existing)
                for i, file in enumerate(files_to_process):
                    if file and file.filename:
                        filename = f"{start_index + i:03d}.jpg"
                        file.save(os.path.join(front_dir, filename))
                processed_poses.append('front')

        logger.info("Files saved for poses: %s", processed_poses)
        return processed_poses

    except Exception as e:
        logger.error("Error saving uploads for %s: %s", subject_id, e)
        logger.error(traceback.format_exc())
        raise


# ===================================================================
#  Helpers
# ===================================================================

def _rotation_matrix_to_quaternion(R):
    """Convert a 3x3 rotation matrix to [qx, qy, qz, qw] (Shepperd's method)."""
    trace = R[0, 0] + R[1, 1] + R[2, 2]
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
    return np.array([qx, qy, qz, qw], dtype=np.float32)


def _llff_pose_to_7d(pose_row):
    """Convert an LLFF pose row (17D) to 7D [x, y, z, qx, qy, qz, qw]."""
    pose_mat = pose_row[:15].reshape(3, 5)
    R = pose_mat[:, :3]
    t = pose_mat[:, 3]
    q = _rotation_matrix_to_quaternion(R)
    return np.concatenate([t, q]).astype(np.float32)


def _load_mesh_data(obj_path):
    """Load an OBJ mesh and return (vertices, faces, colors) as numpy arrays.

    Falls back to trimesh if available, otherwise parses OBJ manually.
    Vertex colors default to 0.7 grey if not present.
    """
    try:
        import trimesh
        mesh = trimesh.load(obj_path, process=False)
        vertices = np.asarray(mesh.vertices, dtype=np.float64)
        faces = np.asarray(mesh.faces, dtype=np.int64)
        if (hasattr(mesh.visual, 'vertex_colors')
                and mesh.visual.vertex_colors is not None
                and len(mesh.visual.vertex_colors) == len(vertices)):
            colors = mesh.visual.vertex_colors[:, :3].astype(np.float64) / 255.0
        else:
            colors = np.full((len(vertices), 3), 0.7)
        return vertices, faces, colors
    except ImportError:
        pass

    # Manual OBJ parser (supports "v x y z r g b" extension)
    vertices, faces, colors = [], [], []
    with open(obj_path, 'r') as f:
        for line in f:
            parts = line.strip().split()
            if not parts:
                continue
            if parts[0] == 'v' and len(parts) >= 4:
                vertices.append([float(parts[1]), float(parts[2]), float(parts[3])])
                if len(parts) >= 7:
                    colors.append([float(parts[4]), float(parts[5]), float(parts[6])])
            elif parts[0] == 'f':
                face_verts = [int(p.split('/')[0]) - 1 for p in parts[1:4]]
                faces.append(face_verts)
    vertices = np.array(vertices, dtype=np.float64)
    faces = np.array(faces, dtype=np.int64)
    if len(colors) == len(vertices):
        colors = np.array(colors, dtype=np.float64)
    else:
        colors = np.full((len(vertices), 3), 0.7)
    return vertices, faces, colors


# ===================================================================
#  Background training
# ===================================================================

def _run_training(training_id, subject_id, config):
    """Background thread that trains a NeRF for *subject_id*.

    Updates ``training_jobs[training_id]`` in-place so the status
    endpoint can poll progress.
    """
    num_epochs = config.get('num_epochs', 50)
    num_rays = config.get('num_rays', 4096)
    learning_rate = config.get('learning_rate', 5e-4)

    with _training_lock:
        training_jobs[training_id] = {
            'status': 'initialising',
            'epoch': 0,
            'total_epochs': num_epochs,
            'psnr': 0.0,
            'loss': float('inf'),
            'best_psnr': 0.0,
            'subject_id': subject_id,
        }

    try:
        from nerf.tiny_nerf import TinyNeRF

        device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
        logger.info(
            "Training %s on %s  (epochs=%d, rays=%d, lr=%g)",
            subject_id, device, num_epochs, num_rays, learning_rate,
        )

        # Load poses_bounds for this subject
        llff_dir = os.path.join(app.config['DATA_FOLDER'], 'llff', subject_id)
        poses_path = os.path.join(llff_dir, 'poses_bounds.npy')
        if not os.path.isfile(poses_path):
            raise FileNotFoundError(
                f"poses_bounds.npy not found for {subject_id}. "
                "Run preprocessing first."
            )

        poses_bounds = np.load(poses_path)

        # Convert LLFF poses to 7D vectors for TinyNeRF
        poses_7d = np.array(
            [_llff_pose_to_7d(row) for row in poses_bounds],
            dtype=np.float32,
        )
        poses_tensor = torch.tensor(poses_7d, device=device)  # (N, 7)

        # Collect training images
        preprocessed_dir = os.path.join(
            app.config['DATA_FOLDER'], 'preprocessed', subject_id,
        )
        image_paths = []
        for root, _dirs, files in os.walk(preprocessed_dir):
            for fname in sorted(files):
                if fname.lower().endswith(('.png', '.jpg', '.jpeg')):
                    image_paths.append(os.path.join(root, fname))

        if not image_paths:
            raise FileNotFoundError(
                f"No preprocessed images for {subject_id}."
            )

        # Load images into tensors (C, H, W format for target_image)
        img_transform = transforms.Compose([
            transforms.Resize((256, 256)),
            transforms.ToTensor(),
        ])
        images = []
        for p in image_paths:
            img = Image.open(p).convert('RGB')
            images.append(img_transform(img))
        images_tensor = torch.stack(images).to(device)  # (N, 3, H, W)

        # Instantiate model
        model = TinyNeRF(image_height=256, image_width=256).to(device)
        optimiser = torch.optim.Adam(model.parameters(), lr=learning_rate)

        n_images = images_tensor.shape[0]
        best_psnr = 0.0
        checkpoint_dir = os.path.join(
            app.config['DATA_FOLDER'], 'checkpoints', subject_id,
        )
        os.makedirs(checkpoint_dir, exist_ok=True)

        with _training_lock:
            training_jobs[training_id]['status'] = 'training'

        for epoch in range(1, num_epochs + 1):
            model.train()
            epoch_loss = 0.0

            for img_idx in range(n_images):
                pose_idx = min(img_idx, len(poses_7d) - 1)
                target_pose = poses_tensor[pose_idx].unsqueeze(0)       # [1, 7]
                target_image = images_tensor[img_idx].unsqueeze(0)      # [1, 3, H, W]

                # TinyNeRF.forward(poses, images, target_pose, target_image, num_rays, train)
                outputs = model(
                    poses=target_pose.unsqueeze(1),   # [1, 1, 7]
                    images=None,
                    target_pose=target_pose,
                    target_image=target_image,
                    num_rays=num_rays,
                    train=True,
                )

                loss = outputs['loss']
                optimiser.zero_grad()
                loss.backward()
                optimiser.step()
                epoch_loss += loss.item()

            avg_loss = epoch_loss / max(n_images, 1)
            psnr = -10.0 * np.log10(avg_loss + 1e-8)
            if psnr > best_psnr:
                best_psnr = psnr

            with _training_lock:
                training_jobs[training_id].update({
                    'epoch': epoch,
                    'loss': avg_loss,
                    'psnr': round(psnr, 2),
                    'best_psnr': round(best_psnr, 2),
                })

            # Checkpoint every 10 epochs
            if epoch % 10 == 0 or epoch == num_epochs:
                ckpt_path = os.path.join(
                    checkpoint_dir, f"checkpoint_epoch{epoch:04d}.pth",
                )
                torch.save({
                    'epoch': epoch,
                    'nerf_state_dict': model.state_dict(),
                    'optimiser_state_dict': optimiser.state_dict(),
                    'loss': avg_loss,
                    'psnr': psnr,
                }, ckpt_path)
                logger.info(
                    "[%s] Epoch %d/%d  loss=%.6f  PSNR=%.2f  (saved)",
                    training_id, epoch, num_epochs, avg_loss, psnr,
                )

        # Save final checkpoint as "best"
        best_path = os.path.join(checkpoint_dir, 'best_checkpoint.pth')
        torch.save({
            'epoch': num_epochs,
            'nerf_state_dict': model.state_dict(),
            'optimiser_state_dict': optimiser.state_dict(),
            'loss': avg_loss,
            'psnr': best_psnr,
        }, best_path)

        logger.info(
            "Training complete for %s (best PSNR %.2f). Extracting mesh...",
            subject_id, best_psnr,
        )

        # Auto-extract mesh so the viewer has something to show
        with _training_lock:
            training_jobs[training_id].update({
                'status': 'extracting_mesh',
                'message': 'Training done. Extracting 3D mesh...',
            })

        try:
            model.eval()
            vertices, faces, colors = extract_mesh(
                model, resolution=128, density_threshold=50.0,
            )
            meshes_dir = os.path.join(
                app.config['DATA_FOLDER'], 'meshes', subject_id,
            )
            os.makedirs(meshes_dir, exist_ok=True)
            export_obj(vertices, faces, colors, os.path.join(meshes_dir, 'mesh.obj'))
            export_glb(vertices, faces, colors, os.path.join(meshes_dir, 'mesh.glb'))
            logger.info(
                "Mesh extracted for %s: %d verts, %d faces",
                subject_id, len(vertices), len(faces),
            )
        except Exception as mesh_err:
            logger.warning("Auto mesh extraction failed: %s", mesh_err)

        with _training_lock:
            training_jobs[training_id].update({
                'status': 'completed',
                'message': f'Training complete. Best PSNR: {best_psnr:.2f}',
            })

    except Exception as e:
        logger.error("Training failed for %s: %s", subject_id, e)
        logger.error(traceback.format_exc())
        with _training_lock:
            training_jobs[training_id].update({
                'status': 'failed',
                'error': str(e),
            })


# ===================================================================
#  Page routes
# ===================================================================

@app.route('/')
@handle_exceptions
def index():
    return render_template('index.html')


@app.route('/webcam-capture')
@handle_exceptions
def webcam_capture():
    return render_template('webcam_capture.html')


@app.route('/upload-files')
@handle_exceptions
def upload_files():
    return render_template('upload_files.html')


@app.route('/viewer/<subject_id>')
@handle_exceptions
def viewer(subject_id):
    """3D model viewer page with Three.js."""
    meshes_dir = os.path.join(
        app.config['DATA_FOLDER'], 'meshes', subject_id,
    )
    has_glb = os.path.isfile(os.path.join(meshes_dir, 'mesh.glb'))
    has_obj = os.path.isfile(os.path.join(meshes_dir, 'mesh.obj'))
    return render_template(
        'viewer.html',
        subject_id=subject_id,
        has_glb=has_glb,
        has_obj=has_obj,
    )


@app.route('/training/<subject_id>')
@handle_exceptions
def training_page(subject_id):
    """Training progress page."""
    return render_template('training.html', subject_id=subject_id)


@app.route('/results/<subject_id>')
@handle_exceptions
def results(subject_id):
    subject_dir = os.path.join(
        app.config['DATA_FOLDER'], 'preprocessed', subject_id,
    )
    if not os.path.exists(subject_dir):
        return render_template(
            'error.html',
            message=f"No results found for subject {subject_id}",
        )

    poses_dir = os.path.join(subject_dir, 'poses')
    pose_images = {}
    if os.path.exists(poses_dir):
        for pose in ['front', 'left', 'right', 'up', 'down']:
            pose_dir = os.path.join(poses_dir, pose)
            if os.path.exists(pose_dir):
                images = [f for f in os.listdir(pose_dir) if f.endswith('.png')]
                pose_images[pose] = images

    actions_dir = os.path.join(subject_dir, 'actions')
    action_frames = {}
    if os.path.exists(actions_dir):
        for action in os.listdir(actions_dir):
            action_dir = os.path.join(actions_dir, action)
            if os.path.isdir(action_dir):
                frames = [f for f in os.listdir(action_dir) if f.endswith('.png')]
                action_frames[action] = len(frames)

    landmark_dir = os.path.join(
        app.config['DATA_FOLDER'], 'landmarks', subject_id,
    )
    has_landmarks = os.path.exists(landmark_dir)

    embedding_dir = os.path.join(
        app.config['DATA_FOLDER'], 'embeddings', subject_id,
    )
    has_embeddings = os.path.exists(embedding_dir)

    llff_dir = os.path.join(app.config['DATA_FOLDER'], 'llff', subject_id)
    has_llff_data = os.path.isfile(os.path.join(llff_dir, 'poses_bounds.npy'))

    train_count = 0
    val_count = 0
    if has_llff_data:
        train_path = os.path.join(llff_dir, 'train_ids.txt')
        val_path = os.path.join(llff_dir, 'val_ids.txt')
        if os.path.exists(train_path):
            with open(train_path) as f:
                train_count = len(f.readlines())
        if os.path.exists(val_path):
            with open(val_path) as f:
                val_count = len(f.readlines())

    # Compute summary counts for the template
    total_images = sum(len(imgs) for imgs in pose_images.values())
    poses_detected = len(pose_images)
    landmarks_extracted = 0
    if has_landmarks:
        for root, _dirs, files in os.walk(landmark_dir):
            landmarks_extracted += sum(1 for f in files if f.endswith('.json'))
    actions_recorded = sum(action_frames.values())

    # Check for mesh / checkpoint artefacts
    meshes_dir = os.path.join(
        app.config['DATA_FOLDER'], 'meshes', subject_id,
    )
    has_mesh = os.path.isfile(os.path.join(meshes_dir, 'mesh.glb'))

    checkpoint_dir = os.path.join(
        app.config['DATA_FOLDER'], 'checkpoints', subject_id,
    )
    has_checkpoint = os.path.isfile(
        os.path.join(checkpoint_dir, 'best_checkpoint.pth'),
    )

    return render_template(
        'results.html',
        subject_id=subject_id,
        pose_images=pose_images,
        action_frames=action_frames,
        has_landmarks=has_landmarks,
        has_embeddings=has_embeddings,
        has_llff_data=has_llff_data,
        train_count=train_count,
        val_count=val_count,
        has_mesh=has_mesh,
        has_checkpoint=has_checkpoint,
        total_images=total_images,
        poses_detected=poses_detected,
        landmarks_extracted=landmarks_extracted,
        actions_recorded=actions_recorded,
    )


# ===================================================================
#  Upload & preprocessing API
# ===================================================================

@app.route('/api/upload-process', methods=['POST'])
@handle_exceptions
def upload_process():
    """Unified endpoint for uploading and processing files."""
    subject_id = request.form.get('subject_id', f'subject_{int(time.time())}')

    has_files = any(
        request.files.getlist(key) and request.files.getlist(key)[0].filename
        for key in request.files
    )
    if not has_files:
        return jsonify({'status': 'error', 'message': 'No files were uploaded'}), 400

    try:
        processed_poses = save_uploaded_files(subject_id)
        if not processed_poses:
            return jsonify({
                'status': 'error',
                'message': 'Failed to save any files',
            }), 400

        success = process_subject(subject_id)
        if success:
            return jsonify({
                'status': 'success',
                'subject_id': subject_id,
                'redirect': f'/results/{subject_id}',
            })
        return jsonify({
            'status': 'error',
            'message': 'Processing error occurred',
        }), 500

    except Exception as e:
        logger.error("Error in upload_process: %s", e)
        return jsonify({'status': 'error', 'message': str(e)}), 500


@app.route('/api/start-pipeline', methods=['POST'])
@handle_exceptions
def start_pipeline():
    """Legacy endpoint for the webcam capture page."""
    return upload_process()


# ===================================================================
#  Training API
# ===================================================================

@app.route('/api/train', methods=['POST'])
@app.route('/api/start-training', methods=['POST'])
@handle_exceptions
def start_training():
    """Start NeRF training for a subject.

    Body (JSON): ``{subject_id, num_epochs, num_rays, learning_rate}``

    Returns ``{training_id}`` for progress tracking.
    """
    data = request.get_json(force=True)
    subject_id = data.get('subject_id')
    if not subject_id:
        return jsonify({'status': 'error', 'message': 'subject_id required'}), 400

    # Verify preprocessing artefacts exist
    llff_path = os.path.join(
        app.config['DATA_FOLDER'], 'llff', subject_id, 'poses_bounds.npy',
    )
    if not os.path.isfile(llff_path):
        return jsonify({
            'status': 'error',
            'message': f'No pose data for {subject_id}. Run preprocessing first.',
        }), 400

    config = {
        'num_epochs': int(data.get('num_epochs', 50)),
        'num_rays': int(data.get('num_rays', 4096)),
        'learning_rate': float(data.get('learning_rate', 5e-4)),
    }

    training_id = f"train_{subject_id}_{uuid.uuid4().hex[:8]}"
    thread = threading.Thread(
        target=_run_training,
        args=(training_id, subject_id, config),
        daemon=True,
    )
    thread.start()

    return jsonify({
        'status': 'started',
        'training_id': training_id,
        'subject_id': subject_id,
    })


@app.route('/api/training-status/<training_id>')
@handle_exceptions
def training_status(training_id):
    """Get training progress.

    Returns ``{status, epoch, total_epochs, psnr, loss}``.
    """
    with _training_lock:
        job = training_jobs.get(training_id)

    if job is None:
        return jsonify({
            'status': 'not_found',
            'message': f'No training job with id {training_id}',
        }), 404

    return jsonify(job)


# ===================================================================
#  Mesh extraction API
# ===================================================================

@app.route('/api/extract-mesh', methods=['POST'])
@handle_exceptions
def extract_mesh_endpoint():
    """Extract mesh from a trained NeRF.

    Body (JSON): ``{subject_id, resolution, density_threshold}``
    """
    data = request.get_json(force=True)
    subject_id = data.get('subject_id')
    if not subject_id:
        return jsonify({'status': 'error', 'message': 'subject_id required'}), 400

    resolution = int(data.get('resolution', 128))
    density_threshold = float(data.get('density_threshold', 50.0))

    checkpoint_path = os.path.join(
        app.config['DATA_FOLDER'], 'checkpoints', subject_id,
        'best_checkpoint.pth',
    )
    if not os.path.isfile(checkpoint_path):
        return jsonify({
            'status': 'error',
            'message': f'No trained checkpoint for {subject_id}. Train first.',
        }), 400

    try:
        from nerf.tiny_nerf import TinyNeRF

        device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
        ckpt = torch.load(checkpoint_path, map_location=device, weights_only=False)
        model = TinyNeRF().to(device)
        model.load_state_dict(ckpt['nerf_state_dict'])
        model.eval()

        vertices, faces, colors = extract_mesh(
            model,
            resolution=resolution,
            density_threshold=density_threshold,
        )

        meshes_dir = os.path.join(
            app.config['DATA_FOLDER'], 'meshes', subject_id,
        )
        os.makedirs(meshes_dir, exist_ok=True)

        obj_path = os.path.join(meshes_dir, 'mesh.obj')
        glb_path = os.path.join(meshes_dir, 'mesh.glb')
        export_obj(vertices, faces, colors, obj_path)
        export_glb(vertices, faces, colors, glb_path)

        return jsonify({
            'status': 'success',
            'vertices': int(vertices.shape[0]),
            'faces': int(faces.shape[0]),
            'obj_url': f'/outputs/{subject_id}/mesh.obj',
            'glb_url': f'/outputs/{subject_id}/mesh.glb',
            'viewer_url': f'/viewer/{subject_id}',
        })

    except Exception as e:
        logger.error("Mesh extraction failed for %s: %s", subject_id, e)
        return jsonify({'status': 'error', 'message': str(e)}), 500


# ===================================================================
#  Relighting API
# ===================================================================

_VALID_PRESETS = ['studio', 'sunset', 'film_noir', 'sci_fi', 'natural']


@app.route('/api/relight', methods=['POST'])
@handle_exceptions
def relight_endpoint():
    """Apply a relighting preset to an extracted mesh.

    Body (JSON): ``{subject_id, preset}``

    Preset must be one of: studio, sunset, film_noir, sci_fi, natural.
    """
    data = request.get_json(force=True)
    subject_id = data.get('subject_id')
    preset_name = data.get('preset', 'studio')

    if not subject_id:
        return jsonify({'status': 'error', 'message': 'subject_id required'}), 400
    if preset_name not in _VALID_PRESETS:
        return jsonify({
            'status': 'error',
            'message': f'Unknown preset: {preset_name}. '
                       f'Choose from: {_VALID_PRESETS}',
        }), 400

    meshes_dir = os.path.join(
        app.config['DATA_FOLDER'], 'meshes', subject_id,
    )
    obj_path = os.path.join(meshes_dir, 'mesh.obj')
    if not os.path.isfile(obj_path):
        return jsonify({
            'status': 'error',
            'message': 'No mesh found. Extract mesh first.',
        }), 400

    try:
        vertices, faces, colors = _load_mesh_data(obj_path)

        relighter = MeshRelighter()
        relit_colors = relighter.relight_mesh(
            vertices, faces, colors, preset=preset_name,
        )

        output_dir = os.path.join(meshes_dir, f'relit_{preset_name}')
        os.makedirs(output_dir, exist_ok=True)

        # Export OBJ
        relit_obj = os.path.join(output_dir, 'mesh.obj')
        export_obj(vertices, faces, relit_colors, relit_obj)

        # Export GLB for web viewer
        relit_glb = os.path.join(output_dir, 'mesh.glb')
        export_glb(vertices, faces, relit_colors, relit_glb)

        return jsonify({
            'status': 'success',
            'preset': preset_name,
            'output_url': f'/outputs/{subject_id}/relit_{preset_name}/mesh.obj',
            'model_url': f'/outputs/{subject_id}/relit_{preset_name}/mesh.glb',
        })

    except Exception as e:
        logger.error("Relighting failed for %s: %s", subject_id, e)
        return jsonify({'status': 'error', 'message': str(e)}), 500


# ===================================================================
#  Rigging API
# ===================================================================

@app.route('/api/rig', methods=['POST'])
@handle_exceptions
def rig_endpoint():
    """Auto-rig the mesh with skeleton and blend shapes.

    Body (JSON): ``{subject_id}``
    """
    data = request.get_json(force=True)
    subject_id = data.get('subject_id')
    if not subject_id:
        return jsonify({'status': 'error', 'message': 'subject_id required'}), 400

    meshes_dir = os.path.join(
        app.config['DATA_FOLDER'], 'meshes', subject_id,
    )
    obj_path = os.path.join(meshes_dir, 'mesh.obj')
    if not os.path.isfile(obj_path):
        return jsonify({
            'status': 'error',
            'message': 'No mesh found. Extract mesh first.',
        }), 400

    try:
        vertices, faces, colors = _load_mesh_data(obj_path)

        # Auto-rig: skeleton with head, neck, jaw joints
        rigger = SimpleRigger()
        rig_data = rigger.auto_rig(vertices, faces)
        joints = rig_data['joints']
        weights = rig_data['weights']
        parent_map = rig_data['parent_map']

        # Export rigged GLB
        rigged_glb = os.path.join(meshes_dir, 'rigged.glb')
        rigger.export_rigged_glb(
            vertices, faces, colors, joints, weights, rigged_glb,
        )

        # Generate blend shapes
        animator = FaceAnimator()
        blend_shapes = animator.generate_blend_shapes(vertices, faces)

        # Export animated GLB with morph targets
        animated_glb = os.path.join(meshes_dir, 'animated.glb')
        animator.export_animated_glb(
            vertices, faces, colors, blend_shapes, animated_glb,
        )

        # Save skeleton info as JSON for debugging / UI
        skeleton_json_path = os.path.join(meshes_dir, 'skeleton.json')
        skeleton_info = {
            'joints': {
                name: pos.tolist() for name, pos in joints.items()
            },
            'parent_map': parent_map,
            'blend_shapes': list(blend_shapes.keys()),
        }
        with open(skeleton_json_path, 'w') as f:
            json.dump(skeleton_info, f, indent=2)

        return jsonify({
            'status': 'success',
            'glb_url': f'/outputs/{subject_id}/rigged.glb',
            'model_url': f'/outputs/{subject_id}/animated.glb',
            'skeleton_url': f'/outputs/{subject_id}/skeleton.json',
            'num_bones': len(joints),
            'num_blend_shapes': len(blend_shapes),
        })

    except Exception as e:
        logger.error("Rigging failed for %s: %s", subject_id, e)
        return jsonify({'status': 'error', 'message': str(e)}), 500


# ===================================================================
#  Export API
# ===================================================================

@app.route('/api/export', methods=['POST'])
@handle_exceptions
def export_endpoint():
    """Export the final model in the requested format.

    Body (JSON): ``{subject_id, format}``  (format: ``glb`` or ``obj``)
    """
    data = request.get_json(force=True)
    subject_id = data.get('subject_id')
    fmt = data.get('format', 'glb').lower()

    if not subject_id:
        return jsonify({'status': 'error', 'message': 'subject_id required'}), 400
    if fmt not in ('glb', 'obj'):
        return jsonify({
            'status': 'error',
            'message': f'Unsupported format: {fmt}. Use glb or obj.',
        }), 400

    meshes_dir = os.path.join(
        app.config['DATA_FOLDER'], 'meshes', subject_id,
    )

    # Prefer rigged mesh if available, otherwise raw mesh
    if fmt == 'glb':
        rigged = os.path.join(meshes_dir, 'rigged.glb')
        raw = os.path.join(meshes_dir, 'mesh.glb')
        export_file = rigged if os.path.isfile(rigged) else raw
    else:
        export_file = os.path.join(meshes_dir, 'mesh.obj')

    if not os.path.isfile(export_file):
        return jsonify({
            'status': 'error',
            'message': f'No {fmt.upper()} file available for {subject_id}.',
        }), 404

    filename = os.path.basename(export_file)
    return jsonify({
        'status': 'success',
        'download_url': f'/outputs/{subject_id}/{filename}',
        'format': fmt,
        'size_bytes': os.path.getsize(export_file),
    })


# ===================================================================
#  File serving
# ===================================================================

@app.route('/outputs/<subject_id>/<path:filename>')
@handle_exceptions
def serve_output(subject_id, filename):
    """Serve generated meshes, textures, previews, etc."""
    meshes_dir = os.path.join(
        app.config['DATA_FOLDER'], 'meshes', subject_id,
    )
    return send_from_directory(meshes_dir, filename)


@app.route('/data/<path:filename>')
@handle_exceptions
def serve_data(filename):
    """Serve files from the data directory."""
    return send_from_directory(app.config['DATA_FOLDER'], filename)


@app.route('/static/<path:filename>')
@handle_exceptions
def serve_static(filename):
    """Serve static files."""
    return send_from_directory('static', filename)


# ===================================================================
#  Main
# ===================================================================

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=5000)
