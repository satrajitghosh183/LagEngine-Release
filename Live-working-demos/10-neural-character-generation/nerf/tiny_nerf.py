import torch
import torch.nn as nn
import torch.nn.functional as F
import numpy as np
from math import log10

class TinyNeRF(nn.Module):
    """
    A lightweight, differentiable Neural Radiance Field (NeRF) implementation.

    This implementation focuses on:
    1. Differentiability (for end-to-end training)
    2. Efficiency (for faster training)
    3. Simplicity (fewer parameters than full NeRF)

    The model renders novel views given input images and camera poses.
    """
    def __init__(self,
                 num_encoding_functions=10,
                 hidden_dim=128,
                 num_layers=4,
                 image_height=256,
                 image_width=256):
        super().__init__()

        # Positional encoding dimensions
        self.num_encoding_functions = num_encoding_functions
        self.in_dim_position = 3 + 3 * 2 * num_encoding_functions  # (x,y,z) + positional encoding
        self.in_dim_direction = 3 + 3 * 2 * num_encoding_functions  # (d_x,d_y,d_z) + positional encoding

        # Image dimensions
        self.image_height = image_height
        self.image_width = image_width

        # MLP for density prediction
        density_layers = []
        density_layers.append(nn.Linear(self.in_dim_position, hidden_dim))
        density_layers.append(nn.ReLU())

        for _ in range(num_layers - 2):
            density_layers.append(nn.Linear(hidden_dim, hidden_dim))
            density_layers.append(nn.ReLU())

        density_layers.append(nn.Linear(hidden_dim, hidden_dim + 1))  # +1 for density sigma

        self.density_net = nn.Sequential(*density_layers)

        # MLP for color prediction
        self.color_net = nn.Sequential(
            nn.Linear(hidden_dim + self.in_dim_direction, hidden_dim//2),
            nn.ReLU(),
            nn.Linear(hidden_dim//2, 3),
            nn.Sigmoid()  # Normalize colors to [0, 1]
        )

    def positional_encoding(self, x, L=10):
        """
        Apply positional encoding to the input.

        Args:
            x (torch.Tensor): Input coordinates (position or direction)
            L (int): Number of encoding functions

        Returns:
            torch.Tensor: Encoded coordinates with shape [..., 3 + 3*2*L]
        """
        encoded = [x]

        for i in range(L):
            for fn in [torch.sin, torch.cos]:
                encoded.append(fn(2.0**i * x))

        return torch.cat(encoded, dim=-1)

    def get_rays(self, poses):
        """
        Generate rays for each pixel in the target image.

        Args:
            poses (torch.Tensor): Camera poses with shape [B, 7] (position + quaternion)

        Returns:
            tuple: (ray_origins, ray_directions) with shapes [B, H*W, 3]
        """
        B = poses.shape[0]
        device = poses.device

        # Extract positions and convert quaternions to rotation matrices
        positions = poses[:, :3]  # [B, 3]
        quaternions = poses[:, 3:]  # [B, 4]

        # Convert quaternions to rotation matrices
        R = self.quaternion_to_rotation_matrix(quaternions)  # [B, 3, 3]

        # Create pixel coordinates grid
        i, j = torch.meshgrid(
            torch.linspace(0, self.image_width - 1, self.image_width),
            torch.linspace(0, self.image_height - 1, self.image_height),
            indexing='ij'
        )
        i, j = i.t().to(device), j.t().to(device)  # Transpose to match image coordinates

        # Scale to [-1, 1] and create homogeneous coordinates
        x = (2 * (i + 0.5) / self.image_width - 1) * torch.tan(torch.tensor(np.pi/4))
        y = (2 * (j + 0.5) / self.image_height - 1) * torch.tan(torch.tensor(np.pi/4))

        # Generate ray directions in camera frame
        directions = torch.stack([x, -y, -torch.ones_like(x)], dim=-1)  # [H, W, 3]
        directions = directions / torch.norm(directions, dim=-1, keepdim=True)  # Normalize

        # Repeat for batch
        directions = directions.unsqueeze(0).repeat(B, 1, 1, 1)  # [B, H, W, 3]

        # Transform ray directions to world frame
        directions = directions.reshape(B, -1, 3)  # [B, H*W, 3]
        ray_directions = torch.bmm(directions, R)  # [B, H*W, 3]

        # Set ray origins to camera positions
        ray_origins = positions.unsqueeze(1).repeat(1, self.image_height * self.image_width, 1)  # [B, H*W, 3]

        return ray_origins, ray_directions

    def quaternion_to_rotation_matrix(self, q):
        """
        Convert quaternions to rotation matrices.

        Args:
            q (torch.Tensor): Quaternions with shape [..., 4] (qx, qy, qz, qw)

        Returns:
            torch.Tensor: Rotation matrices with shape [..., 3, 3]
        """
        # Normalize quaternions
        q = F.normalize(q, dim=-1)

        # Extract quaternion components
        qx, qy, qz, qw = q[..., 0], q[..., 1], q[..., 2], q[..., 3]

        # Compute rotation matrix elements
        R00 = 1 - 2 * (qy**2 + qz**2)
        R01 = 2 * (qx * qy - qw * qz)
        R02 = 2 * (qx * qz + qw * qy)

        R10 = 2 * (qx * qy + qw * qz)
        R11 = 1 - 2 * (qx**2 + qz**2)
        R12 = 2 * (qy * qz - qw * qx)

        R20 = 2 * (qx * qz - qw * qy)
        R21 = 2 * (qy * qz + qw * qx)
        R22 = 1 - 2 * (qx**2 + qy**2)

        # Stack to form rotation matrices
        R = torch.stack([
            torch.stack([R00, R01, R02], dim=-1),
            torch.stack([R10, R11, R12], dim=-1),
            torch.stack([R20, R21, R22], dim=-1)
        ], dim=-2)

        return R

    def render_rays(self, ray_origins, ray_directions, near=2.0, far=6.0, num_samples=64, rand=True):
        """
        Render rays by sampling points and predicting colors and densities.

        Args:
            ray_origins (torch.Tensor): Ray origin points [B, N, 3]
            ray_directions (torch.Tensor): Ray direction vectors [B, N, 3]
            near (float): Near plane distance
            far (float): Far plane distance
            num_samples (int): Number of sample points per ray
            rand (bool): Whether to use randomized sampling

        Returns:
            dict: Output containing rendered colors and depths
        """
        device = ray_origins.device
        batch_size, num_rays = ray_origins.shape[:2]

        # Sample points along each ray
        t_vals = torch.linspace(near, far, num_samples, device=device)

        if rand:
            # Add random offset to sample points for better training
            mids = 0.5 * (t_vals[..., 1:] + t_vals[..., :-1])
            upper = torch.cat([mids, t_vals[..., -1:]], dim=-1)
            lower = torch.cat([t_vals[..., :1], mids], dim=-1)
            t_rand = torch.rand(batch_size, num_rays, num_samples, device=device)
            t_vals = lower.unsqueeze(0).unsqueeze(0) + (upper - lower).unsqueeze(0).unsqueeze(0) * t_rand
        else:
            t_vals = t_vals.expand(batch_size, num_rays, num_samples)

        # Calculate sample point positions along rays
        ray_directions_expanded = ray_directions.unsqueeze(2).expand(-1, -1, num_samples, -1)  # [B, N, S, 3]
        ray_origins_expanded = ray_origins.unsqueeze(2).expand(-1, -1, num_samples, -1)  # [B, N, S, 3]
        sample_points = ray_origins_expanded + t_vals.unsqueeze(-1) * ray_directions_expanded  # [B, N, S, 3]

        # Reshape for batch processing
        flattened_points = sample_points.reshape(-1, 3)  # [B*N*S, 3]

        # Apply positional encoding
        encoded_points = self.positional_encoding(flattened_points, self.num_encoding_functions)  # [B*N*S, 3+3*2*L]

        # Predict density and features
        density_outputs = self.density_net(encoded_points)  # [B*N*S, hidden+1]
        sigma = F.relu(density_outputs[..., 0])  # [B*N*S]
        features = density_outputs[..., 1:]  # [B*N*S, hidden]

        # Reshape directions for batch processing
        flattened_dirs = ray_directions_expanded.reshape(-1, 3)  # [B*N*S, 3]

        # Apply positional encoding to directions
        encoded_dirs = self.positional_encoding(flattened_dirs, self.num_encoding_functions)  # [B*N*S, 3+3*2*L]

        # Predict colors using density features and encoded directions
        color_input = torch.cat([features, encoded_dirs], dim=-1)  # [B*N*S, hidden+3+3*2*L]
        colors = self.color_net(color_input)  # [B*N*S, 3]

        # Reshape outputs
        sigma = sigma.reshape(batch_size, num_rays, num_samples)  # [B, N, S]
        colors = colors.reshape(batch_size, num_rays, num_samples, 3)  # [B, N, S, 3]

        # Perform volume rendering
        delta_dist = t_vals[..., 1:] - t_vals[..., :-1]  # [B, N, S-1]
        delta_dist = torch.cat([
            delta_dist,
            1e10 * torch.ones_like(delta_dist[..., :1])  # Add large value for last sample
        ], dim=-1)  # [B, N, S]

        # Convert sigma to alpha values
        alpha = 1.0 - torch.exp(-sigma * delta_dist)  # [B, N, S]

        # Calculate transmittance
        weights = alpha * torch.cumprod(
            torch.cat([
                torch.ones_like(alpha[..., :1]),
                1.0 - alpha[..., :-1]
            ], dim=-1),
            dim=-1
        )  # [B, N, S]

        # Combine colors with weights
        rgb = torch.sum(weights.unsqueeze(-1) * colors, dim=2)  # [B, N, 3]

        # Calculate depth
        depth = torch.sum(weights * t_vals, dim=-1)  # [B, N]

        return {
            'rgb': rgb,
            'depth': depth,
            'weights': weights
        }

    def extract_density_grid(self, resolution=128, bounds=(-1.5, 1.5)):
        """
        Extract a 3D density grid for mesh extraction via marching cubes.

        Args:
            resolution (int): Grid resolution per axis
            bounds (tuple): (min, max) bounds for the grid

        Returns:
            numpy.ndarray: 3D density grid [resolution, resolution, resolution]
        """
        device = next(self.parameters()).device
        min_b, max_b = bounds

        # Create 3D grid of query points
        coords = torch.linspace(min_b, max_b, resolution, device=device)
        xx, yy, zz = torch.meshgrid(coords, coords, coords, indexing='ij')
        points = torch.stack([xx, yy, zz], dim=-1).reshape(-1, 3)  # [R^3, 3]

        # Process in chunks to avoid OOM
        chunk_size = 64 * 1024
        densities = []

        with torch.no_grad():
            for i in range(0, points.shape[0], chunk_size):
                chunk = points[i:i+chunk_size]
                encoded = self.positional_encoding(chunk, self.num_encoding_functions)
                density_out = self.density_net(encoded)
                sigma = F.relu(density_out[..., 0])
                densities.append(sigma.cpu())

        density_grid = torch.cat(densities, dim=0).numpy()
        density_grid = density_grid.reshape(resolution, resolution, resolution)

        return density_grid

    def forward(self,
                poses,
                images,
                target_pose,
                target_image=None,
                num_rays=None,
                train=True):
        """
        Forward pass: use input poses and images to render a new view from target_pose.

        Args:
            poses (torch.Tensor): Input camera poses [B, N, 7]
            images (torch.Tensor): Input images [B, N, H, W, 3]
            target_pose (torch.Tensor): Target camera pose [B, 7]
            target_image (torch.Tensor, optional): Target image for loss calculation
            num_rays (int, optional): Number of rays to sample during training
            train (bool): Whether in training mode (use ray sampling)

        Returns:
            dict: Output containing the rendered image, depth, and loss if target_image is provided
        """
        B = poses.shape[0]
        device = poses.device
        H, W = self.image_height, self.image_width

        # Generate rays for target view
        ray_origins, ray_directions = self.get_rays(target_pose)

        # if training and we want to subsample fewer than H*W rays:
        if train and num_rays is not None and num_rays < H * W:
            # flatten H*W -> N = H*W
            ray_origins = ray_origins.view(B, H*W, 3)
            ray_directions = ray_directions.view(B, H*W, 3)

            # sample indices
            idx = torch.randint(0, H*W, (B, num_rays), device=device)  # [B, num_rays]

            # gather
            ray_origins   = torch.gather(ray_origins,   1, idx.unsqueeze(-1).expand(-1, -1, 3))
            ray_directions= torch.gather(ray_directions,1, idx.unsqueeze(-1).expand(-1, -1, 3))

            # remember them so we can index target pixels later
            sel_indices = idx
        else:
            # no subsampling -> use all rays
            ray_origins    = ray_origins.view(B, H*W, 3)
            ray_directions = ray_directions.view(B, H*W, 3)
            sel_indices    = torch.arange(H*W, device=device).unsqueeze(0).expand(B, -1)

        # Render rays
        render_output = self.render_rays(
            ray_origins,
            ray_directions,
            near=2.0,
            far=6.0,
            num_samples=64,
            rand=train
        )

        rendered_rgb = render_output['rgb']

        outputs = {"rgb": rendered_rgb}

        # compute loss & PSNR if we have a target
        if train and target_image is not None:
            # flatten target_image [B, C, H, W] -> [B, H*W, C]
            B_,C,H_,W_ = target_image.shape[0], target_image.shape[1], H, W
            tgt = target_image.view(B_, C, H_*W_).permute(0,2,1)  # [B, H*W, C]

            # pick only the same rays we rendered
            sel_tgt = torch.gather(
                tgt, 1, sel_indices.unsqueeze(-1).expand(-1, -1, C)
            ).reshape(-1, C)  # [B*num_rays, C]

            # compute MSE loss
            loss = F.mse_loss(rendered_rgb.reshape(-1, 3), sel_tgt)
            outputs["loss"] = loss

            # PSNR = -10*log10(MSE)
            psnr = -10 * log10(loss.item() if isinstance(loss, torch.Tensor) else loss)
            outputs["psnr"] = torch.tensor(psnr, device=rendered_rgb.device)

        # Reshape rendered image to HxW format if not in training mode
        if not train:
            outputs["rgb"] = outputs["rgb"].reshape(B, H, W, 3)
            if "depth" in render_output:
                outputs["depth"] = render_output["depth"].reshape(B, H, W)

        return outputs
