"""
postprocessing.py -- Post-processing for meshes extracted from NeRF.

Provides:
  - MeshRelighter   : Phong-shaded relighting with named presets.
  - SimpleRigger    : Basic skeletal rig generation for head/bust meshes.
  - FaceAnimator    : Blend shape generation and facial animation.
  - compute_vertex_normals : Per-vertex normal computation helper.

Dependencies: numpy (required), trimesh (optional, for GLB export).
"""

from __future__ import annotations

import json
import logging
import struct
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple, Union

import numpy as np

logger = logging.getLogger(__name__)

# ---------------------------------------------------------------------------
# Optional dependency imports
# ---------------------------------------------------------------------------

try:
    import trimesh as _trimesh
    HAS_TRIMESH = True
except ImportError:
    _trimesh = None
    HAS_TRIMESH = False


# ===================================================================
# Helper
# ===================================================================

def compute_vertex_normals(vertices: np.ndarray, faces: np.ndarray) -> np.ndarray:
    """Compute per-vertex normals by averaging adjacent face normals.

    Parameters
    ----------
    vertices : (V, 3) float array
    faces : (F, 3) int array

    Returns
    -------
    normals : (V, 3) normalized normal vectors.
    """
    vertices = np.asarray(vertices, dtype=np.float64)
    faces = np.asarray(faces, dtype=np.int64)

    v0 = vertices[faces[:, 0]]
    v1 = vertices[faces[:, 1]]
    v2 = vertices[faces[:, 2]]

    # Face normals (area-weighted by the cross product magnitude)
    face_normals = np.cross(v1 - v0, v2 - v0)  # (F, 3)

    # Accumulate face normals onto each vertex
    vertex_normals = np.zeros_like(vertices)
    for i in range(3):
        np.add.at(vertex_normals, faces[:, i], face_normals)

    # Normalize
    norms = np.linalg.norm(vertex_normals, axis=1, keepdims=True)
    norms = np.where(norms < 1e-12, 1.0, norms)
    vertex_normals /= norms

    return vertex_normals


# ===================================================================
# 1. MeshRelighter
# ===================================================================

class MeshRelighter:
    """Applies Phong shading to a mesh with configurable light positions and colors."""

    def __init__(self):
        self.presets: Dict[str, List[Dict[str, Any]]] = {
            "studio": [
                # Key light: warm white, upper right
                {"position": np.array([2.0, 3.0, 2.0]), "color": np.array([1.0, 0.95, 0.9]), "intensity": 0.8},
                # Fill light: cool white, left
                {"position": np.array([-3.0, 1.0, 2.0]), "color": np.array([0.85, 0.9, 1.0]), "intensity": 0.4},
                # Rim light: blue, behind
                {"position": np.array([0.0, 2.0, -3.0]), "color": np.array([0.4, 0.5, 0.9]), "intensity": 0.3},
            ],
            "sunset": [
                # Warm orange directional light from low angle
                {"position": np.array([5.0, 1.0, 2.0]), "color": np.array([1.0, 0.6, 0.2]), "intensity": 1.0},
                # Subtle warm fill
                {"position": np.array([-2.0, 0.5, 1.0]), "color": np.array([0.8, 0.4, 0.2]), "intensity": 0.15},
            ],
            "film_noir": [
                # High-contrast single spotlight from above
                {"position": np.array([0.5, 5.0, 1.0]), "color": np.array([1.0, 1.0, 1.0]), "intensity": 1.2},
            ],
            "sci_fi": [
                # Cool blue/cyan ambient base
                {"position": np.array([0.0, 3.0, 3.0]), "color": np.array([0.3, 0.6, 1.0]), "intensity": 0.6},
                # Neon magenta accent from below-left
                {"position": np.array([-3.0, -1.0, 1.0]), "color": np.array([1.0, 0.2, 0.8]), "intensity": 0.5},
                # Neon cyan accent from right
                {"position": np.array([3.0, 0.0, 1.0]), "color": np.array([0.0, 1.0, 0.9]), "intensity": 0.4},
            ],
            "natural": [
                # Soft daylight from above
                {"position": np.array([0.0, 5.0, 3.0]), "color": np.array([1.0, 0.98, 0.95]), "intensity": 0.7},
                # Gentle fill from below-front
                {"position": np.array([0.0, -1.0, 4.0]), "color": np.array([0.9, 0.92, 1.0]), "intensity": 0.25},
            ],
        }

    def apply_phong_shading(
        self,
        vertices: np.ndarray,
        normals: np.ndarray,
        vertex_colors: np.ndarray,
        lights: List[Dict[str, Any]],
        ambient: float = 0.2,
        diffuse: float = 0.6,
        specular: float = 0.3,
        shininess: float = 32.0,
    ) -> np.ndarray:
        """Apply Phong illumination model to vertex colors.

        Parameters
        ----------
        vertices : (V, 3) vertex positions
        normals : (V, 3) per-vertex normals (should be normalized)
        vertex_colors : (V, 3) base albedo colors in [0, 1]
        lights : list of dicts with 'position', 'color', 'intensity'
        ambient : ambient lighting coefficient
        diffuse : diffuse reflection coefficient
        specular : specular reflection coefficient
        shininess : Phong shininess exponent

        Returns
        -------
        relit_colors : (V, 3) relit vertex colors clipped to [0, 1]
        """
        vertices = np.asarray(vertices, dtype=np.float64)
        normals = np.asarray(normals, dtype=np.float64)
        vertex_colors = np.asarray(vertex_colors, dtype=np.float64)

        V = len(vertices)
        if vertex_colors.shape[0] != V:
            raise ValueError("vertex_colors must have same number of rows as vertices")

        # Ensure normals are unit-length
        n_norms = np.linalg.norm(normals, axis=1, keepdims=True)
        n_norms = np.where(n_norms < 1e-12, 1.0, n_norms)
        N = normals / n_norms  # (V, 3)

        # Compute mesh centroid for view direction approximation
        centroid = vertices.mean(axis=0)
        # View direction: from each vertex toward a camera placed along +Z
        view_pos = centroid + np.array([0.0, 0.0, 5.0 * np.ptp(vertices[:, 2])])
        view_dirs = view_pos[None, :] - vertices  # (V, 3)
        view_norms = np.linalg.norm(view_dirs, axis=1, keepdims=True)
        view_norms = np.where(view_norms < 1e-12, 1.0, view_norms)
        view_dirs /= view_norms  # (V, 3)

        # Start with ambient term
        result = vertex_colors * ambient  # (V, 3)

        for light in lights:
            light_pos = np.asarray(light["position"], dtype=np.float64)
            light_color = np.asarray(light["color"], dtype=np.float64)
            intensity = float(light.get("intensity", 1.0))

            # Light direction from each vertex to light
            L = light_pos[None, :] - vertices  # (V, 3)
            L_norms = np.linalg.norm(L, axis=1, keepdims=True)
            L_norms = np.where(L_norms < 1e-12, 1.0, L_norms)
            L /= L_norms  # (V, 3)

            # Diffuse: Lambert
            NdotL = np.sum(N * L, axis=1, keepdims=True)  # (V, 1)
            NdotL = np.clip(NdotL, 0.0, 1.0)
            diffuse_contrib = diffuse * vertex_colors * NdotL * light_color[None, :] * intensity

            # Specular: Blinn-Phong (half-vector)
            H = L + view_dirs  # (V, 3)
            H_norms = np.linalg.norm(H, axis=1, keepdims=True)
            H_norms = np.where(H_norms < 1e-12, 1.0, H_norms)
            H /= H_norms
            NdotH = np.sum(N * H, axis=1, keepdims=True)  # (V, 1)
            NdotH = np.clip(NdotH, 0.0, 1.0)
            specular_contrib = specular * (NdotH ** shininess) * light_color[None, :] * intensity

            result += diffuse_contrib + specular_contrib

        return np.clip(result, 0.0, 1.0)

    def relight_mesh(
        self,
        vertices: np.ndarray,
        faces: np.ndarray,
        vertex_colors: np.ndarray,
        preset: str = "studio",
        custom_lights: Optional[List[Dict[str, Any]]] = None,
    ) -> np.ndarray:
        """Relight a mesh using a preset or custom lights.

        Computes face normals, averages to vertex normals, and applies Phong shading.

        Parameters
        ----------
        vertices : (V, 3) vertex positions
        faces : (F, 3) face indices
        vertex_colors : (V, 3) base colors in [0, 1]
        preset : name of lighting preset ('studio', 'sunset', 'film_noir', 'sci_fi', 'natural')
        custom_lights : if provided, overrides the preset. List of dicts with
                        'position', 'color', 'intensity' keys.

        Returns
        -------
        relit_colors : (V, 3) relit vertex colors in [0, 1]
        """
        vertices = np.asarray(vertices, dtype=np.float64)
        faces = np.asarray(faces, dtype=np.int64)
        vertex_colors = np.asarray(vertex_colors, dtype=np.float64)

        if vertex_colors.ndim == 1:
            vertex_colors = vertex_colors.reshape(-1, 3)

        # Compute vertex normals
        normals = compute_vertex_normals(vertices, faces)

        # Select lights
        if custom_lights is not None:
            lights = custom_lights
        elif preset in self.presets:
            lights = self.presets[preset]
        else:
            available = list(self.presets.keys())
            raise ValueError(f"Unknown preset '{preset}'. Available presets: {available}")

        return self.apply_phong_shading(vertices, normals, vertex_colors, lights)

    def export_relit_mesh(
        self,
        vertices: np.ndarray,
        faces: np.ndarray,
        colors: np.ndarray,
        output_path: Union[str, Path],
        format: str = "glb",
    ) -> str:
        """Export relit mesh to OBJ or GLB.

        Parameters
        ----------
        vertices : (V, 3) vertex positions
        faces : (F, 3) face indices
        colors : (V, 3) vertex colors in [0, 1]
        output_path : file path for the output
        format : 'glb' or 'obj'

        Returns
        -------
        output_path : str, path of written file
        """
        if not HAS_TRIMESH:
            raise ImportError("trimesh is required for mesh export. Install with: pip install trimesh")

        vertices = np.asarray(vertices, dtype=np.float64)
        faces = np.asarray(faces, dtype=np.int64)
        colors = np.asarray(colors, dtype=np.float64)

        output_path = Path(output_path)
        output_path.parent.mkdir(parents=True, exist_ok=True)

        # Convert colors to uint8 RGBA for trimesh
        if colors.shape[1] == 3:
            alpha = np.ones((len(colors), 1), dtype=np.float64)
            colors = np.concatenate([colors, alpha], axis=1)
        vertex_colors_uint8 = np.clip(colors * 255, 0, 255).astype(np.uint8)

        mesh = _trimesh.Trimesh(
            vertices=vertices,
            faces=faces,
            vertex_colors=vertex_colors_uint8,
            process=False,
        )

        fmt = format.lower().strip(".")
        if fmt == "obj":
            mesh.export(str(output_path), file_type="obj")
        elif fmt == "glb":
            mesh.export(str(output_path), file_type="glb")
        else:
            raise ValueError(f"Unsupported format '{format}'. Use 'obj' or 'glb'.")

        logger.info("Exported relit mesh: %s", output_path)
        return str(output_path)


# ===================================================================
# 2. SimpleRigger
# ===================================================================

class SimpleRigger:
    """Generates a basic skeletal rig for a head/bust mesh."""

    def __init__(self):
        self.skeleton: Optional[Dict[str, Any]] = None

    def auto_rig(
        self,
        vertices: np.ndarray,
        faces: np.ndarray,
    ) -> Dict[str, Any]:
        """Create a basic skeleton with head, neck, and jaw joints.

        Joints are placed heuristically based on vertex distribution:
        - Head joint: center of mass of top 30% vertices by Y coordinate
        - Neck joint: center of mass of middle band (30%-50% by Y)
        - Jaw joint: center of mass of bottom 25% vertices, offset forward

        Parameters
        ----------
        vertices : (V, 3) vertex positions
        faces : (F, 3) face indices

        Returns
        -------
        dict with:
          - 'joints' : dict mapping joint name to (3,) position array
          - 'weights' : (V, num_joints) skinning weight matrix
          - 'parent_map' : dict mapping joint name to parent joint name (or None)
        """
        vertices = np.asarray(vertices, dtype=np.float64)
        faces = np.asarray(faces, dtype=np.int64)

        y_coords = vertices[:, 1]
        y_min, y_max = y_coords.min(), y_coords.max()
        y_range = y_max - y_min
        if y_range < 1e-12:
            y_range = 1.0

        # Normalize Y to [0, 1]
        y_normalized = (y_coords - y_min) / y_range

        # Head joint: center of mass of top 30% vertices
        head_mask = y_normalized >= 0.70
        if head_mask.sum() == 0:
            head_mask = y_normalized >= y_normalized.max() - 0.01
        head_pos = vertices[head_mask].mean(axis=0)

        # Neck joint: center of mass of middle band (30% - 50%)
        neck_mask = (y_normalized >= 0.30) & (y_normalized < 0.50)
        if neck_mask.sum() == 0:
            neck_mask = (y_normalized >= 0.25) & (y_normalized < 0.55)
        if neck_mask.sum() == 0:
            # Fallback: just use the overall centroid lowered
            neck_pos = vertices.mean(axis=0).copy()
            neck_pos[1] = y_min + y_range * 0.4
        else:
            neck_pos = vertices[neck_mask].mean(axis=0)

        # Jaw joint: lower front region
        jaw_mask = y_normalized < 0.25
        if jaw_mask.sum() == 0:
            jaw_mask = y_normalized < 0.35
        if jaw_mask.sum() == 0:
            jaw_pos = vertices.mean(axis=0).copy()
            jaw_pos[1] = y_min + y_range * 0.15
        else:
            jaw_pos = vertices[jaw_mask].mean(axis=0).copy()
            # Offset forward (along Z) for mouth opening pivot
            z_range = vertices[:, 2].max() - vertices[:, 2].min()
            jaw_pos[2] += z_range * 0.1

        joints = {
            "head": head_pos,
            "neck": neck_pos,
            "jaw": jaw_pos,
        }

        parent_map = {
            "neck": None,
            "head": "neck",
            "jaw": "head",
        }

        # Compute skinning weights
        weights = self.compute_skinning_weights(vertices, joints)

        self.skeleton = {
            "joints": joints,
            "weights": weights,
            "parent_map": parent_map,
        }

        return self.skeleton

    def compute_skinning_weights(
        self,
        vertices: np.ndarray,
        joints: Dict[str, np.ndarray],
    ) -> np.ndarray:
        """Compute linear blend skinning weights based on distance to joints.

        Uses heat diffusion approximation (inverse distance weighting).
        Each vertex is assigned weights proportional to 1/distance^2 from
        each joint, normalized so weights sum to 1 per vertex.

        Parameters
        ----------
        vertices : (V, 3) vertex positions
        joints : dict mapping joint name to (3,) position array

        Returns
        -------
        weights : (V, J) weight matrix, normalized per vertex.
                  Columns correspond to joints in sorted key order.
        """
        vertices = np.asarray(vertices, dtype=np.float64)

        joint_names = sorted(joints.keys())
        joint_positions = np.array([joints[name] for name in joint_names], dtype=np.float64)  # (J, 3)
        J = len(joint_names)
        V = len(vertices)

        # Compute distances: (V, J)
        # vertices[:, None, :] is (V, 1, 3), joint_positions[None, :, :] is (1, J, 3)
        diff = vertices[:, None, :] - joint_positions[None, :, :]  # (V, J, 3)
        dists = np.linalg.norm(diff, axis=2)  # (V, J)

        # Inverse distance weighting with epsilon to avoid division by zero
        epsilon = 1e-8
        raw_weights = 1.0 / (dists ** 2 + epsilon)  # (V, J)

        # Normalize per vertex
        row_sums = raw_weights.sum(axis=1, keepdims=True)
        row_sums = np.where(row_sums < 1e-12, 1.0, row_sums)
        weights = raw_weights / row_sums

        return weights

    def export_rigged_glb(
        self,
        vertices: np.ndarray,
        faces: np.ndarray,
        colors: np.ndarray,
        joints: Dict[str, np.ndarray],
        weights: np.ndarray,
        output_path: Union[str, Path],
    ) -> str:
        """Export rigged mesh as GLB with skeleton and skin weights.

        The skeleton and skin weight data are stored as mesh metadata/extras
        within the GLB file for downstream consumption.

        Parameters
        ----------
        vertices : (V, 3) vertex positions
        faces : (F, 3) face indices
        colors : (V, 3) vertex colors in [0, 1]
        joints : dict mapping joint name to (3,) position
        weights : (V, J) skinning weights
        output_path : output file path

        Returns
        -------
        output_path : str, the path of the written file
        """
        if not HAS_TRIMESH:
            raise ImportError("trimesh is required for GLB export. Install with: pip install trimesh")

        vertices = np.asarray(vertices, dtype=np.float64)
        faces = np.asarray(faces, dtype=np.int64)
        colors = np.asarray(colors, dtype=np.float64)

        output_path = Path(output_path)
        output_path.parent.mkdir(parents=True, exist_ok=True)

        # Prepare RGBA vertex colors
        if colors.shape[1] == 3:
            alpha = np.ones((len(colors), 1), dtype=np.float64)
            colors = np.concatenate([colors, alpha], axis=1)
        vertex_colors_uint8 = np.clip(colors * 255, 0, 255).astype(np.uint8)

        mesh = _trimesh.Trimesh(
            vertices=vertices,
            faces=faces,
            vertex_colors=vertex_colors_uint8,
            process=False,
        )

        # Store rig data in mesh metadata
        joint_names = sorted(joints.keys())
        joint_positions = {name: joints[name].tolist() for name in joint_names}

        mesh.metadata["skeleton_joint_names"] = joint_names
        mesh.metadata["skeleton_joint_positions"] = joint_positions

        # Store top-4 joint influences per vertex (compact representation)
        num_joints = weights.shape[1]
        top_k = min(4, num_joints)
        top_indices = np.argsort(-weights, axis=1)[:, :top_k]
        top_weights = np.take_along_axis(weights, top_indices, axis=1)
        # Renormalize after truncation
        tw_sum = top_weights.sum(axis=1, keepdims=True)
        tw_sum = np.where(tw_sum < 1e-12, 1.0, tw_sum)
        top_weights /= tw_sum

        mesh.metadata["skin_joint_indices"] = top_indices.tolist()
        mesh.metadata["skin_weights"] = top_weights.tolist()

        mesh.export(str(output_path), file_type="glb")
        logger.info("Exported rigged GLB: %s", output_path)
        return str(output_path)


# ===================================================================
# 3. FaceAnimator
# ===================================================================

class FaceAnimator:
    """Generates blend shape targets for facial animation."""

    def __init__(self):
        self.blend_shapes: Dict[str, np.ndarray] = {}

    def generate_blend_shapes(
        self,
        vertices: np.ndarray,
        faces: np.ndarray,
        jaw_joint_idx: Optional[int] = None,
    ) -> Dict[str, np.ndarray]:
        """Generate basic facial blend shapes.

        Each blend shape is a displacement array of shape (V, 3) relative to
        the base vertex positions. Vertex regions are identified heuristically
        based on normalized bounding-box coordinates.

        Blend shapes generated:
        - jaw_open: rotate jaw vertices downward
        - smile: move mouth corners up and out
        - blink_left / blink_right: close eyelids
        - brow_raise: move eyebrow vertices up

        Parameters
        ----------
        vertices : (V, 3) base vertex positions
        faces : (F, 3) face indices
        jaw_joint_idx : optional index for jaw joint (unused in heuristic mode,
                        reserved for skeleton-aware generation)

        Returns
        -------
        blend_shapes : dict mapping blend shape name to (V, 3) displacement arrays
        """
        vertices = np.asarray(vertices, dtype=np.float64)
        faces = np.asarray(faces, dtype=np.int64)

        V = len(vertices)
        mins = vertices.min(axis=0)
        maxs = vertices.max(axis=0)
        span = maxs - mins
        span = np.where(span < 1e-12, 1.0, span)
        centre = (mins + maxs) / 2.0

        # Normalize vertex positions to [0, 1] within bounding box
        normalized = (vertices - mins) / span

        scale = span.max()
        shapes: Dict[str, np.ndarray] = {}

        # ---- jaw_open: lower vertices drop down and slightly forward ----
        jaw_disp = np.zeros((V, 3), dtype=np.float64)
        jaw_mask = normalized[:, 1] < 0.30  # bottom 30%
        for i in np.where(jaw_mask)[0]:
            # Stronger displacement for lower vertices
            t = 1.0 - normalized[i, 1] / 0.30
            t = max(0.0, min(1.0, t))
            jaw_disp[i, 1] = -t * 0.04 * scale  # drop down
            jaw_disp[i, 2] = t * 0.008 * scale   # slight forward
        shapes["jaw_open"] = jaw_disp

        # ---- smile: move mouth corners up and outward ----
        smile_disp = np.zeros((V, 3), dtype=np.float64)
        mouth_mask = (normalized[:, 1] > 0.15) & (normalized[:, 1] < 0.40) & \
                     (normalized[:, 2] > 0.35)
        for i in np.where(mouth_mask)[0]:
            side = np.sign(vertices[i, 0] - centre[0])
            dist_from_center = abs(vertices[i, 0] - centre[0]) / (span[0] * 0.5 + 1e-9)
            amount = dist_from_center * 0.025 * scale
            smile_disp[i, 0] = side * amount * 0.4   # outward
            smile_disp[i, 1] = amount                  # upward
        shapes["smile"] = smile_disp

        # ---- blink_left: close left eyelid ----
        blink_left_disp = np.zeros((V, 3), dtype=np.float64)
        # Left eye region (assuming X < center is left in mesh space)
        left_eye_mask = (normalized[:, 1] > 0.55) & (normalized[:, 1] < 0.75) & \
                        (normalized[:, 2] > 0.30) & (vertices[:, 0] < centre[0])
        eye_center_y_left = centre[1] + span[1] * 0.15
        for i in np.where(left_eye_mask)[0]:
            if vertices[i, 1] > eye_center_y_left:
                delta = (vertices[i, 1] - eye_center_y_left) * 0.65
                blink_left_disp[i, 1] = -delta
        shapes["blink_left"] = blink_left_disp

        # ---- blink_right: close right eyelid ----
        blink_right_disp = np.zeros((V, 3), dtype=np.float64)
        right_eye_mask = (normalized[:, 1] > 0.55) & (normalized[:, 1] < 0.75) & \
                         (normalized[:, 2] > 0.30) & (vertices[:, 0] >= centre[0])
        eye_center_y_right = centre[1] + span[1] * 0.15
        for i in np.where(right_eye_mask)[0]:
            if vertices[i, 1] > eye_center_y_right:
                delta = (vertices[i, 1] - eye_center_y_right) * 0.65
                blink_right_disp[i, 1] = -delta
        shapes["blink_right"] = blink_right_disp

        # ---- brow_raise: move eyebrow vertices up ----
        brow_disp = np.zeros((V, 3), dtype=np.float64)
        brow_mask = (normalized[:, 1] > 0.72) & (normalized[:, 1] < 0.88) & \
                    (normalized[:, 2] > 0.30)
        for i in np.where(brow_mask)[0]:
            # Raise amount peaks at center of brow band
            brow_t = 1.0 - abs(normalized[i, 1] - 0.80) / 0.08
            brow_t = max(0.0, min(1.0, brow_t))
            brow_disp[i, 1] = brow_t * 0.02 * scale
        shapes["brow_raise"] = brow_disp

        self.blend_shapes = shapes
        return shapes

    def apply_blend_shape(
        self,
        vertices: np.ndarray,
        blend_shape_name: str,
        weight: float = 1.0,
    ) -> np.ndarray:
        """Apply a single blend shape with given weight.

        Parameters
        ----------
        vertices : (V, 3) base vertex positions
        blend_shape_name : name of the blend shape to apply
        weight : blend weight in [0, 1] (can exceed 1 for exaggeration)

        Returns
        -------
        modified_vertices : (V, 3) deformed vertex positions
        """
        vertices = np.asarray(vertices, dtype=np.float64)

        if blend_shape_name not in self.blend_shapes:
            available = list(self.blend_shapes.keys())
            raise ValueError(
                f"Unknown blend shape '{blend_shape_name}'. Available: {available}"
            )

        displacement = self.blend_shapes[blend_shape_name]
        return vertices + weight * displacement

    def apply_blend_shapes(
        self,
        vertices: np.ndarray,
        weights_dict: Dict[str, float],
    ) -> np.ndarray:
        """Apply multiple blend shapes additively.

        Parameters
        ----------
        vertices : (V, 3) base vertex positions
        weights_dict : dict mapping blend shape names to weights,
                       e.g. {'jaw_open': 0.5, 'smile': 0.3}

        Returns
        -------
        modified_vertices : (V, 3) deformed vertex positions
        """
        vertices = np.asarray(vertices, dtype=np.float64)
        result = vertices.copy()

        for name, weight in weights_dict.items():
            if abs(weight) < 1e-9:
                continue
            if name not in self.blend_shapes:
                logger.warning("Blend shape '%s' not found, skipping.", name)
                continue
            result += weight * self.blend_shapes[name]

        return result

    def export_animated_glb(
        self,
        vertices: np.ndarray,
        faces: np.ndarray,
        colors: np.ndarray,
        blend_shapes: Dict[str, np.ndarray],
        output_path: Union[str, Path],
    ) -> str:
        """Export mesh with morph targets in GLB format for three.js.

        Each blend shape becomes a morph target in the GLB. Target names are
        stored in the mesh extras for three.js compatibility.

        Parameters
        ----------
        vertices : (V, 3) base vertex positions
        faces : (F, 3) face indices
        colors : (V, 3) vertex colors in [0, 1]
        blend_shapes : dict mapping name to (V, 3) displacement arrays
        output_path : output .glb file path

        Returns
        -------
        output_path : str, path of the written file
        """
        if not HAS_TRIMESH:
            raise ImportError("trimesh is required for GLB export. Install with: pip install trimesh")

        vertices = np.asarray(vertices, dtype=np.float64)
        faces = np.asarray(faces, dtype=np.int64)
        colors = np.asarray(colors, dtype=np.float64)

        output_path = Path(output_path)
        output_path.parent.mkdir(parents=True, exist_ok=True)

        # Prepare RGBA vertex colors
        if colors.shape[1] == 3:
            alpha = np.ones((len(colors), 1), dtype=np.float64)
            colors = np.concatenate([colors, alpha], axis=1)
        vertex_colors_uint8 = np.clip(colors * 255, 0, 255).astype(np.uint8)

        # Try pygltflib for proper morph target support
        try:
            import pygltflib
            self._export_morph_targets_pygltflib(
                vertices, faces, vertex_colors_uint8,
                blend_shapes, output_path, pygltflib,
            )
            logger.info("Exported animated GLB with morph targets (pygltflib): %s", output_path)
            return str(output_path)
        except ImportError:
            pass
        except Exception as exc:
            logger.warning("pygltflib export failed (%s); falling back to trimesh.", exc)

        # Fallback: static mesh via trimesh with blend shape data as sidecar files
        mesh = _trimesh.Trimesh(
            vertices=vertices,
            faces=faces,
            vertex_colors=vertex_colors_uint8,
            process=False,
        )
        mesh.metadata["morph_target_names"] = list(blend_shapes.keys())
        mesh.export(str(output_path), file_type="glb")

        # Write blend shape displacements as .npy files alongside the GLB
        bs_dir = output_path.parent / (output_path.stem + "_morph_targets")
        bs_dir.mkdir(parents=True, exist_ok=True)
        for name, disp in blend_shapes.items():
            np.save(str(bs_dir / f"{name}.npy"), np.asarray(disp, dtype=np.float32))

        logger.info("Exported animated GLB (trimesh fallback) + morph targets: %s", output_path)
        return str(output_path)

    @staticmethod
    def _export_morph_targets_pygltflib(
        vertices: np.ndarray,
        faces: np.ndarray,
        vertex_colors_uint8: np.ndarray,
        blend_shapes: Dict[str, np.ndarray],
        output_path: Path,
        pygltflib,
    ) -> None:
        """Build a GLB with morph targets using pygltflib."""

        verts_f32 = np.asarray(vertices, dtype=np.float32)
        faces_u32 = np.asarray(faces, dtype=np.uint32)
        colors_f32 = vertex_colors_uint8.astype(np.float32) / 255.0

        if colors_f32.shape[1] == 3:
            alpha = np.ones((len(colors_f32), 1), dtype=np.float32)
            colors_f32 = np.concatenate([colors_f32, alpha], axis=1)

        num_verts = len(verts_f32)
        num_indices = faces_u32.size

        morph_names = sorted(blend_shapes.keys())
        morph_deltas: List[np.ndarray] = []
        for name in morph_names:
            delta = np.asarray(blend_shapes[name], dtype=np.float32)
            morph_deltas.append(delta)

        # Build binary blob
        blob = bytearray()

        def _append(data: np.ndarray) -> Tuple[int, int]:
            offset = len(blob)
            raw = data.tobytes()
            blob.extend(raw)
            while len(blob) % 4:
                blob.extend(b"\x00")
            return offset, len(raw)

        idx_off, idx_len = _append(faces_u32.flatten())
        pos_off, pos_len = _append(verts_f32)
        col_off, col_len = _append(colors_f32)

        morph_info: List[Tuple[int, int]] = []
        for md in morph_deltas:
            off, ln = _append(md)
            morph_info.append((off, ln))

        # Buffer views and accessors
        buffer_views: List = []
        accessors: List = []

        def _add_bv_acc(off, length, comp_type, count, acc_type,
                        v_min=None, v_max=None, target=None):
            bv_kwargs = dict(buffer=0, byteOffset=off, byteLength=length)
            if target is not None:
                bv_kwargs["target"] = target
            buffer_views.append(pygltflib.BufferView(**bv_kwargs))
            bv_idx = len(buffer_views) - 1
            acc_kwargs = dict(
                bufferView=bv_idx, byteOffset=0,
                componentType=comp_type, count=count, type=acc_type,
            )
            if v_max is not None:
                acc_kwargs["max"] = v_max
            if v_min is not None:
                acc_kwargs["min"] = v_min
            accessors.append(pygltflib.Accessor(**acc_kwargs))
            return len(accessors) - 1

        # Indices accessor
        a_idx = _add_bv_acc(
            idx_off, idx_len, pygltflib.UNSIGNED_INT, num_indices, "SCALAR",
            v_min=[int(faces_u32.min())], v_max=[int(faces_u32.max())],
            target=pygltflib.ELEMENT_ARRAY_BUFFER,
        )
        # Position accessor
        a_pos = _add_bv_acc(
            pos_off, pos_len, pygltflib.FLOAT, num_verts, "VEC3",
            v_min=verts_f32.min(axis=0).tolist(), v_max=verts_f32.max(axis=0).tolist(),
            target=pygltflib.ARRAY_BUFFER,
        )
        # Color accessor
        a_col = _add_bv_acc(
            col_off, col_len, pygltflib.FLOAT, num_verts, "VEC4",
            v_min=[0.0, 0.0, 0.0, 0.0], v_max=[1.0, 1.0, 1.0, 1.0],
            target=pygltflib.ARRAY_BUFFER,
        )

        # Morph target accessors
        morph_acc_indices: List[int] = []
        for md, (mo, ml) in zip(morph_deltas, morph_info):
            ai = _add_bv_acc(
                mo, ml, pygltflib.FLOAT, num_verts, "VEC3",
                v_min=md.min(axis=0).tolist(), v_max=md.max(axis=0).tolist(),
                target=pygltflib.ARRAY_BUFFER,
            )
            morph_acc_indices.append(ai)

        # Mesh with morph targets
        targets = [{"POSITION": ai} for ai in morph_acc_indices]
        primitive = pygltflib.Primitive(
            attributes=pygltflib.Attributes(POSITION=a_pos, COLOR_0=a_col),
            indices=a_idx,
            targets=targets if targets else None,
        )
        mesh_obj = pygltflib.Mesh(
            name="face_mesh",
            primitives=[primitive],
            weights=[0.0] * len(morph_names) if morph_names else None,
            extras={"targetNames": morph_names} if morph_names else None,
        )

        gltf = pygltflib.GLTF2(
            asset=pygltflib.Asset(version="2.0", generator="postprocessing.py"),
            scene=0,
            scenes=[pygltflib.Scene(nodes=[0])],
            nodes=[pygltflib.Node(mesh=0, name="face_mesh")],
            meshes=[mesh_obj],
            bufferViews=buffer_views,
            accessors=accessors,
            buffers=[pygltflib.Buffer(byteLength=len(blob))],
        )
        gltf.set_binary_blob(bytes(blob))
        gltf.save(str(output_path))


# ===================================================================
# CLI entry point
# ===================================================================

if __name__ == "__main__":
    import argparse

    logging.basicConfig(level=logging.INFO)

    parser = argparse.ArgumentParser(
        description="Post-processing: relight, rig, and animate NeRF-extracted face meshes"
    )
    parser.add_argument("--mesh", type=str, default=None,
                        help="Path to input mesh (OBJ/PLY/GLB). Omit for demo sphere.")
    parser.add_argument("--output", type=str, default="postprocessing_output",
                        help="Output directory")
    parser.add_argument("--preset", type=str, default="studio",
                        help="Lighting preset: studio, sunset, film_noir, sci_fi, natural")
    args = parser.parse_args()

    if not HAS_TRIMESH:
        print("ERROR: trimesh is required. Install with: pip install trimesh")
        raise SystemExit(1)

    out = Path(args.output)
    out.mkdir(parents=True, exist_ok=True)

    # Load or generate mesh
    if args.mesh is not None:
        mesh = _trimesh.load(args.mesh, process=False)
        vertices = np.asarray(mesh.vertices, dtype=np.float64)
        faces = np.asarray(mesh.faces, dtype=np.int64)
        if hasattr(mesh.visual, "vertex_colors") and mesh.visual.vertex_colors is not None:
            colors = mesh.visual.vertex_colors[:, :3].astype(np.float64) / 255.0
        else:
            colors = np.full((len(vertices), 3), 0.7)
    else:
        print("No mesh provided, generating demo icosphere...")
        mesh = _trimesh.creation.icosphere(subdivisions=3, radius=1.0)
        vertices = np.asarray(mesh.vertices, dtype=np.float64)
        faces = np.asarray(mesh.faces, dtype=np.int64)
        colors = np.full((len(vertices), 3), 0.7)

    # 1. Relight
    relighter = MeshRelighter()
    relit_colors = relighter.relight_mesh(vertices, faces, colors, preset=args.preset)
    relighter.export_relit_mesh(vertices, faces, relit_colors, out / "relit_mesh.glb", format="glb")
    print(f"  Exported relit mesh with '{args.preset}' preset.")

    # 2. Rig
    rigger = SimpleRigger()
    rig_data = rigger.auto_rig(vertices, faces)
    rigger.export_rigged_glb(
        vertices, faces, relit_colors,
        rig_data["joints"], rig_data["weights"],
        out / "rigged_mesh.glb",
    )
    print("  Exported rigged mesh.")

    # 3. Animate
    animator = FaceAnimator()
    blend_shapes = animator.generate_blend_shapes(vertices, faces)
    animator.export_animated_glb(
        vertices, faces, relit_colors, blend_shapes,
        out / "animated_mesh.glb",
    )
    print(f"  Exported animated mesh with {len(blend_shapes)} blend shapes.")
    print(f"Done. Outputs in: {out.resolve()}")
