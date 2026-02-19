# Tutorial 4: Custom Shaders

Write a vertex and fragment shader and assign it to a material.

## Steps

1. **Create shader files** in `Assets/Shaders/`:
   - `my_shader.vert` – basic vertex shader (position, normals, UVs).
   - `my_shader.frag` – fragment shader (e.g. Phong or toon lighting).
2. **Create a Material** in the Editor or via code.
3. **Assign the shader** to the material (Shader path: `Assets/Shaders/my_shader` or similar).
4. **Assign the material** to a Mesh Renderer component.
5. **Use shader hot-reload** – edit the shader and save; the Editor reloads it automatically.

## Shader Layout

The engine expects:
- `uniform mat4 u_ViewProjection`
- `uniform mat4 u_Model`
- Vertex attributes: `a_Position`, `a_Normal`, `a_TexCoord` (or engine equivalents).
- See existing shaders in `Assets/Shaders/` for reference.
