#!/usr/bin/env python3
"""
Visual Demo -- CMake Generator
==============================

Full end-to-end demo for educators:
  1. Pick a project from the menu
  2. Watch AI analyze the codebase
  3. Watch the LLM generate a CMakeLists.txt (streamed with syntax highlighting)
  4. Auto-build (cmake + make)
  5. Launch the graphical demo
  6. Loop back to menu -- build another without restarting

Run:
    python3 -m cmake_generator
"""

import os
import sys
import time
import subprocess
from pathlib import Path

from rich.console import Console
from rich.panel import Panel
from rich.table import Table
from rich.tree import Tree
from rich.syntax import Syntax
from rich.columns import Columns
from rich.text import Text
from rich.rule import Rule
from rich.live import Live
from rich.spinner import Spinner
from rich.progress import Progress, BarColumn, TextColumn, SpinnerColumn
from rich import box

sys.path.insert(0, str(Path(__file__).parent.parent))
from cmake_generator.generator import CMakeGenerator, SYSTEM_PROMPT
from cmake_generator.project import ProjectConfig
from cmake_generator.scanner import scan_directory
from cmake_generator.demos import DEMOS, DEMO_DIRS, DEMO_BINARIES, list_demos

console = Console()
PROJECT_ROOT = Path(__file__).parent.parent.resolve()


# ═══════════════════════════════════════════════════════════════
#  UI Components
# ═══════════════════════════════════════════════════════════════

def banner():
    b = Text()
    b.append("\n")
    b.append("  ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n", style="bold blue")
    b.append("  ┃                                                       ┃\n", style="bold blue")
    b.append("  ┃", style="bold blue")
    b.append("      C M a k e   G e n e r a t o r", style="bold white")
    b.append("                ┃\n", style="bold blue")
    b.append("  ┃", style="bold blue")
    b.append("      Powered by Local LLM  (Ollama)", style="dim cyan")
    b.append("               ┃\n", style="bold blue")
    b.append("  ┃                                                       ┃\n", style="bold blue")
    b.append("  ┃", style="bold blue")
    b.append("   Point at any C/C++ project. AI writes your build.  ", style="dim white")
    b.append("  ┃\n", style="bold blue")
    b.append("  ┃                                                       ┃\n", style="bold blue")
    b.append("  ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛", style="bold blue")
    console.print(b)
    console.print()


def section(title: str):
    console.print()
    console.print(Rule(f"[bold cyan]{title}[/bold cyan]", style="dim cyan"))
    console.print()


# ═══════════════════════════════════════════════════════════════
#  Step 1 -- Project Selection
# ═══════════════════════════════════════════════════════════════

def select_project() -> tuple:
    """Returns (demo_key, config). demo_key is None for scan mode."""
    table = Table(
        title="[bold]Choose a Demo Project[/bold]",
        box=box.HEAVY_EDGE,
        title_style="bold magenta",
        border_style="blue",
        show_lines=True,
        padding=(0, 1),
    )
    table.add_column("#", style="bold cyan", width=4, justify="center")
    table.add_column("Demo", style="bold white", width=24)
    table.add_column("Type", style="yellow", width=14)
    table.add_column("Description", style="dim white", max_width=52)

    demos_list = list(DEMOS.items())
    icons = {"physics": "Water", "raytracer": "Light", "matrix-viz": "Math", "error-demo": "Hello"}
    for i, (key, cfg) in enumerate(demos_list, 1):
        table.add_row(
            str(i),
            f"{icons.get(key, '')}  {cfg.name}",
            cfg.target_type,
            cfg.description[:50] + "...",
        )

    # "Your own project" option -- natural language, not technical jargon
    table.add_row(
        str(len(demos_list) + 1),
        "Your Own Project",
        "auto-detect",
        "I have my own C/C++ code I want to build",
    )

    # Quit option
    table.add_row(
        "Q",
        "Quit",
        "",
        "Exit the CMake Generator",
        style="dim",
    )

    console.print(table)
    console.print()

    while True:
        choice = console.input(
            f"  [bold yellow]What would you like to build? (1-{len(demos_list)+1} or Q): [/bold yellow]"
        ).strip().lower()

        if choice in ("q", "quit", "exit"):
            return "QUIT", None

        if choice.isdigit():
            idx = int(choice) - 1
            if 0 <= idx < len(demos_list):
                key = demos_list[idx][0]
                return key, demos_list[idx][1]
            elif idx == len(demos_list):
                # Natural language path prompt
                console.print()
                console.print(Panel(
                    Text.from_markup(
                        "[bold white]Where is your project?[/bold white]\n\n"
                        "[dim]Just type or paste the folder path.\n"
                        "On Mac you can also drag the folder from Finder\n"
                        "directly into this terminal window.[/dim]\n\n"
                        "[dim italic]Example: /Users/you/my_cool_project[/dim italic]"
                    ),
                    title="[bold cyan]Your Project[/bold cyan]",
                    border_style="cyan", width=55,
                ))
                console.print()
                path = console.input(
                    "  [bold yellow]Project folder: [/bold yellow]"
                ).strip().strip("'\"")  # strip quotes from drag-and-drop
                p = Path(path).resolve()
                if p.is_dir():
                    return None, scan_directory(str(p))
                console.print(f"  [red]Not a directory: {path}[/red]")
                console.print(
                    "  [dim]Make sure the folder exists and try again.[/dim]"
                )
        else:
            console.print(f"  [red]Enter 1-{len(demos_list)+1} or Q to quit[/red]")


# ═══════════════════════════════════════════════════════════════
#  Step 2 -- Project Analysis
# ═══════════════════════════════════════════════════════════════

def show_analysis(config: ProjectConfig, demo_key: str = None):
    section("Step 1  -  Project Analysis")

    # File tree
    tree = Tree(
        f"[bold blue]{config.name}/[/bold blue]",
        guide_style="blue",
    )
    if config.source_dir != ".":
        src = tree.add(f"[blue]{config.source_dir}/[/blue]")
    else:
        src = tree
    for f in config.source_files:
        src.add(f"[green]{Path(f).name}[/green]")

    if config.include_dir and config.include_dir != config.source_dir:
        inc = tree.add(f"[blue]{config.include_dir}/[/blue]")
    else:
        inc = src
    for f in config.header_files:
        inc.add(f"[yellow]{Path(f).name}[/yellow]")

    tree.add("[dim]CMakeLists.txt[/dim]  [italic magenta]<- AI generates this[/italic magenta]")

    # Settings table
    tbl = Table(box=box.SIMPLE_HEAVY, show_header=False, border_style="dim",
                padding=(0, 1))
    tbl.add_column("", style="bold cyan", width=18)
    tbl.add_column("", style="white")
    tbl.add_row("Project",      config.name)
    tbl.add_row("Language",     config.language)
    tbl.add_row("C++ Standard", f"C++{config.standard}")
    tbl.add_row("Target",       config.target_type)
    tbl.add_row("Dependencies", ", ".join(config.dependencies) or "none")
    tbl.add_row("Sources",      f"{len(config.source_files)} files")
    tbl.add_row("Headers",      f"{len(config.header_files)} files")

    console.print(Columns([
        Panel(tree, title="[bold]Project Structure[/bold]",
              border_style="blue", width=38),
        Panel(tbl, title="[bold]Detected Settings[/bold]",
              border_style="cyan", width=44),
    ], padding=2))


# ═══════════════════════════════════════════════════════════════
#  Step 3 -- Prompt & Generation
# ═══════════════════════════════════════════════════════════════

def show_prompt(config: ProjectConfig):
    section("Step 2  -  LLM Prompt")

    console.print(Panel(
        Text(SYSTEM_PROMPT.strip(), style="dim green"),
        title="[bold]System Prompt[/bold]",
        subtitle="[dim]Instructions for the LLM[/dim]",
        border_style="green", width=82,
    ))
    time.sleep(0.2)

    console.print(Panel(
        Syntax(config.to_prompt_context(), "yaml", theme="monokai",
               line_numbers=False),
        title="[bold]Project Specification[/bold]",
        subtitle="[dim]Injected into prompt[/dim]",
        border_style="yellow", width=82,
    ))

    console.print(
        f"\n  [dim]Total prompt length: "
        f"{len(SYSTEM_PROMPT) + len(config.to_prompt_context())} chars[/dim]"
    )


def generate(gen: CMakeGenerator, config: ProjectConfig) -> str:
    section("Step 3  -  AI Generates CMakeLists.txt")

    console.print(
        f"  [dim]Model:[/dim]  [bold]{gen.client.config.model}[/bold]"
    )
    console.print(
        f"  [dim]Temp:[/dim]   [bold]{gen.client.config.temperature}[/bold]"
    )
    console.print()

    prompt = (f"{SYSTEM_PROMPT}\n\n## Project Specification\n"
              f"{config.to_prompt_context()}\n\n## CMakeLists.txt\n")

    chunks = []
    with Live(
        Panel(Spinner("dots", text=" Waiting for LLM...", style="cyan"),
              title="[bold]Generating[/bold]", border_style="magenta", width=82),
        console=console, refresh_per_second=8,
    ) as live:
        for chunk in gen.client.generate_stream(prompt):
            chunks.append(chunk)
            current = gen._clean_output("".join(chunks))
            try:
                display = Syntax(current, "cmake", theme="monokai",
                                 line_numbers=True, word_wrap=True)
            except Exception:
                display = Text(current)
            live.update(Panel(
                display,
                title="[bold]Generating CMakeLists.txt[/bold]",
                subtitle="[dim italic]streaming from LLM...[/dim italic]",
                border_style="magenta", width=82,
            ))

    return gen._clean_output("".join(chunks))


def show_result(content: str, config: ProjectConfig):
    section("Step 4  -  Generated CMakeLists.txt")

    console.print(Panel(
        Syntax(content, "cmake", theme="monokai",
               line_numbers=True, word_wrap=True),
        title=(f"[bold green]CMakeLists.txt[/bold green]"
               f"  [dim]({config.name})[/dim]"),
        subtitle=f"[dim]{len(content.splitlines())} lines[/dim]",
        border_style="green", width=82,
    ))


# ═══════════════════════════════════════════════════════════════
#  Step 5 -- Build
# ═══════════════════════════════════════════════════════════════

def auto_build(demo_dir: Path) -> tuple:
    """Build the project. Returns (success: bool, error_message: str or None)."""
    section("Step 5  -  Building")

    build_dir = demo_dir / "build"
    build_dir.mkdir(exist_ok=True)
    ncpus = os.cpu_count() or 4

    # --- CMake configure ---
    console.print("  [cyan][1/2][/cyan] Configuring with CMake...")
    result = subprocess.run(
        ["cmake", ".."],
        cwd=str(build_dir),
        capture_output=True, text=True,
    )
    if result.returncode != 0:
        # Extract the meaningful error line(s)
        error_msg = result.stderr.strip()
        short = _extract_error(error_msg)
        console.print(f"  [red]>[/red] CMake failed: [red]{short}[/red]")
        return False, error_msg
    console.print("  [green]>[/green] CMake configured")

    # --- Make ---
    console.print(f"  [cyan][2/2][/cyan] Compiling ({ncpus} threads)...")
    result = subprocess.run(
        ["make", f"-j{ncpus}"],
        cwd=str(build_dir),
        capture_output=True, text=True,
    )
    if result.returncode != 0:
        error_msg = result.stderr.strip()
        short = _extract_error(error_msg)
        console.print(f"  [red]>[/red] Compile failed: [red]{short}[/red]")
        return False, error_msg

    console.print("  [green]>[/green] Build succeeded")
    return True, None


def _extract_error(stderr: str) -> str:
    """Pull out just the CMake/compiler error lines, not the noise."""
    lines = stderr.strip().split("\n")
    errors = [l for l in lines if "error" in l.lower() or "Error" in l]
    if errors:
        return errors[0][:120]
    return lines[-1][:120] if lines else "Unknown error"


def _inject_showcase_error(content: str) -> str:
    """Inject a deliberate error for the error-recovery demo.

    Renames the target in target_link_libraries to a wrong name so
    cmake configure fails with a clear 'Cannot specify link libraries
    for target which is not built by this project' error.
    """
    # Replace the correct target name in target_link_libraries with a wrong one
    import re
    # Find the add_executable target name
    m = re.search(r"add_executable\((\w+)", content)
    if m:
        real_target = m.group(1)
        wrong_target = real_target + "_app"
        # Only replace in target_link_libraries (not add_executable)
        content = re.sub(
            r"(target_link_libraries\()" + re.escape(real_target),
            r"\g<1>" + wrong_target,
            content,
        )
    return content


def regenerate_with_error(gen: CMakeGenerator, config: ProjectConfig,
                           prev_cmake: str, error_msg: str) -> str:
    """Ask the LLM to fix its own CMakeLists.txt given the build error."""
    section("Auto-Fix  -  Sending error to LLM")

    fix_prompt = (
        f"{SYSTEM_PROMPT}\n\n"
        f"## Project Specification\n{config.to_prompt_context()}\n\n"
        f"## Previous CMakeLists.txt (has errors)\n"
        f"```cmake\n{prev_cmake}```\n\n"
        f"## Build Error\n{error_msg[-800:]}\n\n"
        f"## Fixed CMakeLists.txt\n"
        f"Fix the error above. Output ONLY the corrected CMakeLists.txt:\n"
    )

    chunks = []
    with Live(
        Panel(Spinner("dots", text=" LLM is fixing the error...", style="yellow"),
              title="[bold yellow]Auto-Fixing[/bold yellow]",
              border_style="yellow", width=82),
        console=console, refresh_per_second=8,
    ) as live:
        for chunk in gen.client.generate_stream(fix_prompt):
            chunks.append(chunk)
            current = gen._clean_output("".join(chunks))
            try:
                display = Syntax(current, "cmake", theme="monokai",
                                 line_numbers=True, word_wrap=True)
            except Exception:
                display = Text(current)
            live.update(Panel(
                display,
                title="[bold yellow]Fixed CMakeLists.txt[/bold yellow]",
                subtitle="[dim italic]streaming fix from LLM...[/dim italic]",
                border_style="yellow", width=82,
            ))

    return gen._clean_output("".join(chunks))


# ═══════════════════════════════════════════════════════════════
#  Step 6 -- Run
# ═══════════════════════════════════════════════════════════════

def launch_demo(demo_dir: Path, binary_name: str):
    binary = demo_dir / "build" / binary_name
    if not binary.exists():
        console.print(f"  [red]Binary not found: {binary}[/red]")
        return

    section("Launching Demo")

    console.print(Panel(
        Text(f"  Starting {binary_name}...\n\n"
             "  The graphical window will open now.\n"
             "  Close it or press Esc when you're done.\n"
             "  You'll return here to build another project.",
             style="bold white"),
        border_style="green", width=55,
    ))
    console.print()

    subprocess.run([str(binary)], cwd=str(demo_dir / "build"))

    console.print()
    console.print("  [green]>[/green] Demo finished.")


# ═══════════════════════════════════════════════════════════════
#  Single build-and-run cycle
# ═══════════════════════════════════════════════════════════════

def run_one_project(gen: CMakeGenerator) -> bool:
    """Run the full pipeline for one project. Returns True to continue, False to quit."""

    # 1. Select project
    demo_key, config = select_project()

    if demo_key == "QUIT":
        return False

    # 2. Show analysis
    show_analysis(config, demo_key)

    # 3. Show prompt
    show_prompt(config)

    console.print()
    console.input(
        "  [bold yellow]Press Enter to generate CMakeLists.txt...[/bold yellow]"
    )

    # 4. Generate
    content = generate(gen, config)

    # Error-demo showcase: inject a deliberate error so the first build fails
    if demo_key == "error-demo":
        content = _inject_showcase_error(content)

    show_result(content, config)

    # Determine where to save & build
    if demo_key and demo_key in DEMO_DIRS:
        demo_dir = PROJECT_ROOT / DEMO_DIRS[demo_key]
        binary_name = DEMO_BINARIES[demo_key]
    else:
        demo_dir = Path(config.description.split("from ")[-1]) if \
            "Auto-detected" in config.description else PROJECT_ROOT / "demos" / config.name.lower()
        binary_name = config.name.lower()
        demo_dir.mkdir(parents=True, exist_ok=True)

    # Save generated CMakeLists.txt
    cmake_path = demo_dir / "CMakeLists.txt"
    cmake_path.write_text(content)
    console.print(f"\n  [dim]Saved:[/dim] [bold]{cmake_path}[/bold]")

    # 5. Build & Run prompt
    console.print()
    answer = console.input(
        "  [bold yellow]Build and run the demo? [Y/n]: [/bold yellow]"
    ).strip().lower()

    if answer not in ("", "y", "yes"):
        console.print("\n  [dim]Skipped. You can build manually:[/dim]")
        console.print(f"    cd {demo_dir}/build && cmake .. && make")
        return True

    # --- Build attempt loop (LLM retry + reference fallback) ---
    max_retries = 2
    for attempt in range(max_retries + 1):
        success, error_msg = auto_build(demo_dir)
        if success:
            launch_demo(demo_dir, binary_name)
            return True

        # Build failed — offer options
        console.print()
        if attempt < max_retries:
            console.print(
                "  [yellow]>[/yellow] Build failed. "
                "I can ask the AI to fix it automatically."
            )
            fix_choice = console.input(
                "  [bold yellow]Ask AI to fix it? [Y/n]: [/bold yellow]"
            ).strip().lower()

            if fix_choice in ("", "y", "yes"):
                # Feed the error back to the LLM and regenerate
                fixed = regenerate_with_error(gen, config, content, error_msg)
                show_result(fixed, config)
                content = fixed
                cmake_path.write_text(content)
                console.print(f"  [dim]Saved fixed version.[/dim]")
                continue
            else:
                break
        else:
            console.print("  [yellow]>[/yellow] Still failing after AI retry.")

    # Last resort: reference CMakeLists.txt
    if demo_key:
        ref = PROJECT_ROOT / DEMO_DIRS[demo_key] / "CMakeLists.txt.ref"
        if ref.exists():
            console.print(
                "\n  [cyan]>[/cyan] Falling back to reference CMakeLists.txt..."
            )
            cmake_path.write_text(ref.read_text())
            success, _ = auto_build(demo_dir)
            if success:
                launch_demo(demo_dir, binary_name)
                return True

    console.print("  [red]>[/red] Build failed. Check the error above.")
    return True


# ═══════════════════════════════════════════════════════════════
#  Main Flow -- loops until user quits
# ═══════════════════════════════════════════════════════════════

def run_demo():
    banner()

    # Check Ollama
    gen = CMakeGenerator()
    if not gen.check_ready():
        console.print(
            "\n  [bold red]Ollama is not running.[/bold red]"
            "\n  [dim]Start it with:[/dim] [bold]ollama serve[/bold]\n"
        )
        sys.exit(1)

    console.print(
        f"  [green]>[/green] Ollama connected  -  "
        f"model: [bold]{gen.client.config.model}[/bold]"
    )
    console.print()

    # Main loop -- keeps running until user chooses to quit
    while True:
        keep_going = run_one_project(gen)
        if not keep_going:
            break

        # After a project finishes, offer to continue or quit
        console.print()
        console.print(Rule("[bold cyan]What's next?[/bold cyan]", style="dim cyan"))
        console.print()
        again = console.input(
            "  [bold yellow]Build another project? [Y/n]: [/bold yellow]"
        ).strip().lower()

        if again not in ("", "y", "yes"):
            break

        # Clear some space and show a mini separator
        console.print()
        console.print(Rule("[dim]Starting a new project[/dim]", style="dim blue"))
        console.print()

    console.print()
    console.print(Panel(
        Text("  Thanks for using CMake Generator!\n"
             "  Your generated CMakeLists.txt files are saved in each project.",
             style="bold white"),
        border_style="blue", width=60,
    ))
    console.print()


if __name__ == "__main__":
    run_demo()
