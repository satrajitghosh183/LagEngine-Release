# Tutorial 5: Cloth and Soft Bodies

Set up a cloth simulation with wind and collision.

## Steps

1. **Create an entity** and add **Soft Body (XPBD)** component.
2. **Configure grid:** Set Grid Res X/Y, Width, Height to define the cloth resolution.
3. **Enable constraints:** Distance, Bending, Volume, Collision as needed.
4. **Add wind:** In the Cloth/Soft Body system (or via scripting) apply wind forces.
5. **Add a collision sphere** for the cloth to interact with.
6. **Press Play** to simulate.

## Parameters

- **Mass, Damping:** Affects how heavy and responsive the cloth feels.
- **Sub Steps:** More steps improve stability but cost performance.
- **Compliance:** Lower values make constraints stiffer.

See the **ClothSimulation** example for a full setup.
