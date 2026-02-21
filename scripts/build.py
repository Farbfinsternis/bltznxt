import os
import sys
import glob
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from common import run_command, ensure_dir

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
IS_WIN = sys.platform == 'win32'
EXE_EXT = ".exe" if IS_WIN else ""

# Default to bundled tools on Windows, fallback to system tools
CXX = os.path.join(ROOT, "tools", "mingw64", "bin", "g++.exe")
AR = os.path.join(ROOT, "tools", "mingw64", "bin", "ar.exe")

if not IS_WIN or not os.path.exists(CXX):
    CXX = "g++"
    AR = "ar"

BUILD_DIR = os.path.join(ROOT, "_build")
LIB_DIR = os.path.join(ROOT, "lib")

# SDL3 paths differ on Windows (bundled) vs Linux/Mac
SDL_INC = []
if IS_WIN:
    SDL_INC = ["-I" + os.path.join(ROOT, "libs", "SDL3", "x86_64-w64-mingw32", "include")]

RT_INCLUDES = [
    "-I" + os.path.join(ROOT, "src", "runtime"),
] + SDL_INC + ["-DBB_RUNTIME_COMPILING"]

COMMON_FLAGS = ["-std=c++17", "-O2"]
FORCE_INCLUDE = ["-include", os.path.join(ROOT, "src", "transpiler", "std.h")]

def build_runtime():
    print("Building Runtime Library...")
    ensure_dir(BUILD_DIR)
    ensure_dir(LIB_DIR)
    
    rt_src_dir = os.path.join(ROOT, "src", "runtime")
    sources = glob.glob(os.path.join(rt_src_dir, "*.cpp"))
    objects = []
    
    for src in sources:
        obj = os.path.join(BUILD_DIR, "rt_" + os.path.basename(src).replace(".cpp", ".o"))
        objects.append(obj)
        print(f"Compiling {os.path.basename(src)}...")
        cmd = [CXX] + COMMON_FLAGS + RT_INCLUDES + ["-c", src, "-o", obj]
        if not run_command(cmd):
            return False
            
    print("Packing libbbruntime.a...")
    lib_path = os.path.join(LIB_DIR, "libbbruntime.a")
    cmd = [AR, "rcs", lib_path] + objects
    return run_command(cmd)

def build_transpiler():
    print("\nBuilding Transpiler...")
    trans_src_dir = os.path.join(ROOT, "src", "transpiler")
    
    # List of modules to compile
    modules = [
        "main", "toker", "parser", "decl", "declnode", "environ", 
        "exprnode", "node", "prognode", "stmtnode", "type", 
        "varnode", "cpp_generator", "memory"
    ]
    
    objects = []
    for name in modules:
        src = os.path.join(trans_src_dir, name + ".cpp")
        obj = os.path.join(BUILD_DIR, name + ".o")
        objects.append(obj)
        print(f"Compiling {name}.cpp...")
        cmd = [CXX] + COMMON_FLAGS + ["-I" + trans_src_dir] + FORCE_INCLUDE + ["-c", src, "-o", obj]
        if not run_command(cmd):
            return False
            
    print("Linking Transpiler...")
    exe_path = os.path.join(BUILD_DIR, "bbc_cpp" + EXE_EXT)
    cmd = [CXX] + COMMON_FLAGS + objects + ["-o", exe_path]
    if not run_command(cmd):
        return False
        
    if IS_WIN:
        print("\nBuilding BlitzCC Wrapper...")
        wrapper_src = os.path.join(ROOT, "src", "blitzcc_main.cpp")
        wrapper_exe = os.path.join(BUILD_DIR, "blitzcc.exe")
        # Use static linking for the wrapper to make it standalone
        cmd = [CXX] + COMMON_FLAGS + ["-static", "-static-libgcc", "-static-libstdc++", wrapper_src, "-o", wrapper_exe]
        if not run_command(cmd):
            return False
            
    return True

def main():
    # If using absolute path (bundled), verify it exists
    if os.path.isabs(CXX) and not os.path.exists(CXX):
        print("Compiler not found at " + CXX)
        print("Please run 'python scripts/setup.py' first.")
        sys.exit(1)
        
    if not build_runtime():
        print("Runtime build failed.")
        sys.exit(1)
        
    if not build_transpiler():
        print("Transpiler build failed.")
        sys.exit(1)
        
    print("\nBuild Complete!")

if __name__ == "__main__":
    main()
