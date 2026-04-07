# Changelog

All notable changes to LAG Engine will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-04-05

### Added

#### Graphics
- Deferred rendering pipeline with PBR materials
- Image-Based Lighting (IBL) with HDR environment maps
- Shadow mapping with multiple sampling techniques
- Screen Space Ambient Occlusion (SSAO)
- Post-processing stack: bloom, tone mapping (ACES/Reinhard/Filmic), FXAA, vignette, color grading
- G-Buffer with position, normal, albedo, and metallic-roughness attachments
- Material library with shader hot-reload
- Frustum culling
- GPU instanced particle rendering
- 2D batch renderer with sprite support
- 2D camera with follow, zoom, shake, and parallax
- Sprite system with atlas/spritesheet support and frame animation
- Tile map system with multi-layer support and auto-tiling
- Procedural mesh generation (cube, sphere, cylinder, capsule, plane, torus, cone)

#### Physics
- 3D rigid body dynamics with sequential impulse constraint solver
- Spatial hash broadphase collision detection
- SAT-based narrowphase with contact manifold generation
- Joint system: distance, fixed, hinge, slider, ball-socket
- XPBD soft body simulation
- Tearable cloth simulation with wind forces
- SPH fluid solver with spatial grid acceleration
- Verlet integration for rope/cloth
- Kinematic character controller
- 2D physics engine: rigid bodies, circle/box/polygon/capsule/edge colliders
- 2D joints: distance, revolute, prismatic, weld, mouse
- 2D collision callbacks (enter, exit, stay)
- 2D raycasting and spatial queries

#### Scene Management
- Entity-Component System with UUID-based identification
- 20+ built-in component types
- Parent-child entity hierarchy
- JSON scene serialization with versioning
- Prefab system
- Scene lifecycle management

#### Scripting
- Lua 5.4 scripting with hot-reload
- Entity, component, math, input, and audio API bindings
- Exposed script variables with editor integration

#### Audio
- OpenAL-based 3D spatial audio
- Audio source with volume, pitch, loop, and attenuation
- Doppler effect support

#### Animation
- Skeletal animation with keyframe interpolation (LERP + SLERP)
- Bone hierarchy and skeleton management
- Animation playback control (play, pause, stop, loop, speed)

#### Navigation
- Grid-based navigation mesh generation
- A* pathfinding with heuristic search
- Catmull-Rom path smoothing
- Navigation agent with steering behaviors
- Partial path support for unreachable destinations

#### AI Integration
- Ollama integration for local LLM code assistance
- AI-powered shader generation and editing
- AI-powered CMake project generation
- Code completion, explanation, and refactoring via CodeLLaMA/DeepSeek

#### Editor
- ImGui-based editor with docking layout
- 3D viewport with transform gizmos (translate, rotate, scale)
- Scene hierarchy panel with entity management
- Component inspector with property editing
- Asset browser with import support
- Material editor with real-time PBR preview
- Code editor with Lua syntax highlighting
- Scripting console (Lua REPL)
- Animation panel
- Build panel with CMake integration
- Graphics settings panel
- Profiler window
- Console panel with log output
- Theme editor
- Welcome screen with project templates
- AI assistant panel
- Shader assistant panel
- Object spawner panel
- Hotkey system

#### Build System
- CMake 3.20+ with cross-platform support (Windows, Linux, macOS)
- CPack integration (NSIS, DMG, DEB, TGZ)
- Interactive setup script with dependency management
- 10 example applications
- 11 live working demos

#### Testing
- Google Test integration with ~185 test cases
- Test coverage for physics, scene, graphics, animation, audio, scripting, math

#### Documentation
- API documentation (Core, Scene)
- Architecture overview
- Getting started guide
- 5 tutorials (First Scene, Physics, Scripting, Shaders, Cloth)

#### CI/CD
- GitHub Actions for cross-platform builds (Ubuntu, Windows, macOS)
- Automated release pipeline with binary artifacts
- Issue and PR templates
