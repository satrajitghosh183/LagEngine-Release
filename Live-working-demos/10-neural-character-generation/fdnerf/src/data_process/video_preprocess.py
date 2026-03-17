import os
import subprocess
from glob import glob

root_dir = "/scratch/network/hy4522/FDNeRF_data/mp4"
target_dir = "/scratch/network/hy4522/FDNeRF_data/mp4/processed"
max_frames = 128

for id_dir in glob(os.path.join(root_dir, "id*")):
    print(id_dir)
    id_name = os.path.basename(id_dir)
    
    for sub_dir in glob(os.path.join(id_dir, "*")):
        if not os.path.isdir(sub_dir):
            continue
        video_files = sorted(glob(os.path.join(sub_dir, "*.mp4")))
        if not video_files:
            continue
        
        selected_video = video_files[0]
        # selected_video = random.choice(video_files) 
        exp_name = os.path.basename(sub_dir)
        save_dir = os.path.join(target_dir, id_name, exp_name, "01_images_colmap")
        
        if os.path.exists(save_dir):
            print(f"Skipping existing: {save_dir}")
            continue
        os.makedirs(save_dir, exist_ok=True)
        
        cmd = [
            "ffmpeg",
            "-i", selected_video,
            "-frames:v", str(max_frames), 
            "-f", "image2",              
            os.path.join(save_dir, "frame_%05d.png").replace("\\", "/")
        ]
        
        subprocess.run(cmd)
        print(f"Processed: {selected_video} → {save_dir}")