import os
import sys
import torch
import numpy as np
import open3d as o3d
from pyhocon import ConfigFactory
from model import make_model
from data import get_split_dataset
from torch.utils.data import DataLoader

from util.recon import marching_cubes, save_obj

from skimage import measure
import trimesh

T = 6.0

# === Project path correction ===
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "../src")))

# === Configuration paths ===
DEVICE = torch.device("cuda" if torch.cuda.is_available() else "cpu")
EXPERIMENT = "2Dimplicitdeform_reconstruct"
CONF_PATH = f"./results/{EXPERIMENT}/fp_mixexp_2D_implicit.conf"
CHECKPOINT_PATH = f"./results/{EXPERIMENT}/checkpoints/pixel_nerf_latest"
EXPORT_PATH = f"./results/{EXPERIMENT}/export/volume_points.ply"

# === Load configuration and model ===
print("Loading config and model...")
conf = ConfigFactory.parse_file(CONF_PATH)
model = make_model(conf["model"])
model.load_state_dict(torch.load(CHECKPOINT_PATH))
model.cuda().eval()

# === Load test data ===
test_dset, val_dset, _ = get_split_dataset("fp_admixexp",
                                    '/scratch/network/hy4522/FDNeRF_data/FDNeRF_converted',
                                    n_view_in=12,
                                    list_prefix="mixwild",
                                    sem_win=1,
                                    with_mask=True)

test_loader = DataLoader(test_dset, batch_size=1, shuffle=False)
data = next(iter(test_loader))

print("Encoding source views...")
images = data["images"][0, :12].to(DEVICE)  # Take 12 source images
poses = data["poses"][0, :12].to(DEVICE)
focal = data["focal"][0, :12].to(DEVICE)
c = data["c"][0, :12].to(DEVICE)
semantic_src = data["semantic_src"].to(DEVICE)
semantic_cdn = data["semantic_cdn"].to(DEVICE)

# Fix semantic input for the model
sem_src = semantic_src[0, :, :, 0].to(DEVICE)
sem_cdn = semantic_cdn[0, :, :, 0].to(DEVICE)

print(f"sem_src shape: {sem_src.shape}")
print(f"sem_cdn shape: {sem_cdn.shape}")

semantic = {
    "semantic_src": sem_src,
    "semantic_cdn": sem_cdn,
}

# Encode the images
model.encode(images.unsqueeze(0), poses.unsqueeze(0), focal, c, semantic=semantic)

# 使用 recon.py 中的 marching_cubes 替代您的体素化方法
# print("运行 marching cubes 算法...")
# vertices, triangles = marching_cubes(
#     model,
#     c1=[-1.5, -1.5, -1.5],  # 扩大边界以确保捕获整个头部
#     c2=[1.5, 1.5, 1.5],
#     reso=[256, 256, 256],   # 增加分辨率
#     isosurface=0.5,         # 减小阈值以获取更多体积
#     eval_batch_size=65536,
#     device=DEVICE
# )

# # 保存为 OBJ 和 PLY 双格式
# mesh_obj_path = EXPORT_PATH.replace("volume_points.ply", "volume_mesh.obj")
# save_obj(vertices, triangles, mesh_obj_path)
# print(f"网格已导出到 {mesh_obj_path}")

# # 也可以使用 trimesh 保存为 PLY
# import trimesh
# mesh = trimesh.Trimesh(vertices=vertices, faces=triangles)
# mesh_path = EXPORT_PATH.replace("volume_points.ply", "volume_mesh.ply") 
# mesh.export(mesh_path)
# print(f"网格已导出到 {mesh_path}")

@torch.no_grad()
def direct_query_density(model, pts):
    """
    A simplified density query that bypasses the complex forward pass
    and directly queries the MLP with a constant feature vector
    """
    # Process points in manageable chunks
    batch_size = 1024
    sigmas = []

    # ??? 
    dummy_feature = torch.zeros(model.latent_size, device=pts.device)
    
    for i in range(0, pts.shape[0], batch_size):
        # Extract current batch
        pts_batch = pts[i:i+batch_size]
        num_pts = pts_batch.shape[0]
        
        # Prepare positional encoding input
        if model.use_xyz:
            pos_input = pts_batch
        else:
            pos_input = pts_batch[:, 2:3]  # Only z coordinate
        
        # Apply positional encoding
        if model.use_code:
            pos_encoded = model.code(pos_input)
        else:
            pos_encoded = pos_input
            
        # Add dummy viewdirs if needed
        if model.use_viewdirs:
            # Default view direction (negative z)
            viewdirs = torch.zeros_like(pts_batch)
            viewdirs[:, 2] = -1.0
            
            if model.use_code_viewdirs and not model.use_code_separate:
                # Encode and combine with pos_encoded
                if model.use_code:
                    view_pos_encoded = torch.cat([pos_input, viewdirs], dim=1)
                    encoded = model.code(view_pos_encoded)
                else:
                    encoded = torch.cat([pos_encoded, viewdirs], dim=1)
            elif model.use_code_viewdirs and model.use_code_separate:
                # Encode positions and viewdirs separately
                viewdirs_encoded = model.code_dir(viewdirs)
                encoded = torch.cat([pos_encoded, viewdirs_encoded], dim=1)
            else:
                # Just append raw viewdirs
                encoded = torch.cat([pos_encoded, viewdirs], dim=1)
        else:
            encoded = pos_encoded
        
        # Replicate the dummy feature for each point
        repeated_dummy = dummy_feature.expand(num_pts, -1)
        
        # Concatenate with encoded positions (and view dirs if used)
        mlp_input = torch.cat([repeated_dummy, encoded], dim=1)
        
        # Forward pass through just the MLP
        if torch.isnan(mlp_input).any():
            mlp_input = torch.where(torch.isnan(mlp_input), 
                                   torch.full_like(mlp_input, 0),
                                   mlp_input)
        
        # Run through the MLP directly
        with torch.no_grad():
            output = model.mlp_coarse(mlp_input)
        
        # Extract density (sigma) and apply activation
        sigma = output[:, 3]
        sigma = torch.relu(sigma)  # Ensure non-negative density
        
        sigmas.append(sigma)
    
    # Combine all chunks
    return torch.cat(sigmas, dim=0)

# === Generate voxel grid ===
print("Generating voxel grid...")
grid_size = 256
bound = 6.0
x = torch.linspace(-bound, bound, grid_size)
y = torch.linspace(-bound, bound, grid_size)
z = torch.linspace(-bound, bound, grid_size)
X, Y, Z = torch.meshgrid(x, y, z)
pts = torch.stack([X, Y, Z], dim=-1).reshape(-1, 3).cuda()

# === Extract density ===
print("Querying model...")
sigmas = []
CHUNK = 65536  # Adjust this based on your GPU memory
total_chunks = (pts.shape[0] + CHUNK - 1) // CHUNK

for i in range(0, pts.shape[0], CHUNK):
    print(f"Processing chunk {i//CHUNK + 1}/{total_chunks}")
    chunk_sigma = direct_query_density(model, pts[i:i+CHUNK])
    sigmas.append(chunk_sigma)

sigmas = torch.cat(sigmas, dim=0)

print("Reshaping sigma into 3D volume...")
density_volume = sigmas.reshape(grid_size, grid_size, grid_size).detach().cpu().numpy()

# === Apply Marching Cubes ===
iso_level = T  # Same as sigma_thresh
print(f"Applying marching cubes with iso-level {iso_level}...")
verts, faces, normals, _ = measure.marching_cubes(density_volume, level=iso_level)

# === Convert voxel indices to world coordinates
scale = (2 * bound) / grid_size  # scale from voxel to world space
verts_world = verts * scale - bound  # shift to [-bound, bound]

# === Create mesh and export
mesh = trimesh.Trimesh(vertices=verts_world, faces=faces, vertex_normals=normals)
mesh_path = EXPORT_PATH.replace("volume_points.ply", "volume_mesh.ply")
mesh.export(mesh_path)

print("Mesh exported to", mesh_path)

# === Filter valid points ===
sigma_thresh = T
mask = sigmas < sigma_thresh
pts_valid = pts[mask]

print(f"Selected {pts_valid.shape[0]} points with σ > {sigma_thresh}")
os.makedirs(os.path.dirname(EXPORT_PATH), exist_ok=True)

# === Export to .ply ===
if pts_valid.shape[0] > 0:
    pcd = o3d.geometry.PointCloud()
    pcd.points = o3d.utility.Vector3dVector(pts_valid.detach().cpu().numpy())
    o3d.io.write_point_cloud(EXPORT_PATH, pcd)
    print("Exported to", EXPORT_PATH)
else:
    print("No points above threshold. Nothing to export.")