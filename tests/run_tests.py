import os
import sys
import subprocess
import glob
import shlex

# Configuration
ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD_DIR = os.path.join(ROOT_DIR, "_build")
LIB_DIR = os.path.join(ROOT_DIR, "lib")
TOOLS_DIR = os.path.join(ROOT_DIR, "tools")
TESTS_DIR = os.path.join(ROOT_DIR, "tests")

TRANSPILER = os.path.join(BUILD_DIR, "bbc_cpp.exe")
RUNTIME_LIB = os.path.join(LIB_DIR, "libbbruntime.a")
CXX = os.path.join(TOOLS_DIR, "mingw64", "bin", "g++.exe")

INCLUDES = [
    "-I" + os.path.join(ROOT_DIR, "src", "runtime"),
    "-I" + os.path.join(ROOT_DIR, "libs", "SDL3", "x86_64-w64-mingw32", "include")
]

LIBS = [
    "-L" + os.path.join(ROOT_DIR, "libs", "SDL3", "x86_64-w64-mingw32", "lib"),
    "-lmingw32", "-lSDL3", "-lopengl32", "-lglu32", "-luser32", "-lgdi32", "-lwinmm", "-limm32", "-lole32", "-loleaut32", "-lversion", "-luuid", "-lsetupapi" 
]

def run_command(cmd, cwd=None, capture=False):
    """Running a shell command."""
    try:
        result = subprocess.run(
            cmd,
            cwd=cwd,
            check=True,
            stdout=subprocess.PIPE if capture else None,
            stderr=subprocess.PIPE if capture else None,
            text=True
        )
        return result.stdout
    except subprocess.CalledProcessError as e:
        print(f"Error running command: {' '.join(cmd)}")
        if capture:
            print("STDOUT:", e.stdout)
            print("STDERR:", e.stderr)
        raise

def run_test(test_file):
    """Runs a single test case."""
    test_name = os.path.basename(test_file).replace(".bb", "")
    print(f"Testing {test_name}...", end=" ", flush=True)

    cpp_file = os.path.join(BUILD_DIR, f"{test_name}.cpp")
    exe_file = os.path.join(BUILD_DIR, f"{test_name}.exe")
    expected_file = test_file.replace(".bb", ".expected")

    try:
        # 1. Transpile
        # We need to run via cmd /c for redirection if we were doing it in shell,
        # but here we can capture stdout directly.
        # However, bbc_cpp.exe prints to stdout.
        
        # Note: bbc_cpp.exe expects the input file as argument
        
        # Run transpiler and capture stdout to write to cpp_file
        with open(cpp_file, "w") as f_cpp:
            subprocess.run([TRANSPILER, test_file], stdout=f_cpp, check=True, stderr=subprocess.PIPE)

        # 2. Compile C++
        cmd = [CXX, "-std=c++17", "-O2", "-static-libgcc", "-static-libstdc++", "-o", exe_file, cpp_file, RUNTIME_LIB] + INCLUDES + LIBS
        # Ensure we link static lib - sometimes order matters in MinGW, put lib last is usually safer
        # Also need libraries for runtime dependencies if not fully static
        # But for now assuming libbbruntime.a handles it or we need -lmingw32 -lSDL3 etc if linked dynamically
        # Since we use a sanitized environment, we might need explicit linker flags if libbbruntime relies on SDL
        
        # For now, let's try basic compilation. If it fails due to missing symbols, we'll add flags.
        # The build_all.ps1 doesn't show semantic linking of test.cpp, only the library creation.
        
        # But wait, linking libbbruntime.a implies we need its dependencies too (SDL, etc)
        # For simplicity, check if we need extra flags. The 'test.cpp' was compiled with just imports.
        # Let's add standard windows libs just in case.
        
        run_command(cmd, cwd=BUILD_DIR, capture=True)

        # 3. Execute
        output = run_command([exe_file], cwd=BUILD_DIR, capture=True)

        # 4. Verify
        if os.path.exists(expected_file):
            with open(expected_file, "r") as f:
                expected = f.read().strip()
            
            # Normalize line endings
            output = output.strip().replace("\r\n", "\n")
            expected = expected.replace("\r\n", "\n")

            if output == expected:
                print("PASS")
                return True
            else:
                print("FAIL")
                print("Expected:\n" + expected)
                print("Got:\n" + output)
                return False
        else:
            print("PASS (No expected file)")
            return True

    except Exception as e:
        print("FAIL (Exception)")
        print(e)
        return False

def main():
    if not os.path.exists(TRANSPILER):
        print("Transpiler not found. Please run scripts/build.py first.")
        sys.exit(1)

    bb_files = glob.glob(os.path.join(TESTS_DIR, "*.bb"))
    if not bb_files:
        print("No tests found.")
        return

    passed = 0
    total = 0

    for bb_file in bb_files:
        total += 1
        if run_test(bb_file):
            passed += 1

    print("-" * 40)
    print(f"Results: {passed}/{total} passed.")

    if passed != total:
        sys.exit(1)

if __name__ == "__main__":
    main()
