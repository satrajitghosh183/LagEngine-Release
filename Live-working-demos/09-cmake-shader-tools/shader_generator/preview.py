"""
Shader Preview Window
=====================

Real-time preview of generated shaders using ModernGL and pygame.
Renders on a rotating 3D sphere for better visualization.
"""

import numpy as np
from pathlib import Path
from typing import Optional
import time
import math
import platform

from .generator import ShaderPair


class ShaderPreview:
    """
    Real-time shader preview window.
    
    Renders shaders on a rotating 3D sphere with proper lighting.
    """
    
    def __init__(self, width: int = 800, height: int = 600, title: str = "Shader Preview"):
        self.width = width
        self.height = height
        self.title = title
    
    def run(self, shaders: ShaderPair, duration: Optional[float] = None) -> bool:
        """Run the preview window with the given shaders."""
        try:
            import pygame
            from pygame.locals import DOUBLEBUF, OPENGL
            import moderngl
        except ImportError as e:
            print(f"[ERROR] Missing dependency: {e}")
            print("   Install with: pip3 install pygame moderngl numpy")
            return False
        
        # Initialize pygame
        pygame.init()
        
        # Request OpenGL 3.3 Core on macOS
        if platform.system() == "Darwin":
            pygame.display.gl_set_attribute(pygame.GL_CONTEXT_MAJOR_VERSION, 3)
            pygame.display.gl_set_attribute(pygame.GL_CONTEXT_MINOR_VERSION, 3)
            pygame.display.gl_set_attribute(pygame.GL_CONTEXT_PROFILE_MASK, pygame.GL_CONTEXT_PROFILE_CORE)
            pygame.display.gl_set_attribute(pygame.GL_CONTEXT_FORWARD_COMPATIBLE_FLAG, 1)
        
        try:
            screen = pygame.display.set_mode((self.width, self.height), DOUBLEBUF | OPENGL)
            pygame.display.set_caption(self.title)
            ctx = moderngl.create_context()
        except Exception as e:
            print(f"[ERROR] Failed to create OpenGL context: {e}")
            pygame.quit()
            return False
        
        print(f"   OpenGL: {ctx.info['GL_VERSION']}")
        
        # Try to compile the user's shaders, fall back to test shader if it fails
        prog = self._try_compile_shaders(ctx, shaders)
        if prog is None:
            pygame.quit()
            return False
        
        # Create sphere geometry
        vao = self._create_sphere(ctx, prog)
        
        # Create texture
        texture = self._create_texture(ctx)
        
        # Main loop
        clock = pygame.time.Clock()
        start_time = time.time()
        running = True
        
        # Mouse/rotation state
        auto_rotate = True
        rot_x, rot_y = 0.3, 0.0
        zoom = 4.0
        dragging = False
        last_pos = (0, 0)
        
        print(f"\nShader Preview")
        print(f"   ESC: Exit | SPACE: Toggle rotation | Mouse: Drag to rotate | Scroll: Zoom")
        
        while running:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    running = False
                elif event.type == pygame.KEYDOWN:
                    if event.key == pygame.K_ESCAPE:
                        running = False
                    elif event.key == pygame.K_SPACE:
                        auto_rotate = not auto_rotate
                elif event.type == pygame.MOUSEBUTTONDOWN:
                    if event.button == 1:
                        dragging = True
                        last_pos = event.pos
                    elif event.button == 4:
                        zoom = max(2.0, zoom - 0.3)
                    elif event.button == 5:
                        zoom = min(10.0, zoom + 0.3)
                elif event.type == pygame.MOUSEBUTTONUP:
                    if event.button == 1:
                        dragging = False
                elif event.type == pygame.MOUSEMOTION:
                    if dragging:
                        dx = event.pos[0] - last_pos[0]
                        dy = event.pos[1] - last_pos[1]
                        rot_y += dx * 0.01
                        rot_x = max(-1.5, min(1.5, rot_x + dy * 0.01))
                        last_pos = event.pos
            
            elapsed = time.time() - start_time
            if duration and elapsed >= duration:
                running = False
            
            # Clear
            ctx.clear(0.1, 0.1, 0.15, 1.0)
            ctx.enable(ctx.DEPTH_TEST)
            
            # Update rotation
            if auto_rotate:
                current_rot_y = elapsed * 0.5
            else:
                current_rot_y = rot_y
            
            # Set uniforms
            camera_pos = (0.0, 0.0, zoom)
            self._set_uniforms(prog, elapsed, rot_x, current_rot_y, camera_pos, texture)
            
            # Render
            vao.render()
            
            pygame.display.flip()
            clock.tick(60)
        
        pygame.quit()
        return True
    
    def _try_compile_shaders(self, ctx, shaders: ShaderPair):
        """Try to compile user shaders, with fallback."""
        # First, try the user's shaders
        try:
            prog = ctx.program(
                vertex_shader=shaders.vertex,
                fragment_shader=shaders.fragment
            )
            print("   User shaders compiled")
            return prog
        except Exception as e:
            print(f"   User shader error: {str(e)[:100]}")
            print("   Using fallback shader...")
        
        # Fallback shader that always works
        fallback_vert = """#version 330 core
in vec3 in_position;
in vec3 in_normal;
in vec2 in_texcoord;

out vec3 v_normal;
out vec3 v_position;
out vec2 v_texcoord;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;

void main() {
    v_position = vec3(u_model * vec4(in_position, 1.0));
    v_normal = mat3(u_model) * in_normal;
    v_texcoord = in_texcoord;
    gl_Position = u_proj * u_view * u_model * vec4(in_position, 1.0);
}
"""
        fallback_frag = """#version 330 core
in vec3 v_normal;
in vec3 v_position;
in vec2 v_texcoord;

out vec4 f_color;

uniform float u_time;
uniform vec3 u_camera_pos;
uniform sampler2D u_texture;

void main() {
    vec3 norm = normalize(v_normal);
    vec3 light_dir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 view_dir = normalize(u_camera_pos - v_position);
    
    float diff = max(dot(norm, light_dir), 0.0);
    float fresnel = pow(1.0 - max(dot(view_dir, norm), 0.0), 3.0);
    
    vec3 base_color = texture(u_texture, v_texcoord).rgb;
    vec3 color = base_color * (0.2 + diff * 0.8);
    color += vec3(0.4, 0.7, 1.0) * fresnel * 0.6;
    
    f_color = vec4(color, 1.0);
}
"""
        try:
            prog = ctx.program(vertex_shader=fallback_vert, fragment_shader=fallback_frag)
            print("   Fallback shader loaded")
            return prog
        except Exception as e:
            print(f"   [ERROR] Fallback also failed: {e}")
            return None
    
    def _create_sphere(self, ctx, prog, radius=1.5, sectors=32, stacks=16):
        """Create a UV sphere."""
        vertices = []
        indices = []
        
        for i in range(stacks + 1):
            lat = math.pi * i / stacks
            sin_lat, cos_lat = math.sin(lat), math.cos(lat)
            
            for j in range(sectors + 1):
                lon = 2 * math.pi * j / sectors
                sin_lon, cos_lon = math.sin(lon), math.cos(lon)
                
                x = radius * sin_lat * cos_lon
                y = radius * cos_lat
                z = radius * sin_lat * sin_lon
                
                nx, ny, nz = sin_lat * cos_lon, cos_lat, sin_lat * sin_lon
                s, t = j / sectors, i / stacks
                
                vertices.extend([x, y, z, nx, ny, nz, s, t])
        
        for i in range(stacks):
            for j in range(sectors):
                k1 = i * (sectors + 1) + j
                k2, k3, k4 = k1 + 1, k1 + (sectors + 1), k1 + (sectors + 2)
                indices.extend([k1, k3, k2, k2, k3, k4])
        
        vertices = np.array(vertices, dtype='f4')
        indices = np.array(indices, dtype='i4')
        
        vbo = ctx.buffer(vertices.tobytes())
        ibo = ctx.buffer(indices.tobytes())
        
        # Find which attribute names the shader uses
        pos_name = 'aPos' if 'aPos' in prog else ('in_position' if 'in_position' in prog else None)
        norm_name = 'aNormal' if 'aNormal' in prog else ('in_normal' if 'in_normal' in prog else None)
        tex_name = 'aTexCoord' if 'aTexCoord' in prog else ('in_texcoord' if 'in_texcoord' in prog else None)
        
        # Build the vertex array based on what attributes exist in the shader
        if pos_name and norm_name and tex_name:
            # Shader uses all 3 attributes
            return ctx.vertex_array(prog, [(vbo, '3f 3f 2f', pos_name, norm_name, tex_name)], ibo)
        elif pos_name and tex_name:
            # Shader only uses position and texcoord (skip normal in buffer)
            # Repack vertices without normals: pos(3) + tex(2)
            verts_no_norm = []
            for i in range(0, len(vertices), 8):
                verts_no_norm.extend([vertices[i], vertices[i+1], vertices[i+2],  # pos
                                      vertices[i+6], vertices[i+7]])  # texcoord
            verts_no_norm = np.array(verts_no_norm, dtype='f4')
            vbo2 = ctx.buffer(verts_no_norm.tobytes())
            return ctx.vertex_array(prog, [(vbo2, '3f 2f', pos_name, tex_name)], ibo)
        elif pos_name and norm_name:
            # Shader uses position and normal
            verts_no_tex = []
            for i in range(0, len(vertices), 8):
                verts_no_tex.extend([vertices[i], vertices[i+1], vertices[i+2],  # pos
                                     vertices[i+3], vertices[i+4], vertices[i+5]])  # normal
            verts_no_tex = np.array(verts_no_tex, dtype='f4')
            vbo2 = ctx.buffer(verts_no_tex.tobytes())
            return ctx.vertex_array(prog, [(vbo2, '3f 3f', pos_name, norm_name)], ibo)
        elif pos_name:
            # Shader only uses position
            verts_pos_only = []
            for i in range(0, len(vertices), 8):
                verts_pos_only.extend([vertices[i], vertices[i+1], vertices[i+2]])
            verts_pos_only = np.array(verts_pos_only, dtype='f4')
            vbo2 = ctx.buffer(verts_pos_only.tobytes())
            return ctx.vertex_array(prog, [(vbo2, '3f', pos_name)], ibo)
        else:
            raise RuntimeError("Shader has no recognized position attribute (aPos or in_position)")
    
    def _create_texture(self, ctx, size=256):
        """Create a colorful gradient texture."""
        data = np.zeros((size, size, 4), dtype='u1')
        for y in range(size):
            for x in range(size):
                r = int(128 + 127 * math.sin(x * 0.1))
                g = int(128 + 127 * math.sin(y * 0.1))
                b = int(128 + 127 * math.sin((x + y) * 0.05))
                data[y, x] = [r, g, b, 255]
        
        texture = ctx.texture((size, size), 4, data.tobytes())
        texture.filter = (ctx.LINEAR, ctx.LINEAR)
        return texture
    
    def _set_uniforms(self, prog, elapsed, rot_x, rot_y, camera_pos, texture):
        """Set all shader uniforms."""
        # Matrices
        model = self._rotation_matrix(rot_x, rot_y)
        view = self._look_at(camera_pos, (0, 0, 0), (0, 1, 0))
        proj = self._perspective(math.radians(45), self.width / self.height, 0.1, 100)
        normal_mat = np.linalg.inv(model).T
        
        # Try both naming conventions
        uniform_values = {
            # New style (fallback shader)
            'u_model': model, 'u_view': view, 'u_proj': proj,
            'u_time': elapsed, 'u_camera_pos': camera_pos, 'u_texture': 0,
            # Old style (user shaders)
            'model': model, 'view': view, 'projection': proj,
            'normalMatrix': normal_mat, 'time': elapsed,
            'cameraPos': camera_pos, 'resolution': (float(self.width), float(self.height)),
            'texture0': 0,
        }
        
        for name, value in uniform_values.items():
            if name in prog:
                try:
                    if isinstance(value, np.ndarray):
                        prog[name].write(value.tobytes())
                    elif isinstance(value, tuple):
                        prog[name].value = value
                    else:
                        prog[name].value = value
                except:
                    pass
        
        texture.use(0)
    
    def _rotation_matrix(self, angle_x, angle_y):
        """Create rotation matrix."""
        cx, sx = math.cos(angle_x), math.sin(angle_x)
        cy, sy = math.cos(angle_y), math.sin(angle_y)
        
        rx = np.array([[1,0,0,0], [0,cx,-sx,0], [0,sx,cx,0], [0,0,0,1]], dtype='f4')
        ry = np.array([[cy,0,sy,0], [0,1,0,0], [-sy,0,cy,0], [0,0,0,1]], dtype='f4')
        return (ry @ rx).astype('f4')
    
    def _look_at(self, eye, center, up):
        """Create view matrix."""
        eye, center, up = np.array(eye, 'f4'), np.array(center, 'f4'), np.array(up, 'f4')
        f = center - eye
        f = f / np.linalg.norm(f)
        s = np.cross(f, up)
        s = s / np.linalg.norm(s)
        u = np.cross(s, f)
        m = np.eye(4, dtype='f4')
        m[0,:3], m[1,:3], m[2,:3] = s, u, -f
        m[3,:3] = [-np.dot(s, eye), -np.dot(u, eye), np.dot(f, eye)]
        return m.T
    
    def _perspective(self, fov, aspect, near, far):
        """Create perspective matrix."""
        f = 1.0 / math.tan(fov / 2.0)
        return np.array([
            [f/aspect, 0, 0, 0],
            [0, f, 0, 0],
            [0, 0, (far+near)/(near-far), -1],
            [0, 0, (2*far*near)/(near-far), 0]
        ], dtype='f4')


def preview_shader(shaders: ShaderPair, width: int = 800, height: int = 600, duration: Optional[float] = None) -> bool:
    """Convenience function to preview a shader."""
    return ShaderPreview(width, height).run(shaders, duration)


def preview_files(vertex_path: str, fragment_path: str, **kwargs) -> bool:
    """Preview shaders from files."""
    vertex = Path(vertex_path).read_text()
    fragment = Path(fragment_path).read_text()
    return preview_shader(ShaderPair(vertex=vertex, fragment=fragment), **kwargs)
