#!/usr/bin/env python3
"""
RL_Backend full benchmark suite.
Runs: ctest (unit tests), jobbench (job system at various workers/depth/breadth),
      optional Python env workload at different cube/body sizes.
Writes: benchmark_results_<timestamp>.txt with all details.

Usage:
  python scripts/run_benchmarks.py [--build-dir DIR] [--no-env] [--out FILE]
  From RL_Backend root or GameEngine root (script finds RL_Backend).
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
from datetime import datetime
from pathlib import Path


def find_rl_demo_root() -> Path:
    script = Path(__file__).resolve()
    if "RL_Backend" in script.parts:
        idx = script.parts.index("RL_Backend")
        return Path(*script.parts[: idx + 1])
    # Assume cwd is RL_Backend or GameEngine
    cwd = Path.cwd()
    if (cwd / "RL_Backend").is_dir():
        return cwd / "RL_Backend"
    if (cwd / "scripts" / "run_benchmarks.py").exists():
        return cwd
    return cwd


def find_build_dir(rl_demo: Path, build_dir_arg: str | None) -> Path | None:
    if build_dir_arg:
        d = Path(build_dir_arg)
        if d.is_absolute():
            return d if d.is_dir() else None
        return (rl_demo / d).resolve() if (rl_demo / d).is_dir() else None
    # Default: build next to RL_Backend
    for name in ["build", "out/build", "cmake-build-release", "cmake-build-debug"]:
        d = rl_demo / name
        if d.is_dir():
            return d.resolve()
    return None


def run_cmd(cmd: list[str], cwd: Path, env: dict | None = None, timeout: int = 300) -> tuple[int, str, str]:
    env = env or {}
    full_env = {**os.environ, **env}
    try:
        r = subprocess.run(
            cmd,
            cwd=cwd,
            capture_output=True,
            text=True,
            timeout=timeout,
            env=full_env,
        )
        return r.returncode, r.stdout or "", r.stderr or ""
    except subprocess.TimeoutExpired:
        return -1, "", "Timeout"
    except FileNotFoundError:
        return -1, "", f"Command not found: {cmd[0]}"
    except Exception as e:
        return -1, "", str(e)


def run_ctest(build_dir: Path) -> tuple[int, str]:
    ctest = "ctest"
    if sys.platform == "win32":
        ctest_exe = build_dir / "bin" / "ctest.exe"
        if not ctest_exe.exists():
            ctest_exe = build_dir / "ctest.exe"
        if ctest_exe.exists():
            ctest = str(ctest_exe)
    code, out, err = run_cmd(
        [ctest, "--test-dir", str(build_dir), "--output-on-failure", "-V"],
        cwd=build_dir,
        timeout=120,
    )
    return code, (out + "\n" + err).strip()


def _get_jobbench_exe(build_dir: Path) -> str | None:
    # Check multiple possible locations for jobbench executable
    candidates = []
    
    if sys.platform == "win32":
        # Windows: check Release/Debug/bin subdirs
        for sub in ["Release", "Debug", ""]:
            p = (build_dir / sub / "bin" / "jobbench.exe") if sub else (build_dir / "bin" / "jobbench.exe")
            candidates.append(p)
    else:
        # Linux/WSL/macOS: check common locations
        candidates = [
            build_dir / "bin" / "jobbench",
            build_dir / "demos" / "jobbench" / "jobbench",
            build_dir / "jobbench",
        ]
    
    for p in candidates:
        if p.exists():
            return str(p)
    
    # Debug: print what we searched
    print(f"  [DEBUG] jobbench not found in: {[str(c) for c in candidates]}", file=sys.stderr)
    return None


def run_jobbench(
    build_dir: Path,
    workers_list: list[int],
    depth_breadth_pairs: list[tuple[int, int]],
    machine: bool = True,
    timeout: int = 600,
) -> tuple[int, str]:
    exe = _get_jobbench_exe(build_dir)
    if not exe or not Path(exe).exists():
        return -1, "jobbench not found. Build the project first (demos/jobbench)."

    workers_str = ",".join(map(str, workers_list))
    cmd = [exe, "--workers", workers_str, "--machine"] if machine else [exe]
    if depth_breadth_pairs:
        depths = ",".join(str(d) for d in set(p[0] for p in depth_breadth_pairs))
        breadths = ",".join(str(b) for b in set(p[1] for p in depth_breadth_pairs))
        cmd.extend(["--depth", depths, "--breadth", breadths])
    code, out, err = run_cmd(cmd, cwd=build_dir, timeout=timeout)
    return code, (out + "\n" + err).strip()


def run_jobbench_per_worker(
    build_dir: Path,
    workers_list: list[int],
    depth_breadth_pairs: list[tuple[int, int]],
    timeout_total: int = 600,
) -> tuple[int, str]:
    """Run jobbench once per worker count so we get partial results if one config hangs."""
    exe = _get_jobbench_exe(build_dir)
    if not exe or not Path(exe).exists():
        return -1, "jobbench not found. Build the project first (demos/jobbench)."

    per_run = max(60, timeout_total // max(len(workers_list), 1))
    all_out = []
    any_ok = False
    for w in workers_list:
        cmd = [exe, "--workers", str(w), "--machine"]
        if depth_breadth_pairs:
            depths = ",".join(str(d) for d in set(p[0] for p in depth_breadth_pairs))
            breadths = ",".join(str(b) for b in set(p[1] for p in depth_breadth_pairs))
            cmd.extend(["--depth", depths, "--breadth", breadths])
        code, out, err = run_cmd(cmd, cwd=build_dir, timeout=per_run)
        all_out.append(f"[workers={w} timeout={per_run}s exit={code}]")
        all_out.append(out if out else "(no output)")
        if err and err != "Timeout":
            all_out.append(err)
        if code == 0:
            any_ok = True
    return 0 if any_ok else -1, "\n".join(all_out)


def _find_rldemo_env_path(rl_demo: Path, build_dir: Path) -> list[Path]:
    """Directories to search for rldemo_env.*.so (dirs with .so first so import finds it)."""
    candidates = [
        rl_demo / "rl",
        build_dir / "bindings",
        build_dir / "lib",
        build_dir,
        rl_demo / "build" / "lib",
        rl_demo / "build" / "bindings",
    ]
    with_so = []
    rest = []
    for d in candidates:
        if not d.is_dir():
            continue
        try:
            # Check for .so (Linux), .pyd (Windows), .dylib (macOS)
            has_lib = (
                any(d.glob("rldemo_env*.so")) or 
                any(d.glob("rldemo_env*.pyd")) or
                any(d.glob("*rldemo_env*.dylib"))
            )
            if has_lib:
                with_so.append(d)
            else:
                rest.append(d)
        except OSError:
            rest.append(d)
    return with_so + rest


def run_python_env_benchmark(
    rl_demo: Path, build_dir: Path, cube_counts: list[int], steps_per_run: int = 50
) -> str:
    # Insert in reverse so dirs that contain rldemo_env.*.so are searched first
    for d in reversed(_find_rldemo_env_path(rl_demo, build_dir)):
        s = str(d)
        if s not in sys.path:
            sys.path.insert(0, s)
    try:
        import rldemo_env
    except ImportError as e:
        return f"rldemo_env not available (build with -DRL_BACKEND_BUILD_RL=ON and install bindings). Skip Python env benchmark. ({e})"

    lines = ["\n--- Python FrameSchedulerEnv workload (cube count vs frame time) ---"]
    env = rldemo_env.FrameSchedulerEnv()
    obs = env.reset()
    for n in cube_counts:
        # action: [cube_normalized, cam_angle, ...]; cube 100–100k
        norm = (n - 100) / 99900.0
        norm = max(0.0, min(1.0, norm))
        action = [norm, 0.5, 0.0, 0.0]
        frame_times = []
        env.reset()
        for _ in range(steps_per_run):
            obs, reward, done, info = env.step(action)
            frame_times.append(info["frame_time"])
            if done:
                env.reset()
        avg_ms = sum(frame_times) / len(frame_times)
        lines.append(f"  cube_count={n}  avg_frame_time_ms={avg_ms:.3f}  min={min(frame_times):.3f}  max={max(frame_times):.3f}")
    return "\n".join(lines)


def run_python_physics_benchmark(
    rl_demo: Path, build_dir: Path, body_counts: list[int], steps_per_run: int = 30
) -> str:
    for d in reversed(_find_rldemo_env_path(rl_demo, build_dir)):
        s = str(d)
        if s not in sys.path:
            sys.path.insert(0, s)
    try:
        import rldemo_env
    except ImportError:
        return ""

    lines = ["\n--- Python WorkStealSchedulerEnv workload (body count vs frame time) ---"]
    env = rldemo_env.WorkStealSchedulerEnv()
    for n in body_counts:
        env.set_body_count(n)
        obs = env.reset()
        frame_times = []
        for _ in range(steps_per_run):
            obs, reward, done, info = env.step([0.25, 0.25, 0.25, 0.25])
            frame_times.append(info.get("frame_time", 0))
            if done:
                obs = env.reset()
        avg_ms = sum(frame_times) / len(frame_times)
        lines.append(f"  body_count={n}  avg_frame_time_ms={avg_ms:.3f}  min={min(frame_times):.3f}  max={max(frame_times):.3f}")
    return "\n".join(lines)


def main() -> int:
    ap = argparse.ArgumentParser(description="Run RL_Backend benchmarks and write results to a text file.")
    ap.add_argument("--build-dir", default=None, help="CMake build directory (default: RL_Backend/build)")
    ap.add_argument("--no-env", action="store_true", help="Skip Python env benchmarks (FrameSchedulerEnv, WorkStealSchedulerEnv)")
    ap.add_argument("--out", default=None, help="Output file (default: benchmark_results_<timestamp>.txt)")
    ap.add_argument("--workers", default="1,2,4,8", help="Comma-separated worker counts for jobbench (default: 1,2,4,8)")
    ap.add_argument("--quick", action="store_true", help="Fewer jobbench runs (workers 2,4 only; depth/breadth (3,4),(4,3)); 10 min timeout")
    ap.add_argument("--jobbench-timeout", type=int, default=600, metavar="SEC", help="Timeout per jobbench invocation in seconds (default: 600)")
    ap.add_argument("--cube-counts", default="1000,5000,10000,50000,100000", help="Cube counts for FrameSchedulerEnv")
    ap.add_argument("--body-counts", default="1000,2000,4000,8000", help="Body counts for WorkStealSchedulerEnv")
    args = ap.parse_args()

    rl_demo = find_rl_demo_root()
    build_dir = find_build_dir(rl_demo, args.build_dir)
    if not build_dir:
        print("Build directory not found. Create it with: mkdir build && cd build && cmake .. && cmake --build .", file=sys.stderr)
        return 1

    out_path = args.out
    if not out_path:
        out_path = f"benchmark_results_{datetime.now().strftime('%Y%m%d_%H%M%S')}.txt"
    out_path = Path(out_path)
    if not out_path.is_absolute():
        out_path = rl_demo / out_path

    if args.quick:
        workers_list = [2, 4]
        depth_breadth_pairs = [(3, 4), (4, 3)]
        jobbench_timeout = 300
    else:
        workers_list = [int(x.strip()) for x in args.workers.split(",") if x.strip().isdigit()]
        if not workers_list:
            workers_list = [1, 2, 4, 8]
        depth_breadth_pairs = [(3, 4), (4, 3), (5, 2)]
        jobbench_timeout = args.jobbench_timeout

    cube_counts = [int(x.strip()) for x in args.cube_counts.split(",") if x.strip().isdigit()]
    if not cube_counts:
        cube_counts = [1000, 10000, 50000, 100000]
    body_counts = [int(x.strip()) for x in args.body_counts.split(",") if x.strip().isdigit()]
    if not body_counts:
        body_counts = [1000, 2000, 4000, 8000]

    sections = []
    sections.append("=" * 70)
    sections.append("RL_Backend Benchmark Suite")
    sections.append("=" * 70)
    sections.append(f"Date: {datetime.now().isoformat()}")
    sections.append(f"RL_Backend root: {rl_demo}")
    sections.append(f"Build dir:    {build_dir}")
    sections.append(f"Output file: {out_path}")
    sections.append("")

    # 1) CTest
    sections.append("--- CTest (unit tests) ---")
    ctest_code, ctest_out = run_ctest(build_dir)
    sections.append(ctest_out)
    sections.append(f"CTest exit code: {ctest_code}")
    sections.append("")

    # 2) JobBench — run once per worker count so a single slow config doesn't timeout the whole run
    sections.append("--- JobBench (job system: workers × depth × breadth) ---")
    sections.append(f"Workers: {workers_list} (one invocation per worker count)")
    sections.append(f"Depth×breadth: {depth_breadth_pairs}")
    per_run_timeout = max(90, jobbench_timeout // max(len(workers_list), 1))
    sections.append(f"Timeout: {jobbench_timeout}s total, ~{per_run_timeout}s per worker count")
    jb_code, jb_out = run_jobbench_per_worker(
        build_dir, workers_list, depth_breadth_pairs, timeout_total=jobbench_timeout
    )
    sections.append(jb_out)
    sections.append(f"JobBench exit code: {jb_code} (0 = at least one worker count completed)")
    sections.append("")

    # 3) Optional: human-readable jobbench (skip in --quick to save time)
    if not args.quick:
        sections.append("--- JobBench (human-readable summary) ---")
        jb2_code, jb2_out = run_jobbench(build_dir, workers_list, [], machine=False, timeout=per_run_timeout * len(workers_list))
        sections.append(jb2_out)
        sections.append("")

    # 4) Python env benchmarks
    if not args.no_env:
        sections.append(run_python_env_benchmark(rl_demo, build_dir, cube_counts))
        sections.append("")
        phys = run_python_physics_benchmark(rl_demo, build_dir, body_counts)
        if phys:
            sections.append(phys)
            sections.append("")

    # Parsed JobBench summary table (from machine output)
    sections.append("--- JobBench parsed summary (workers | depth | breadth | ms | jobs | steals | throughput) ---")
    for line in jb_out.splitlines():
        m = re.match(
            r"workers=(\d+)\s+depth=(\d+)\s+breadth=(\d+)\s+ms=([\d.]+)\s+jobs=(\d+)\s+steals=(\d+).*throughput=([\d.]+)",
            line,
        )
        if m:
            sections.append(f"  {m.group(1):>2} | {m.group(2):>2} | {m.group(3):>2} | {m.group(4):>8} ms | {m.group(5):>6} | {m.group(6):>6} | {m.group(7):>10} jobs/s")

    sections.append("")
    sections.append("=" * 70)
    sections.append("End of benchmark report")
    sections.append("=" * 70)

    report = "\n".join(sections)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(report, encoding="utf-8")
    print(f"Benchmark report written to: {out_path}")
    print(report)
    return 0 if ctest_code == 0 and jb_code == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
