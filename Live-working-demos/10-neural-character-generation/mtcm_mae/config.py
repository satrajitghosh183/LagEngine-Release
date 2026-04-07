class MTCMConfig:
    """Configuration class for the MTCM MAE model"""
    
    def __init__(
        self,
        input_dim=394,
        model_dim=128,
        mlp_dim=256,
        depth=2,
        heads=8,
        drop_path=0.1,
        mask_ratio=0.3,
        learning_rate=1e-4,
        weight_decay=0.05,
        batch_size=1,
        max_seq_len=64,
        predict_weights=True,
        predict_poses=True
    ):
        self.input_dim = input_dim
        self.model_dim = model_dim
        self.mlp_dim = mlp_dim
        self.depth = depth
        self.heads = heads
        self.drop_path = drop_path
        self.mask_ratio = mask_ratio
        self.learning_rate = learning_rate
        self.weight_decay = weight_decay
        self.batch_size = batch_size
        self.max_seq_len = max_seq_len
        self.predict_weights = predict_weights
        self.predict_poses = predict_poses

    def __str__(self):
        return f"""MTCMConfig:
  - input_dim: {self.input_dim}
  - model_dim: {self.model_dim}
  - mlp_dim: {self.mlp_dim}
  - depth: {self.depth}
  - heads: {self.heads}
  - drop_path: {self.drop_path}
  - mask_ratio: {self.mask_ratio}
  - learning_rate: {self.learning_rate}
  - weight_decay: {self.weight_decay}
  - batch_size: {self.batch_size}
  - max_seq_len: {self.max_seq_len}
  - predict_weights: {self.predict_weights}
  - predict_poses: {self.predict_poses}
"""
