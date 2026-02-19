-- Example Lua script demonstrating Wicked Engine-style API usage
-- This script can be executed in the scripting console or saved as startup.lua

print("=== Wicked Style Example Script ===")

-- Get current scene
local currentScene = scene.GetScene()
print("Current scene: " .. (currentScene.name or "Unknown"))

-- Example: Load a model (uncomment to use)
-- local modelEntity = scene.LoadModel("Assets/Models/cube.obj")
-- print("Loaded model entity ID: " .. tostring(modelEntity))

-- Create RenderPath3D
local path3D = RenderPath3D.new()
path3D.setFXAAEnabled(true)
path3D.setBloomEnabled(true)
path3D.setSSAOEnabled(false)

-- Set scene on render path
if currentScene then
    path3D.setScene(currentScene)
end

-- Activate render path
application.SetActivePath(path3D)
print("RenderPath3D activated with FXAA and Bloom")

-- Example: Audio (uncomment to use)
-- local sound = audio.CreateSound("Assets/Audio/example.wav")
-- if sound then
--     local instance = audio.CreateSoundInstance(sound)
--     audio.SetVolume(0.5, instance)
--     audio.Play(instance)
--     print("Playing audio")
-- end

-- Example: Input handling
print("\nInput examples:")
print("  Press SPACE to see a message")
print("  Hold W to see continuous messages")

-- Example update function (would be called each frame in a real game loop)
function Update()
    -- Check for key press (just pressed this frame)
    if input.Press(input.KEYBOARD_BUTTON_SPACE) then
        print("Space key pressed!")
    end
    
    -- Check for key held down
    if input.Down(input.KEYBOARD_BUTTON_W) then
        -- print("W key held down")
    end
end

-- Example: Math operations
print("\nMath examples:")
local v1 = Vec3(1.0, 2.0, 3.0)
local v2 = Vec3(4.0, 5.0, 6.0)
local sum = Vec3_Add(v1, v2)
print("Vec3 addition: (" .. v1.x .. ", " .. v1.y .. ", " .. v1.z .. ") + (" .. v2.x .. ", " .. v2.y .. ", " .. v2.z .. ")")

local q = Quat_FromEuler(Vec3(90, 0, 0))
print("Quaternion from Euler angles (90, 0, 0)")

-- Example: Introspection
print("\nUsing getprops() to inspect objects:")
getprops(currentScene)
getprops(path3D)

print("\n=== Script loaded successfully ===")
