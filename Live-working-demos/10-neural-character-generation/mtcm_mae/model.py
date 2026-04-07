import torch
import torch.nn as nn
from torch.utils.checkpoint import checkpoint_sequential

from .blocks import Block
from .heads import ReconstructionHead, SelectionHead, PoseRegressionHead

class MTCM_MAE(nn.Module):
    """
    Multi-Token Context Model with Masked Autoencoder capabilities.
    
    Enhanced to support:
    1. Token reconstruction (MAE-style pretraining)
    2. View selection weights prediction
    3. Pose regression (position + quaternion)
    
    The model can be used for joint training with NeRF supervision.
    """
    def __init__(self, 
                 input_dim=394, 
                 model_dim=128, 
                 depth=2, 
                 heads=8,
                 mlp_ratio=2.0, 
                 drop_path=0.1, 
                 predict_weights=True,
                 predict_poses=True):
        super().__init__()
        
        # Input projection and masking token
        self.input_proj = nn.Linear(input_dim, model_dim)
        self.mask_token = nn.Parameter(torch.zeros(1, 1, model_dim))
        nn.init.trunc_normal_(self.mask_token, std=0.02)
        
        # Transformer blocks
        self.blocks = nn.ModuleList([
            Block(model_dim, heads, model_dim * mlp_ratio, qkv_bias=True, drop_path=drop_path)
            for _ in range(depth)
        ])
        self.norm = nn.LayerNorm(model_dim)
        
        # Prediction heads
        self.recon_head = ReconstructionHead(model_dim, input_dim)
        
        # Optional heads based on configuration
        self.predict_weights = predict_weights
        if predict_weights:
            self.weight_head = SelectionHead(model_dim)
            
        self.predict_poses = predict_poses
        if predict_poses:
            self.pose_head = PoseRegressionHead(model_dim)

    def forward(self, tokens, mask_indices=None):
        """
        Forward pass through the MTCM_MAE model
        
        Args:
            tokens (torch.Tensor): Input tokens [B, N, input_dim]
                B: Batch size
                N: Number of tokens/images
                input_dim: Token dimension (DINOv2 + metadata)
            mask_indices (list of torch.Tensor, optional): Indices of tokens to mask in each batch
        
        Returns:
            dict: Output containing any of:
                - 'reconstructed_tokens': Reconstructed masked tokens
                - 'selection_weights': View selection weights
                - 'pose_predictions': Predicted poses for each token
        """
        B, N, _ = tokens.shape
        x = self.input_proj(tokens)
        
        # Apply masking if specified
        if mask_indices is not None:
            x = x.clone()
            for b in range(B):
                x[b, mask_indices[b]] = self.mask_token
        
        # Apply transformer blocks with gradient checkpointing for memory efficiency
        segments = 2 if len(self.blocks) > 1 else 1
        x = checkpoint_sequential(self.blocks, segments, x)
        x = self.norm(x)
        
        # Prepare outputs
        out = {}
        
        # Token reconstruction (if masking was applied)
        if mask_indices is not None:
            out['reconstructed_tokens'] = torch.stack([
                self.recon_head(x[b, mask_indices[b]]) for b in range(B)
            ])
        
        # View selection weights (if enabled)
        if self.predict_weights:
            out['selection_weights'] = self.weight_head(x).squeeze(-1)
        
        # Pose prediction (if enabled)
        if self.predict_poses:
            raw_poses = self.pose_head(x)
            
            # Normalize quaternions during inference
            if not self.training:
                raw_poses = self.pose_head.normalize_quaternions(raw_poses)
                
            out['pose_predictions'] = raw_poses
        
        return out