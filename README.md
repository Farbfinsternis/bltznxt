<p align="center">
  <img src="logo.png" alt="BlitzNext Logo" width="480">
</p>

# BlitzNext

**BlitzNext** is a modern compiler that converts Blitz3D (`.bb`) source files directly into native Windows executables via a C++17 transpilation pipeline. It targets 100% command parity with the original Blitz3D engine, using a bundled MinGW toolchain and SDL3 for audio and graphics.

> **Status: active development — 35 of 65 milestones complete.**
> See [roadmap.md](roadmap.md) for the full milestone list and [DEVLOG.md](DEVLOG.md) for the changelog.

---

## In Memory of Mark Sibly

Blitz3D was the work of one person. Mark Sibly built a language that let thousands of people make their first game, write their first loop, see something move on screen for the first time. It was direct, it was fast, and it got out of the way. No engine before it was quite so honest about what programming could feel like when the barrier was low enough.

Mark passed away in 2025. BlitzNext exists to carry his idea forward — the belief that making something should be simple, and that simple things can still be powerful.

---

## Who Is This For?

**Blitz3D veterans** — Your old `.bb` projects deserve to run on modern hardware. BlitzNext brings the language you know forward without changing its feel. No rewrites, no ports — just compile and go.

**Retro enthusiasts** — Blitz3D had a charm that modern engines don't replicate: instant results, no boilerplate, a game on screen in ten lines of code. That philosophy is alive here.

**Beginners** — Blitz3D was one of the friendliest ways to learn programming ever made. `Print "Hello"` works. No classes, no frameworks, no build configuration. BlitzNext keeps that low floor intact while targeting modern Windows natively.

---

## Compatibility Progress

| Area | Done | Goal | Coverage |
|------|------|------|----------|
| **Language features** | Grammar, types, control flow, functions, arrays, includes, operators | 100% Blitz3D language spec | ~90% |
| **Runtime (built-in commands)** | 283 functions across 11 modules | ~480 total projected | ~59% |
| **Roadmap milestones** | 35 of 65 | 65 | 54% |

**Language** is nearly complete — all core constructs (variables, types, functions, control flow, operators, `#Include`, `Data/Read`, `Dim`, `Goto/Gosub`) are implemented. Remaining gaps are edge cases in the parser, not missing constructs.

**Runtime** coverage grows phase by phase. The non-graphical half (math, strings, files, banks, input, audio) is done. 2D graphics is partially complete (window, buffer, color, pixel operations). The 3D graphics layer (Phases L–T, Milestones 47–70) makes up the bulk of what remains.

---

## Getting Started

### Requirements
- Windows 10/11
- `curl` and `tar` (built into modern Windows)

### Setup
```bat
git clone https://github.com/your-org/bltznxt
cd bltznxt
build_windows.bat
```

`build_windows.bat` downloads the MinGW toolchain and SDL3, then builds `bin\blitzcc.exe`. No manual dependency installation required.

### Your first program

Create `hello.bb`:
```blitz3d
Print "Hello from BlitzNext!"
WaitKey
```

Compile it:
```bat
bin\blitzcc.exe hello.bb
```

`hello.exe` appears next to the source file and runs standalone.

---

## What Works Today

### Language

| Feature | Status |
|---------|--------|
| `If / ElseIf / Else / EndIf` | ✓ |
| `While / Wend` | ✓ |
| `Repeat / Until / Forever` | ✓ |
| `For / Next` (with `Step`, including negative) | ✓ |
| `Select / Case / Default` | ✓ |
| `Function / Return / Exit / End` | ✓ |
| `Global / Local` with type hints (`%`, `#`, `!`, `$`) | ✓ |
| `Const` — `constexpr int/float` and `const bbString` | ✓ |
| `Dim` — 1D and multi-dimensional arrays | ✓ |
| `Goto / Gosub / Return` (label-based flow) | ✓ |
| `Data / Read / Restore` | ✓ |
| `Type` declarations with fields, `New`, `Delete` | ✓ |
| Type field access (`\` operator) | ✓ |
| Type iteration — `First`, `Last`, `Before`, `After`, `Each` | ✓ |
| `True`, `False`, `Null` | ✓ |
| `#Include` with circular dependency protection | ✓ |
| Operators: `And`, `Or`, `Xor`, `Not`, `Mod`, `Shl`, `Shr`, `Sar`, `^` | ✓ |

### Built-in Commands (103 total)

**Math** — `Sin`, `Cos`, `Tan`, `ASin`, `ACos`, `ATan`, `ATan2`, `Sqr`, `Abs`, `Log`, `Log10`, `Exp`, `Floor`, `Ceil`, `Sgn`, `Pi`

**Random** — `Rnd`, `Rand`, `SeedRnd`, `RndSeed`

**Strings** — `Str`, `Int`, `Float`, `Len`, `Left`, `Right`, `Mid`, `Instr`, `Replace`, `Upper`, `Lower`, `Trim`, `LSet`, `RSet`, `Chr`, `Asc`, `Hex`, `Bin`, `String`

**Time & System** — `MilliSecs`, `CreateTimer`, `WaitTimer`, `FreeTimer`, `AppTitle`, `SystemProperty`, `RuntimeError`, `ExecFile`, `Delay`

**File I/O** — `OpenFile`, `ReadFile`, `WriteFile`, `CloseFile`, `SeekFile`, `FilePos`, `FileSize`, `ReadLine`, `ReadByte`, `ReadShort`, `ReadInt`, `ReadFloat`, `ReadString`, `WriteLine`, `WriteByte`, `WriteShort`, `WriteInt`, `WriteFloat`, `WriteString`, `FileType`, `CurrentDir`, `ChangeDir`, `CreateDir`, `DeleteDir`, `DeleteFile`, `NextFile`, `FirstFile`, `CopyFile`

**Banks** — `CreateBank`, `FreeBank`, `BankSize`, `ResizeBank`, `CopyBank`, `PeekByte`, `PeekShort`, `PeekInt`, `PeekFloat`, `PokeByte`, `PokeShort`, `PokeInt`, `PokeFloat`

**Input** — `KeyDown`, `KeyHit`, `WaitKey`, `FlushKeys`, `Input`, `MouseX`, `MouseY`, `MouseZ`, `MouseXSpeed`, `MouseYSpeed`, `MouseDown`, `MouseHit`, `WaitMouse`, `FlushMouse`, `MoveMouse`, `JoyType`, `JoyX`, `JoyY`, `JoyDown`, `JoyHit`

**Audio** — `LoadSound`, `FreeSound`, `PlaySound`, `LoopSound`, `StopChannel`, `ChannelPlaying`, `ChannelVolume`, `ChannelPan`, `ChannelPitch`, `PauseChannel`, `ResumeChannel`, `PlayMusic`, `StopMusic`, `MusicPlaying`, `Load3DSound`, `Channel3DPosition`, `ListenerPosition`

**2D Graphics** — `Graphics`, `EndGraphics`, `GraphicsWidth`, `GraphicsHeight`, `Cls`, `Flip`, `BackBuffer`, `FrontBuffer`, `SetBuffer`, `CopyRect`, `Color`, `ClsColor`, `GetColor`, `Plot`, `TotalVidMem`, `AvailVidMem`

### Compiler & Tooling
- **One-step build**: `blitzcc myfile.bb` → transpile to C++ → compile → `myfile.exe`
- **GCC-compatible error format**: `file:line:col: error: message` (parseable by any IDE)
- **Exit codes**: 0 = success, 1 = parse error, 2 = compile error
- **`-k` / `+k`**: dumps all known built-in names / signatures (Blitz3D IDE compatible)
- **`BLITZPATH`** env var: fallback toolchain root for non-standard installs

---

## CLI Reference

```
blitzcc [options] <file.bb>

  -h          Show help
  -v          Show version
  -q          Quiet mode (suppress progress output)
  +q          Very quiet mode
  -c          Transpile only — emit .cpp, skip compile step
  -d          Debug build (passes -g to g++, keeps .cpp)
  -release    Release build (default; explicit flag for IDE compatibility)
  -o <name>   Output executable name (without .exe)
  -k          List all known built-in command names
  +k          List built-in commands with parameter signatures

Environment:
  BLITZPATH   Fallback root for toolchain lookup (after CWD and ../)
```

---

## Architecture

BlitzNext is a single-pass transpiler. The entire compiler fits in `src/compiler/`:

```
src/compiler/
  blitzcc.cpp       ← entry point, CLI, build orchestration
  lexer.h           ← case-insensitive tokenizer
  preprocessor.h    ← #Include handling
  token.h           ← token types
  ast.h             ← AST node definitions
  parser.h          ← recursive-descent parser
  emitter.h         ← C++17 code generator (Visitor)
  bb_runtime.h      ← core runtime (types, data, I/O)
  bb_math.h         ← math functions
  bb_string.h       ← string functions
  bb_system.h       ← time, system, process
  bb_file.h         ← file I/O
  bb_bank.h         ← memory banks
  bb_sdl.h          ← SDL3 window & event loop
  bb_input.h        ← keyboard, mouse, joystick
  bb_sound.h        ← audio playback
  bb_sound3d.h      ← 3D positional audio
  bb_graphics2d.h   ← 2D graphics
```

The runtime is **header-only** — the generated `.cpp` file `#include`s only what it needs, then gets compiled by the bundled MinGW g++.

---

## Roadmap Overview

35 of 65 milestones complete. See [roadmap.md](roadmap.md) for full detail.

| Phase | Scope | Status |
|-------|-------|--------|
| A — IDE & CLI | Error format, CLI flags | ✓ Done |
| B — Language Core | Types, Arrays, Const, Data, Goto | ✓ Done |
| C — Math | Trig, random | ✓ Done |
| D — Strings | Extraction, transformation, encoding | ✓ Done |
| E — Time & System | MilliSecs, Timer, AppTitle, ExecFile | ✓ Done |
| F — File I/O | Open/Read/Write/Dir | ✓ Done |
| G — Banks | Alloc, Peek, Poke | ✓ Done |
| H — SDL3 | Window init, event loop | ✓ Done |
| I — Input | Keyboard, Mouse, Joystick | ✓ Done |
| J — Audio | Sound, Music, 3D audio | ✓ Done |
| K — 2D Graphics | Window, buffer, color, pixel, shapes, text, images | **In progress** (M38–40 ✓, M41–46 open) |
| L–T — 3D Graphics | Scene, textures, mesh, entities, camera, collision, animation | Planned |

---

## Building from Source

```bat
build_windows.bat
```

This script downloads MinGW and SDL3 on first run, then builds the compiler via CMake. Subsequent runs skip the download if the toolchain is already present.

Linux build:
```bash
bash build_linux.sh
```

---

## Running Tests

```bash
bash tests/run_tests.sh
```

Compiles all `tests/test_*.bb` files and compares output against `tests/*.expected`. Negative tests (`tests/neg_*.bb`) verify that malformed programs are rejected with exit code 1.

---

## Developer Log

See [DEVLOG.md](DEVLOG.md) for a full session-by-session changelog.
