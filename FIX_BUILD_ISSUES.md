# Fixing Build Issues

## Issue: Path with Spaces

Your project path contains spaces: `/mnt/d/Master Things/...`

This causes linker errors. Solutions:

### Option 1: Move Project (Recommended)
Move the project to a path without spaces:
```bash
# In WSL
cd /mnt/d
mkdir -p Projects
mv "Master Things" Projects/
cd Projects/"Master Things"/Fall\ Sem\ Classes\ 2025/MasterThesisResearch/GameEngine
```

### Option 2: Use Symbolic Link
Create a symlink without spaces:
```bash
# In WSL
cd /mnt/d
ln -s "Master Things" MasterThings
cd MasterThings/Fall\ Sem\ Classes\ 2025/MasterThesisResearch/GameEngine
```

### Option 3: Build in WSL Home Directory
Copy project to WSL home (no spaces):
```bash
# In WSL
cp -r "/mnt/d/Master Things/Fall Sem Classes 2025/MasterThesisResearch/GameEngine" ~/GameEngine
cd ~/GameEngine
```

## Current Fixes Applied

1. **Disabled OpenAL** - OpenAL has linker issues with paths containing spaces
2. **Disabled Install/Export** - External libraries not in export sets (not needed for building)
3. **Added compiler flags** - Explicitly set GCC compilers

## Rebuild

After moving/copying to a path without spaces:

```bash
bash build_wsl.sh
```

Or manually:
```bash
rm -rf build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON
cmake --build . -j$(nproc)
```

## If CMAKE_C_COMPILE_OBJECT Errors Persist

This is usually a corrupted CMake cache. Clean everything:

```bash
rm -rf build
rm -rf External/*/build
rm -rf External/*/CMakeCache.txt
rm -rf External/*/CMakeFiles
```

Then rebuild.

