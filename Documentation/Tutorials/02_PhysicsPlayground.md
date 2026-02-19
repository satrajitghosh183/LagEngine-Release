# Tutorial 2: Physics Playground

Add rigid bodies and colliders to create a simple physics scene.

## Steps

1. **Create a ground:** Add an Entity with Mesh Renderer (plane) and **Collider Component** (Box Collider). Add **Rigid Body Component** and set **Use Gravity** to false so the ground doesn’t fall.
2. **Create falling objects:** Add entities with Cube/Sphere meshes, Rigid Body, and Box/Sphere Collider. Keep **Use Gravity** enabled.
3. **Position objects above the ground** (e.g. Y = 5).
4. **Press Play** to watch them fall and collide.

## Tips

- Mass, Linear Damping, and Restitution affect behavior.
- Use **Spawn on spacebar** in the Physics example app for inspiration.
