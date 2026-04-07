# # mtcm_mae/utils.py
# import logging

# def setup_logger(name="mtcm"):
#     logging.basicConfig(level=logging.INFO)
#     return logging.getLogger(name)

# def check_tensor_shape(tensor, expected_shape, name="Tensor"):
#     if tensor.shape != expected_shape:
#         raise ValueError(f"[{name}] Expected shape {expected_shape}, got {tensor.shape}")
# mtcm_mae/utils.py
import logging

def setup_logger(name="mtcm"):
    logging.basicConfig(level=logging.INFO)
    return logging.getLogger(name)

def check_tensor_shape(tensor, expected_shape, name="Tensor"):
    if tensor.shape != expected_shape:
        raise ValueError(f"[{name}] Expected shape {expected_shape}, got {tensor.shape}")