#!/usr/bin/env python3
"""
Export TensorBoard scalar data to CSV and/or PNG graphs.
Usage:
  python export_tensorboard.py runs/FrameSchedulerEnv_ppo_s0
  python export_tensorboard.py runs/FrameSchedulerEnv_ppo_s0 --out export_dir
  python export_tensorboard.py runs/FrameSchedulerEnv_ppo_s0 --plots
  python export_tensorboard.py runs --plots   # all runs under runs/
"""

import argparse
import csv
from pathlib import Path
import sys


def find_event_dirs(logdir: Path):
    """Yield directories that contain TensorBoard event files."""
    logdir = Path(logdir).resolve()
    if not logdir.exists():
        return
    # Single run dir: contains events.*
    if any(logdir.glob("events.out.tfevents.*")):
        yield logdir
        return
    # Parent dir: each subdir is a run
    for sub in sorted(logdir.iterdir()):
        if sub.is_dir() and any(sub.glob("events.out.tfevents.*")):
            yield sub


def export_run(event_dir: Path, out_dir: Path, do_plots: bool):
    try:
        from tensorboard.backend.event_processing.event_accumulator import EventAccumulator
    except ImportError:
        print("Install tensorboard: pip install tensorboard", file=sys.stderr)
        sys.exit(1)

    acc = EventAccumulator(str(event_dir), size_guidance={"scalars": 0})
    acc.Reload()

    tags = acc.Tags().get("scalars") or []
    if not tags:
        print(f"No scalars in {event_dir}")
        return

    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    run_name = event_dir.name

    for tag in tags:
        events = acc.Scalars(tag)
        if not events:
            continue
        # CSV: step, value, wall_time
        safe_name = tag.replace("/", "_")
        csv_path = out_dir / f"{run_name}_{safe_name}.csv"
        with open(csv_path, "w", newline="") as f:
            w = csv.writer(f)
            w.writerow(["step", "value", "wall_time"])
            for e in events:
                w.writerow([e.step, e.value, e.wall_time])
        print(f"  {csv_path.name}")

    if do_plots:
        try:
            import matplotlib
            matplotlib.use("Agg")
            import matplotlib.pyplot as plt
        except ImportError:
            print("  (install matplotlib for --plots: pip install matplotlib)")
            return
        for tag in tags:
            events = acc.Scalars(tag)
            if not events:
                continue
            steps = [e.step for e in events]
            values = [e.value for e in events]
            safe_name = tag.replace("/", "_")
            fig, ax = plt.subplots(figsize=(8, 4))
            ax.plot(steps, values)
            ax.set_xlabel("Step")
            ax.set_ylabel(tag)
            ax.set_title(f"{run_name} — {tag}")
            ax.grid(True, alpha=0.3)
            png_path = out_dir / f"{run_name}_{safe_name}.png"
            fig.savefig(png_path, dpi=150, bbox_inches="tight")
            plt.close()
            print(f"  {png_path.name}")


def main():
    ap = argparse.ArgumentParser(description="Export TensorBoard scalars to CSV and optionally PNG.")
    ap.add_argument("logdir", nargs="?", default="runs", help="TensorBoard logdir (e.g. runs/ or runs/RunName)")
    ap.add_argument("--out", "-o", default="tensorboard_export", help="Output directory for CSV/PNG files")
    ap.add_argument("--plots", "-p", action="store_true", help="Also save PNG graphs (requires matplotlib)")
    args = ap.parse_args()

    logdir = Path(args.logdir)
    out_dir = Path(args.out)
    event_dirs = list(find_event_dirs(logdir))
    if not event_dirs:
        print(f"No TensorBoard event directories found under {logdir}")
        sys.exit(1)

    print(f"Exporting {len(event_dirs)} run(s) to {out_dir}/")
    for ed in event_dirs:
        print(ed.name)
        export_run(ed, out_dir, args.plots)
    print("Done.")


if __name__ == "__main__":
    main()
