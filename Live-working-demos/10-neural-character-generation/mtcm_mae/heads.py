import torch
import torch.nn as nn

class ReconstructionHead(nn.Module):
    """
    Head module for token reconstruction in MAE-style pretraining
    """
    def __init__(self, model_dim, output_dim):
        super().__init__()
        self.head = nn.Linear(model_dim, output_dim)
        
        # Initialize with small weights for better convergence
        nn.init.xavier_uniform_(self.head.weight, gain=0.01)
        nn.init.zeros_(self.head.bias)
    
    def forward(self, x):
        """
        Forward pass through the reconstruction head
        
        Args:
            x (torch.Tensor): Input features [B*M, model_dim]
                B: Batch size
                M: Number of masked tokens
                model_dim: Feature dimension
                
        Returns:
            torch.Tensor: Reconstructed tokens [B*M, output_dim]
        """
        return self.head(x)

class SelectionHead(nn.Module):
    """
    Head module for predicting view selection scores
    """
    def __init__(self, model_dim):
        super().__init__()
        self.head = nn.Sequential(
            nn.Linear(model_dim, model_dim // 2),
            nn.ReLU(),
            nn.Linear(model_dim // 2, 1)
        )
    
    def forward(self, x):
        """
        Forward pass through the selection head
        
        Args:
            x (torch.Tensor): Input features [B, N, model_dim]
                B: Batch size
                N: Number of tokens
                model_dim: Feature dimension
                
        Returns:
            torch.Tensor: Selection scores [B, N, 1]
        """
        return self.head(x)

class PoseRegressionHead(nn.Module):
    """
    Head module for predicting 7-dimensional pose vectors:
    3D position (x, y, z) and quaternion orientation (qx, qy, qz, qw)
    """
    def __init__(self, model_dim):
        super().__init__()
        self.head = nn.Linear(model_dim, 7)  # (x, y, z, qx, qy, qz, qw)
        
        # Initialize with small weights for better convergence
        nn.init.xavier_uniform_(self.head.weight, gain=0.01)
        nn.init.zeros_(self.head.bias)
    
    def forward(self, x):
        """
        Forward pass through the pose regression head
        
        Args:
            x (torch.Tensor): Input features [B, N, model_dim]
                B: Batch size
                N: Number of tokens
                model_dim: Feature dimension
                
        Returns:
            torch.Tensor: Predicted poses [B, N, 7]
        """
        return self.head(x)
    
    def normalize_quaternions(self, poses):
        """
        Normalize the quaternion part of the pose vectors
        
        Args:
            poses (torch.Tensor): Raw pose predictions [B, N, 7]
            
        Returns:
            torch.Tensor: Poses with normalized quaternions [B, N, 7]
        """
        positions = poses[..., :3]
        quats = poses[..., 3:]
        
        # Normalize quaternions to unit length
        quats_norm = torch.nn.functional.normalize(quats, dim=-1)
        
        # Concatenate position and normalized quaternion
        return torch.cat([positions, quats_norm], dim=-1)