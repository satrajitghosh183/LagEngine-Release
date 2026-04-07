import os
import json
import random
from PIL import Image, ExifTags
from tqdm import tqdm

# Configuration
RAW_ROOT = "data2/raw_images"
EXIF_ROOT = "data2/metadata"

# Ensure EXIF output directory exists
os.makedirs(EXIF_ROOT, exist_ok=True)

# Reverse EXIF tag dictionary
EXIF_TAGS = {v: k for k, v in ExifTags.TAGS.items()}

# Sample real camera models for synthetic fallback
CAMERA_MODELS = [
    "Sony A7R V", "Canon EOS R5", "Nikon Z7II", "Leica M11",
    "Panasonic Lumix DC-S1R", "Nikon D850", "Sony A1", "Fujifilm GFX 100S",
    "Pentax 645Z", "Canon EOS R3", "Sony A7 IV"
]

# Random focal length range (mm)
FOCAL_RANGE = (28.0, 135.0)

# Default orientation
DEFAULT_ORIENTATION = "Horizontal (normal)"  # EXIF tag 1

def extract_metadata(image_path):
    metadata = {}
    try:
        with Image.open(image_path) as img:
            exif_data = img._getexif()
            if exif_data:
                for tag_id, value in exif_data.items():
                    tag = EXIF_TAGS.get(tag_id, tag_id)
                    metadata[tag] = value

            # Extract size directly
            width, height = img.size
            metadata["width"] = width
            metadata["height"] = height
            metadata["format"] = img.format

    except Exception as e:
        print(f"⚠️ Error reading EXIF for {image_path}: {e}")
        return None

    # Normalize and synthesize fields
    normalized = {
        "timestamp": metadata.get("DateTimeOriginal", None),
        "focal": float(metadata["FocalLength"]) if "FocalLength" in metadata else round(random.uniform(*FOCAL_RANGE), 2),
        "camera_model": metadata.get("Model", random.choice(CAMERA_MODELS)),
        "orientation": metadata.get("Orientation", DEFAULT_ORIENTATION),
        "width": metadata.get("width", None),
        "height": metadata.get("height", None)
    }

    return normalized


def process_identity(identity):
    print(f"📸 Processing identity: {identity}")
    identity_dir = os.path.join(RAW_ROOT, identity)
    exif_output_dir = os.path.join(EXIF_ROOT, identity)
    os.makedirs(exif_output_dir, exist_ok=True)

    output_path = os.path.join(exif_output_dir, "exif.json")
    metadata_dict = {}

    for fname in tqdm(sorted(os.listdir(identity_dir)), desc=f"→ {identity}", ncols=70):
        if not fname.lower().endswith(".jpg"):
            continue

        img_path = os.path.join(identity_dir, fname)
        exif_data = extract_metadata(img_path)

        if exif_data:
            metadata_dict[fname] = exif_data

    with open(output_path, "w") as f:
        json.dump(metadata_dict, f, indent=2)

    print(f"✅ Saved EXIF metadata to {output_path}\n")


def main():
    all_identities = sorted(os.listdir(RAW_ROOT))
    for identity in all_identities:
        identity_path = os.path.join(RAW_ROOT, identity)
        if os.path.isdir(identity_path):
            process_identity(identity)


if __name__ == "__main__":
    main()
