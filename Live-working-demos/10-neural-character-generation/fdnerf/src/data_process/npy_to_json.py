import json


focal_len = 1200.0
cx, cy = 256.0, 256.0
near, far = 0.1, 100.0  

json_data = {
    "focal_len": focal_len,
    "cx": cx,
    "cy": cy,
    "near": near,
    "far": far,
    "frames": []
}

# for i, pose in enumerate(face_poses):  
#     img_id = valid_ids[i]  
#     json_data["frames"].append({
#         "img_id": img_id,
#         "transform_matrix": pose.tolist()
#     })

# json_path = os.path.join(args.savepath, "face_transforms_pose.json")
# with open(json_path, 'w') as f:
#     json.dump(json_data, f, indent=2)