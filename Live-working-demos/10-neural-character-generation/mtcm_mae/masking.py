# # mtcm_mae/masking.py
# import torch

# def random_mask_indices(batch_size, seq_len, mask_ratio, device='cpu'):
#     num_mask = int(seq_len * mask_ratio)
#     mask_indices = torch.zeros((batch_size, num_mask), dtype=torch.long, device=device)

#     for b in range(batch_size):
#         perm = torch.randperm(seq_len, device=device)
#         mask_indices[b] = perm[:num_mask]

#     return mask_indices
# mtcm_mae/masking.py
import torch

def random_mask_indices(batch_size, seq_len, mask_ratio, device='cpu'):
    num_mask = int(seq_len * mask_ratio)
    mask_indices = torch.zeros((batch_size, num_mask), dtype=torch.long, device=device)

    for b in range(batch_size):
        perm = torch.randperm(seq_len, device=device)
        mask_indices[b] = perm[:num_mask]

    return mask_indices