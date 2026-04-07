import os
import cv2
import numpy as np
import mediapipe as mp
from tqdm import tqdm
from scipy.spatial.transform import Rotation
from pathlib import Path
import argparse

# Facial landmarks and 3D model points
FACE_3D_MODEL = {
    "nose_tip": [0.0, 0.0, 0.0],
    "chin": [0.0, -63.6, -12.5],
    "left_eye_left_corner": [-42.0, 32.0, -26.0],
    "right_eye_right_corner": [42.0, 32.0, -26.0],
    "left_mouth_corner": [-28.0, -28.9, -20.0],
    "right_mouth_corner": [28.0, -28.9, -20.0]
}

MP_INDEXES = {
    "nose_tip": 1,
    "chin": 152,
    "left_eye_left_corner": 263,
    "right_eye_right_corner": 33,
    "left_mouth_corner": 61,
    "right_mouth_corner": 291
}


def estimate_pose(img, w, h, face_mesh):
    """Estimate pose using Mediapipe and SolvePnP."""
    rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    result = face_mesh.process(rgb)

    if not result.multi_face_landmarks:
        return None, None

    landmarks = result.multi_face_landmarks[0]
    image_points = np.array([
        (landmarks.landmark[MP_INDEXES[k]].x * w, landmarks.landmark[MP_INDEXES[k]].y * h)
        for k in MP_INDEXES
    ], dtype=np.float32)
    model_points = np.array([FACE_3D_MODEL[k] for k in MP_INDEXES], dtype=np.float32)

    focal_length = w
    center = (w / 2, h / 2)
    camera_matrix = np.array([
        [focal_length, 0, center[0]],
        [0, focal_length, center[1]],
        [0, 0, 1]
    ], dtype=np.float32)
    dist_coeffs = np.zeros((4, 1))

    success, rvec, tvec = cv2.solvePnP(model_points, image_points, camera_matrix, dist_coeffs)
    if not success:
        return None, None

    rot_matrix, _ = cv2.Rodrigues(rvec)
    quat = Rotation.from_matrix(rot_matrix).as_quat()  # [x, y, z, w]
    return rot_matrix, tvec.reshape(-1), quat


def build_poses_bounds(images_dir, output_dir, near=0.1, far=1.0):
    mp_face_mesh = mp.solutions.face_mesh
    all_entries = []

    img_fnames = sorted([f for f in os.listdir(images_dir) if f.lower().endswith(('.jpg', '.jpeg', '.png'))])

    if len(img_fnames) == 0:
        raise RuntimeError(f"No images found in {images_dir}")

    with mp_face_mesh.FaceMesh(static_image_mode=True, max_num_faces=1, refine_landmarks=True) as face_mesh:
        for fname in tqdm(img_fnames, desc="Processing poses"):
            img_path = os.path.join(images_dir, fname)
            img = cv2.imread(img_path)
            if img is None:
                continue
            h, w, _ = img.shape

            # rot_matrix, tvec, quat = estimate_pose(img, w, h, face_mesh)
            # if rot_matrix is None:
            #     continue
            result = estimate_pose(img, w, h, face_mesh)
            if result is None or len(result) != 3:
                print(f"⚠️ Failed to estimate pose for {fname}")
                continue
            rot_matrix, tvec, quat = result


            pose = np.eye(4)
            pose[:3, :3] = rot_matrix
            pose[:3, 3] = tvec

            # LLFF format expects: [pose(3x5) | near | far]
            pose_3x5 = np.concatenate([pose[:3, :4], np.array([[h], [w], [w]])], axis=1)  # h, w, focal
            pose_bound = np.concatenate([pose_3x5.reshape(15), [near, far]])
            all_entries.append(pose_bound)

    all_entries = np.stack(all_entries, axis=0)
    np.save(os.path.join(output_dir, "poses_bounds.npy"), all_entries)
    print(f"✅ Saved poses_bounds.npy to {output_dir}")


def split_train_val(images_dir, output_dir, train_ratio=0.875):
    img_fnames = sorted([f for f in os.listdir(images_dir) if f.lower().endswith(('.jpg', '.jpeg', '.png'))])
    N = len(img_fnames)
    indices = np.arange(N)

    np.random.seed(42)
    np.random.shuffle(indices)
    split = int(N * train_ratio)
    train_ids = indices[:split]
    val_ids = indices[split:]

    np.savetxt(os.path.join(output_dir, "train_ids.txt"), train_ids, fmt='%d')
    np.savetxt(os.path.join(output_dir, "val_ids.txt"), val_ids, fmt='%d')

    print(f"✅ Saved train_ids.txt ({len(train_ids)}) and val_ids.txt ({len(val_ids)}) to {output_dir}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--images_dir', required=True, help="Path to LLFF-style images folder")
    parser.add_argument('--output_dir', required=False, default=None, help="Path to write outputs (defaults to parent)")
    args = parser.parse_args()

    images_dir = args.images_dir
    output_dir = args.output_dir if args.output_dir else str(Path(images_dir).parent)

    assert os.path.exists(images_dir), f"Image folder doesn't exist: {images_dir}"
    os.makedirs(output_dir, exist_ok=True)

    build_poses_bounds(images_dir, output_dir)
    split_train_val(images_dir, output_dir)


if __name__ == "__main__":
    main()
