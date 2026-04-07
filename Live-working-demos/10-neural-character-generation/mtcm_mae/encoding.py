import torch
import torch.nn as nn
import math


class PositionalEncoding(nn.Module):
    """
    Sinusoidal positional encoding that adds sequence position information.
    
    This helps the transformer model understand the ordering of views in the sequence.
    """
    def __init__(self, d_model: int, dropout: float = 0.1, max_len: int = 64):
        super().__init__()
        self.dropout = nn.Dropout(p=dropout)
        
        # Create position encoding buffer
        position = torch.arange(max_len).unsqueeze(1)
        div_term = torch.exp(torch.arange(0, d_model, 2) * (-math.log(10000.0) / d_model))
        pe = torch.zeros(max_len, d_model)
        pe[:, 0::2] = torch.sin(position * div_term)
        pe[:, 1::2] = torch.cos(position * div_term)
        self.register_buffer('pe', pe)
        
    def forward(self, x: torch.Tensor) -> torch.Tensor:
        """
        Args:
            x: Tensor of shape [batch_size, seq_len, embedding_dim]
        Returns:
            Tensor with positional encoding added
        """
        x = x + self.pe[:x.size(1)]
        return self.dropout(x)


class ViewDirectionalEncoding(nn.Module):
    """
    Encodes camera pose and focal information into the token embeddings.
    
    This helps the model understand spatial relationships between different views
    based on their 3D positions and orientations.
    """
    def __init__(self, d_model: int, encoding_dim: int = 32):
        super().__init__()
        self.d_model = d_model
        self.encoding_dim = encoding_dim
        
        # Network to encode pose and focal information
        self.pose_encoder = nn.Sequential(
            nn.Linear(9, encoding_dim),  # 7D pose + 2D focal
            nn.LayerNorm(encoding_dim),
            nn.ReLU(),
            nn.Linear(encoding_dim, encoding_dim),
            nn.LayerNorm(encoding_dim),
            nn.ReLU(),
        )
        
        # Projection to model dimension
        self.proj = nn.Linear(encoding_dim, d_model)
        
    def forward(self, x: torch.Tensor, pose_focal: torch.Tensor) -> torch.Tensor:
        """
        Args:
            x: Tensor of shape [batch_size, seq_len, embedding_dim]
            pose_focal: Tensor of shape [batch_size, seq_len, 9] (7D pose + 2D focal)
        Returns:
            Tensor with directional encoding added
        """
        # Encode pose and focal information
        pose_encoding = self.pose_encoder(pose_focal)
        pose_embedding = self.proj(pose_encoding)
        
        # Add to the input embedding
        return x + pose_embedding


def extract_pose_focal(tokens):
    """
    Extract pose and focal information from tokens.
    
    Args:
        tokens: Tensor of shape [batch_size, seq_len, token_dim]
    Returns:
        Tensor of shape [batch_size, seq_len, 9] with pose (7D) and focal (2D)
    """
    # Extract pose (7D) and focal (2D)
    pose = tokens[..., 384:391]
    focal = tokens[..., 391:393]
    return torch.cat([pose, focal], dim=-1)