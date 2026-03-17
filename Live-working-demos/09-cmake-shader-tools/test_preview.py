#!/usr/bin/env python3
"""
Simple test script to verify the preview window works.
Run: python3 test_preview.py
"""

import pygame
from pygame.locals import DOUBLEBUF, OPENGL
import moderngl
import numpy as np
import math
import time
import platform

def main():
    print("Testing Shader Preview...")
    print(f"   Platform: {platform.system()}")
    
    # Initialize pygame
    pygame.init()
    
    width, height = 800, 600
    
    # Request OpenGL 3.3 Core on macOS
    if platform.system() == "Darwin":
        pygame.display.gl_set_attribute(pygame.GL_CONTEXT_MAJOR_VERSION, 3)
        pygame.display.gl_set_attribute(pygame.GL_CONTEXT_MINOR_VERSION, 3)
        pygame.display.gl_set_attribute(pygame.GL_CONTEXT_PROFILE_MASK, pygame.GL_CONTEXT_PROFILE_CORE)
        pygame.display.gl_set_attribute(pygame.GL_CONTEXT_FORWARD_COMPATIBLE_FLAG, 1)
    
    screen = pygame.display.set_mode((width, height), DOUBLEBUF | OPENGL)
    pygame.display.set_caption("Shader Test")
    
    # Create ModernGL context
    ctx = moderngl.create_context()
    print(f"   OpenGL: {ctx.info['GL_VERSION']}")
    print(f"   Renderer: {ctx.info['GL_RENDERER']}")
    
    # Simple vertex shader
    vert_shader = """
    #version 330 core
    
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
    
    # Simple fragment shader with lighting
    frag_shader = """
    #version 330 core
    
    in vec3 v_normal;
    in vec3 v_position;
    in vec2 v_texcoord;
    
    out vec4 f_color;
    
    uniform float u_time;
    uniform vec3 u_camera_pos;
    
    void main() {
        vec3 norm = normalize(v_normal);
        vec3 light_dir = normalize(vec3(1.0, 1.0, 1.0));
        vec3 view_dir = normalize(u_camera_pos - v_position);
        
        // Diffuse
        float diff = max(dot(norm, light_dir), 0.0);
        
        // Fresnel edge glow
        float fresnel = pow(1.0 - max(dot(view_dir, norm), 0.0), 3.0);
        
        // Animated base color
        vec3 base_color = vec3(
            sin(u_time * 0.5) * 0.3 + 0.5,
            sin(u_time * 0.7 + 2.0) * 0.3 + 0.4,
            sin(u_time * 0.3 + 4.0) * 0.3 + 0.6
        );
        
        vec3 color = base_color * (0.2 + diff * 0.8);
        color += vec3(0.4, 0.7, 1.0) * fresnel * 0.8;
        
        f_color = vec4(color, 1.0);
    }
    """
    
    # Create shader program
    try:
        prog = ctx.program(vertex_shader=vert_shader, fragment_shader=frag_shader)
        print("   Shaders compiled successfully")
    except Exception as e:
        print(f"   [ERROR] Shader error: {e}")
        pygame.quit()
        return
    
    # Create sphere geometry
    def create_sphere(radius=1.0, sectors=32, stacks=16):
        vertices = []
        indices = []
        
        for i in range(stacks + 1):
            lat = math.pi * i / stacks
            sin_lat = math.sin(lat)
            cos_lat = math.cos(lat)
            
            for j in range(sectors + 1):
                lon = 2 * math.pi * j / sectors
                sin_lon = math.sin(lon)
                cos_lon = math.cos(lon)
                
                x = radius * sin_lat * cos_lon
                y = radius * cos_lat
                z = radius * sin_lat * sin_lon
                
                nx, ny, nz = sin_lat * cos_lon, cos_lat, sin_lat * sin_lon
                s, t = j / sectors, i / stacks
                
                vertices.extend([x, y, z, nx, ny, nz, s, t])
        
        for i in range(stacks):
            for j in range(sectors):
                k1 = i * (sectors + 1) + j
                k2 = k1 + 1
                k3 = k1 + (sectors + 1)
                k4 = k3 + 1
                indices.extend([k1, k3, k2, k2, k3, k4])
        
        return np.array(vertices, dtype='f4'), np.array(indices, dtype='i4')
    
    vertices, indices = create_sphere(radius=1.5)
    print(f"   Sphere: {len(vertices)//8} vertices, {len(indices)//3} triangles")
    
    vbo = ctx.buffer(vertices.tobytes())
    ibo = ctx.buffer(indices.tobytes())
    
    vao = ctx.vertex_array(prog, [(vbo, '3f 3f 2f', 'in_position', 'in_normal', 'in_texcoord')], ibo)
    
    # Matrix helpers
    def perspective(fov, aspect, near, far):
        f = 1.0 / math.tan(fov / 2.0)
        return np.array([
            [f/aspect, 0, 0, 0],
            [0, f, 0, 0],
            [0, 0, (far+near)/(near-far), -1],
            [0, 0, (2*far*near)/(near-far), 0]
        ], dtype='f4')
    
    def look_at(eye, center, up):
        f = np.array(center) - np.array(eye)
        f = f / np.linalg.norm(f)
        s = np.cross(f, up)
        s = s / np.linalg.norm(s)
        u = np.cross(s, f)
        m = np.eye(4, dtype='f4')
        m[0,:3] = s
        m[1,:3] = u
        m[2,:3] = -f
        m[3,:3] = [-np.dot(s,eye), -np.dot(u,eye), np.dot(f,eye)]
        return m.T
    
    def rotation_y(angle):
        c, s = math.cos(angle), math.sin(angle)
        return np.array([
            [c, 0, s, 0],
            [0, 1, 0, 0],
            [-s, 0, c, 0],
            [0, 0, 0, 1]
        ], dtype='f4')
    
    # Main loop
    clock = pygame.time.Clock()
    start_time = time.time()
    running = True
    
    print("\nPreview running!")
    print("   Press ESC to exit")
    
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    running = False
        
        elapsed = time.time() - start_time
        
        # Clear
        ctx.clear(0.1, 0.1, 0.15, 1.0)
        ctx.enable(ctx.DEPTH_TEST)
        
        # Set uniforms
        camera_pos = (0.0, 0.0, 5.0)
        
        prog['u_model'].write(rotation_y(elapsed * 0.5).tobytes())
        prog['u_view'].write(look_at(camera_pos, (0,0,0), (0,1,0)).tobytes())
        prog['u_proj'].write(perspective(math.radians(45), width/height, 0.1, 100).tobytes())
        prog['u_time'].value = elapsed
        prog['u_camera_pos'].value = camera_pos
        
        # Render
        vao.render()
        
        pygame.display.flip()
        clock.tick(60)
    
    pygame.quit()
    print("Test complete!")

if __name__ == "__main__":
    main()
