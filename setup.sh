#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════════════════════
#  LAGEngine — Universal Setup, Build & Run Script
#  Detects OS, installs dependencies, builds every target from the main engine
#  to RL_Backend to all 11 Live-working-demos, and provides an interactive menu.
#
#  Usage:
#    bash setup.sh              # Interactive menu
#    bash setup.sh --build-all  # Build everything non-interactively
#    bash setup.sh --help       # Show all CLI flags
# ═══════════════════════════════════════════════════════════════════════════════

# Don't use 'set -e' — we handle errors per-command so the menu stays alive

# ─────────────────────── colours & formatting ────────────────────────────────
if [ -t 1 ] && [ "$(tput colors 2>/dev/null)" -ge 8 ] 2>/dev/null; then
    RED='\033[0;31m';   GREEN='\033[0;32m'; YELLOW='\033[1;33m'
    BLUE='\033[0;34m';  CYAN='\033[0;36m';  MAGENTA='\033[0;35m'
    BOLD='\033[1m';     DIM='\033[2m';      UNDERLINE='\033[4m'
    NC='\033[0m'
else
    RED=''; GREEN=''; YELLOW=''; BLUE=''; CYAN=''; MAGENTA=''
    BOLD=''; DIM=''; UNDERLINE=''; NC=''
fi

hr()      { printf "${DIM}"; printf '━%.0s' $(seq 1 72); printf "${NC}\n"; }
hr_thin() { printf "${DIM}"; printf '─%.0s' $(seq 1 72); printf "${NC}\n"; }
info()    { printf "  ${BLUE}▸${NC} %s\n" "$*"; }
ok()      { printf "  ${GREEN}✔${NC} %s\n" "$*"; }
warn()    { printf "  ${YELLOW}⚠${NC} %s\n" "$*"; }
fail()    { printf "  ${RED}✘${NC} %s\n" "$*"; }
section() { printf "\n  ${BOLD}${CYAN}%s${NC}\n" "$*"; hr_thin; }

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)"
LIVE="$ROOT/Live-working-demos"
NPROC=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# ─────────────────────────── OS detection ────────────────────────────────────
detect_os() {
    case "$(uname -s)" in
        Linux*)
            if grep -qi microsoft /proc/version 2>/dev/null; then OS="wsl"
            elif [ -f /etc/debian_version ];  then OS="debian"
            elif [ -f /etc/fedora-release ];  then OS="fedora"
            elif [ -f /etc/arch-release ];    then OS="arch"
            else OS="linux"; fi ;;
        Darwin*)                 OS="macos" ;;
        MINGW*|MSYS*|CYGWIN*)   OS="windows" ;;
        *)                       OS="unknown" ;;
    esac
}

# ────────────────────── dependency installation ──────────────────────────────
install_deps() {
    section "Installing system dependencies ($OS)"

    case "$OS" in
        debian|wsl)
            sudo apt-get update -qq
            sudo apt-get install -y --no-install-recommends \
                build-essential cmake git pkg-config ca-certificates \
                libgl1-mesa-dev libglu1-mesa-dev \
                libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
                libglfw3-dev libglm-dev libglew-dev \
                libsfml-dev libsdl1.2-dev freeglut3-dev \
                libopenal-dev libassimp-dev \
                python3 python3-pip python3-venv python3-dev \
                libeigen3-dev
            ;;
        fedora)
            sudo dnf install -y \
                gcc-c++ cmake git pkgconf-pkg-config \
                mesa-libGL-devel mesa-libGLU-devel \
                libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel \
                glfw-devel glm-devel glew-devel \
                SFML-devel SDL-devel freeglut-devel \
                openal-soft-devel assimp-devel \
                python3 python3-pip python3-devel \
                eigen3-devel
            ;;
        arch)
            sudo pacman -S --needed --noconfirm \
                base-devel cmake git pkgconf \
                mesa glu libx11 libxrandr libxinerama libxcursor libxi \
                glfw glm glew sfml sdl freeglut \
                openal assimp \
                python python-pip \
                eigen
            ;;
        macos)
            if ! command -v brew &>/dev/null; then
                fail "Homebrew not found — install from https://brew.sh"
                return 1
            fi
            brew install cmake pkg-config llvm \
                glfw glm glew sfml sdl12-compat freeglut \
                openal-soft assimp eigen python3
            ;;
        windows)
            warn "On Windows install these via vcpkg, MSYS2 or Visual Studio Installer:"
            printf "    cmake, glfw3, glm, glew, sfml, freeglut, openal-soft, assimp, eigen3\n"
            printf "    Visual Studio 2019+ with C++ Desktop workload, Python 3.10+\n\n"
            warn "Or use MSYS2/MinGW — the script continues assuming tools are in PATH."
            ;;
        *)
            warn "Unknown OS — install these packages manually:"
            printf "    cmake gcc/clang OpenGL GLFW GLM GLEW SFML SDL freeglut\n"
            printf "    OpenAL Assimp Eigen3 Python3\n"
            ;;
    esac

    ok "System dependencies ready"
}

# ──────────────────────── pre-flight checks ──────────────────────────────────
check_tool()  { command -v "$1" &>/dev/null; }

preflight() {
    section "Pre-flight checks"
    local missing=0

    for tool in cmake git; do
        if check_tool "$tool"; then
            ok "$tool  →  $(command -v "$tool")"
        else
            fail "$tool not found"
            missing=1
        fi
    done

    # Python (try python3 then python)
    if check_tool python3; then
        ok "python3  →  $(python3 --version 2>&1)"
    elif check_tool python; then
        ok "python  →  $(python --version 2>&1)"
    else
        fail "python3 not found"
        missing=1
    fi

    # C++ compiler
    if check_tool g++; then ok "g++  →  $(g++ --version 2>&1 | head -1)"
    elif check_tool clang++; then ok "clang++  →  $(clang++ --version 2>&1 | head -1)"
    elif check_tool cl; then ok "cl (MSVC) found"
    else fail "No C++ compiler found"; missing=1; fi

    # Optional: CUDA
    if check_tool nvcc; then
        ok "nvcc  →  $(nvcc --version 2>&1 | tail -1)"
    else
        info "CUDA not found (optional — demos 05, 06 will be skipped)"
    fi

    # CMake version
    local cmake_ver
    cmake_ver=$(cmake --version 2>/dev/null | head -1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')
    if [ -n "$cmake_ver" ]; then
        ok "CMake version: $cmake_ver"
    fi

    printf "\n"
    if [ "$missing" -eq 1 ]; then
        warn "Missing tools detected. Run option 1 (Install dependencies) first."
        return 1
    else
        ok "All required tools present!"
    fi
}

# ────────────────────────── build helpers ────────────────────────────────────
build_cmake_project() {
    local dir="$1" name="$2" extra_args="${3:-}"
    info "Building ${BOLD}$name${NC} ..."

    if [ ! -f "$dir/CMakeLists.txt" ]; then
        fail "No CMakeLists.txt in $dir"
        return 1
    fi

    cd "$dir"
    mkdir -p build && cd build

    # Configure
    if ! eval cmake .. -DCMAKE_BUILD_TYPE=Release $extra_args 2>&1 | tail -8; then
        fail "CMake configure failed for $name"
        cd "$ROOT"
        return 1
    fi

    # Build
    if cmake --build . --config Release -j"$NPROC" 2>&1 | tail -8; then
        ok "$name built successfully"
    else
        fail "$name build failed"
        cd "$ROOT"
        return 1
    fi

    cd "$ROOT"
}

# ─────────────── main engine ─────────────────────────────────────────────────
build_engine() {
    section "Building LAGEngine (engine + editor + examples)"
    build_cmake_project "$ROOT" "LAGEngine"
}

# ─────────────── RL_Backend ──────────────────────────────────────────────────
build_rl_backend() {
    section "Building RL_Backend (C++20 multithreaded rendering + RL suite)"
    local cuda_flag=""
    if check_tool nvcc; then
        cuda_flag="-DRL_BACKEND_BUILD_CUDA=ON"
        info "CUDA detected — enabling GPU physics"
    fi
    build_cmake_project "$ROOT/RL_Backend" "RL_Backend" \
        "-DRL_BACKEND_BUILD_RL=ON -DRL_BACKEND_BUILD_TESTS=ON $cuda_flag"
}

setup_rl_python() {
    section "Setting up Python RL environment"
    local py_cmd="python3"
    check_tool python3 || py_cmd="python"

    cd "$ROOT/RL_Backend/rl"
    if [ ! -d ".venv" ]; then
        info "Creating virtual environment..."
        "$py_cmd" -m venv .venv
    fi
    # Cross-platform activation
    if [ -f ".venv/bin/activate" ]; then
        source .venv/bin/activate
    elif [ -f ".venv/Scripts/activate" ]; then
        source .venv/Scripts/activate
    fi
    info "Installing Python packages..."
    pip install --quiet --upgrade pip 2>/dev/null
    pip install --quiet -r requirements.txt 2>/dev/null
    ok "Python RL environment ready (.venv in RL_Backend/rl/)"
    cd "$ROOT"
}

# ─────────────── Live-working-demos ──────────────────────────────────────────
#
# Each demo's dependency strategy:
#   01  SFML + GLFW + OpenGL               (system packages, GLAD/stb vendored)
#   02  SDL 1.2 + GLEW + OpenGL            (system packages, AntTweakBar/PartyKel vendored)
#   03  GLFW + GLM + OpenGL                (system packages, GLAD/Eigen/ImGui vendored)
#   04  freeglut + OpenGL                  (system packages only)
#   05  CUDA + OpenGL                      (GLFW via FetchContent, GLAD vendored)
#   06  CUDA + GLFW + OpenGL               (system packages, GLAD/ImGui vendored, GLM fallback)
#   07  GLEW + OpenGL                      (system packages, GLFW/Eigen/ImGui/ImGuizmo vendored)
#   08  OpenGL                             (GLFW via FetchContent, GLAD/ImGui/GLM vendored)
#   09  Python only                        (pip install)
#   10  Python + PyTorch                   (pip install, WIP)
#   11  OpenGL                             (GLFW/GLM/ImGui via FetchContent, GLAD vendored)

DEMO_NAMES=(
    "01-verlet-physics"
    "02-spring-cloth-simulation"
    "03-xpbd-soft-body"
    "04-particle-system"
    "05-gpu-particles-cuda"
    "06-deferred-rendering"
    "07-robot-arm-kinematics"
    "08-imgui-editor"
    "09-cmake-shader-tools"
    "10-neural-character-generation"
    "11-spring-block-sim"
)

DEMO_DESCRIPTIONS=(
    "Verlet cloth 2D (SFML) + 3D (OpenGL)"
    "Spring-mass cloth with PartyKel (SDL + GLEW)"
    "XPBD soft-body deformable (GLFW + Eigen)"
    "Freeglut particle system"
    "CUDA-OpenGL GPU particles + RL interface"
    "Deferred rendering with render graph + CUDA"
    "FK/IK robot arm (GLEW + Eigen + ImGuizmo)"
    "ImGui editor prototype"
    "Python CMake/shader generation tools"
    "Neural character generation (WIP, Python)"
    "PBR spring-block simulator with bloom"
)

DEMO_TYPES=(cpp cpp cpp cpp cuda cuda cpp cpp python python cpp)

build_live_demo() {
    local idx="$1"
    local name="${DEMO_NAMES[$idx]}"
    local dir="$LIVE/$name"

    if [ ! -d "$dir" ]; then
        warn "Directory not found: $name"
        return 1
    fi

    case "${DEMO_TYPES[$idx]}" in
        python)
            if [ -f "$dir/requirements.txt" ]; then
                info "Installing Python deps for $name..."
                cd "$dir"
                python3 -m pip install --quiet -r requirements.txt 2>/dev/null || \
                    warn "pip install failed for $name (non-fatal)"
                cd "$ROOT"
                ok "$name Python deps installed"
            elif [ -f "$dir/environment.yml" ]; then
                info "$name uses conda — run: conda env create -f $dir/environment.yml"
                ok "$name — conda environment.yml found"
            else
                ok "$name — no build needed (Python scripts)"
            fi
            ;;
        cuda)
            if check_tool nvcc; then
                build_cmake_project "$dir" "$name" || warn "Build failed: $name (non-fatal)"
            else
                warn "Skipping $name — CUDA (nvcc) not found"
            fi
            ;;
        cpp)
            if [ -f "$dir/CMakeLists.txt" ]; then
                build_cmake_project "$dir" "$name" || warn "Build failed: $name (non-fatal)"
            else
                warn "No CMakeLists.txt in $name"
            fi
            ;;
    esac
}

build_all_live_demos() {
    section "Building all Live-working-demos"
    local built=0 skipped=0 failed=0
    for i in "${!DEMO_NAMES[@]}"; do
        if build_live_demo "$i"; then
            ((built++))
        else
            ((skipped++))
        fi
    done
    printf "\n"
    ok "Live-working-demos: $built built, $skipped skipped"
}

build_everything() {
    local start_time=$SECONDS
    install_deps
    preflight
    build_engine
    build_rl_backend
    setup_rl_python
    build_all_live_demos
    local elapsed=$(( SECONDS - start_time ))
    printf "\n"
    hr
    printf "  ${GREEN}${BOLD}Full build complete!${NC}  ${DIM}(${elapsed}s)${NC}\n"
    hr
}

# ─────────────────────────── menu system ─────────────────────────────────────
show_banner() {
    clear 2>/dev/null || true
    printf "\n"
    printf "${CYAN}${BOLD}"
    cat << 'BANNER'
      ██╗      █████╗  ██████╗ ███████╗███╗   ██╗ ██████╗ ██╗███╗   ██╗███████╗
      ██║     ██╔══██╗██╔════╝ ██╔════╝████╗  ██║██╔════╝ ██║████╗  ██║██╔════╝
      ██║     ███████║██║  ███╗█████╗  ██╔██╗ ██║██║  ███╗██║██╔██╗ ██║█████╗
      ██║     ██╔══██║██║   ██║██╔══╝  ██║╚██╗██║██║   ██║██║██║╚██╗██║██╔══╝
      ███████╗██║  ██║╚██████╔╝███████╗██║ ╚████║╚██████╔╝██║██║ ╚████║███████╗
      ╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝╚═╝  ╚═══╝ ╚═════╝ ╚═╝╚═╝  ╚═══╝╚══════╝
BANNER
    printf "${NC}\n"
    printf "  ${DIM}C++17/20 Game Engine  •  Physics  •  Rendering  •  AI  •  RL${NC}\n"
    printf "  ${DIM}OS: ${BOLD}$OS${NC}${DIM}  •  Cores: ${BOLD}$NPROC${NC}\n"
    hr
}

show_main_menu() {
    show_banner
    printf "\n"
    printf "  ${BOLD}${UNDERLINE}Setup & Build${NC}\n\n"
    printf "    ${GREEN}1${NC}  Install system dependencies          ${DIM}(apt/dnf/pacman/brew)${NC}\n"
    printf "    ${GREEN}2${NC}  Pre-flight check                     ${DIM}(verify tools)${NC}\n"
    printf "    ${GREEN}3${NC}  Build LAGEngine                      ${DIM}(engine + editor + examples)${NC}\n"
    printf "    ${GREEN}4${NC}  Build RL_Backend                     ${DIM}(C++20 rendering + RL)${NC}\n"
    printf "    ${GREEN}5${NC}  Setup Python RL environment           ${DIM}(venv + pip)${NC}\n"
    printf "    ${GREEN}6${NC}  Build all Live-working-demos          ${DIM}(11 prototypes)${NC}\n"
    printf "    ${GREEN}7${NC}  Build a specific Live demo            ${DIM}(pick one)${NC}\n"
    printf "    ${GREEN}8${NC}  Build ${BOLD}everything${NC}                     ${DIM}(full setup)${NC}\n"
    printf "\n"
    printf "  ${BOLD}${UNDERLINE}Run${NC}\n\n"
    printf "    ${CYAN}e${NC}  Run GameEngine Editor\n"
    printf "    ${CYAN}r${NC}  RL_Backend launcher\n"
    printf "    ${CYAN}d${NC}  Run a Live-working-demo\n"
    printf "    ${CYAN}t${NC}  Run tests\n"
    printf "\n"
    printf "  ${BOLD}${UNDERLINE}Info${NC}\n\n"
    printf "    ${MAGENTA}s${NC}  Show project structure\n"
    printf "    ${MAGENTA}p${NC}  Show dependency matrix\n"
    printf "    ${YELLOW}h${NC}  Help / CLI flags\n"
    printf "    ${YELLOW}q${NC}  Quit\n"
    printf "\n"
    hr
    printf "  ${BOLD}▶ Select: ${NC}"
}

show_demo_picker() {
    printf "\n"
    section "Live-working-demos"
    for i in "${!DEMO_NAMES[@]}"; do
        local name="${DEMO_NAMES[$i]}"
        local desc="${DEMO_DESCRIPTIONS[$i]}"
        local status="${DIM}[not built]${NC}"
        if [ -d "$LIVE/$name/build" ]; then
            status="${GREEN}[built]${NC}"
        fi
        printf "    ${GREEN}%2d${NC}  %-36s %b  ${DIM}%s${NC}\n" "$((i+1))" "$name" "$status" "$desc"
    done
    printf "\n     ${YELLOW}0${NC}  Back\n\n"
    hr
}

pick_and_build_demo() {
    show_demo_picker
    printf "  ${BOLD}Build demo #: ${NC}"
    read -r choice
    if [[ "$choice" =~ ^[0-9]+$ ]] && [ "$choice" -ge 1 ] && [ "$choice" -le ${#DEMO_NAMES[@]} ]; then
        build_live_demo "$((choice-1))"
    elif [ "$choice" != "0" ]; then
        warn "Invalid selection"
    fi
    read -rp "  Press Enter to continue..."
}

run_live_demo() {
    show_demo_picker
    printf "  ${BOLD}Run demo #: ${NC}"
    read -r choice
    if [[ "$choice" =~ ^[0-9]+$ ]] && [ "$choice" -ge 1 ] && [ "$choice" -le ${#DEMO_NAMES[@]} ]; then
        local idx=$((choice-1))
        local name="${DEMO_NAMES[$idx]}"
        local dir="$LIVE/$name"

        case "${DEMO_TYPES[$idx]}" in
            python)
                info "Launching $name..."
                cd "$dir"
                if [ -f "main.py" ]; then python3 main.py
                elif [ -f "pipeline.py" ]; then python3 pipeline.py
                elif [ -f "test_preview.py" ]; then python3 test_preview.py
                else warn "No entry point found — explore $dir manually"; fi
                cd "$ROOT" ;;
            *)
                # Find executable in build/
                local exe=""
                if [ -d "$dir/build" ]; then
                    exe=$(find "$dir/build" -maxdepth 3 -type f \
                          \( -perm -u+x -o -name "*.exe" \) \
                          ! -name "*.cmake" ! -name "*.o" ! -name "*.obj" \
                          2>/dev/null | head -1)
                fi
                if [ -n "$exe" ]; then
                    info "Running $exe ..."
                    cd "$(dirname "$exe")" && "./$( basename "$exe" )"; cd "$ROOT"
                else
                    fail "No executable found. Build $name first (option 6 or 7)."
                fi ;;
        esac
    elif [ "$choice" != "0" ]; then
        warn "Invalid selection"
    fi
    read -rp "  Press Enter to continue..."
}

show_rl_menu() {
    show_banner
    printf "\n"
    printf "  ${BOLD}${UNDERLINE}RL_Backend Executables${NC}\n\n"
    printf "    ${GREEN}1${NC}  demo_hello_triangle       ${DIM}Hello triangle + ImGui overlay${NC}\n"
    printf "    ${GREEN}2${NC}  demo_indirectdraw          ${DIM}10k-100k cubes, parallel CPU prep${NC}\n"
    printf "    ${GREEN}3${NC}  demo_physics_slow          ${DIM}Single-threaded N-body baseline${NC}\n"
    printf "    ${GREEN}4${NC}  demo_physics_parallel      ${DIM}Parallel N-body with JobSystem${NC}\n"
    printf "    ${GREEN}5${NC}  demo_physics_rl            ${DIM}N-body + work distribution UI${NC}\n"
    printf "    ${GREEN}6${NC}  jobbench                   ${DIM}Job system microbenchmark (headless)${NC}\n"
    printf "\n"
    printf "  ${BOLD}${UNDERLINE}RL Training${NC}\n\n"
    printf "    ${CYAN}t${NC}  Train PPO on FrameSchedulerEnv\n"
    printf "    ${CYAN}c${NC}  Compare all 12 algorithms\n"
    printf "    ${CYAN}v${NC}  View TensorBoard\n"
    printf "\n"
    printf "    ${YELLOW}b${NC}  Back to main menu\n\n"
    hr
    printf "  ${BOLD}▶ Select: ${NC}"
}

run_rl_executable() {
    local exe="$1"
    local build_dir="$ROOT/RL_Backend/build"
    local path=""
    # Check multiple possible locations (single-config and multi-config generators)
    for p in "$build_dir/bin/$exe" "$build_dir/bin/$exe.exe" \
             "$build_dir/bin/Release/$exe" "$build_dir/bin/Release/$exe.exe"; do
        if [ -f "$p" ]; then path="$p"; break; fi
    done
    if [ -n "$path" ]; then
        info "Running $exe ..."
        "$path"
    else
        fail "$exe not found — build RL_Backend first (option 4 from main menu)."
    fi
    read -rp "  Press Enter to continue..."
}

rl_activate_venv() {
    cd "$ROOT/RL_Backend/rl"
    if [ -f ".venv/bin/activate" ]; then source .venv/bin/activate
    elif [ -f ".venv/Scripts/activate" ]; then source .venv/Scripts/activate
    else warn "No venv found. Run option 5 first."; return 1; fi
}

rl_menu_loop() {
    while true; do
        show_rl_menu
        read -r choice
        case "$choice" in
            1) run_rl_executable "demo_hello_triangle" ;;
            2) run_rl_executable "demo_indirectdraw" ;;
            3) run_rl_executable "demo_physics_slow" ;;
            4) run_rl_executable "demo_physics_parallel" ;;
            5) run_rl_executable "demo_physics_rl" ;;
            6) run_rl_executable "jobbench" ;;
            t)
                info "Training PPO on FrameSchedulerEnv (50k steps)..."
                if rl_activate_venv; then
                    python train.py --algo ppo --env FrameSchedulerEnv --steps 50000 --seed 0
                    cd "$ROOT"
                fi
                read -rp "  Press Enter to continue..." ;;
            c)
                info "Running algorithm comparison..."
                if rl_activate_venv; then
                    python ../scripts/compare_algorithms.py
                    cd "$ROOT"
                fi
                read -rp "  Press Enter to continue..." ;;
            v)
                info "Launching TensorBoard on http://localhost:6006 ..."
                if rl_activate_venv; then
                    tensorboard --logdir runs --bind_all &
                    cd "$ROOT"
                fi
                read -rp "  Press Enter to continue..." ;;
            b|q) break ;;
            *) warn "Invalid option"; sleep 0.5 ;;
        esac
    done
}

run_tests() {
    section "Running Tests"

    # Engine tests (GTest)
    if [ -d "$ROOT/build" ]; then
        info "Running engine tests..."
        cd "$ROOT/build"
        if ctest --output-on-failure 2>/dev/null; then
            ok "Engine tests passed"
        else
            warn "Engine tests: some failed or not built yet"
        fi
        cd "$ROOT"
    else
        warn "Engine not built. Run option 3 first."
    fi

    # RL_Backend tests (Catch2)
    local test_exe=""
    for p in "$ROOT/RL_Backend/build/bin/test_jobs" \
             "$ROOT/RL_Backend/build/bin/test_jobs.exe" \
             "$ROOT/RL_Backend/build/bin/Release/test_jobs.exe"; do
        [ -f "$p" ] && test_exe="$p" && break
    done
    if [ -n "$test_exe" ]; then
        info "Running RL_Backend tests..."
        if "$test_exe"; then
            ok "RL_Backend tests passed"
        else
            fail "RL_Backend tests failed"
        fi
    else
        warn "RL_Backend tests not built. Run option 4 first."
    fi

    # Python smoke test
    if [ -f "$ROOT/RL_Backend/rl/test_env_quick.py" ]; then
        info "Running Python RL smoke test..."
        cd "$ROOT/RL_Backend/rl"
        if rl_activate_venv 2>/dev/null; then
            python test_env_quick.py 2>/dev/null && ok "Python RL smoke test passed" \
                || warn "Python RL smoke test failed (rldemo_env may not be built)"
        fi
        cd "$ROOT"
    fi
}

show_structure() {
    show_banner
    section "Project Structure"
    cat << 'TREE'
    LAGEngine/
    ├── Engine/                  Core engine library (C++17)
    │   ├── Core/               Application, Logger, JobSystem, Time
    │   ├── Graphics/           Camera3D, GBuffer, MeshGenerator, BatchRenderer
    │   ├── Physics/            RigidBody, Constraints, Fluids (SPH), Collision
    │   ├── Scene/              ECS, Components, SceneSerializer
    │   ├── Scripting/          Lua ScriptEngine, OllamaClient
    │   ├── Audio/              AudioEngine (OpenAL)
    │   └── Animation/          Animator, SpriteAnimation
    │
    ├── Editor/                 ImGui editor (13+ panels)
    │   ├── Core/               Command system, EditorApp
    │   └── Panels/             Viewport, Hierarchy, Components, Shader Assistant
    │
    ├── Examples/               7 example applications
    │   ├── 01_BasicRendering   02_PhysicsDemo   03_ClothSimulation
    │   ├── 04_AudioDemo        05_FluidSimulation
    │   └── 06_AnimationDemo    07_DeferredRendering
    │
    ├── External/               Vendored: GLFW, GLM, ImGui, GLAD, Lua, Assimp,
    │                           nlohmann_json, stb_image, OpenAL Soft
    │
    ├── RL_Backend/             Standalone C++20 rendering + RL benchmark suite
    │   ├── engine/             JobSystem, RenderThread, IndirectDrawBuilder
    │   ├── demos/              6 executables (hello_triangle → jobbench)
    │   ├── bindings/           pybind11 environments (Frame/WorkSteal/Job)
    │   ├── rl/                 Python: 12 RL algorithms, train.py, eval.py
    │   └── third_party/        FetchContent: GLFW, GLM, ImGui, spdlog, Catch2
    │
    ├── Live-working-demos/     11 foundational physics/rendering prototypes
    │   ├── 01-verlet-physics           Verlet cloth 2D + 3D
    │   ├── 02-spring-cloth-simulation  Spring-mass cloth (PartyKel)
    │   ├── 03-xpbd-soft-body           XPBD deformable bodies
    │   ├── 04-particle-system          Freeglut particles
    │   ├── 05-gpu-particles-cuda       CUDA GPU particles + RL
    │   ├── 06-deferred-rendering       Render graph + CUDA post-processing
    │   ├── 07-robot-arm-kinematics     FK/IK robot arm (DH params)
    │   ├── 08-imgui-editor             Editor prototype
    │   ├── 09-cmake-shader-tools       Python CMake & GLSL generators
    │   ├── 10-neural-character-gen     Neural character generation (WIP)
    │   └── 11-spring-block-sim         PBR spring-block with bloom
    │
    ├── Documentation/          API, Architecture, Tutorials
    ├── Tests/                  GTest unit tests (25 files, ~185 test cases)
    ├── Assets/                 Shaders, textures, scenes
    └── Tools/                  Python project creation utility
TREE
    printf "\n"
    read -rp "  Press Enter to return..."
}

show_dep_matrix() {
    show_banner
    section "Dependency Matrix"
    printf "  ${BOLD}%-35s  %-12s  %-s${NC}\n" "Target" "Type" "Dependencies"
    hr_thin
    printf "  %-35s  %-12s  %s\n" "LAGEngine (engine+editor+examples)" "C++17"      "OpenGL GLFW GLM ImGui GLAD Lua Assimp OpenAL"
    printf "  %-35s  %-12s  %s\n" "RL_Backend"                         "C++20"      "OpenGL (all via FetchContent + vendored GLAD)"
    printf "  %-35s  %-12s  %s\n" "RL_Backend Python"                  "Python 3"   "torch gymnasium numpy tensorboard"
    hr_thin
    printf "  %-35s  %-12s  %s\n" "01 Verlet Physics"     "C++17"    "SFML GLFW OpenGL (GLAD/stb vendored)"
    printf "  %-35s  %-12s  %s\n" "02 Spring Cloth"       "C++11"    "SDL1.2 GLEW OpenGL (AntTweakBar/PartyKel vendored)"
    printf "  %-35s  %-12s  %s\n" "03 XPBD Soft Body"     "C++17"    "GLFW GLM OpenGL (GLAD/Eigen/ImGui vendored)"
    printf "  %-35s  %-12s  %s\n" "04 Particle System"    "C++11"    "freeglut OpenGL"
    printf "  %-35s  %-12s  %s\n" "05 GPU Particles"      "CUDA/C++" "CUDA OpenGL (GLFW FetchContent, GLAD vendored)"
    printf "  %-35s  %-12s  %s\n" "06 Deferred Rendering" "CUDA/C++" "CUDA GLFW OpenGL (GLAD/ImGui vendored, GLM fallback)"
    printf "  %-35s  %-12s  %s\n" "07 Robot Arm"          "C++17"    "GLEW OpenGL (GLFW/Eigen/ImGui/ImGuizmo vendored)"
    printf "  %-35s  %-12s  %s\n" "08 ImGui Editor"       "C++17"    "OpenGL (GLFW FetchContent, GLAD/ImGui/GLM vendored)"
    printf "  %-35s  %-12s  %s\n" "09 CMake/Shader Tools" "Python"   "requests moderngl pygame numpy"
    printf "  %-35s  %-12s  %s\n" "10 Neural Char Gen"    "Python"   "torch torchvision (WIP, conda)"
    printf "  %-35s  %-12s  %s\n" "11 Spring Block Sim"   "C++17"    "OpenGL (GLFW/GLM/ImGui FetchContent, GLAD vendored)"
    printf "\n"
    printf "  ${DIM}FetchContent = downloaded automatically by CMake at configure time${NC}\n"
    printf "  ${DIM}vendored     = source included in the repo (external/ or lib/)${NC}\n\n"
    read -rp "  Press Enter to return..."
}

show_help() {
    printf "\n"
    section "CLI Usage"
    printf "  ${BOLD}bash setup.sh${NC} [OPTION]\n\n"
    printf "  ${BOLD}Options:${NC}\n"
    printf "    ${GREEN}--install${NC}         Install system dependencies\n"
    printf "    ${GREEN}--preflight${NC}       Check required tools\n"
    printf "    ${GREEN}--build-engine${NC}    Build main engine only\n"
    printf "    ${GREEN}--build-rl${NC}        Build RL_Backend only\n"
    printf "    ${GREEN}--build-demos${NC}     Build all Live-working-demos\n"
    printf "    ${GREEN}--build-all${NC}       Build everything\n"
    printf "    ${GREEN}--setup-python${NC}    Setup RL Python venv\n"
    printf "    ${GREEN}--test${NC}            Run all tests\n"
    printf "    ${GREEN}-h, --help${NC}        Show this help\n"
    printf "    ${DIM}(no args)${NC}         Interactive menu\n\n"

    printf "  ${BOLD}Quick Start:${NC}\n\n"
    printf "    ${DIM}# Full setup from scratch (Linux/WSL/macOS):${NC}\n"
    printf "    bash setup.sh --install && bash setup.sh --build-all\n\n"
    printf "    ${DIM}# Just the engine:${NC}\n"
    printf "    bash setup.sh --build-engine\n\n"
    printf "    ${DIM}# Interactive mode:${NC}\n"
    printf "    bash setup.sh\n\n"

    printf "  ${BOLD}Windows users:${NC}\n"
    printf "    Run from ${CYAN}Git Bash${NC}, ${CYAN}WSL${NC}, or ${CYAN}MSYS2${NC} — not PowerShell.\n"
    printf "    Or use the companion: ${CYAN}setup.bat${NC}\n\n"
}

# ─────────────────────── main menu loop ──────────────────────────────────────
main_menu_loop() {
    while true; do
        show_main_menu
        read -r choice
        case "$choice" in
            1)  install_deps;         read -rp "  Press Enter to continue..." ;;
            2)  preflight;            read -rp "  Press Enter to continue..." ;;
            3)  build_engine;         read -rp "  Press Enter to continue..." ;;
            4)  build_rl_backend;     read -rp "  Press Enter to continue..." ;;
            5)  setup_rl_python;      read -rp "  Press Enter to continue..." ;;
            6)  build_all_live_demos; read -rp "  Press Enter to continue..." ;;
            7)  pick_and_build_demo ;;
            8)  build_everything;     read -rp "  Press Enter to continue..." ;;
            e)
                local editor=""
                for p in "$ROOT/build/bin/GameEngineEditor" \
                         "$ROOT/build/bin/GameEngineEditor.exe" \
                         "$ROOT/build/bin/Release/GameEngineEditor" \
                         "$ROOT/build/bin/Release/GameEngineEditor.exe" \
                         "$ROOT/build/Release/GameEngineEditor.exe"; do
                    [ -f "$p" ] && editor="$p" && break
                done
                if [ -n "$editor" ]; then
                    info "Launching GameEngine Editor..."
                    "$editor"
                else
                    fail "Editor not built. Run option 3 first."
                fi
                read -rp "  Press Enter to continue..." ;;
            r)  rl_menu_loop ;;
            d)  run_live_demo ;;
            t)  run_tests; read -rp "  Press Enter to continue..." ;;
            s)  show_structure ;;
            p)  show_dep_matrix ;;
            h)  show_help; read -rp "  Press Enter to continue..." ;;
            q)
                printf "\n  ${GREEN}${BOLD}Goodbye!${NC}\n\n"
                exit 0 ;;
            *)  warn "Invalid option"; sleep 0.5 ;;
        esac
    done
}

# ────────────────────────── entry point ──────────────────────────────────────
detect_os

case "${1:-}" in
    --install)       install_deps; exit 0 ;;
    --preflight)     preflight; exit 0 ;;
    --build-engine)  build_engine; exit 0 ;;
    --build-rl)      build_rl_backend; exit 0 ;;
    --build-demos)   build_all_live_demos; exit 0 ;;
    --setup-python)  setup_rl_python; exit 0 ;;
    --test)          run_tests; exit 0 ;;
    --build-all)
        preflight
        build_engine
        build_rl_backend
        setup_rl_python
        build_all_live_demos
        printf "\n"; hr
        printf "  ${GREEN}${BOLD}Full build complete!${NC}\n"
        hr
        exit 0 ;;
    -h|--help)       show_help; exit 0 ;;
    "")              info "Detected OS: ${BOLD}$OS${NC}"; main_menu_loop ;;
    *)               fail "Unknown option: $1"; show_help; exit 1 ;;
esac
