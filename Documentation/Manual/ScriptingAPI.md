
# Lua Scripting API Reference

## Overview

The GameEngine provides a comprehensive Lua scripting API inspired by Wicked Engine, allowing you to control the application, load models, play audio, handle input, and more.

## Global Objects

### `application`

The main application object.

```lua
-- Get active render path
local path = application.GetActivePath()

-- Set active render path
application.SetActivePath(renderPath, fadeSeconds)
```

### `scene`

Scene management functions.

```lua
-- Get current global scene
local currentScene = scene.GetScene()

-- Load model into global scene
local entity = scene.LoadModel("Assets/Models/cube.obj")

-- Load model into specific scene
local entity = scene.LoadModel(currentScene, "Assets/Models/sphere.obj")
```

### `audio`

Audio playback functions.

```lua
-- Create sound
local sound = audio.CreateSound("Assets/Audio/music.wav")

-- Create sound instance
local instance = audio.CreateSoundInstance(sound)

-- Play sound
audio.Play(instance)

-- Stop sound
audio.Stop(instance)

-- Set volume (master or instance)
audio.SetVolume(0.5)  -- Master volume
audio.SetVolume(0.8, instance)  -- Instance volume
```

### `input`

Input state checking.

```lua
-- Check if key was just pressed this frame
if input.Press(input.KEYBOARD_BUTTON_SPACE) then
    print("Space pressed!")
end

-- Check if key is currently held down
if input.Down(input.KEYBOARD_BUTTON_W) then
    print("W key held")
end
```

### Key Constants

```lua
input.KEYBOARD_BUTTON_SPACE
input.KEYBOARD_BUTTON_LEFT
input.KEYBOARD_BUTTON_RIGHT
input.KEYBOARD_BUTTON_UP
input.KEYBOARD_BUTTON_DOWN
input.KEYBOARD_BUTTON_ESCAPE
input.KEYBOARD_BUTTON_ENTER
input.KEYBOARD_BUTTON_TAB
input.KEYBOARD_BUTTON_HOME
input.KEYBOARD_BUTTON_F1  -- through F12
input.MOUSE_BUTTON_LEFT
input.MOUSE_BUTTON_RIGHT
input.MOUSE_BUTTON_MIDDLE
```

## Render Paths

### RenderPath3D

```lua
-- Create RenderPath3D
local path3D = RenderPath3D.new()

-- Set post-process options
path3D.setFXAAEnabled(true)
path3D.setBloomEnabled(true)
path3D.setSSAOEnabled(false)

-- Set scene
path3D.setScene(currentScene)

-- Activate
application.SetActivePath(path3D)
```

### RenderPath2D

```lua
-- Create RenderPath2D
local path2D = RenderPath2D.new()

-- Activate
application.SetActivePath(path2D)
```

## Math Types

### Vec3

```lua
-- Create vector
local v = Vec3(1.0, 2.0, 3.0)

-- Operations
local sum = Vec3_Add(v1, v2)
local diff = Vec3_Sub(v1, v2)
local scaled = Vec3_Mul(v, 2.0)
local dot = Vec3_Dot(v1, v2)
local cross = Vec3_Cross(v1, v2)
local normalized = Vec3_Normalize(v)
local length = Vec3_Length(v)
local distance = Vec3_Distance(v1, v2)
local lerped = Vec3_Lerp(v1, v2, t)
```

### Quat

```lua
-- Create quaternion
local q = Quat(0, 0, 0, 1)  -- Identity

-- From Euler angles
local q = Quat_FromEuler(Vec3(90, 0, 0))

-- To Euler angles
local euler = Quat_ToEuler(q)

-- From axis-angle
local q = Quat_FromAxisAngle(Vec3(0, 1, 0), 90)

-- Spherical interpolation
local q = Quat_Slerp(q1, q2, t)

-- Multiply quaternions
local q = Quat_Mul(q1, q2)

-- Rotate vector
local rotated = Quat_RotateVec3(q, v)
```

## Utility Functions

### `getprops()`

Introspect Lua objects to see their properties and methods.

```lua
getprops(someObject)  -- Prints object structure to console
```

### Logging

```lua
Log("Info message")
LogWarning("Warning message")
LogError("Error message")
```

## Example Script

```lua
-- startup.lua example

-- Load a model
local cube = scene.LoadModel("Assets/Models/cube.obj")

-- Create a render path
local path3D = RenderPath3D.new()
path3D.setFXAAEnabled(true)
path3D.setBloomEnabled(true)
application.SetActivePath(path3D)

-- Play background music
local music = audio.CreateSound("Assets/Audio/background.wav")
local musicInstance = audio.CreateSoundInstance(music)
audio.SetVolume(0.5, musicInstance)
audio.Play(musicInstance)

-- Input handling example
function Update()
    if input.Press(input.KEYBOARD_BUTTON_SPACE) then
        print("Space pressed!")
    end
    
    if input.Down(input.KEYBOARD_BUTTON_W) then
        -- Move forward
    end
end
```
