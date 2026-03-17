"""
OpenGL Shader Compiler
======================

Compiles GLSL shaders using OpenGL to validate them.
Uses ModernGL for a cleaner API, with fallback to PyOpenGL.
"""

from dataclasses import dataclass
from typing import Optional, Tuple
import re


@dataclass
class CompilationResult:
    """Result of shader compilation."""
    success: bool
    error_message: str = ""
    shader_type: str = ""  # "vertex" or "fragment"
    line_number: Optional[int] = None
    
    @property
    def has_error(self) -> bool:
        return not self.success


class ShaderCompiler:
    """
    Compiles GLSL shaders using OpenGL.
    
    Supports two backends:
    - ModernGL (preferred): Cleaner API, easier context management
    - PyOpenGL (fallback): More widely available
    """
    
    def __init__(self, backend: str = "auto"):
        """
        Initialize the shader compiler.
        
        Args:
            backend: "moderngl", "pyopengl", or "auto" (try moderngl first)
        """
        self.backend = backend
        self._ctx = None
        self._initialized = False
        self._backend_name = None
    
    def _init_moderngl(self) -> bool:
        """Initialize ModernGL backend."""
        try:
            import moderngl
            import platform
            
            # On macOS, we need to request a specific OpenGL version
            if platform.system() == "Darwin":
                # macOS requires explicit version request for core profile
                # Try OpenGL 4.1 first (max supported on macOS), then 3.3
                for version in [(4, 1), (3, 3)]:
                    try:
                        self._ctx = moderngl.create_standalone_context(
                            require=version[0] * 100 + version[1] * 10
                        )
                        self._backend_name = "moderngl"
                        return True
                    except Exception:
                        continue
                # Fallback to default
                self._ctx = moderngl.create_standalone_context()
            else:
                self._ctx = moderngl.create_standalone_context()
            
            self._backend_name = "moderngl"
            return True
        except Exception as e:
            return False
    
    def _init_pyopengl(self) -> bool:
        """Initialize PyOpenGL backend with hidden window."""
        try:
            # Try using GLFW for context creation
            import glfw
            from OpenGL.GL import (
                glCreateShader, glShaderSource, glCompileShader,
                glGetShaderiv, glGetShaderInfoLog, glDeleteShader,
                GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, GL_COMPILE_STATUS
            )
            
            if not glfw.init():
                return False
            
            # Create hidden window for OpenGL context
            glfw.window_hint(glfw.VISIBLE, glfw.FALSE)
            glfw.window_hint(glfw.CONTEXT_VERSION_MAJOR, 3)
            glfw.window_hint(glfw.CONTEXT_VERSION_MINOR, 3)
            glfw.window_hint(glfw.OPENGL_PROFILE, glfw.OPENGL_CORE_PROFILE)
            glfw.window_hint(glfw.OPENGL_FORWARD_COMPAT, glfw.TRUE)
            
            window = glfw.create_window(1, 1, "ShaderCompiler", None, None)
            if not window:
                glfw.terminate()
                return False
            
            glfw.make_context_current(window)
            self._ctx = window
            self._backend_name = "pyopengl"
            return True
            
        except Exception as e:
            return False
    
    def _init_osmesa(self) -> bool:
        """Initialize OSMesa backend (software rendering, no display needed)."""
        try:
            from OpenGL import osmesa
            from OpenGL.GL import (
                glCreateShader, glShaderSource, glCompileShader,
                glGetShaderiv, glGetShaderInfoLog, glDeleteShader,
                GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, GL_COMPILE_STATUS
            )
            
            # Create OSMesa context
            ctx = osmesa.OSMesaCreateContext(osmesa.OSMESA_RGBA, None)
            if not ctx:
                return False
            
            # Create a small buffer
            import ctypes
            buffer = (ctypes.c_ubyte * (4 * 4 * 4))()
            
            if not osmesa.OSMesaMakeCurrent(ctx, buffer, osmesa.GL_UNSIGNED_BYTE, 4, 4):
                osmesa.OSMesaDestroyContext(ctx)
                return False
            
            self._ctx = ctx
            self._backend_name = "osmesa"
            return True
            
        except Exception:
            return False
    
    def initialize(self) -> bool:
        """
        Initialize the OpenGL context.
        
        Returns:
            True if initialization succeeded
        """
        if self._initialized:
            return True
        
        backends_to_try = []
        
        if self.backend == "auto":
            backends_to_try = [
                ("moderngl", self._init_moderngl),
                ("pyopengl", self._init_pyopengl),
                ("osmesa", self._init_osmesa),
            ]
        elif self.backend == "moderngl":
            backends_to_try = [("moderngl", self._init_moderngl)]
        elif self.backend == "pyopengl":
            backends_to_try = [("pyopengl", self._init_pyopengl)]
        elif self.backend == "osmesa":
            backends_to_try = [("osmesa", self._init_osmesa)]
        
        for name, init_func in backends_to_try:
            if init_func():
                self._initialized = True
                return True
        
        return False
    
    def compile_vertex(self, source: str) -> CompilationResult:
        """Compile a vertex shader."""
        return self._compile(source, "vertex")
    
    def compile_fragment(self, source: str) -> CompilationResult:
        """Compile a fragment shader."""
        return self._compile(source, "fragment")
    
    def _compile(self, source: str, shader_type: str) -> CompilationResult:
        """
        Compile a shader.
        
        Args:
            source: GLSL source code
            shader_type: "vertex" or "fragment"
            
        Returns:
            CompilationResult with success status and any error message
        """
        if not self._initialized:
            if not self.initialize():
                return CompilationResult(
                    success=False,
                    error_message="Failed to initialize OpenGL context. "
                                  "Install moderngl or PyOpenGL+GLFW.",
                    shader_type=shader_type
                )
        
        if self._backend_name == "moderngl":
            return self._compile_moderngl(source, shader_type)
        else:
            return self._compile_pyopengl(source, shader_type)
    
    def _compile_moderngl(self, source: str, shader_type: str) -> CompilationResult:
        """Compile using ModernGL."""
        try:
            # Ensure proper version directive for macOS compatibility
            # macOS requires #version 330 core or #version 410 core
            if "#version" not in source:
                source = "#version 330 core\n" + source
            
            if shader_type == "vertex":
                # For vertex shader validation, create a fragment shader that
                # consumes all the outputs from the vertex shader
                # Extract 'out' declarations from vertex shader
                import re
                out_vars = re.findall(r'out\s+(\w+)\s+(\w+)\s*;', source)
                
                # Build fragment shader that uses all vertex outputs
                frag_inputs = "\n".join([f"in {t} {n};" for t, n in out_vars])
                frag_usage = " + ".join([f"{n}.x" if t == "vec3" else (f"{n}.x" if t == "vec2" else n) for t, n in out_vars]) if out_vars else "0.0"
                
                minimal_frag = f"""#version 330 core
out vec4 FragColor;
{frag_inputs}
void main() {{ 
    float dummy = {frag_usage};
    FragColor = vec4(1.0); 
}}
"""
                self._ctx.program(vertex_shader=source, fragment_shader=minimal_frag)
            else:
                # For fragment shader validation, create a vertex shader that
                # provides all the inputs the fragment shader needs
                # Extract 'in' declarations from fragment shader
                import re
                in_vars = re.findall(r'in\s+(\w+)\s+(\w+)\s*;', source)
                
                # Build vertex shader that provides all fragment inputs
                vert_outputs = "\n".join([f"out {t} {n};" for t, n in in_vars])
                vert_assignments = "\n    ".join([
                    f"{n} = {'aTexCoord' if t == 'vec2' else ('vec3(model * vec4(aPos, 1.0))' if 'Pos' in n else 'aNormal')};" 
                    for t, n in in_vars
                ])
                
                minimal_vert = f"""#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;

{vert_outputs}

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {{
    {vert_assignments if vert_assignments else "// no outputs needed"}
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}}
"""
                self._ctx.program(vertex_shader=minimal_vert, fragment_shader=source)
            
            return CompilationResult(success=True, shader_type=shader_type)
            
        except Exception as e:
            error_msg = str(e)
            
            # Check if it's just a warning about unused variables (not a real error)
            if "WARNING" in error_msg and "not read by" in error_msg:
                # This is just a warning, not an error - shader is valid
                return CompilationResult(success=True, shader_type=shader_type)
            
            line_num = self._extract_line_number(error_msg)
            return CompilationResult(
                success=False,
                error_message=error_msg,
                shader_type=shader_type,
                line_number=line_num
            )
    
    def _compile_pyopengl(self, source: str, shader_type: str) -> CompilationResult:
        """Compile using PyOpenGL."""
        from OpenGL.GL import (
            glCreateShader, glShaderSource, glCompileShader,
            glGetShaderiv, glGetShaderInfoLog, glDeleteShader,
            GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, GL_COMPILE_STATUS
        )
        
        gl_type = GL_VERTEX_SHADER if shader_type == "vertex" else GL_FRAGMENT_SHADER
        shader = None
        
        try:
            shader = glCreateShader(gl_type)
            glShaderSource(shader, source)
            glCompileShader(shader)
            
            status = glGetShaderiv(shader, GL_COMPILE_STATUS)
            
            if status:
                return CompilationResult(success=True, shader_type=shader_type)
            else:
                error_log = glGetShaderInfoLog(shader)
                if isinstance(error_log, bytes):
                    error_log = error_log.decode('utf-8')
                
                line_num = self._extract_line_number(error_log)
                return CompilationResult(
                    success=False,
                    error_message=error_log,
                    shader_type=shader_type,
                    line_number=line_num
                )
                
        except Exception as e:
            return CompilationResult(
                success=False,
                error_message=str(e),
                shader_type=shader_type
            )
        finally:
            if shader:
                glDeleteShader(shader)
    
    def _extract_line_number(self, error_msg: str) -> Optional[int]:
        """Extract line number from error message."""
        # Common patterns: "0(15):", "line 15:", "at line 15"
        patterns = [
            r"0\((\d+)\)",
            r"line\s+(\d+)",
            r":(\d+):",
        ]
        
        for pattern in patterns:
            match = re.search(pattern, error_msg, re.IGNORECASE)
            if match:
                return int(match.group(1))
        
        return None
    
    def validate_pair(
        self,
        vertex_source: str,
        fragment_source: str
    ) -> Tuple[CompilationResult, CompilationResult]:
        """
        Validate a vertex/fragment shader pair together.
        
        This is more accurate than validating separately since it
        checks that outputs/inputs match between shaders.
        
        Returns:
            Tuple of (vertex_result, fragment_result)
        """
        if not self._initialized:
            if not self.initialize():
                error = CompilationResult(
                    success=False,
                    error_message="Failed to initialize OpenGL context"
                )
                return error, error
        
        if self._backend_name == "moderngl":
            return self._validate_pair_moderngl(vertex_source, fragment_source)
        else:
            # Fallback to separate validation
            vert_result = self.compile_vertex(vertex_source)
            frag_result = self.compile_fragment(fragment_source)
            return vert_result, frag_result
    
    def _validate_pair_moderngl(
        self,
        vertex_source: str,
        fragment_source: str
    ) -> Tuple[CompilationResult, CompilationResult]:
        """Validate shader pair together using ModernGL."""
        try:
            # Ensure version directives
            if "#version" not in vertex_source:
                vertex_source = "#version 330 core\n" + vertex_source
            if "#version" not in fragment_source:
                fragment_source = "#version 330 core\n" + fragment_source
            
            # Try to compile them together
            self._ctx.program(
                vertex_shader=vertex_source,
                fragment_shader=fragment_source
            )
            
            # Both succeeded
            return (
                CompilationResult(success=True, shader_type="vertex"),
                CompilationResult(success=True, shader_type="fragment")
            )
            
        except Exception as e:
            error_msg = str(e)
            
            # Check if it's just a warning (not a real error)
            if "WARNING" in error_msg and "ERROR" not in error_msg:
                return (
                    CompilationResult(success=True, shader_type="vertex"),
                    CompilationResult(success=True, shader_type="fragment")
                )
            
            # Try to determine which shader has the error
            if "vertex" in error_msg.lower() or "vertex_shader" in error_msg.lower():
                return (
                    CompilationResult(
                        success=False,
                        error_message=error_msg,
                        shader_type="vertex",
                        line_number=self._extract_line_number(error_msg)
                    ),
                    CompilationResult(success=True, shader_type="fragment")
                )
            elif "fragment" in error_msg.lower() or "fragment_shader" in error_msg.lower():
                return (
                    CompilationResult(success=True, shader_type="vertex"),
                    CompilationResult(
                        success=False,
                        error_message=error_msg,
                        shader_type="fragment",
                        line_number=self._extract_line_number(error_msg)
                    )
                )
            else:
                # Can't determine, report as fragment error (more common)
                return (
                    CompilationResult(success=True, shader_type="vertex"),
                    CompilationResult(
                        success=False,
                        error_message=error_msg,
                        shader_type="fragment",
                        line_number=self._extract_line_number(error_msg)
                    )
                )
    
    def cleanup(self):
        """Clean up OpenGL resources."""
        if self._backend_name == "pyopengl" and self._ctx:
            try:
                import glfw
                glfw.destroy_window(self._ctx)
                glfw.terminate()
            except:
                pass
        self._ctx = None
        self._initialized = False


class BasicValidator:
    """
    Basic shader validation without OpenGL.
    
    Use this when OpenGL is not available. It catches common
    syntax errors but cannot validate full GLSL semantics.
    """
    
    @staticmethod
    def validate(source: str, shader_type: str) -> CompilationResult:
        """
        Perform basic validation on shader source.
        
        Args:
            source: GLSL source code
            shader_type: "vertex" or "fragment"
            
        Returns:
            CompilationResult with validation status
        """
        errors = []
        
        # Check for version directive
        if "#version" not in source:
            errors.append("Missing #version directive")
        
        # Check for main function
        if "void main()" not in source and "void main(void)" not in source:
            errors.append("Missing main() function")
        
        # Check for required outputs
        if shader_type == "vertex":
            if "gl_Position" not in source:
                errors.append("Vertex shader must set gl_Position")
        elif shader_type == "fragment":
            if "out vec4" not in source and "out vec3" not in source:
                errors.append("Fragment shader missing output variable")
        
        # Check for deprecated functions
        deprecated_funcs = ["texture2D", "texture3D", "textureCube", "shadow2D"]
        for func in deprecated_funcs:
            if func + "(" in source:
                errors.append(f"Deprecated function '{func}' - use 'texture()' instead")
        
        # Check for deprecated keywords
        if re.search(r'\battribute\s+', source):
            errors.append("'attribute' is deprecated - use 'in'")
        if re.search(r'\bvarying\s+', source):
            errors.append("'varying' is deprecated - use 'in'/'out'")
        
        # Check for gl_FragColor (deprecated in core profile)
        if "gl_FragColor" in source:
            errors.append("'gl_FragColor' is deprecated - declare 'out vec4 FragColor'")
        
        # Check bracket balance
        open_braces = source.count("{")
        close_braces = source.count("}")
        if open_braces != close_braces:
            errors.append(f"Unbalanced braces: {open_braces} '{{' vs {close_braces} '}}'")
        
        open_parens = source.count("(")
        close_parens = source.count(")")
        if open_parens != close_parens:
            errors.append(f"Unbalanced parentheses: {open_parens} '(' vs {close_parens} ')'")
        
        if errors:
            return CompilationResult(
                success=False,
                error_message="; ".join(errors),
                shader_type=shader_type
            )
        
        return CompilationResult(success=True, shader_type=shader_type)
