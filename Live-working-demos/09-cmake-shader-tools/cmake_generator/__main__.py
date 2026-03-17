"""
Allow running as:
    python -m cmake_generator              # launches visual demo
    python -m cmake_generator --scan DIR   # CLI mode
    python -m cmake_generator --demo NAME  # CLI mode
"""
import sys

if len(sys.argv) > 1:
    from cmake_generator.cli import main
    main()
else:
    from cmake_generator.demo import run_demo
    run_demo()
