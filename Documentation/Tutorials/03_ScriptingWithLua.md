# Tutorial 3: Scripting with Lua

Use the Lua scripting console to move an entity and react to input.

## Steps

1. **Create an entity** with Transform.
2. **Open the Scripting Console** (Window → Scripting Console).
3. **Get the scene and an entity:**
   ```lua
   local scene = GetScene()
   local e = scene:GetEntityByName("Cube")
   if e and e:HasComponent("Transform") then
       local t = e:GetComponent("Transform")
       t.Position = {1, 0, 0}
   end
   ```
4. **Use `print()`** – output appears in the Scripting Console.
5. **Add a Script Component** to an entity and set a `.lua` file path. The script runs in play mode (OnStart, OnUpdate, OnDestroy).

## API Reference

See [ScriptingAPI.md](../Manual/ScriptingAPI.md) for full Lua API.
