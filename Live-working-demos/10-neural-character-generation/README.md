

# Neural Character Generation

We propose a unified pipeline for generating photorealistic, animatable 3D avatars from unstructured image collections without known camera intrinsics or dense viewpoint coverage. Our system is built around a tight integration of three core components:

* **NeRFtrinsic-inspired pose refinement**, using Gaussian Fourier Features and a pose-focal MLP supervised by a NeRF rendering loss.
* **Transformer-based Multi-Token Context Model (MTCM)** for optimal view selection, trained with TinyNeRF supervision.
* **TinyNeRF with attention-based refinement**, consuming selected views to produce high-fidelity NeRF reconstructions.

The final outputs support mesh extraction, relighting, and NeRF-based texture synthesis for downstream animation and deployment.

---

## 🧠 Core Pipeline Overview

1. **Pose Refinement**: Initial coarse poses from MediaPipe are refined using `nerftrinsic_four`, which learns camera extrinsics and intrinsics via NeRF supervision and Gaussian Fourier Features.
2. **Multimodal Token Creation**: DINOv2 visual embeddings are fused with refined poses, focal lengths, segmentation mask areas, and resolution into 394D multimodal tokens.
3. **View Selection**: The MTCM transformer selects the most informative views using a learned selection mechanism optimized via TinyNeRF rendering loss.
4. **NeRF Rendering**: Selected views are fed into a modified NeRF with attention-based refinement for novel view synthesis and volumetric rendering.

---

## 📁 Project Structure

```plaintext
.
├── nerftrinsic_four/          # Forked and modified from NeRFtrinsic-Four (pose + focal learning)
├── mtcm_mae/                  # MTCM transformer for view selection and pose prediction
├── nerf/                      # TinyNeRF and weighted variants with attention refinement
├── fdnerf/                    # FDNeRF for few shot avatar reconstruction. Deployment guide within sub-directory.
├── scripts/                   # App runner (includes app.py web interface)
├── dataset_joint_mtcm_nerf.py
├── train_joint_mtcm_nerf.py
├── training_utils.py
├── environment.yml            # Conda environment with pinned versions
├── requirements.txt           # Additional pip packages
└── README.md
```

---

## 🔧 Setup Instructions

### Step 1: Clone This Repo

```bash
git clone git@github.com:satrajitghosh183/NeuralCharacterGeneration.git
cd NeuralCharacterGeneration
```

### Step 2: Set Up Conda Environment

```bash
conda env create -f environment.yml
conda activate neural-character-gen
```

(Optional) Also install pip packages:

```bash
pip install -r requirements.txt
```

---

## 📂 Dataset

We recommend the [Celebrity Face Dataset](https://www.kaggle.com/datasets/vishesh1412/celebrity-face-image-dataset/data).

### Step-by-Step Processing:

1. **Run LLFF preprocessing**:

   ```bash
   python nerftrinsic_four/scripts/generate_llff.py --input_dir /path/to/raw/images
   ```

2. **Train NeRFtrinsic module**:

   ```bash
   python nerftrinsic_four/tasks/train_gf.py --data_dir /path/to/llff_output
   ```

3. **Extract poses and focal lengths**:

   ```bash
   python nerftrinsic_four/scripts/extract_nerftrinsic_poses.py --output_dir nerftrinsic_outputs/
   ```

4. **Preprocess for MTCM-NeRF** using the scripts shown in the screenshots (image embeddings, mask generation, pose alignment, etc.).

---

## 🏋️ Training

### Run joint training (MTCM + NeRF):

```bash
python train_joint_mtcm_nerf.py \
  --data-dir path/to/preprocessed/data \
  --output-dir experiments/ \
  --batch-size 8 \
  --num-epochs 50 \
  --num-selected-views 5 \
  --debug
```

This script:

* Loads multimodal tokens (DINOv2 + pose + focal + mask area + resolution)
* Predicts optimal views and poses via transformer
* Supervises with NeRF rendering loss for consistent reconstructions

---

## 🌐 Web Interface

To launch the demo or visual debugging app:

```bash
python scripts/app.py
```

---

## 📝 Notes

* Most of the code in `nerftrinsic_four/` is adapted from the original [NeRFtrinsic Four](https://github.com/facebookresearch/NeRFtrinsic-Four) repository, with minor modifications to accept our data pipeline and produce pose+focal outputs compatible with token construction.
* FDNeRF is used as architectural reference for attention-based refinement in the NeRF stage, although it is not fully integrated as-is.
![image](https://github.com/user-attachments/assets/fa3adfa2-8bbd-4e49-b502-9204c78140a8)

![image](https://github.com/user-attachments/assets/b5fcbbda-a3db-4715-a1a1-de8c90f6f96a)

![image](https://github.com/user-attachments/assets/061ffa31-4b25-4450-8dd3-8c4d89040036)
![image](https://github.com/user-attachments/assets/4f8c76c5-bfe4-4414-82c8-c2f3dfdf2e98)
![image](https://github.com/user-attachments/assets/acdad0c0-5a33-4a7e-bb91-e20d70358441)
![image](https://github.com/user-attachments/assets/29f288a5-123b-4bae-aac6-4007857ab2e2)
![image](https://github.com/user-attachments/assets/380fc862-4e16-43db-9d84-b8f6fbd77e70)
![image](https://github.com/user-attachments/assets/43881b2b-b1ad-4410-bc50-659d44e1c3d8)
