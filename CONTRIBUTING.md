# Contributing to LAG Engine

Thank you for your interest in contributing to LAG Engine! This document provides guidelines and instructions for contributing.

## Code of Conduct

By participating in this project, you agree to abide by our [Code of Conduct](CODE_OF_CONDUCT.md).

## How to Contribute

### Reporting Bugs

1. Check [existing issues](../../issues) to avoid duplicates
2. Use the **Bug Report** issue template
3. Include: steps to reproduce, expected vs actual behavior, platform, engine version
4. Attach screenshots or logs if applicable

### Suggesting Features

1. Use the **Feature Request** issue template
2. Describe the use case and motivation
3. Consider how it fits the education-focused mission of LAG Engine

### Pull Requests

1. **Fork** the repository and create a feature branch from `main`
2. Follow the [code style guide](#code-style) below
3. Write tests for new functionality
4. Ensure all existing tests pass: `./bin/GameEngineTests`
5. Apply clang-format: `clang-format -i <your-files>`
6. Submit a PR using the pull request template

## Development Setup

```bash
# Clone your fork
git clone https://github.com/<your-username>/LAGEngine.git
cd LAGEngine

# Install dependencies and build
bash setup.sh --install
bash setup.sh --build-engine

# Run tests
bash setup.sh --test
```

See [Documentation/Manual/GettingStarted.md](Documentation/Manual/GettingStarted.md) for detailed setup instructions.

## Code Style

- **Standard**: C++17
- **Naming**:
  - Classes/Structs: `PascalCase`
  - Functions/Methods: `camelCase`
  - Member variables: `m_PascalCase`
  - Namespaces: `GameEngine`
- **Indentation**: 4 spaces (no tabs)
- **Column limit**: 120 characters
- **Braces**: Attach style
- **Smart pointers**: Use `Ref<T>` (`shared_ptr`) and `Scope<T>` (`unique_ptr`) from `Base.hpp`
- **Include guards**: `#pragma once`

Run `clang-format -i <file>` before committing. The `.clang-format` file at the repo root defines the style.

## Architecture Overview

```
Engine/         Core runtime (Graphics, Physics, Audio, Scripting, Scene)
Editor/         ImGui-based editor application
Tests/          Google Test suite
External/       Vendored dependencies
Assets/         Shaders, content
Examples/       Example applications
```

See [Documentation/Architecture/EngineOverview.md](Documentation/Architecture/EngineOverview.md) for detailed architecture documentation.

## Adding New Features

### New Component
1. Create header in `Engine/Scene/Components/`
2. Add serialization in `Scene::Serialize`/`Deserialize`
3. Add editor UI in `Editor/Panels/ComponentsWindow.cpp`

### New Editor Panel
1. Create header and source in `Editor/Panels/`
2. Register in `Editor/EditorApp.cpp`
3. Add to docking layout in `Editor/UI/UIRenderer.cpp`

### New Physics Subsystem
1. Implement under `Engine/Physics/<Subsystem>/`
2. Wire into physics update loop
3. Add corresponding component type

## Testing

- Tests use Google Test (fetched via CMake FetchContent)
- Test files mirror engine structure: `Tests/Physics/`, `Tests/Scene/`, etc.
- Run: `cd build && ctest --output-on-failure`
- **Important**: Define `GLM_FORCE_CTOR_INIT` on test targets to avoid ABI mismatch

## License

By contributing, you agree that your contributions will be licensed under the MIT License.
