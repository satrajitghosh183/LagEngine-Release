#!/usr/bin/env python3
"""
Minimal test script
"""

print("Starting test...")

# Basic imports
import os
import sys
import torch
print("Basic imports OK")

# Test mtcm_mae imports one by one
print("Testing mtcm_mae imports...")
from mtcm_mae import model
print("mtcm_mae.model imported")
from mtcm_mae.model import MTCM_MAE
print("MTCM_MAE imported")
from mtcm_mae.config import MTCMConfig
print("MTCMConfig imported")

# Test nerf imports one by one
print("Testing nerf imports...")
import nerf
print("nerf package imported")
from nerf import tiny_nerf
print("nerf.tiny_nerf imported")
from nerf.tiny_nerf import TinyNeRF
print("TinyNeRF imported")
from nerf.weighted_tiny_nerf import WeightedTinyNeRF
print("WeightedTinyNeRF imported")
from nerf.nerf_config import NeRFConfig, WeightedNeRFConfig, JointTrainingConfig
print("nerf_config classes imported")

# Test other imports
print("Testing utility imports...")
from dataset_joint_mtcm_nerf import JointMTCMNeRFDataset, ViewSelectionDataModule
print("dataset imports OK")
import data_visualization
print("data_visualization imported")
import training_utils
print("training_utils imported")

print("All imports successful!")