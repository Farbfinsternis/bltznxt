import os
import shutil
import sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from common import ensure_dir, clean_dir

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DIST_DIR = os.path.join(ROOT, "dist")
_BUILD_DIR = os.path.join(ROOT, "_build")
LIB_DIR = os.path.join(ROOT, "lib")
SRC_DIR = os.path.join(ROOT, "src")
LIBS_DIR = os.path.join(ROOT, "libs")
TOOLS_DIR = os.path.join(ROOT, "tools")

IS_WIN = sys.platform == 'win32'

def main():
    print(f"Assembling SDK Distribution Package for {sys.platform}...")
    clean_dir(DIST_DIR)
    
    # SDK Structure:
    # bin/      - Executables and DLLs
    # lib/      - Static libraries (.a)
    # include/  - Runtime headers (.h)
    # scripts/  - Helper scripts for setup
    # python/   - Bundled Python (Windows only)
    
    bins = os.path.join(DIST_DIR, "bin")
    libs = os.path.join(DIST_DIR, "lib")
    incs = os.path.join(DIST_DIR, "include")
    scripts = os.path.join(DIST_DIR, "scripts")
    
    ensure_dir(bins)
    ensure_dir(libs)
    ensure_dir(incs)
    ensure_dir(scripts)
    
    # 1. Copy Core Transpiler
    print("Copying Transpiler...")
    transpiler_exe = "bbc_cpp.exe" if IS_WIN else "bbc_cpp"
    shutil.copy2(os.path.join(_BUILD_DIR, transpiler_exe), os.path.join(bins, "bbc.exe" if IS_WIN else "bbc"))
    
    # 2. Copy BlitzCC Wrapper (Windows only)
    if IS_WIN:
        print("Copying BlitzCC Wrapper...")
        shutil.copy2(os.path.join(_BUILD_DIR, "blitzcc.exe"), bins)

    # 3. Copy Runtime Library
    print("Copying Runtime Library...")
    shutil.copy2(os.path.join(LIB_DIR, "libbbruntime.a"), libs)
    
    # 4. Copy Headers
    print("Copying Headers...")
    for h in os.listdir(os.path.join(SRC_DIR, "runtime")):
        if h.endswith(".h"):
            shutil.copy2(os.path.join(SRC_DIR, "runtime", h), incs)
            
    # 5. Copy Scripts & Setup
    print("Copying Scripts...")
    shutil.copy2(os.path.join(ROOT, "scripts", "common.py"), scripts)
    shutil.copy2(os.path.join(ROOT, "scripts", "setup.py"), scripts)

    if IS_WIN:
        # Create/Copy Windows Batch
        print("Creating setup.bat...")
        with open(os.path.join(DIST_DIR, "setup.bat"), "w") as f:
            f.write("@echo off\npython\\python.exe scripts\\setup.py\npause")
            
        print("Copying Portable Python...")
        if os.path.exists(os.path.join(TOOLS_DIR, "python")):
            shutil.copytree(os.path.join(TOOLS_DIR, "python"), os.path.join(DIST_DIR, "python"))

    # 6. Documentation
    print("Copying Documentation...")
    shutil.copy2(os.path.join(ROOT, "README.md"), DIST_DIR)
    if os.path.exists(os.path.join(ROOT, "LICENSE")):
        shutil.copy2(os.path.join(ROOT, "LICENSE"), DIST_DIR)

    # 7. Platform Specific Libraries (SDL3)
    if IS_WIN:
        print("Copying SDL3 Binaries...")
        sdl_bin_dir = os.path.join(LIBS_DIR, "SDL3", "x86_64-w64-mingw32", "bin")
        shutil.copy2(os.path.join(sdl_bin_dir, "SDL3.dll"), bins)
        
        sdl_lib_dir = os.path.join(LIBS_DIR, "SDL3", "x86_64-w64-mingw32", "lib")
        for f in os.listdir(sdl_lib_dir):
            if f.endswith(".a"):
                shutil.copy2(os.path.join(sdl_lib_dir, f), libs)
    else:
        # For Mac/Linux, we might bundle libs OR expect system libs.
        # Given the requirement, we mostly ship SOURCE or expect system SDL3.
        # We can add a simple setup.sh here.
        print("Creating setup.sh...")
        setup_sh = """#!/bin/bash
# BltzNxt Native Setup
echo "BltzNxt Native Setup"
echo "Trying to install dependencies..."
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    sudo apt-get install -y build-essential libsdl3-dev
elif [[ "$OSTYPE" == "darwin"* ]]; then
    brew install sdl3
fi
echo "Setup complete. You can now use the 'bbc' tool."
"""
        with open(os.path.join(DIST_DIR, "setup.sh"), "w") as f:
            f.write(setup_sh)
        os.chmod(os.path.join(DIST_DIR, "setup.sh"), 0o755)

    print(f"\nSDK distribution package created at: {DIST_DIR}")

if __name__ == "__main__":
    main()
