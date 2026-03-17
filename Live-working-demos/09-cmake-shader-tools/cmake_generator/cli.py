"""
CLI for CMake Generator.

Usage:
    python -m cmake_generator --scan ./my_project         # Auto-detect and generate
    python -m cmake_generator --demo physics              # Run built-in demo
    python -m cmake_generator --name MyApp --desc "..."   # Manual config
"""

import argparse
import sys
from pathlib import Path

from cmake_generator.generator import CMakeGenerator
from cmake_generator.project import ProjectConfig
from cmake_generator.scanner import scan_directory
from cmake_generator.demos import get_demo_config, list_demos


def main():
    parser = argparse.ArgumentParser(
        description="Generate CMakeLists.txt using local LLMs via Ollama"
    )
    parser.add_argument("--scan", metavar="DIR",
                        help="Auto-detect project settings from a directory")
    parser.add_argument("--name", help="Project name")
    parser.add_argument("--desc", help="Project description")
    parser.add_argument("--lang", default="CXX", help="Language: C, CXX (default: CXX)")
    parser.add_argument("--std", default="17", help="C++ standard (default: 17)")
    parser.add_argument("--src-dir", default="src", help="Source directory")
    parser.add_argument("--sources", help="Comma-separated source files")
    parser.add_argument("--headers", help="Comma-separated header files")
    parser.add_argument("--deps", help="Comma-separated dependencies")
    parser.add_argument("--type", default="executable", choices=["executable", "library"])
    parser.add_argument("--extra", default="", help="Extra instructions for the LLM")
    parser.add_argument("--model", default=None, help="Ollama model to use")
    parser.add_argument("--output", "-o", default=None, help="Output directory (default: project dir)")
    parser.add_argument("--stream", action="store_true", help="Stream output")
    parser.add_argument("--demo", help="Run a built-in demo (use --demo list to see options)")

    args = parser.parse_args()

    # --- Resolve config ---
    if args.scan:
        # Auto-detect project settings
        scan_path = Path(args.scan).resolve()
        if not scan_path.is_dir():
            print(f"Error: {scan_path} is not a directory")
            sys.exit(1)

        print(f"Scanning {scan_path} ...")
        config = scan_directory(str(scan_path))

        print(f"\n  Project:      {config.name}")
        print(f"  Language:     {config.language}")
        print(f"  Standard:     C++{config.standard}")
        print(f"  Source dir:   {config.source_dir}")
        print(f"  Include dir:  {config.include_dir}")
        print(f"  Sources:      {len(config.source_files)} files")
        print(f"  Headers:      {len(config.header_files)} files")
        print(f"  Dependencies: {', '.join(config.dependencies) or 'none detected'}")
        print(f"  Target type:  {config.target_type}")
        print()

        if args.output is None:
            args.output = str(scan_path)

    elif args.demo:
        if args.demo == "list":
            print("Available demos:")
            for name, desc in list_demos():
                print(f"  {name:20s} - {desc}")
            return
        config = get_demo_config(args.demo)
        if config is None:
            print(f"Unknown demo: {args.demo}")
            print("Use --demo list to see available demos.")
            sys.exit(1)

    elif args.name and args.desc:
        config = ProjectConfig(
            name=args.name,
            description=args.desc,
            language=args.lang,
            standard=args.std,
            source_dir=args.src_dir,
            source_files=[s.strip() for s in args.sources.split(",")] if args.sources else [],
            header_files=[h.strip() for h in args.headers.split(",")] if args.headers else [],
            dependencies=[d.strip() for d in args.deps.split(",")] if args.deps else [],
            target_type=args.type,
            extra_instructions=args.extra,
        )
    else:
        parser.print_help()
        sys.exit(1)

    if args.extra and hasattr(config, "extra_instructions"):
        config.extra_instructions = args.extra

    # --- Generate ---
    gen = CMakeGenerator(model=args.model)
    if not gen.check_ready():
        sys.exit(1)

    if args.stream:
        content = gen.generate_streaming(config)
    else:
        content = gen.generate(config)
        print("\n" + content)

    # Save
    out = Path(args.output or ".")
    out.mkdir(parents=True, exist_ok=True)
    path = out / "CMakeLists.txt"
    path.write_text(content)
    print(f"\nSaved: {path.resolve()}")


if __name__ == "__main__":
    main()
