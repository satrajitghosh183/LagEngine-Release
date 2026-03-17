"""
Test script to verify the CUDA GL demo executable works correctly
"""
import os
import subprocess
import json
import sys

def test_executable(executable_path=None):
    """Test if the executable works"""
    
    if executable_path is None:
        # Try to find it
        possible_paths = [
            "../build/cuda_gl_demo_rl",
            "../build/cuda_gl_demo",
            "build/cuda_gl_demo_rl",
            "build/cuda_gl_demo",
            "./cuda_gl_demo_rl",
            "./cuda_gl_demo",
        ]
        
        for path in possible_paths:
            if os.path.exists(path):
                executable_path = os.path.abspath(path)
                break
    
    if executable_path is None:
        print("ERROR: Could not find executable")
        print("Please specify the path or build the project:")
        print("  cd ../build && cmake .. && make")
        return False
    
    if not os.path.exists(executable_path):
        print(f"ERROR: Executable not found: {executable_path}")
        return False
    
    if not os.access(executable_path, os.X_OK):
        print(f"ERROR: Executable is not executable: {executable_path}")
        print("Try: chmod +x " + executable_path)
        return False
    
    print(f"Testing executable: {executable_path}")
    print(f"Working directory: {os.getcwd()}")
    print()
    
    # Test with minimal parameters
    test_metrics = "test_metrics.json"
    cmd = [
        executable_path,
        "--particles", "10000",
        "--blocksize", "256",
        "--frames", "10",  # Just 10 frames for quick test
        "--output", test_metrics
    ]
    
    print(f"Running command: {' '.join(cmd)}")
    print()
    
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=30.0,
            cwd=os.path.dirname(executable_path) or "."
        )
        
        print(f"Return code: {result.returncode}")
        print()
        
        if result.stdout:
            print("STDOUT:")
            print(result.stdout)
            print()
        
        if result.stderr:
            print("STDERR:")
            print(result.stderr)
            print()
        
        if result.returncode != 0:
            print("ERROR: Executable failed!")
            return False
        
        # Check if metrics file was created
        metrics_path = os.path.abspath(test_metrics)
        if not os.path.exists(metrics_path):
            # Try in the executable directory
            metrics_path = os.path.join(os.path.dirname(executable_path) or ".", test_metrics)
        
        if os.path.exists(metrics_path):
            print(f"Metrics file found: {metrics_path}")
            try:
                with open(metrics_path, 'r') as f:
                    metrics = json.load(f)
                print("\nMetrics:")
                print(json.dumps(metrics, indent=2))
                
                # Clean up
                os.remove(metrics_path)
                
                if metrics.get('avg_fps', 0) > 0:
                    print("\n✓ SUCCESS: Executable works correctly!")
                    return True
                else:
                    print("\n⚠ WARNING: Executable ran but FPS is 0")
                    return False
            except json.JSONDecodeError as e:
                print(f"\nERROR: Failed to parse JSON: {e}")
                with open(metrics_path, 'r') as f:
                    print(f"File contents: {f.read()[:500]}")
                return False
        else:
            print(f"ERROR: Metrics file not found: {metrics_path}")
            print(f"Current directory: {os.getcwd()}")
            print(f"Files in current directory: {os.listdir('.')[:10]}")
            return False
            
    except subprocess.TimeoutExpired:
        print("ERROR: Executable timed out after 30 seconds")
        return False
    except Exception as e:
        print(f"ERROR: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    executable = sys.argv[1] if len(sys.argv) > 1 else None
    success = test_executable(executable)
    sys.exit(0 if success else 1)

