import os
import torch
import numpy as np
import imageio
from tqdm import tqdm
from pathlib import Path
import argparse

from utils.pos_enc import encode_position
from utils.volume_op import volume_sampling_ndc, volume_rendering
from utils.comp_ray_dir import comp_ray_dir_cam_fxfy
from models.nerf_models import OfficialNerf
from models.intrinsics import LearnFocalCamDependent
from models.poses import LearnPoseGF
from dataloader.with_colmap_numnorm import DataloaderParameterLearning


def load_model_and_data(base_dir, scene_name, ckpt_dir, device):
    """Load model weights & data."""
    # data root expects a LLFF/<scene_name> subtree
    scene_root = os.path.join(base_dir, "LLFF")
    if not os.path.isdir(scene_root):
        raise FileNotFoundError(f"`{scene_root}` not found—did you run generate_llff_format?")
    scene_path = os.path.join(scene_root, scene_name)

    # build the DataLoader
    scene = DataloaderParameterLearning(
        base_dir=base_dir,
        scene_name=os.path.join("LLFF", scene_name),
        data_type="train",
        res_ratio=4,
        num_img_to_load=-1,
        skip=1,
        use_ndc=True
    )

    # instantiate networks
    pos_enc_dim = (2 * 10 + 1) * 3
    dir_enc_dim = (2 * 4 + 1) * 3
   
    model = OfficialNerf(pos_enc_dim, dir_enc_dim, 128).to(device)

    focal = LearnFocalCamDependent(len(scene.HWFocal), req_grad=True, fx_only=True, order=2).to(device)
    pose  = LearnPoseGF(scene.N_imgs, init_c2w=None, embedding_scale=10).to(device)

    # load each checkpoint
    def _load(net, fname):
        path = os.path.join(ckpt_dir, fname)
        ckpt = torch.load(path, map_location=device)
        if "model_state_dict" in ckpt:
            net.load_state_dict(ckpt["model_state_dict"])
        else:
            # fallback if someone saved raw state_dict()
            net.load_state_dict(ckpt)
        net.eval()

    _load(model,      "latest_nerf.pth")
    _load(focal,      "latest_focal.pth")
    _load(pose,       "latest_pose.pth")

    return model, focal, pose, scene


def render_image(model, focal, pose, scene, device, idx):
    """Render one frame by index."""
    H, W = scene.imgs[idx].shape[:2]
    img = scene.imgs[idx].to(device)

    # get focal
    fidx = scene.HWFocal[f"{H}{W}"][0]
    fxfy = focal(fidx, H, W)

    # rays
    rays_cam = comp_ray_dir_cam_fxfy(H, W, fxfy[0], fxfy[1])
    c2w = pose(idx)

    # sample depths & pts
    t_vals = torch.linspace(scene.near, scene.far, scene.num_sample if hasattr(scene, "num_sample") else 128, device=device)
    sample_pos, _, ray_dir_world, t_noisy = volume_sampling_ndc(
        c2w, rays_cam, t_vals, scene.near, scene.far, H, W, fxfy, perturb_t=False
    )

    # encode
    pe = encode_position(sample_pos, levels=10, inc_input=True)
    dir_norm = torch.nn.functional.normalize(ray_dir_world, p=2, dim=2)
    de = encode_position(dir_norm, levels=4, inc_input=True).unsqueeze(2).expand(-1, -1, t_vals.shape[0], -1)

    # forward & volume render
    rgbden = model(pe, de)
    out = volume_rendering(rgbden, t_noisy, sigma_noise_std=0.0, rgb_act_fn=torch.sigmoid)
    rgb   = (out["rgb"].cpu().numpy() * 255).astype(np.uint8)

    depth_map = out.get("depth_map")
    depth_png = None
    if depth_map is not None:
        d = depth_map.cpu().numpy()
        dn = (d - d.min()) / (d.max() - d.min() + 1e-8)
        depth_png = (dn * 255).astype(np.uint8)

    return rgb, depth_png


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--base_dir",  required=True, help="project/data root (contains LLFF/)")
    p.add_argument("--scene_name", required=True, help="subfolder under LLFF/")
    p.add_argument("--ckpt_dir",  required=True, help="where latest_*.pth live")
    p.add_argument("--output_dir", default="renders", help="where to dump pngs")
    p.add_argument("--render_depth", action="store_true")
    args = p.parse_args()

    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    print("Using device:", device)

    # load
    try:
        model, focal, pose, scene = load_model_and_data(
            args.base_dir, args.scene_name, args.ckpt_dir, device
        )
    except Exception as e:
        print("Failed to load:", e)
        return

    print(f"Loaded scene with {scene.N_imgs} frames.")

    # prepare outputs
    odir_rgb = Path(args.output_dir) / args.scene_name / "rgb"
    odir_rgb.mkdir(parents=True, exist_ok=True)
    odir_d  = Path(args.output_dir) / args.scene_name / "depth" if args.render_depth else None
    if odir_d: odir_d.mkdir(parents=True, exist_ok=True)

    # loop
    for i in tqdm(range(scene.N_imgs), desc="Rendering"):
        try:
            rgb, depth = render_image(model, focal, pose, scene, device, i)
            imageio.imwrite(str(odir_rgb / f"{i:03d}.png"), rgb)
            if args.render_depth and depth is not None:
                imageio.imwrite(str(odir_d  / f"{i:03d}.png"), depth)
        except Exception as ex:
            print(f"❌ frame {i} failed:", ex)

    print("✅ Done. renders in", args.output_dir)


if __name__ == "__main__":
    main()