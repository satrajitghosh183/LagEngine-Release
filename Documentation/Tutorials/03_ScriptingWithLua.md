# Tutorial 3: Scripting with Lua

Use the Lua scripting console to move an entity and react to input.

## Steps

1. **Create an entity** with Transform.
2. **Open the Scripting Console** (Window → Scripting Console).
3. **Get position and move an entity:**
   ```lua
   -- Get the current position of entity 0
   local x, y, z = Transform_GetPosition(0)
   Log("Position: " .. x .. ", " .. y .. ", " .. z)

   -- Move it to a new position
   Transform_SetPosition(0, 1, 0, 0)

   -- Or translate relative to current position
   Transform_Translate(0, 0.5, 0, 0)
   ```
4. **Use `print()`** – output appears in the Scripting Console.
5. **Add a Script Component** to an entity and set a `.lua` file path. The script runs in play mode (OnStart, OnUpdate, OnDestroy).

## API Reference

See [ScriptingAPI.md](../Manual/ScriptingAPI.md) for full Lua API.
