#!/usr/bin/env python3
"""
GLSL Shader Generator CLI
=========================

Command-line interface for generating and previewing GLSL shaders.

Usage:
    python -m shader_generator "animated fire effect"
    python -m shader_generator --preview "rainbow gradient"
    python -m shader_generator --model deepseek-coder-v2:16b "phong lighting"
"""

import argparse
import sys
from pathlib import Path

from .generator import ShaderGenerator, GenerationResult
from .ollama_client import OllamaConfig
from .shader_library import SHADER_LIBRARY, search_shaders, ShaderExample


def print_result(result: GenerationResult, verbose: bool = True):
    """Pretty print the generation result."""
    print("\n" + "=" * 60)
    
    if result.success and result.shaders:
        print("SHADER GENERATED SUCCESSFULLY")
        print(f"   Attempts: {result.attempts}")
        print("=" * 60)
        
        print("\nVERTEX SHADER:")
        print("-" * 40)
        print(result.shaders.vertex)
        
        print("\nFRAGMENT SHADER:")
        print("-" * 40)
        print(result.shaders.fragment)
    else:
        print("SHADER GENERATION FAILED")
        print(f"   Attempts: {result.attempts}")
        print(f"   Error: {result.error}")
        
        if result.compilation_errors:
            print("\n   Compilation errors encountered:")
            for err in result.compilation_errors:
                print(f"   - {err[:80]}...")
        
        if verbose and result.raw_response:
            print("\nRAW RESPONSE (truncated):")
            print("-" * 40)
            response = result.raw_response
            print(response[:500] + "..." if len(response) > 500 else response)
    
    print("\n" + "=" * 60)


def save_shaders(result: GenerationResult, output_dir: str, name: str = "generated"):
    """Save shaders to files."""
    if result.success and result.shaders:
        result.shaders.save(output_dir, name)
        print(f"Saved to: {output_dir}/{name}.vert and {output_dir}/{name}.frag")


def run_preview(result: GenerationResult):
    """Run the shader preview window."""
    if not result.success or not result.shaders:
        print("[ERROR] Cannot preview - no valid shaders")
        return
    
    try:
        from .preview import preview_shader
        preview_shader(result.shaders)
    except ImportError as e:
        print(f"[ERROR] Preview requires pygame and moderngl: {e}")
        print("   Install with: pip install pygame moderngl numpy")


def interactive_mode(generator: ShaderGenerator):
    """Run in interactive mode."""
    print("\nInteractive Shader Generator")
    print("   Type a description and press Enter")
    print("   Type 'quit' or 'exit' to stop")
    print("   Type 'preview' after generation to see the shader")
    print("-" * 40)
    
    last_result = None
    
    while True:
        try:
            user_input = input("\n> ").strip()
        except (EOFError, KeyboardInterrupt):
            print("\nGoodbye!")
            break
        
        if not user_input:
            continue
        
        if user_input.lower() in ('quit', 'exit', 'q'):
            print("Goodbye!")
            break
        
        if user_input.lower() == 'preview':
            if last_result:
                run_preview(last_result)
            else:
                print("No shader to preview. Generate one first.")
            continue
        
        if user_input.lower() == 'save':
            if last_result and last_result.success:
                save_shaders(last_result, "output")
            else:
                print("No shader to save. Generate one first.")
            continue
        
        # Generate shader
        result = generator.generate(user_input)
        last_result = result
        print_result(result, verbose=False)


def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description="Generate GLSL shaders from natural language descriptions",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s "animated fire effect with embers"
  %(prog)s --preview "rainbow gradient shader"
  %(prog)s --model qwen2.5-coder:14b "phong lighting"
  %(prog)s --interactive
  %(prog)s --output ./shaders --name fire "fire effect"
        """
    )
    
    parser.add_argument(
        "description",
        nargs="?",
        help="Natural language description of the shader"
    )
    
    parser.add_argument(
        "-m", "--model",
        default="qwen2.5:7b-instruct-q4_K_M",
        help="Ollama model to use (default: qwen2.5:7b-instruct-q4_K_M)"
    )
    
    parser.add_argument(
        "-p", "--preview",
        action="store_true",
        help="Open preview window after generation"
    )
    
    parser.add_argument(
        "-o", "--output",
        default="output",
        help="Output directory for shader files (default: output)"
    )
    
    parser.add_argument(
        "-n", "--name",
        default="generated",
        help="Base name for output files (default: generated)"
    )
    
    parser.add_argument(
        "--no-validate",
        action="store_true",
        help="Skip OpenGL validation (faster but less reliable)"
    )
    
    parser.add_argument(
        "--retries",
        type=int,
        default=3,
        help="Maximum retry attempts (default: 3)"
    )
    
    parser.add_argument(
        "-i", "--interactive",
        action="store_true",
        help="Run in interactive mode"
    )
    
    parser.add_argument(
        "-q", "--quiet",
        action="store_true",
        help="Suppress progress messages"
    )
    
    parser.add_argument(
        "--prompt-file",
        help="Custom prompt template file"
    )
    
    parser.add_argument(
        "--list-models",
        action="store_true",
        help="List available Ollama models and exit"
    )
    
    parser.add_argument(
        "-l", "--list",
        action="store_true",
        help="List all built-in shaders in the library"
    )
    
    parser.add_argument(
        "-s", "--search",
        help="Search for shaders matching a query"
    )
    
    parser.add_argument(
        "--use",
        help="Use a specific shader from the library by name"
    )
    
    args = parser.parse_args()
    
    # Initialize generator
    config = OllamaConfig(model=args.model)
    generator = ShaderGenerator(
        ollama_config=config,
        prompt_file=args.prompt_file,
        use_opengl_validation=not args.no_validate,
        max_retries=args.retries,
        verbose=not args.quiet
    )
    
    # List built-in shaders
    if args.list:
        print("Built-in Shader Library:")
        print("-" * 50)
        for shader in SHADER_LIBRARY:
            tags = ", ".join(shader.tags[:4])
            print(f"   • {shader.name}")
            print(f"     {shader.description}")
            print(f"     Tags: {tags}")
            print()
        print(f"Total: {len(SHADER_LIBRARY)} shaders")
        print("\nUsage: python3 -m shader_generator --use \"Polka Dots\" --preview")
        return 0
    
    # Search shaders
    if args.search:
        results = search_shaders(args.search, limit=5)
        if results:
            print(f"Search results for '{args.search}':")
            print("-" * 50)
            for shader in results:
                print(f"   • {shader.name}: {shader.description}")
            print(f"\nUsage: python3 -m shader_generator --use \"{results[0].name}\" --preview")
        else:
            print(f"No shaders found matching '{args.search}'")
        return 0
    
    # Use specific shader from library
    if args.use:
        from .generator import ShaderPair
        shader = None
        for s in SHADER_LIBRARY:
            if s.name.lower() == args.use.lower():
                shader = s
                break
        
        if not shader:
            # Try search
            results = search_shaders(args.use, limit=1)
            if results:
                shader = results[0]
        
        if shader:
            print(f"Using built-in shader: {shader.name}")
            print(f"   {shader.description}")
            
            result = GenerationResult()
            result.success = True
            result.shaders = ShaderPair(vertex=shader.vertex, fragment=shader.fragment)
            result.attempts = 0
            
            print_result(result, verbose=not args.quiet)
            save_shaders(result, args.output, args.name)
            
            if args.preview:
                run_preview(result)
            return 0
        else:
            print(f"[ERROR] Shader '{args.use}' not found in library")
            print("   Use --list to see available shaders")
            return 1
    
    # List models mode
    if args.list_models:
        from .ollama_client import OllamaClient
        client = OllamaClient(config)
        
        if not client.is_available():
            print("[ERROR] Cannot connect to Ollama. Is it running?")
            return 1
        
        models = client.list_models()
        print("Available Ollama models:")
        for model in models:
            marker = "  (default)" if args.model in model else ""
            print(f"   - {model}{marker}")
        return 0
    
    # Interactive mode
    if args.interactive:
        interactive_mode(generator)
        return 0
    
    # Single generation mode
    if not args.description:
        parser.print_help()
        print("\n[ERROR] Please provide a shader description or use --interactive")
        return 1
    
    print(f"GLSL Shader Generator")
    print(f"   Model: {args.model}")
    print(f"   Description: {args.description}")
    
    # Check Ollama
    if not generator.check_ollama():
        return 1
    
    # Generate
    result = generator.generate(args.description)
    print_result(result, verbose=not args.quiet)
    
    if result.success:
        # Save shaders
        save_shaders(result, args.output, args.name)
        
        # Preview if requested
        if args.preview:
            run_preview(result)
        
        return 0
    else:
        return 1


if __name__ == "__main__":
    sys.exit(main())
