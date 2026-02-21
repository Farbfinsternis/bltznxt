# BltzNxt
**Version 0.6.0**

> [!NOTE]
> **Project Transparency**: This project is a collaborative effort between a Human Project Manager and Gemini (AI). 
> The Human serves as the Lead Architect, Project Manager, and Code Reviewer, while Gemini acts as the primary Programmer. 
> No code is committed to this project without the Human's review, understanding, and ability to modify it.

The next generation of Blitz3D, built on modern tech.

## Toolchain
- **Compiler**: MinGW-w64 (GCC 14.2.0) - *downloaded via setup*
- **Runtime**: OpenGL 2.1 / SDL3 - *bundled*
- **Language**: Transpiled C++17
- **Setup**: Portable Python 3.11 - *bundled*

## Directory Structure
- `scripts/`: Python scripts for setup, build, and packaging
- `src/`: Source code for the Transpiler (C++) and Runtime (C++/OpenGL)
- `tests/`: Regression tests and test runner
- `examples/`: Blitz3D example files (.bb)
- `tools/`: Toolchain dependencies (MinGW, Portable Python) - *auto-generated*
- `libs/`: External libraries (SDL3) - *auto-generated*
- `_build/`: Build artifacts and transpiler executable - *auto-generated*

## 🚀 Getting Started

BltzNxt can be used either as a pre-built **SDK** (recommended for users) or as a **Source Repository** (for developers).

### 🪟 Windows (Recommended)
The toolchain setup is fully automated:
1.  **Clone** or **Extract** the repository.
2.  Run **`setup.bat`**: This automatically downloads the MinGW-w64 toolchain and SDL3 development libraries using a bundled portable Python.
3.  **Build**: Once setup is complete, run the build script:
    ```cmd
    python scripts/build.py
    ```
4.  **Use `_build/blitzcc.exe`**: You can now point your IDE (like IDEal) to this executable.

### 🐧 Linux / 🍎 macOS (Native Setup)
1.  **Clone** the repository.
2.  **Install Dependencies**: Use your package manager to install `SDL3` (e.g., `brew install sdl3` or `sudo apt install libsdl3-dev`).
3.  **Build**:
    ```bash
    make
    ```
4.  **Usage**: Use the transpiler (`_build/bbc_cpp`) and `g++` directly.

## 📂 Project Structure
- `scripts/`: Python scripts for `setup`, `build`, and `package`.
- `src/`: Source code for the Transpiler (C++) and Runtime (C++/OpenGL).
- `tests/`: Automated regression test suite.
- `tools/`: Auto-generated toolchain (MinGW, Portable Python).
- `libs/`: Auto-generated external libraries (SDL3).
- `_build/`: Auto-generated build artifacts.

## 🛠️ Building & Developing
For developers wishing to contribute:
1.  **Initialize**: Run `setup.bat` (Windows) or install system SDL3 (Unix).
2.  **Compile**: 
    ```bash
    python scripts/build.py
    ```
    This builds the Transpiler, the Runtime library, and the Windows wrapper.
3.  **Test**: Run the regression suite to ensure stability:
    ```bash
    python tests/run_tests.py
    ```

## 📦 Packaging
To bundle your current build into a clean, distributable SDK:
```bash
python scripts/package.py
```
This will assemble all necessary files into the `dist/` directory.

## What is BLTZNXT?
BLTZNXT is a modern, high-performance successor to the Blitz3D ecosystem. While it is based on the original Blitz3D source code, the compiler and runtime have been **extensively rewritten and optimized** to meet modern standards. By converting `.bb` source code into optimized **C++17**, BLTZNXT bridges the gap between classic ease-of-use and modern execution speed.

### 🚀 Key Optimizations
- **Native C++17 Transpilation**: Instead of outdated x86 assembly, BLTZNXT generates clean C++, allowing modern compilers (GCC/Clang) to apply advanced optimizations like inlining and vectorization.
- **Modern Backend**: Replaces the legacy DirectX 7 stack with a cross-platform **SDL3/OpenGL** runtime, ensuring stability on modern Windows, Linux, and macOS.
- **Advanced Memory Management**: A custom `MemoryManager` handles resource tracking and automated LIFO cleanup, significantly reducing memory leaks and fragmentation.
- **Enhanced Language Features**: Extends the original syntax with modern capabilities like **Function Overloading** (available to both internal runtime and end-user functions), providing more flexibility for developers.
- **Optimized Toolchain**: A streamlined, Python-powered build system replaces complex IDE requirements with a single command setup.
- **Performance**: Significant speed improvements in math-heavy logic and string processing by leveraging modern C++ standard library features.

## Current Status & Features
The project is currently in active development, with the core language and basic engine features firmly in place. Recent updates have focused on **3D Collisions**, **Font management**, and **IDE compatibility**.

### 🔹 Language Support
- **Core Logic**: Fully supports Types, Functions, Global/Local variables, and Arrays.
- **Function Overloading**: Define multiple versions of a function for different data types—a feature exclusive to BltzNxt.
- **Transpilation**: Seamless conversion of Blitz3D syntax to modern C++17.
- **Stability**: Includes an automated regression test suite to ensure compiler accuracy.

### 🔹 Runtime & Graphics
- **2D Engine**: Complete support for primitive drawing (`Rect`, `Oval`, `Line`), Colors, and Image handling.
- **3D Engine**: Entity system with support for Meshes, Cameras, Perspectives, and Lights.
- **Collision System**: Robust **Sphere-Sphere** and **Sphere-Mesh** detection with automated hierarchy exclusion. Supports response modes and state queries (`PickedEntity/X/Y/Z`, `CollisionX/Y/Z`, etc.).
- **Extended Font System**: Dynamic loading from `C:\Windows\Fonts` (Arial, etc.) with support for bold/italic variants and metrics (`StringWidth`, `FontHeight`).
- **Timer Support**: Frame-pacing and event timing via `CreateTimer`, `WaitTimer`, and `FreeTimer`.
- **I/O & System**: Console I/O (`Print`, `Input`), Input handling (Keyboard/Mouse), and Math/String libraries. Fully supports **modern system paths** (AppData, Documents, Desktop).
- **Graphics Control**: Support for Fullscreen mode and native desktop auto-resolution.

### 🔹 Build System
- **Portable & Automated**: A Python-based toolchain that handles setup, building, and packaging without complex system dependencies.
- **IDE Compatible**: Includes a `blitzcc` wrapper and keyword export for ease of use with external IDEs like IDEal.
- **Cross-Platform**: Natively supports **Linux** and **macOS** builds via `make` and `scripts/build.py`.
## 🎯 Compatibility & Beyond

BLTZNXT aims for seamless integration with existing Blitz3D projects while providing a futuristic feature set.

### 📊 Compatibility Score
- **Core Language (**`100%`**)**: Every standard Blitz3D language construct (Types, Arrays, Functions, Select, Loops) is fully supported and transpiles to optimized C++.
- **Runtime API (**`~40%`**)**: All essential 2D, System, and basic 3D commands are implemented. Advanced 3D features (Shadows, Terrains) and Sound are currently in active development.

### 🌟 Exclusive BLTZNXT Features
Unlike the original Blitz3D, BLTZNXT offers:
- **Function Overloading**: Define multiple versions of a function for different data types.
- **Modern Hardware Support**: Runs natively on modern systems via **SDL3/OpenGL**, bypassing legacy DirectX 7 limitations.
- **Compilation Speed**: Blazing fast builds leveraging multi-threaded C++ compilers.
- **Memory Safety**: Automatic LIFO resource cleanup and a secure `MemoryManager` backend.
- **Cross-Platform Potential**: Architected from the ground up to be portable across Windows, Linux, and beyond.

---

## 📜 Changelog

### v0.6.0 (2026-02-20)
- **IDE Compatibility**: Introduced the `blitzcc.exe` wrapper for seamless integration with external IDEs.
- **3D Collision System**: Implemented Sphere-Sphere and Sphere-Mesh detection, sliding response, and full query API parity.
- **Advanced Font Support**: Dynamic system font loading and metrics calculation via `stb_truetype`.
- **SDK Structure**: Reorganized binaries into a portable `bin/` directory.
- **Cross-Platform**: Verified build support for Linux and macOS via updated `Makefile` and `build.py`.
- **Timer Subsystem**: Added `CreateTimer`, `WaitTimer`, and `FreeTimer` commands.

### v0.5.3 (2026-02-19)
- **Script Migration**: Fully migrated to a cross-platform Python build system (`setup.py`, `build.py`, `package.py`, `run_tests.py`).
- **Smart Distribution**: Refined packaging to include Portable Python while offloading MinGW to an on-demand setup script, greatly reducing initial download size.
- **Convenience Wrappers**: Added root-level `setup.bat` for quick developer environment initialization.
- **Improved Docs**: Detailed technical documentation on OpenGL 2.1 usage, architectural rewrites, and performance optimizations.
- **Verification**: Formally verified and documented **Function Overloading** support for user-defined BlitzBasic functions.
- **Graphics Alignment**: Updated all documentation to reflect the accurate **OpenGL 2.1** runtime (instead of the placeholder Vulkan 1.3).
