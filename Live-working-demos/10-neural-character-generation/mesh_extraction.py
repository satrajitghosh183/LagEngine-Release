#!/usr/bin/env python3
"""
Mesh extraction from a trained NeRF model using marching cubes.

This module extracts a 3D mesh from a trained TinyNeRF or WeightedTinyNeRF model
by querying the density field on a voxel grid, running marching cubes to obtain
a triangle mesh, coloring vertices via the NeRF color network, and exporting
the result to OBJ (with vertex colors) and GLB (for web/three.js viewers).

Optionally performs UV unwrapping with xatlas and bakes NeRF colors into a texture.

Usage:
    python mesh_extraction.py --checkpoint path/to/checkpoint.pth \
                              --output_dir path/to/output \
                              --resolution 128 \
                              --density_threshold 50.0

Dependencies:
    torch, numpy, scikit-image (skimage.measure.marching_cubes),
    trimesh (GLB export), xatlas (optional UV unwrapping),
    PIL/Pillow (texture baking)
"""

import os
import sys
import logging
import argparse
from pathlib import Path
from typing import Tuple, Optional

import numpy as np
import torch
import torch.nn.functional as F

# ---------------------------------------------------------------------------
# Logging
# ---------------------------------------------------------------------------
logger = logging.getLogger(__name__)


# ---------------------------------------------------------------------------
# Mesh extraction
# ---------------------------------------------------------------------------

def extract_mesh(
    nerf_model,
    resolution: int = 128,
    density_threshold: float = 50.0,
    bounds: Tuple[float, float] = (-1.5, 1.5),
) -> Tuple[np.ndarray, np.ndarray, np.ndarray]:
    """
    Extract a triangle mesh from a trained NeRF model.

    The function queries the NeRF density field on a regular voxel grid, runs
    marching cubes to extract an isosurface, and evaluates the NeRF color
    network at each vertex to obtain per-vertex RGB colors.

    Args:
        nerf_model: A ``TinyNeRF`` instance (already loaded with trained weights).
        resolution: Number of voxels along each axis of the query grid.
        density_threshold: Iso-value passed to marching cubes.  Higher values
            produce tighter surfaces; lower values produce larger, fuzzier
            meshes.
        bounds: ``(min, max)`` spatial extent of the query volume.

    Returns:
        vertices: ``(V, 3)`` float32 array of vertex positions in world space.
        faces: ``(F, 3)`` int32 array of triangle indices.
        colors: ``(V, 3)`` float32 array of RGB vertex colors in ``[0, 1]``.

    Raises:
        RuntimeError: If marching cubes finds no surface at the given threshold.
    """
    try:
        from skimage.measure import marching_cubes
    except ImportError:
        raise ImportError(
            "scikit-image is required for marching cubes.  "
            "Install it with: pip install scikit-image"
        )

    device = next(nerf_model.parameters()).device
    nerf_model.eval()

    # ------------------------------------------------------------------
    # 1. Build the density grid
    # ------------------------------------------------------------------
    logger.info(
        "Extracting density grid at resolution %d (bounds %.2f .. %.2f) ...",
        resolution, bounds[0], bounds[1],
    )
    density_grid = nerf_model.extract_density_grid(
        resolution=resolution, bounds=bounds
    )
    logger.info(
        "Density range: min=%.4f, max=%.4f, mean=%.4f",
        density_grid.min(), density_grid.max(), density_grid.mean(),
    )

    # ------------------------------------------------------------------
    # 2. Marching cubes
    # ------------------------------------------------------------------
    try:
        verts_grid, faces, normals, _ = marching_cubes(
            density_grid, level=density_threshold
        )
    except (ValueError, RuntimeError) as exc:
        raise RuntimeError(
            f"Marching cubes failed (threshold={density_threshold}).  "
            f"Density range is [{density_grid.min():.2f}, {density_grid.max():.2f}].  "
            f"Try lowering --density_threshold.  Original error: {exc}"
        ) from exc

    if verts_grid.shape[0] == 0:
        raise RuntimeError(
            "Marching cubes produced zero vertices.  "
            f"Density range is [{density_grid.min():.2f}, {density_grid.max():.2f}].  "
            "Try lowering --density_threshold."
        )

    # Convert voxel indices to world coordinates
    min_b, max_b = bounds
    verts_world = verts_grid / (resolution - 1) * (max_b - min_b) + min_b
    verts_world = verts_world.astype(np.float32)
    faces = faces.astype(np.int32)

    logger.info(
        "Marching cubes produced %d vertices and %d faces.",
        verts_world.shape[0], faces.shape[0],
    )

    # ------------------------------------------------------------------
    # 3. Query the NeRF color network for per-vertex colors
    # ------------------------------------------------------------------
    colors = _query_vertex_colors(nerf_model, verts_world, device)

    return verts_world, faces, colors


def _query_vertex_colors(
    nerf_model,
    vertices: np.ndarray,
    device: torch.device,
    chunk_size: int = 64 * 1024,
) -> np.ndarray:
    """
    Evaluate the NeRF color MLP at each vertex position.

    Because the color network expects both positional features and a view
    direction, we use a canonical view direction (looking down -Z) for all
    vertices.  This produces a view-independent "albedo-like" color.

    Args:
        nerf_model: The TinyNeRF model.
        vertices: ``(V, 3)`` vertex positions.
        device: Torch device to run inference on.
        chunk_size: Number of vertices to process per batch.

    Returns:
        ``(V, 3)`` float32 numpy array of RGB colors in ``[0, 1]``.
    """
    logger.info("Querying NeRF color network for %d vertices ...", len(vertices))
    all_colors = []

    # Canonical view direction: looking along -Z
    canonical_dir = np.array([0.0, 0.0, -1.0], dtype=np.float32)

    with torch.no_grad():
        for start in range(0, len(vertices), chunk_size):
            end = min(start + chunk_size, len(vertices))
            pts = torch.from_numpy(vertices[start:end]).to(device)  # (C, 3)

            # Positional encoding for positions
            encoded_pts = nerf_model.positional_encoding(
                pts, nerf_model.num_encoding_functions
            )

            # Density network -> features
            density_out = nerf_model.density_net(encoded_pts)
            features = density_out[..., 1:]  # drop sigma column

            # Build encoded view directions (same for all vertices)
            dirs = (
                torch.from_numpy(canonical_dir)
                .unsqueeze(0)
                .expand(pts.shape[0], -1)
                .to(device)
            )
            encoded_dirs = nerf_model.positional_encoding(
                dirs, nerf_model.num_encoding_functions
            )

            # Color network
            color_input = torch.cat([features, encoded_dirs], dim=-1)
            rgb = nerf_model.color_net(color_input)  # (C, 3), already sigmoid
            all_colors.append(rgb.cpu().numpy())

    colors = np.concatenate(all_colors, axis=0).astype(np.float32)
    colors = np.clip(colors, 0.0, 1.0)
    return colors


# ---------------------------------------------------------------------------
# UV unwrapping and texture baking (optional xatlas path)
# ---------------------------------------------------------------------------

def _try_uv_unwrap(
    vertices: np.ndarray,
    faces: np.ndarray,
) -> Optional[Tuple[np.ndarray, np.ndarray]]:
    """
    Attempt UV unwrapping via xatlas.  Returns ``None`` if xatlas is not
    installed or if the unwrap fails.

    Returns:
        ``(uv_coords, uv_faces)`` on success, or ``None``.
    """
    try:
        import xatlas
    except ImportError:
        logger.info("xatlas not available; falling back to vertex colors.")
        return None

    try:
        logger.info("Running xatlas UV unwrap ...")
        vmapping, uv_indices, uv_coords = xatlas.parametrize(
            vertices.astype(np.float32),
            faces.astype(np.uint32),
        )
        logger.info(
            "UV unwrap complete: %d UV coords, %d UV faces.",
            uv_coords.shape[0], uv_indices.shape[0],
        )
        return uv_coords, uv_indices
    except Exception:
        logger.warning("xatlas UV unwrapping failed; falling back to vertex colors.",
                       exc_info=True)
        return None


def bake_texture(
    nerf_model,
    vertices: np.ndarray,
    faces: np.ndarray,
    uv_coords: np.ndarray,
    uv_faces: np.ndarray,
    texture_size: int = 1024,
) -> np.ndarray:
    """
    Bake NeRF colors onto a UV-mapped texture atlas.

    For each texel in the output image, the function determines which triangle
    it belongs to (via a rasterisation lookup), computes the corresponding 3D
    position using barycentric interpolation, and queries the NeRF color
    network to obtain the RGB value.

    Args:
        nerf_model: Trained TinyNeRF model.
        vertices: ``(V, 3)`` mesh vertex positions.
        faces: ``(F, 3)`` triangle indices into *vertices*.
        uv_coords: ``(U, 2)`` UV coordinates produced by the unwrapper.
        uv_faces: ``(F, 3)`` triangle indices into *uv_coords*.
        texture_size: Width and height of the output texture in pixels.

    Returns:
        ``(texture_size, texture_size, 3)`` uint8 RGB texture image.
    """
    device = next(nerf_model.parameters()).device
    nerf_model.eval()

    texture = np.zeros((texture_size, texture_size, 3), dtype=np.float32)
    mask = np.zeros((texture_size, texture_size), dtype=bool)

    logger.info(
        "Baking texture at %dx%d from %d triangles ...",
        texture_size, texture_size, faces.shape[0],
    )

    # For each triangle, rasterise into UV space and query the NeRF
    for fi in range(faces.shape[0]):
        # UV triangle corners  (in [0,1] range)
        uv_tri = uv_coords[uv_faces[fi]]  # (3, 2)
        # 3D triangle corners
        pos_tri = vertices[faces[fi]]  # (3, 3)

        # Compute bounding box in texel space
        uv_px = uv_tri * texture_size
        u_min = max(int(np.floor(uv_px[:, 0].min())), 0)
        u_max = min(int(np.ceil(uv_px[:, 0].max())), texture_size - 1)
        v_min = max(int(np.floor(uv_px[:, 1].min())), 0)
        v_max = min(int(np.ceil(uv_px[:, 1].max())), texture_size - 1)

        if u_min > u_max or v_min > v_max:
            continue

        # Generate texel centers inside the bounding box
        us = np.arange(u_min, u_max + 1) + 0.5
        vs = np.arange(v_min, v_max + 1) + 0.5
        grid_u, grid_v = np.meshgrid(us, vs, indexing="ij")
        texels = np.stack([grid_u.ravel(), grid_v.ravel()], axis=-1)  # (K, 2)

        # Barycentric coordinates in UV space
        bary = _barycentric_coords(texels, uv_px)  # (K, 3)
        inside = np.all(bary >= -1e-4, axis=-1)
        if not inside.any():
            continue

        bary_in = bary[inside]
        texel_idx = texels[inside].astype(int)

        # Interpolate 3D positions
        pts_3d = bary_in @ pos_tri  # (M, 3)
        pts_tensor = torch.from_numpy(pts_3d.astype(np.float32)).to(device)

        with torch.no_grad():
            encoded_pts = nerf_model.positional_encoding(
                pts_tensor, nerf_model.num_encoding_functions
            )
            density_out = nerf_model.density_net(encoded_pts)
            features = density_out[..., 1:]

            dirs = torch.tensor(
                [[0.0, 0.0, -1.0]], device=device
            ).expand(pts_tensor.shape[0], -1)
            encoded_dirs = nerf_model.positional_encoding(
                dirs, nerf_model.num_encoding_functions
            )
            color_input = torch.cat([features, encoded_dirs], dim=-1)
            rgb = nerf_model.color_net(color_input).cpu().numpy()

        for idx_in_batch, (tu, tv) in enumerate(texel_idx):
            tu_clamped = min(max(tu, 0), texture_size - 1)
            tv_clamped = min(max(tv, 0), texture_size - 1)
            texture[tv_clamped, tu_clamped] = rgb[idx_in_batch]
            mask[tv_clamped, tu_clamped] = True

    # Clamp and convert
    texture = np.clip(texture, 0.0, 1.0)
    texture_uint8 = (texture * 255).astype(np.uint8)

    filled_ratio = mask.sum() / mask.size * 100
    logger.info("Texture baking done: %.1f%% of texels filled.", filled_ratio)

    return texture_uint8


def _barycentric_coords(
    points: np.ndarray,
    triangle: np.ndarray,
) -> np.ndarray:
    """
    Compute barycentric coordinates of *points* w.r.t. a 2D *triangle*.

    Args:
        points: ``(N, 2)`` query points.
        triangle: ``(3, 2)`` triangle vertices.

    Returns:
        ``(N, 3)`` barycentric coordinates.
    """
    v0 = triangle[2] - triangle[0]
    v1 = triangle[1] - triangle[0]
    v2 = points - triangle[0]

    dot00 = np.dot(v0, v0)
    dot01 = np.dot(v0, v1)
    dot11 = np.dot(v1, v1)

    dot02 = v2 @ v0
    dot12 = v2 @ v1

    denom = dot00 * dot11 - dot01 * dot01
    if abs(denom) < 1e-12:
        return np.full((points.shape[0], 3), -1.0)

    inv_denom = 1.0 / denom
    u = (dot11 * dot02 - dot01 * dot12) * inv_denom
    v = (dot00 * dot12 - dot01 * dot02) * inv_denom
    w = 1.0 - u - v

    return np.stack([w, v, u], axis=-1)


# ---------------------------------------------------------------------------
# Export helpers
# ---------------------------------------------------------------------------

def export_obj(
    vertices: np.ndarray,
    faces: np.ndarray,
    colors: np.ndarray,
    output_path: str,
) -> None:
    """
    Save a mesh as Wavefront OBJ with per-vertex colors.

    Vertex colors are stored as additional ``r g b`` values on each ``v``
    line (a common extension supported by MeshLab, Blender, etc.).

    Args:
        vertices: ``(V, 3)`` vertex positions.
        faces: ``(F, 3)`` triangle indices (0-based).
        colors: ``(V, 3)`` RGB colors in ``[0, 1]``.
        output_path: Destination ``.obj`` file path.
    """
    output_path = str(output_path)
    os.makedirs(os.path.dirname(output_path) or ".", exist_ok=True)

    logger.info("Writing OBJ to %s ...", output_path)
    with open(output_path, "w") as f:
        f.write("# Mesh extracted from NeRF via marching cubes\n")
        f.write(f"# Vertices: {vertices.shape[0]}  Faces: {faces.shape[0]}\n\n")

        for vi in range(vertices.shape[0]):
            x, y, z = vertices[vi]
            r, g, b = colors[vi]
            f.write(f"v {x:.6f} {y:.6f} {z:.6f} {r:.6f} {g:.6f} {b:.6f}\n")

        f.write("\n")

        # OBJ faces are 1-indexed
        for fi in range(faces.shape[0]):
            i0, i1, i2 = faces[fi] + 1
            f.write(f"f {i0} {i1} {i2}\n")

    logger.info("OBJ saved (%d verts, %d faces).", vertices.shape[0], faces.shape[0])


def export_glb(
    vertices: np.ndarray,
    faces: np.ndarray,
    colors: np.ndarray,
    output_path: str,
) -> None:
    """
    Save a mesh in GLB (binary glTF) format for three.js / web viewers.

    Uses the ``trimesh`` library to construct the scene and serialise it.

    Args:
        vertices: ``(V, 3)`` vertex positions.
        faces: ``(F, 3)`` triangle indices (0-based).
        colors: ``(V, 3)`` RGB colors in ``[0, 1]``.
        output_path: Destination ``.glb`` file path.
    """
    try:
        import trimesh
    except ImportError:
        raise ImportError(
            "trimesh is required for GLB export.  "
            "Install it with: pip install trimesh"
        )

    output_path = str(output_path)
    os.makedirs(os.path.dirname(output_path) or ".", exist_ok=True)

    logger.info("Writing GLB to %s ...", output_path)

    # trimesh expects uint8 RGBA vertex colors
    rgba = np.zeros((colors.shape[0], 4), dtype=np.uint8)
    rgba[:, :3] = (np.clip(colors, 0.0, 1.0) * 255).astype(np.uint8)
    rgba[:, 3] = 255

    mesh = trimesh.Trimesh(
        vertices=vertices,
        faces=faces,
        vertex_colors=rgba,
        process=False,
    )

    mesh.export(output_path, file_type="glb")
    file_size_mb = os.path.getsize(output_path) / (1024 * 1024)
    logger.info("GLB saved (%.2f MB).", file_size_mb)


# ---------------------------------------------------------------------------
# End-to-end pipeline
# ---------------------------------------------------------------------------

def run_mesh_extraction(
    checkpoint_path: str,
    output_dir: str,
    resolution: int = 128,
    density_threshold: float = 50.0,
    bounds: Tuple[float, float] = (-1.5, 1.5),
    texture_size: int = 1024,
) -> None:
    """
    Load a trained NeRF from a checkpoint and export the mesh in OBJ and GLB.

    The checkpoint is expected to contain at least the key
    ``"nerf_state_dict"`` (the ``WeightedTinyNeRF.state_dict()``).  The
    underlying ``TinyNeRF`` is accessed via ``WeightedTinyNeRF.nerf``.

    If ``xatlas`` is available, a UV-unwrapped texture is also baked and
    saved alongside the OBJ.

    Args:
        checkpoint_path: Path to the ``.pth`` checkpoint file.
        output_dir: Directory where output files will be written.
        resolution: Voxel grid resolution per axis.
        density_threshold: Marching-cubes iso-value.
        bounds: ``(min, max)`` spatial extent of the query volume.
        texture_size: Resolution of the baked texture (if UV unwrap succeeds).
    """
    # Lazy imports so the module can be imported without these installed
    from nerf.tiny_nerf import TinyNeRF
    from nerf.weighted_tiny_nerf import WeightedTinyNeRF

    checkpoint_path = str(checkpoint_path)
    output_dir = str(output_dir)
    os.makedirs(output_dir, exist_ok=True)

    # ------------------------------------------------------------------
    # 1. Load checkpoint
    # ------------------------------------------------------------------
    logger.info("Loading checkpoint from %s ...", checkpoint_path)
    if not os.path.isfile(checkpoint_path):
        raise FileNotFoundError(f"Checkpoint not found: {checkpoint_path}")

    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    checkpoint = torch.load(checkpoint_path, map_location=device, weights_only=False)

    if "nerf_state_dict" not in checkpoint:
        raise KeyError(
            "Checkpoint does not contain 'nerf_state_dict'.  "
            f"Available keys: {list(checkpoint.keys())}"
        )

    nerf_state = checkpoint["nerf_state_dict"]

    # ------------------------------------------------------------------
    # 2. Determine model type and instantiate
    # ------------------------------------------------------------------
    # Heuristic: if state_dict keys start with "nerf." it is a
    # WeightedTinyNeRF; otherwise it is a bare TinyNeRF.
    is_weighted = any(k.startswith("nerf.") for k in nerf_state.keys())

    if is_weighted:
        logger.info("Detected WeightedTinyNeRF checkpoint.")
        weighted_model = WeightedTinyNeRF()
        weighted_model.load_state_dict(nerf_state)
        weighted_model.to(device).eval()
        nerf_model = weighted_model.nerf
    else:
        logger.info("Detected bare TinyNeRF checkpoint.")
        nerf_model = TinyNeRF()
        nerf_model.load_state_dict(nerf_state)
        nerf_model.to(device).eval()

    logger.info("Model loaded on %s.", device)

    # ------------------------------------------------------------------
    # 3. Extract mesh
    # ------------------------------------------------------------------
    vertices, faces, colors = extract_mesh(
        nerf_model,
        resolution=resolution,
        density_threshold=density_threshold,
        bounds=bounds,
    )

    # ------------------------------------------------------------------
    # 4. Export OBJ
    # ------------------------------------------------------------------
    obj_path = os.path.join(output_dir, "mesh.obj")
    export_obj(vertices, faces, colors, obj_path)

    # ------------------------------------------------------------------
    # 5. Export GLB
    # ------------------------------------------------------------------
    try:
        glb_path = os.path.join(output_dir, "mesh.glb")
        export_glb(vertices, faces, colors, glb_path)
    except ImportError:
        logger.warning("Skipping GLB export (trimesh not installed).")

    # ------------------------------------------------------------------
    # 6. Optional: UV unwrap + texture bake
    # ------------------------------------------------------------------
    uv_result = _try_uv_unwrap(vertices, faces)
    if uv_result is not None:
        uv_coords, uv_faces = uv_result
        texture = bake_texture(
            nerf_model, vertices, faces, uv_coords, uv_faces,
            texture_size=texture_size,
        )
        # Save texture
        try:
            from PIL import Image
            tex_path = os.path.join(output_dir, "texture.png")
            Image.fromarray(texture).save(tex_path)
            logger.info("Texture saved to %s.", tex_path)
        except ImportError:
            logger.warning(
                "Pillow not installed; saving texture as raw numpy array."
            )
            npy_path = os.path.join(output_dir, "texture.npy")
            np.save(npy_path, texture)
            logger.info("Texture array saved to %s.", npy_path)

    logger.info("Mesh extraction complete.  Outputs in %s", output_dir)


# ---------------------------------------------------------------------------
# CLI entry-point
# ---------------------------------------------------------------------------

def _parse_args(argv=None):
    parser = argparse.ArgumentParser(
        description="Extract a 3D mesh from a trained NeRF checkpoint."
    )
    parser.add_argument(
        "--checkpoint", type=str, required=True,
        help="Path to the .pth checkpoint file.",
    )
    parser.add_argument(
        "--output_dir", type=str, required=True,
        help="Directory for output mesh files.",
    )
    parser.add_argument(
        "--resolution", type=int, default=128,
        help="Voxel grid resolution per axis (default: 128).",
    )
    parser.add_argument(
        "--density_threshold", type=float, default=50.0,
        help="Marching-cubes iso-value (default: 50.0).",
    )
    parser.add_argument(
        "--bounds_min", type=float, default=-1.5,
        help="Minimum spatial bound (default: -1.5).",
    )
    parser.add_argument(
        "--bounds_max", type=float, default=1.5,
        help="Maximum spatial bound (default: 1.5).",
    )
    parser.add_argument(
        "--texture_size", type=int, default=1024,
        help="Baked texture resolution (default: 1024).",
    )
    return parser.parse_args(argv)


def main(argv=None):
    args = _parse_args(argv)

    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
    )

    run_mesh_extraction(
        checkpoint_path=args.checkpoint,
        output_dir=args.output_dir,
        resolution=args.resolution,
        density_threshold=args.density_threshold,
        bounds=(args.bounds_min, args.bounds_max),
        texture_size=args.texture_size,
    )


if __name__ == "__main__":
    main()
