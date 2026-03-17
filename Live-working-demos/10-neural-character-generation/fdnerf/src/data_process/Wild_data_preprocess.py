"""
Author: Eckert ZHANG
Date: 2022-02-20 09:47:29
LastEditTime: 2022-03-23 15:46:28
LastEditors: Eckert ZHANG
Description: 
"""
import argparse
import os, sys
import json
import torch
import torchvision.transforms as transforms
import imageio, glob, cv2
import numpy as np

from AlignmentCode.wild_fit_base import fitting
from SegmentCode.model import BiSeNet
from SegmentCode.get_pair_parsing import vis_parsing_maps

# os.environ['CUDA_VISIBLE_DEVICES'] = '0'
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")


def project(lm3d, pose, debug=False, save_path='./', img_name='img_ldm_test'):
    K = np.array([[1200, 0, 256], [0, 1200, 256], [0, 0, 1]])
    # get Rt
    Rt = np.eye(4)
    M = np.array([[0, -1, 0], [1, 0, 0], [0, 0, 1]])
    Rt[:3, :3] = pose[:3, :3].T  # pose[:3,:3] --> RT
    Rt[:3, 3] = -pose[:3, :3].T.dot(pose[:3, 3])  # T = R.dot(C)

    # project 3d to 2d
    lm2d = K @ Rt[:3, :] @ (np.concatenate(
        [lm3d, np.ones([lm3d.shape[0], 1])], 1).T)
    lm2d_half = lm2d // lm2d[2, :]
    lm2d = np.round(lm2d_half).astype(
        np.int64)[:2, :].T @ M[:2, :2]  # .T[:,:2]  #[68,2]

    lm2d[:, 1] = 512 + lm2d[:, 1]
    # print(lm2d.max(), lm2d.min())
    if debug == True:
        img = np.zeros([256, 256, 3])
        lm2d_view = lm2d.astype(np.int64) // 2
        img[lm2d_view[:, 0], lm2d_view[:, 1], :] = np.ones([3])

        imageio.imsave(os.path.join(save_path, f'{img_name}.png'),
                       img.astype(np.int8))
    return lm2d


def main(args,
         folder_name_in='01_images_colmap',
         folder_name_out='images_align',
         folder_name_out_msk='images_masked'):

    # folders = sorted([
    #         x for x in os.listdir(args.datapath)
    #         if os.path.isdir(os.path.join(args.datapath, x))
    #     ])

    print(args.datapath)
    folders = os.listdir(args.datapath)
    # folders = ["m--20180226--0000--6674443--GHS",
    #             "m--20180406--0000--8870559--GHS",
    #             "m--20180426--0000--002643814--GHS",
    #             "m--20180510--0000--5372021--GHS",
    #             "m--20180927--0000--7889059--GHS"]
    # folders = ['id00419', 'id08696', 'id03041', 'id01567', 'id08552', 'id02725', 'id04950', 'id01437', 'id04232', 'id08374', 'id02542', 'id06484', 'id04536', 'id00154', 'id02577', 'id07354', 'id07621', 'id03030', 'id03382', 'id07494', 'id05654', 'id04657', 'id03789', 'id02019', 'id01000', 'id04862', 'id07868', 'id03839', 'id07426', 'id04656', 'id04366', 'id02286', 'id00812', 'id02181', 'id01041', 'id07663', 'id01228', 'id05850', 'id08392', 'id02057', 'id05459', 'id00817', 'id04006', 'id07620', 'id04030', 'id04094', 'id07874', 'id02576', 'id08548', 'id04276', 'id00866', 'id03677', 'id03127', 'id04478', 'id01593', 'id08911', 'id05202', 'id02086', 'id03978', 'id02317', 'id01298', 'id05055', 'id05124', 'id06310', 'id00061', 'id01822', 'id05594', 'id00926', 'id01224', 'id07961', 'id01618', 'id06913', 'id09017', 'id05816', 'id02685', 'id01333', 'id03981', 'id03862', 'id01892', 'id04627', 'id08149', 'id03178', 'id05176', 'id06816', 'id01989', 'id07312', 'id06692', 'id04253', 'id08701', 'id03969', 'id02465', 'id08456', 'id03524', 'id02745', 'id06104', 'id04570', 'id00017', 'id07396', 'id01066', 'id05999', 'id07414', 'id04295', 'id01106', 'id00562', 'id05015', 'id01541', 'id01460', 'id04119', 'id03347', 'id00081', 'id06209', 'id01509', 'id02548']
    print("--- Folders ---")
    print(folders)
    
    size_tar = args.resolution_tar  # 512
    dshape = [size_tar, size_tar, 3]
    fitter = fitting(
        lm_file=
        "./src/data_process/AlignmentCode/shape_predictor_68_face_landmarks.dat"
    )

    # masked config
    n_classes = 19
    model_path = './src/data_process/SegmentCode/Seg_79999_iter.pth'
    net = BiSeNet(n_classes=n_classes)
    net.to(device)
    net.load_state_dict(torch.load(model_path, map_location = device))
    # net.load_state_dict(torch.load(model_path, map_location=torch.device('cpu')))
    net.eval()
    to_tensor = transforms.Compose([
        transforms.ToTensor(),
        transforms.Normalize((0.485, 0.456, 0.406), (0.229, 0.224, 0.225)),
    ])
    CLOTHES_CORLOR = np.array([0, 255, 0]).astype(np.uint8)
    BG_COLOR = np.array([0, 0, 0]).astype(np.uint8)
    color_list = [CLOTHES_CORLOR, BG_COLOR]

    for folder in folders:
        print(f'\n-------- Start to process {folder} ---------')
        root_dir = os.path.join(args.datapath, folder)
        save_dir = os.path.join(args.savepath, folder)
        if not os.path.exists(root_dir):
            print(f'The data path ({root_dir}) is NOT existing!')
            continue
        os.makedirs(save_dir, exist_ok=True)
        exps = sorted([
            x for x in os.listdir(root_dir)
            if os.path.isdir(os.path.join(root_dir, x))
        ])
        print(f"exps: {exps}")
        '''
        ## ========== Pre-checking (To avoid no face cases) ===========
        # img_n_del_angle = []
        # print('***** checking:')
        # for exp in ['1_neutral']:
        #     img_paths = sorted([
        #         x for x in glob.glob(
        #             os.path.join(root_dir, exp, folder_name_in, '*'))
        #         if (x.endswith('.jpg') or x.endswith('.png'))
        #     ])
        #     for ii in range(len(img_paths)):
        #         img = imageio.imread(img_paths[ii])
        #         img_name_origi = img_paths[ii].split('/')[-1]
        #         if img_name_origi in img_n_del_angle:
        #             continue
        #         try:
        #             faces = fitter.detector(img, 1)
        #         except:
        #             img_n_del_angle.append(img_name_origi)
        #             continue
        #         if len(faces) < 1:
        #             img_n_del_angle.append(img_name_origi)
        #             continue
        #         pts = fitter.face_pred(img, faces[0])
        #         kp2d_raw = np.array(([[p.x, p.y] for p in pts.parts()]))
        #         M, scale = fitter.transformation_from_points(
        #             src_points=kp2d_raw, tmpt_points=fitter.tmpLM)
        #         out = fitter.warp_im(img, M, dshape)
        #         try:
        #             faces = fitter.detector(out, 1)
        #         except:
        #             img_n_del_angle.append(img_name_origi)
        #             continue
        #         if len(faces) < 1:
        #             img_n_del_angle.append(img_name_origi)
        #             continue
        # print('***** checking finish!')
        '''

        ## ========== alignment & pose-estimated (based on neural exp) ===========
        n_exp = 1
        valid_ids = []
        for exp in exps:
            # print(f"processing: {exp}")
            # imgs_path = os.path.join(root_dir, exp, folder_name_in)
            imgs_path = os.path.join(root_dir, exp)
            img_save_path = os.path.join(save_dir, exp, folder_name_out)
            os.makedirs(img_save_path, exist_ok=True)
            mask_face_save_path = os.path.join(save_dir, exp, 'masks_face')
            os.makedirs(mask_face_save_path, exist_ok=True)
            parse_save_path = os.path.join(save_dir, exp, 'parsing')
            os.makedirs(parse_save_path, exist_ok=True)
            mask_save_path = os.path.join(save_dir, exp, 'masks')
            os.makedirs(mask_save_path, exist_ok=True)
            img_masked_save_path = os.path.join(save_dir, exp,
                                                folder_name_out_msk)
            os.makedirs(img_masked_save_path, exist_ok=True)
            # print(f"images path: {imgs_path}")
            img_list = sorted([
                x for x in glob.glob(os.path.join(imgs_path, "*"))
                if (x.endswith(".jpg") or x.endswith(".png"))
                # x for x in glob.glob(imgs_path)
                # if (x.endswith(".jpg") or x.endswith(".png"))
            ])
            # print(img_list)
            # print(f"img list length: {len(img_list)}")
            face_poses = []
            num = 0
            for img in img_list:
                print(f"processing img: {img}")
                img_name = img.split('/')[-1]
                img = cv2.imread(img)

                # coarse alignment
                try:
                    kp2d, img_scaled, M1 = fitter.detect_kp2d(
                        img,
                        is_show_img=False,
                        dshape=dshape,
                    )
                except:
                    print(f"key point detection failure")
                    continue

                # pose-estimating (detect?可视化)
                pos, trans = fitter.get_pose_from_kp2d(kp2d)

                # keypoint-tuning
                lm3d_tmplate = fitter.fcFitter.tmpLM.copy()
                debug_path = os.path.join(save_dir, exp, 'ldm_visual')
                os.makedirs(debug_path, exist_ok=True)
                lm2d_tmplate = project(lm3d_tmplate,
                                       pos,
                                       debug=False,
                                       save_path=debug_path,
                                       img_name=img_name)
                try:
                    kp2d, img_scaled, M2 = fitter.detect_kp2d(
                        cv2.cvtColor(img_scaled, cv2.COLOR_RGB2BGR),
                        tar_kp=lm2d_tmplate,
                        is_show_img=False,
                        dshape=dshape,
                    )
                except:
                    continue
                face_poses.append(np.array(pos, dtype=np.float32))
                imageio.imsave(
                    os.path.join(img_save_path,
                                 '%02d_%05d.png' % (n_exp, num)), img_scaled)
                faces = fitter.detector(img_scaled, 1)
                img_noface = np.ones_like(img_scaled) * 255
                if len(faces) == 0:
                    print(f"Warning, no human face detected, skipping {img_name}")
                    continue
                l, t, r, b = faces[0].left(), faces[0].top(), faces[0].right(
                ), faces[0].bottom()
                img_noface[t:b, l:r, :] = 0

                # debug
                if True:
                    img_with_lm = img_scaled.copy()
                    lm2d_view = lm2d_tmplate.astype(np.int64)
                    img_with_lm[lm2d_view[:, 0],
                                lm2d_view[:, 1], :] = np.ones([3])
                    img_with_lm[lm2d_view[:, 0] - 1,
                                lm2d_view[:, 1], :] = np.ones([3])
                    img_with_lm[lm2d_view[:, 0],
                                lm2d_view[:, 1] - 1, :] = np.ones([3])
                    img_with_lm[lm2d_view[:, 0] + 1,
                                lm2d_view[:, 1], :] = np.ones([3])
                    img_with_lm[lm2d_view[:, 0],
                                lm2d_view[:, 1] + 1, :] = np.ones([3])
                    imageio.imsave(
                        os.path.join(debug_path,
                                     '%02d_%05d.png' % (n_exp, num)),
                        img_with_lm)

                ## ========= Parsing & masked =========
                with torch.no_grad():
                    img_tensor = to_tensor(img_scaled)
                    img_tensor = torch.unsqueeze(img_tensor, 0).to(device)
                    out = net(img_tensor)[0]
                parsing = out.squeeze(0).cpu().numpy().argmax(0)
                img_parsing = vis_parsing_maps(dshape[0],
                                               dshape[1],
                                               img_scaled,
                                               parsing,
                                               stride=1,
                                               save_im=False)
                imageio.imsave(
                    os.path.join(parse_save_path,
                                 '%02d_%05d.png' % (n_exp, num)), img_parsing)

                mask = (np.ones_like(img_parsing) * 255).astype(np.uint8)
                mask[450:, ...] = 0
                for color in color_list:
                    index = np.where(np.all(img_parsing == color, axis=-1))
                    mask[index[0], index[1]] = 0
                img_masked = np.bitwise_and(mask, img_scaled)
                imageio.imsave(
                    os.path.join(mask_save_path,
                                 '%02d_%05d.png' % (n_exp, num)), mask)
                imageio.imsave(
                    os.path.join(img_masked_save_path,
                                 '%02d_%05d.png' % (n_exp, num)), img_masked)
                img_noface = np.bitwise_and(mask, img_noface)
                imageio.imsave(
                    os.path.join(mask_face_save_path,
                                 '%02d_%05d.png' % (n_exp, num)), img_noface)
                num += 1
                valid_id = os.path.splitext(img_name)[0]
                valid_ids.append(valid_id)
            n_exp += 1
            face_poses = np.array(np.stack(face_poses))
            pose_save_path = os.path.join(img_masked_save_path,
                                          'poses_face.npy')
            np.save(pose_save_path, face_poses)
            
            valid_ids_path = os.path.join(args.savepath, f"{folder}/{exp}/valid_img_ids.txt")
            os.makedirs(os.path.dirname(valid_ids_path), exist_ok=True)
            with open(valid_ids_path, 'w') as f:
                f.write('\n'.join(valid_ids))
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

            for i, pose in enumerate(face_poses):  
                img_id = valid_ids[i]  
                json_data["frames"].append({
                    "img_id": img_id,
                    "transform_matrix": pose.tolist()
                })

            json_path = os.path.join(args.savepath, f"{folder}/{exp}/face_transforms_pose.json")
            with open(json_path, 'w') as f:
                json.dump(json_data, f, indent=2)

            print(f'Finish exp [{exp}] !')
    
        print('Finish model', folder, '!')


if __name__ == '__main__':
    """
    Extracts and aligns all faces from images, estimate the face pose for each image
    """
    print("--- 程序启动（强制刷新）---", flush=True)  # 确保 SLURM 立即显示
    print("---检查设备---", flush=True)
    print(device, flush=True)
    print("---设备检查完毕---", flush=True)
    
    parser = argparse.ArgumentParser()
    parser.add_argument("--datapath",
                        type=str,
                        default='/home/zhangjingbo/Datasets/NeRF_data/ours')
    parser.add_argument("--savepath",
                        type=str,
                        default='/home/zhangjingbo/Datasets/NeRF_data/ours'
                        )  #/data/zhangjingbo/FaceScape_rendered/wild
    parser.add_argument("--resolution_tar", type=int, default=512)
    args = parser.parse_args()
    
    # 检查路径是否存在
    print(f"check datapath: {os.path.exists(args.datapath)}", flush=True)
    print(f"datapath content: {os.listdir(args.datapath)[:3]}", flush=True)
    
    model_path = './src/data_process/SegmentCode/Seg_79999_iter.pth'
    # 强制加载模型并打印
    try:
        print("--- Loading Model ---", flush=True)
        net = BiSeNet(n_classes=19)
        net.load_state_dict(torch.load(model_path, map_location=device))
        print("--- Loading Model succeed ---", flush=True)
    except Exception as e:
        print(f"Loading Model failed: {str(e)}", flush=True)
        sys.exit(1)

    main(args)