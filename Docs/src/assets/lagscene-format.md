# `.lagscene` Text Format

Godot-inspired human-readable scene format. Stable, diffable, VCS-friendly.

## Structure

```
[lagscene format=1 uid="uid://abc123"]

[ext_resource type="Mesh" path="res://meshes/cube.obj" id="1_mesh"]
[ext_resource type="Material" uid="uid://def456" id="2_mat"]

[sub_resource type="SpriteFrames" id="SpriteFrames_1"]
frames = [1, 2, 3]

[node name="Player" type="Node3D"]
transform = Transform(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0)

[node name="Mesh" type="MeshInstance" parent="Player"]
mesh = ExtResource("1_mesh")
material = ExtResource("2_mat")
```

## Sections

| Section | Header attrs | Meaning |
|---------|--------------|---------|
| `lagscene` | `format`, `uid` | File metadata (required, first) |
| `ext_resource` | `type`, `path` \| `uid`, `id` | Reference to an external asset |
| `sub_resource` | `type`, `id` | Embedded resource with properties |
| `node` | `name`, `type`, `parent`, `instance` | Scene node |

## Property value types

```
// Primitives
value = 42
value = 3.14
value = true
value = "hello"
value = null

// Vectors (Y-up, meters)
position = Vector3(1.0, 2.0, 3.0)
rotation = Quat(0, 0, 0, 1)

// Transform (3x4 matrix, column-major — 9 basis floats + 3 origin)
transform = Transform(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0)

// Resource references
mesh = ExtResource("1_mesh")
material = SubResource("MaterialData_1")
shared = UID("uid://abc123")

// Collections
tags = ["enemy", "humanoid"]
stats = {"health": 100, "mana": 50}
```

## Round-trip guarantee

`LagsceneSerializer::Parse` and `LagsceneSerializer::Write` are inverses —
load → save → load produces identical output. Safe for version control.
