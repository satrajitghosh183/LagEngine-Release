import cv2
import os
import argparse
import time
import json

POSE_INSTRUCTIONS = {
    "front": "Look straight at the camera",
    "left": "Turn LEFT and look at the camera",
    "right": "Turn RIGHT and look at the camera",
    "up": "Look UP towards the ceiling",
    "down": "Look DOWN towards the floor"
}

ACTION_INSTRUCTIONS = {
    "idle": "STAND STILL",
    "walk": "WALK IN PLACE",
    "run": "RUN IN PLACE"
}


def create_output_dirs(output_base, subject_id, subfolders):
    for folder in subfolders:
        path = os.path.join(output_base, subject_id, folder)
        os.makedirs(path, exist_ok=True)


def capture_multi_view(subject_id, output_base="data/dataset", poses=None, frames_per_pose=10):
    if poses is None:
        poses = ["front", "left", "right", "up", "down"]

    create_output_dirs(output_base, subject_id, ["poses/" + pose for pose in poses])
    metadata_log = []

    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("Webcam not found!")
        return

    pose_idx = 0
    frame_count = 0

    while pose_idx < len(poses):
        current_pose = poses[pose_idx]
        ret, frame = cap.read()
        if not ret:
            continue

        # Overlay Instructions
        instruction = POSE_INSTRUCTIONS.get(current_pose, "Default pose instruction")
        cv2.putText(frame, f"POSE: {current_pose.upper()}", (50, 60), cv2.FONT_HERSHEY_SIMPLEX, 1.2, (0, 255, 0), 3)
        cv2.putText(frame, instruction, (50, 100), cv2.FONT_HERSHEY_SIMPLEX, 0.9, (0, 255, 255), 2)
        cv2.putText(frame, "Press SPACEBAR to start capture", (50, 450), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (200, 200, 200), 1)
        cv2.imshow("Multi-View Avatar Capture", frame)

        key = cv2.waitKey(1)

        if key == ord(' '):
            # Countdown on-screen
            for i in range(3, 0, -1):
                ret, frame = cap.read()
                cv2.putText(frame, f"{i}", (frame.shape[1]//2 - 20, frame.shape[0]//2), cv2.FONT_HERSHEY_DUPLEX, 5, (0, 0, 255), 10)
                cv2.imshow("Multi-View Avatar Capture", frame)
                cv2.waitKey(1000)

            # Capture frame
            ret, frame = cap.read()
            if not ret:
                continue

            save_dir = os.path.join(output_base, subject_id, "poses", current_pose)
            save_path = os.path.join(save_dir, f"{frame_count:03d}.jpg")
            cv2.imwrite(save_path, frame)

            # Overlay capture feedback
            cv2.putText(frame, "CAPTURED!", (frame.shape[1]//2 - 100, frame.shape[0]//2), cv2.FONT_HERSHEY_SIMPLEX, 2, (0, 255, 0), 5)
            cv2.imshow("Multi-View Avatar Capture", frame)
            cv2.waitKey(500)

            metadata_log.append({
                "subject_id": subject_id,
                "pose": current_pose,
                "frame": frame_count,
                "file_path": save_path,
                "timestamp": time.time()
            })

            frame_count += 1

            if frame_count >= frames_per_pose:
                pose_idx += 1
                frame_count = 0

        if key == ord('q'):
            break

    cap.release()
    cv2.destroyAllWindows()

    meta_path = os.path.join(output_base, subject_id, "capture_metadata.json")
    with open(meta_path, "w") as f:
        json.dump(metadata_log, f, indent=4)

    print("✅ Static pose capture session completed.")


def capture_action_videos(subject_id, output_base="data/dataset", actions=None, duration=10):
    if actions is None:
        actions = ["idle", "walk", "run"]

    create_output_dirs(output_base, subject_id, ["actions"])

    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("Webcam not found!")
        return

    for action in actions:
        print(f"⚠ Ready to record '{action.upper()}' action. Press SPACEBAR to start...")
        waiting = True
        while waiting:
            ret, frame = cap.read()
            instruction = ACTION_INSTRUCTIONS.get(action, "Perform action")
            cv2.putText(frame, f"ACTION: {action.upper()}", (50, 60), cv2.FONT_HERSHEY_SIMPLEX, 1.2, (0, 255, 0), 3)
            cv2.putText(frame, instruction, (50, 100), cv2.FONT_HERSHEY_SIMPLEX, 0.9, (0, 255, 255), 2)
            cv2.putText(frame, "Press SPACEBAR to start recording", (50, 450), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (200, 200, 200), 1)
            cv2.imshow("Action Video Capture", frame)

            key = cv2.waitKey(1)
            if key == ord(' '):
                waiting = False

        # Setup Video Writer
        save_path = os.path.join(output_base, subject_id, "actions", f"{action}.mp4")
        fourcc = cv2.VideoWriter_fourcc(*'mp4v')
        fps = 30
        height, width = frame.shape[:2]
        out = cv2.VideoWriter(save_path, fourcc, fps, (width, height))

        print(f"🎥 Recording '{action}' for {duration} seconds...")
        start_time = time.time()
        while time.time() - start_time < duration:
            ret, frame = cap.read()
            if not ret:
                break
            out.write(frame)
            cv2.putText(frame, f"Recording {action.upper()}...", (50, 50), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 255), 3)
            cv2.imshow("Action Video Capture", frame)
            cv2.waitKey(1)

        out.release()
        print(f"✅ Saved video: {save_path}")

    cap.release()
    cv2.destroyAllWindows()
    print("✅ Action video capture session completed.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Dual-mode capture script")
    parser.add_argument('--subject_id', type=str, required=True, help="Subject ID (e.g., subject_01)")
    parser.add_argument('--mode', type=str, choices=['poses', 'actions'], required=True, help="'poses' for static images, 'actions' for videos")
    parser.add_argument('--frames', type=int, default=5, help="Frames per pose (pose mode only)")
    args = parser.parse_args()

    if args.mode == 'poses':
        capture_multi_view(subject_id=args.subject_id, frames_per_pose=args.frames)
    elif args.mode == 'actions':
        capture_action_videos(subject_id=args.subject_id)
