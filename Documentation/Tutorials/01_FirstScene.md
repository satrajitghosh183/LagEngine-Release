# Tutorial 1: Your First Scene

Create a basic scene with a cube, camera, and light.

## Steps

1. **Launch the Editor** and ensure you have an empty scene tab.
2. **Create a Cube:** In the Scene Hierarchy, click **Entity → Cube** (or Create Empty and add Mesh Renderer + assign a cube mesh).
3. **Create a Camera:** **Entity → Camera**. In the Components panel, enable **Main Camera**.
4. **Position the camera:** Select the camera entity; set Transform Position to `(0, 2, 5)` and ensure it looks at the cube.
5. **Add a Light:** **Entity → Directional Light**. Set Transform rotation so the light shines on the cube.
6. **Press Play** to see the scene running.

## Next

- Try adding materials and changing colors in the Mesh Renderer component.
- See [02_PhysicsPlayground](02_PhysicsPlayground.md) for physics.
