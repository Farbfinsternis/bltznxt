import sys
import os
import shutil
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from common import download_file, extract_zip, ensure_dir

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOOLS_DIR = os.path.join(ROOT, "tools")
LIBS_DIR = os.path.join(ROOT, "libs")

MINGW_URL = "https://github.com/brechtsanders/winlibs_mingw/releases/download/14.2.0posix-19.1.7-12.0.0-msvcrt-r3/winlibs-x86_64-posix-seh-gcc-14.2.0-llvm-19.1.7-mingw-w64msvcrt-12.0.0-r3.zip"
SDL3_URL = "https://github.com/libsdl-org/SDL/releases/download/release-3.2.4/SDL3-devel-3.2.4-mingw.zip"

def is_sdk_mode():
    """Detect if we are running in a packaged SDK (dist) or source repo."""
    # In SDK, bbc.exe is in bin/
    return os.path.exists(os.path.join(ROOT, "bin", "bbc.exe"))

def setup_mingw():
    is_sdk = is_sdk_mode()
    
    # Target location for extraction
    if is_sdk:
        # SDK expects mingw64 in root
        target_dir = ROOT
        extract_to = ROOT
        dest_check = os.path.join(ROOT, "mingw64")
    else:
        # Source repo expects mingw64 in tools/
        target_dir = TOOLS_DIR
        extract_to = TOOLS_DIR
        dest_check = os.path.join(TOOLS_DIR, "mingw64")
        
    if os.path.exists(dest_check):
        print("MinGW already installed.")
        return True
    
    zip_path = os.path.join(target_dir, "mingw.zip")
    
    print(f"Installing MinGW to {dest_check}...")
    if download_file(MINGW_URL, zip_path):
        if extract_zip(zip_path, extract_to):
            os.remove(zip_path)
            # If extraction created 'mingw64' subfolder, we are good.
            # winlibs zip usually contains 'mingw64' folder.
            return True
    return False

def setup_sdl3():
    is_sdk = is_sdk_mode()
    
    # Target location
    if is_sdk:
        # In SDK, SDL3.dll is in bin/ and libs are in lib/
        # Currently setup.py in SDK mode is mostly for tools.
        # But let's handle the extraction similarly.
        target_dir = ROOT
        extract_to = LIBS_DIR
        dest_check = os.path.join(LIBS_DIR, "SDL3")
    else:
        target_dir = LIBS_DIR
        extract_to = LIBS_DIR
        dest_check = os.path.join(LIBS_DIR, "SDL3")
        
    if os.path.exists(dest_check):
        print("SDL3 already installed.")
        return True
    
    zip_path = os.path.join(target_dir, "sdl3.zip")
    
    print(f"Installing SDL3 to {dest_check}...")
    if download_file(SDL3_URL, zip_path):
        if extract_zip(zip_path, extract_to):
            os.remove(zip_path)
            # The zip contains 'SDL3-3.2.4' folder, we want to rename it to 'SDL3'
            extracted_folder = os.path.join(extract_to, "SDL3-3.2.4")
            if os.path.exists(extracted_folder):
                if os.path.exists(dest_check):
                    shutil.rmtree(dest_check)
                os.rename(extracted_folder, dest_check)
            return True
    return False

def check_environment():
    print("Checking environment...")
    sdk = is_sdk_mode()
    print(f"Mode: {'SDK' if sdk else 'Source repository'}")
    
    if sdk:
        python_path = os.path.join(ROOT, "python")
        sdl_dll = os.path.join(ROOT, "bin", "SDL3.dll")
    else:
        python_path = os.path.join(TOOLS_DIR, "python")
        sdl_dll = os.path.join(LIBS_DIR, "SDL3", "x86_64-w64-mingw32", "bin", "SDL3.dll")
        
    if not os.path.exists(python_path):
        print(f"Note: Portable Python folder not found at {python_path}")
    else:
        print("Portable Python: Found")
        
    if not os.path.exists(sdl_dll):
        print(f"Warning: SDL3 runtime missing at {sdl_dll}")
    else:
        print("SDL3 Runtime: OK")
    
    return True

def main():
    if sys.platform != 'win32':
        print("Automatic setup is currently only supported on Windows.")
        print("On Mac/Linux, please install SDL3 via your package manager and use 'make'.")
        return

    ensure_dir(TOOLS_DIR)
    ensure_dir(LIBS_DIR)
    
    print("\n--- Installing Toolchain ---")
    if not setup_mingw(): 
        print("Failed to install MinGW.")
        sys.exit(1)
        
    print("\n--- Installing Dependencies ---")
    if not setup_sdl3():
        print("Failed to install SDL3.")
        sys.exit(1)
    
    check_environment()
    
    print("\nSetup complete. BltzNxt is ready!")

if __name__ == "__main__":
    main()
