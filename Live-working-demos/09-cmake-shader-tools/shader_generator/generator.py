"""
GLSL Shader Generator
=====================

Main module for generating GLSL shaders from natural language descriptions.
"""

import re
from pathlib import Path
from typing import Optional, List
from dataclasses import dataclass, field

from .ollama_client import OllamaClient, OllamaConfig
from .compiler import ShaderCompiler, BasicValidator, CompilationResult
from .shader_library import search_shaders, ShaderExample, SHADER_LIBRARY


@dataclass
class ShaderPair:
    """A pair of vertex and fragment shaders."""
    vertex: str
    fragment: str
    
    def save(self, base_path: str, name: str = "shader"):
        """
        Save shaders to files.
        
        Args:
            base_path: Directory to save to
            name: Base name for the shader files
        """
        path = Path(base_path)
        path.mkdir(parents=True, exist_ok=True)
        
        (path / f"{name}.vert").write_text(self.vertex)
        (path / f"{name}.frag").write_text(self.fragment)
    
    @classmethod
    def load(cls, base_path: str, name: str = "shader") -> "ShaderPair":
        """Load shaders from files."""
        path = Path(base_path)
        vertex = (path / f"{name}.vert").read_text()
        fragment = (path / f"{name}.frag").read_text()
        return cls(vertex=vertex, fragment=fragment)


@dataclass
class GenerationResult:
    """Result of shader generation."""
    shaders: Optional[ShaderPair] = None
    success: bool = False
    error: str = ""
    attempts: int = 0
    raw_response: str = ""
    compilation_errors: List[str] = field(default_factory=list)
    
    @property
    def failed(self) -> bool:
        return not self.success


class ShaderGenerator:
    """
    Generate GLSL shaders from natural language descriptions.
    
    Uses Ollama for LLM inference and optionally validates
    generated shaders using OpenGL compilation.
    
    Example:
        generator = ShaderGenerator()
        result = generator.generate("animated fire effect")
        
        if result.success:
            result.shaders.save("output/", "fire")
    """
    
    # Default prompt template - loaded from file or uses minimal fallback
    DEFAULT_PROMPT = '''You are a GLSL shader expert. Generate OpenGL 3.3 shaders.

Output ONLY shader code in this format:
```vertex
#version 330 core
// vertex shader
```

```fragment
#version 330 core
// fragment shader
```

Inputs: aPos (vec3), aTexCoord (vec2), aNormal (vec3)
Uniforms: model, view, projection (mat4), time (float), cameraPos (vec3)
Output: FragColor (vec4)

Rules: Use mod() not %, use 1.0 not 1, use texture() not texture2D()

User: {USER_PROMPT}
'''
    
    ERROR_FIX_TEMPLATE = '''The shader failed to compile with this error:

--- COMPILER ERROR ---
{error}
--- END ERROR ---

Here is the problematic {shader_type} shader:

```{shader_type_lower}
{shader_code}
```

Fix ONLY the compilation error. Output the corrected shader:

```{shader_type_lower}
#version 330 core
// corrected code
```

Do not output any explanation, only the corrected shader code block.
'''
    
    def __init__(
        self,
        ollama_config: Optional[OllamaConfig] = None,
        prompt_file: Optional[str] = None,
        use_opengl_validation: bool = True,
        max_retries: int = 3,
        verbose: bool = True
    ):
        """
        Initialize the shader generator.
        
        Args:
            ollama_config: Configuration for Ollama client
            prompt_file: Path to custom prompt template file
            use_opengl_validation: Whether to use real OpenGL compilation
            max_retries: Maximum number of generation/fix attempts
            verbose: Whether to print progress messages
        """
        self.client = OllamaClient(ollama_config)
        self.max_retries = max_retries
        self.verbose = verbose
        self.use_opengl_validation = use_opengl_validation
        
        # Load prompt template - try default location first
        if prompt_file and Path(prompt_file).exists():
            self.prompt_template = Path(prompt_file).read_text()
        else:
            # Try to find COMPLETE_PROMPT.txt relative to this file
            default_prompt_path = Path(__file__).parent.parent / "files" / "COMPLETE_PROMPT.txt"
            if default_prompt_path.exists():
                self.prompt_template = default_prompt_path.read_text()
            else:
                self.prompt_template = self.DEFAULT_PROMPT
        
        # Initialize compiler if using OpenGL validation
        self._compiler: Optional[ShaderCompiler] = None
        if use_opengl_validation:
            self._compiler = ShaderCompiler()
    
    def _log(self, message: str):
        """Print message if verbose mode is enabled."""
        if self.verbose:
            print(message)
    
    def check_ollama(self) -> bool:
        """
        Check if Ollama is available and has the required model.
        
        Returns:
            True if ready to generate
        """
        if not self.client.is_available():
            self._log("[ERROR] Cannot connect to Ollama. Is it running?")
            self._log("   Start it with: ollama serve")
            return False
        
        if not self.client.has_model():
            self._log(f"[ERROR] Model '{self.client.config.model}' not found.")
            self._log(f"   Install with: ollama pull {self.client.config.model}")
            available = self.client.list_models()
            if available:
                self._log(f"   Available models: {', '.join(available[:5])}")
            return False
        
        return True
    
    def parse_shaders(self, response: str) -> Optional[ShaderPair]:
        """
        Extract vertex and fragment shaders from LLM response.
        
        Args:
            response: Raw LLM response text
            
        Returns:
            ShaderPair if both shaders found, None otherwise
        """
        # Pattern to match ```vertex ... ``` and ```fragment ... ```
        vertex_pattern = r"```vertex\s*([\s\S]*?)```"
        fragment_pattern = r"```fragment\s*([\s\S]*?)```"
        
        vertex_match = re.search(vertex_pattern, response)
        fragment_match = re.search(fragment_pattern, response)
        
        if vertex_match and fragment_match:
            vertex = vertex_match.group(1).strip()
            fragment = fragment_match.group(1).strip()
            return ShaderPair(vertex=vertex, fragment=fragment)
        
        # Try alternative patterns (some models use different formatting)
        # Pattern: ```glsl with comment indicating type
        glsl_blocks = re.findall(r"```(?:glsl)?\s*(#version[\s\S]*?)```", response)
        
        if len(glsl_blocks) >= 2:
            vertex = None
            fragment = None
            
            for block in glsl_blocks:
                block = block.strip()
                if "gl_Position" in block and vertex is None:
                    vertex = block
                elif ("FragColor" in block or "gl_FragColor" in block) and fragment is None:
                    fragment = block
            
            if vertex and fragment:
                return ShaderPair(vertex=vertex, fragment=fragment)
        
        return None
    
    def validate_shader(
        self,
        source: str,
        shader_type: str
    ) -> CompilationResult:
        """
        Validate a shader using OpenGL or basic validation.
        
        Args:
            source: GLSL source code
            shader_type: "vertex" or "fragment"
            
        Returns:
            CompilationResult with validation status
        """
        if self._compiler and self.use_opengl_validation:
            if shader_type == "vertex":
                return self._compiler.compile_vertex(source)
            else:
                return self._compiler.compile_fragment(source)
        else:
            return BasicValidator.validate(source, shader_type)
    
    def _build_prompt_with_examples(self, description: str) -> str:
        """Build a prompt using relevant examples from the shader library."""
        # Find similar shaders from our library
        similar = search_shaders(description, limit=3)
        
        prompt = """You are a GLSL shader expert. Generate OpenGL 3.3 Core Profile shaders.

OUTPUT FORMAT (STRICT - only output shader code):
```vertex
#version 330 core
// vertex shader
```

```fragment
#version 330 core
// fragment shader
```

INTERFACE:
- Vertex inputs: aPos (vec3, location 0), aTexCoord (vec2, location 1), aNormal (vec3, location 2)
- Uniforms: model, view, projection (mat4), time (float), cameraPos (vec3), resolution (vec2, screen size), texture0 (sampler2D)
- Output: FragColor (vec4)

RULES:
- Use mod() not % for floats
- Use 1.0 not 1
- Use texture() not texture2D()
- Declare ALL uniforms you use at the top of the shader (e.g. uniform vec2 resolution;)
- For loops: declare the loop variable (e.g. for(int i = 0; i < 100; i++)) and use constant bounds only

"""
        # Add relevant examples
        if similar:
            prompt += "REFERENCE EXAMPLES (adapt these for the user's request):\n\n"
            for i, shader in enumerate(similar, 1):
                prompt += f"### Example {i}: {shader.name}\n"
                prompt += f"Description: {shader.description}\n\n"
                prompt += f"```vertex\n{shader.vertex.strip()}\n```\n\n"
                prompt += f"```fragment\n{shader.fragment.strip()}\n```\n\n"
        
        prompt += f"---\n\nUser request: {description}\n\nGenerate the shaders:"
        
        return prompt
    
    def generate(self, description: str) -> GenerationResult:
        """
        Generate shaders from a natural language description.
        
        Args:
            description: Natural language description of desired shader effect
            
        Returns:
            GenerationResult with shaders if successful
        """
        result = GenerationResult(attempts=0)
        
        # Check Ollama availability
        if not self.check_ollama():
            result.error = "Ollama not available"
            return result
        
        # Build prompt with relevant examples from library (used as references, not direct matches)
        current_prompt = self._build_prompt_with_examples(description)
        self._log(f"   Using {len(search_shaders(description, limit=3))} similar shaders as reference examples")
        
        for attempt in range(self.max_retries):
            result.attempts += 1
            self._log(f"\nAttempt {attempt + 1}/{self.max_retries}...")
            
            try:
                # Generate from LLM
                response = self.client.generate(current_prompt)
                result.raw_response = response
                
                # Parse shaders
                shaders = self.parse_shaders(response)
                
                if not shaders:
                    self._log("   [ERROR] Failed to parse shaders from response")
                    # Retry with stronger instruction
                    current_prompt = self.prompt_template.replace(
                        "{USER_PROMPT}",
                        f"{description}\n\nIMPORTANT: Output ONLY the shader code blocks in ```vertex and ```fragment format, no explanations."
                    )
                    continue
                
                # Validate shader pair together (better for catching interface mismatches)
                if self._compiler and self.use_opengl_validation:
                    vert_result, frag_result = self._compiler.validate_pair(
                        shaders.vertex, shaders.fragment
                    )
                else:
                    vert_result = BasicValidator.validate(shaders.vertex, "vertex")
                    frag_result = BasicValidator.validate(shaders.fragment, "fragment")
                
                if not vert_result.success:
                    error_msg = vert_result.error_message
                    self._log(f"   [ERROR] Vertex shader error: {error_msg[:100]}...")
                    result.compilation_errors.append(f"Vertex: {error_msg}")
                    
                    # Build error fix prompt
                    current_prompt = self.ERROR_FIX_TEMPLATE.format(
                        error=error_msg,
                        shader_type="VERTEX",
                        shader_type_lower="vertex",
                        shader_code=shaders.vertex
                    )
                    continue
                
                if not frag_result.success:
                    error_msg = frag_result.error_message
                    self._log(f"   [ERROR] Fragment shader error: {error_msg[:100]}...")
                    result.compilation_errors.append(f"Fragment: {error_msg}")
                    
                    # Build error fix prompt
                    current_prompt = self.ERROR_FIX_TEMPLATE.format(
                        error=error_msg,
                        shader_type="FRAGMENT",
                        shader_type_lower="fragment",
                        shader_code=shaders.fragment
                    )
                    continue
                
                # Success!
                result.shaders = shaders
                result.success = True
                self._log("   Shader generated and validated successfully!")
                return result
                
            except Exception as e:
                result.error = str(e)
                self._log(f"   [ERROR] Error: {e}")
        
        result.error = f"Max retries ({self.max_retries}) exceeded"
        return result
    
    def generate_from_file(self, prompt_file: str) -> GenerationResult:
        """
        Generate shaders from a prompt stored in a file.
        
        Args:
            prompt_file: Path to file containing the shader description
            
        Returns:
            GenerationResult
        """
        description = Path(prompt_file).read_text().strip()
        return self.generate(description)
    
    def cleanup(self):
        """Clean up resources."""
        if self._compiler:
            self._compiler.cleanup()


def generate_shader(
    description: str,
    model: str = "qwen2.5-coder:7b",
    validate: bool = True,
    verbose: bool = True
) -> GenerationResult:
    """
    Convenience function to generate a shader.
    
    Args:
        description: Natural language description
        model: Ollama model to use
        validate: Whether to validate with OpenGL
        verbose: Whether to print progress
        
    Returns:
        GenerationResult
    """
    config = OllamaConfig(model=model)
    generator = ShaderGenerator(
        ollama_config=config,
        use_opengl_validation=validate,
        verbose=verbose
    )
    
    try:
        return generator.generate(description)
    finally:
        generator.cleanup()
