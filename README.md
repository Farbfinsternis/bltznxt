<p align="center">
  <img src="logo.png" alt="BlitzNext Logo" width="480">
</p>

# BlitzNext

**BlitzNext** is a modern compiler that converts Blitz3D (`.bb`) source files directly into native Windows executables via a C++17 transpilation pipeline. It targets 100% command parity with the original Blitz3D engine, using a bundled MinGW toolchain and SDL3 for audio and graphics.

> **Status: active development — v0.5.0.** BlitzNext compiles the unmodified game **blox-n-balls**. The runtime now includes sprites, mirrors, collisions and line/entity picking; full gameplay compatibility is still being verified.
> **[KNOWN_ISSUES.md](KNOWN_ISSUES.md) lists everything that does not yet behave like Blitz3D** — please check it before reporting a bug.
> See [roadmap.md](roadmap.md) and [ROADMAP3D.md](ROADMAP3D.md) for the milestones and [DEVLOG.md](DEVLOG.md) for the changelog.

---

## In Memory of Mark Sibly

Blitz3D was the work of one person. Mark Sibly built a language that let thousands of people make their first game, write their first loop, see something move on screen for the first time. It was direct, it was fast, and it got out of the way. No engine before it was quite so honest about what programming could feel like when the barrier was low enough.

Mark passed away in 2024. BlitzNext exists to carry his idea forward — the belief that making something should be simple, and that simple things can still be powerful.

---

## Who Is This For?

**Blitz3D veterans** — Your old `.bb` projects deserve to run on modern hardware. BlitzNext brings the language you know forward without changing its feel. No rewrites, no ports — just compile and go.

**Retro enthusiasts** — Blitz3D had a charm that modern engines don't replicate: instant results, no boilerplate, a game on screen in ten lines of code. That philosophy is alive here.

**Beginners** — Blitz3D was one of the friendliest ways to learn programming ever made. `Print "Hello"` works. No classes, no frameworks, no build configuration. BlitzNext keeps that low floor intact while targeting modern Windows natively.

---

## Compatibility Progress

| Area | State (2026-09-18) |
|------|--------------------|
| **Language** | All constructs except `Handle` and `Object` |
| **Built-in commands** | 458 entries in `src/compiler/commands.h`, including extensions; this counts signatures, not verified behaviour |
| **2D milestones** | Milestones 6–46 complete ([roadmap.md](roadmap.md)) |
| **3D runtime** | Meshes, surfaces, brushes, sprites, mirrors, collisions and line/entity picking available; remaining work in [ROADMAP3D.md](ROADMAP3D.md) |
| **Known deviations** | 46 open bugs, all reproduced against Blitz3D 11.8 ([KNOWN_ISSUES.md](KNOWN_ISSUES.md)) |
| **Primary integration test** | blox-n-balls: all 26 source files unchanged, full executable build verified; complete gameplay not yet verified |

Compatibility is measured, not estimated: questions about the language are answered from the
[original source](https://github.com/blitz-research/blitz3d), and results are compared with a
running Blitz3D 11.8. In the 2026-09-15/16 baseline, 67 of 156 example sources were accepted
and all 67 built; of the 70 accepted only by Blitz3D, 58 failed solely on missing commands.
That corpus has not been fully remeasured after the latest additions; these are historical
baseline figures, not current coverage percentages.

**Language.** Every construct except `Handle` and `Object` is implemented, but several still
differ from Blitz3D in detail — most importantly float-to-integer conversion (truncates instead
of rounding), `Include` on a line with other statements, and nested `Gosub`. See
[KNOWN_ISSUES.md](KNOWN_ISSUES.md).

**Runtime.** Math, strings, files, banks, input, audio and 2D graphics are available. A few
commands remain incomplete, including pixel-accurate `ImagesCollide`, `SystemProperty` and
`CallDLL`. `CopyRect`, image/texture buffer drawing and multi-camera rendering have received
compatibility fixes. In 3D, entities, cameras, lights, textures, brushes, primitive meshes,
`.x`/`.3ds` loading, the surface API, sprites, mirrors, collisions and line/entity picking
are available. Camera picking/projection, animation, terrain, fog and planes remain missing.
TCP streams and hostname lookup are available; UDP and DirectPlay remain missing.

**Blitz2D compatibility** is a practical secondary target. The 2D runtime is available, with
remaining gaps listed in [KNOWN_ISSUES.md](KNOWN_ISSUES.md); no broad game-compatibility rate
has been established.

**Test priority.** blox-n-balls is the primary real-game integration test: changes should
preserve its build and be checked through loading, menus, input, gameplay, level transitions
and save/load where applicable. It uses only a subset of Blitz3D. Passing this game does
not establish complete language, command or runtime compatibility, and features it does not
use remain in scope. Focused regression tests, comparisons with Blitz3D 11.8 and additional
demos/games must cover those gaps.

---

## Getting Started

### Requirements
- Windows 10/11
- `curl` and `tar` (built into modern Windows)

### Setup
```bat
git clone https://github.com/Farbfinsternis/bltznxt
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
| `If / ElseIf / Else / EndIf` (also `End If`, `End Function` etc.) | ✓ |
| `While / Wend` | ✓ |
| `Repeat / Until / Forever` | ✓ |
| `For / Next` (with `Step`, including negative) | ✓ |
| `Select / Case / Default` | ✓ |
| `Function / Return / Exit / End` | ✓ |
| `Global / Local` with type hints (`%`, `#`, `!`, `$`) | ✓ |
| `Const` | ✓ ¹ |
| `Dim` — 1D and multi-dimensional arrays | ✓ |
| `Goto / Gosub / Return` (label-based flow) | ✓ ¹ |
| `Data / Read / Restore` | ✓ ¹ |
| `Type` declarations with fields, `New`, `Delete` | ✓ |
| Type field access (`\` operator) | ✓ |
| Type iteration — `First`, `Last`, `Before`, `After`, `Each` | ✓ |
| `True`, `False`, `Null` | ✓ |
| `Include` / `#Include` with circular dependency protection | ✓ ¹ |
| Operators: `And`, `Or`, `Xor`, `Not`, `Mod`, `Shl`, `Shr`, `Sar`, `^` | ✓ |
| `Handle`, `Object` | — |

¹ Works, with known deviations from Blitz3D — see [KNOWN_ISSUES.md](KNOWN_ISSUES.md).

### Built-in Commands (458 table entries)

The groups below are an overview. `blitzcc -k` prints the complete list, `blitzcc +k` the signatures. Commands that exist but do not
yet work like Blitz3D are listed in [KNOWN_ISSUES.md](KNOWN_ISSUES.md).

**Math** — `Sin`, `Cos`, `Tan`, `ASin`, `ACos`, `ATan`, `ATan2`, `Sqr`, `Log`, `Log10`, `Exp`, `Floor`, `Ceil`, `Min`, `Max` (`Abs` and `Sgn` are reserved words, not commands: unary operators over the following expression, so `Abs -3` needs no parentheses. `Pi` likewise is a reserved word for the constant.) (`Pi` is not a command but a reserved word for the constant, as in Blitz3D — it takes no parentheses and cannot be declared)

**Random** — `Rnd`, `Rand`, `SeedRnd`, `RndSeed`

**Strings** — `Len`, `Left`, `Right`, `Mid`, `Instr`, `Replace`, `Upper`, `Lower`, `Trim`, `LSet`, `RSet`, `Chr`, `Asc`, `Hex`, `Bin`, `String` (`Str`, `Int` and `Float` are reserved words, not commands: casts over the following expression, written `Str n` or `Str$ n` as in Blitz3D.)

**Time & System** — `MilliSecs`, `CurrentDate`, `CurrentTime`, `CreateTimer`, `WaitTimer`, `FreeTimer`, `AppTitle`, `CommandLine`, `GetEnv`, `SetEnv`, `SystemProperty`, `RuntimeError`, `ExecFile`, `CallDLL`, `Delay`, `ShowPointer`, `HidePointer`, `Notify`, `Confirm`, `Proceed`

**File I/O** — `OpenFile`, `ReadFile`, `WriteFile`, `CloseFile`, `SeekFile`, `FilePos`, `Eof`, `ReadAvail`, `FileSize`, `ReadBytes`, `WriteBytes`, `ReadDir`, `CloseDir`, `ReadLine`, `ReadByte`, `ReadShort`, `ReadInt`, `ReadFloat`, `ReadString`, `WriteLine`, `WriteByte`, `WriteShort`, `WriteInt`, `WriteFloat`, `WriteString`, `FileType`, `CurrentDir`, `ChangeDir`, `CreateDir`, `DeleteDir`, `DeleteFile`, `NextFile`, `FirstFile`, `CopyFile`

**Banks** — `CreateBank`, `FreeBank`, `BankSize`, `ResizeBank`, `CopyBank`, `PeekByte`, `PeekShort`, `PeekInt`, `PeekFloat`, `PokeByte`, `PokeShort`, `PokeInt`, `PokeFloat`

**Input** — `KeyDown`, `KeyHit`, `GetKey`, `WaitKey`, `FlushKeys`, `Input`, `MouseX`, `MouseY`, `MouseZ`, `MouseXSpeed`, `MouseYSpeed`, `MouseZSpeed`, `MouseDown`, `GetMouse`, `MouseHit`, `WaitMouse`, `FlushMouse`, `MoveMouse`, `JoyType`, `JoyX`, `JoyY`, `JoyZ`, `JoyU`, `JoyV`, `JoyHat`, `JoyDown`, `JoyHit`, `WaitJoy`, `GetJoy`, `FlushJoy`

**Audio** — `LoadSound`, `FreeSound`, `PlaySound`, `LoopSound`, `StopChannel`, `ChannelPlaying`, `ChannelVolume`, `ChannelPan`, `ChannelPitch`, `PauseChannel`, `ResumeChannel`, `SoundVolume`, `SoundPan`, `SoundPitch`, `PlayMusic`, `StopMusic`, `MusicPlaying`, `PlayCDTrack`, `Load3DSound`, `SoundRange`, `Channel3DPosition`, `Channel3DVelocity`, `ListenerPosition`, `ListenerOrientation`, `ListenerVelocity`, `WaitSound`

**2D Graphics — Window & Buffer** — `Graphics`, `EndGraphics`, `GraphicsWidth`, `GraphicsHeight`, `GraphicsDepth`, `GraphicsRate`, `GraphicsMode`, `TotalVidMem`, `AvailVidMem`, `BackBuffer`, `FrontBuffer`, `SetBuffer`, `Cls`, `Flip`, `CopyRect`, `Origin`, `Viewport`

**2D Graphics — Color & Drawing** — `Color`, `ClsColor`, `ColorRed`, `ColorGreen`, `ColorBlue`, `GetColor`, `Rgb`, `Plot`, `Line`, `Rect`, `Oval`, `Poly`

**2D Graphics — Text & Fonts** — `Write`, `Locate`, `Text`, `LoadFont`, `SetFont`, `FreeFont`, `FontWidth`, `FontHeight`, `StringWidth`, `StringHeight`

**2D Graphics — Images** — `LoadImage`, `LoadAnimImage`, `CreateImage`, `FreeImage`, `DrawImage`, `DrawImageRect`, `DrawBlock`, `DrawBlockRect`, `GrabImage`, `CopyImage`, `SaveImage`, `ImageWidth`, `ImageHeight`

**2D Graphics — Image Manipulation** — `HandleImage`, `MidHandle`, `AutoMidHandle`, `ImageXHandle`, `ImageYHandle`, `ScaleImage`, `RotateImage`, `FlipImage`, `MirrorImage`, `MaskImage`, `TileImage`, `TileBlock`, `DrawImageEllipse`, `ImagesOverlap`, `ImageRectOverlap`, `ImagesCollide`, `ImageRectCollide`, `ImagesColl`, `ImageXColl`, `ImageYColl`

**2D Graphics — Pixel Buffer** — `ImageBuffer`, `LockBuffer`, `UnlockBuffer`, `ReadPixel`, `WritePixel`, `ReadPixelFast`, `WritePixelFast`, `CopyPixel`, `CopyPixelFast`, `LoadBuffer`, `SaveBuffer`, `BufferWidth`, `BufferHeight`

**3D Graphics — Graphics Modes** — `CountGfxDrivers`, `GfxDriverName`, `SetGfxDriver`, `CountGfxModes`, `CountGfxModes3D`, `GfxModeExists`, `GfxModeWidth`, `GfxModeHeight`, `GfxModeDepth`, `Windowed3D`

**3D Graphics — Context & Scene** — `Graphics3D`, `UpdateWorld`, `RenderWorld`, `ClearWorld`, `CaptureWorld`, `TrisRendered`, `AmbientLight`, `Wireframe`, `Dither`, `WBuffer`, `AntiAlias`, `HWMultiTex`, `CameraClsMode`, `CameraClsColor`

**3D Graphics — Entities** — `CreatePivot`, `CopyEntity`, `FreeEntity`, `HideEntity`, `ShowEntity`, `NameEntity`, `EntityName`, `EntityClass`, `EntityParent`, `GetParent`, `CountChildren`, `GetChild`, `FindChild`, `EntityOrder`

**3D Graphics — Entity Appearance** — `EntityColor`, `EntityAlpha`, `EntityShininess`, `EntityBlend`, `EntityFX`, `EntityAutoFade`, `EntityTexture`, `PaintEntity`, `GetEntityBrush`

**3D Graphics — Transforms** — `PositionEntity`, `MoveEntity`, `TranslateEntity`, `RotateEntity`, `TurnEntity`, `ScaleEntity`, `PointEntity`, `AlignToVector`, `ResetEntity`, `EntityX`, `EntityY`, `EntityZ`, `EntityPitch`, `EntityYaw`, `EntityRoll`, `EntityDistance`

**3D Graphics — Camera** — `CreateCamera`, `CameraRange`, `CameraZoom`, `CameraProjMode`, `CameraViewport`, `CameraClsMode`, `CameraClsColor`

**3D Graphics — Lights** — `CreateLight`, `LightColor`, `LightRange`, `LightConeAngles`

**3D Graphics — Textures** — `CreateTexture`, `LoadTexture`, `LoadAnimTexture`, `FreeTexture`, `TextureBlend`, `TextureCoords`, `ScaleTexture`, `PositionTexture`, `RotateTexture`, `TextureWidth`, `TextureHeight`, `TextureBuffer`, `TextureName`, `TextureFilter`, `ClearTextureFilters`, `SetCubeFace`, `SetCubeMode`, `ActiveTextures`, `HWTexUnits`

**3D Graphics — Sprites & Mirrors** — `CreateSprite`, `LoadSprite`, `RotateSprite`, `ScaleSprite`, `HandleSprite`, `SpriteViewMode`, `CreateMirror`

**3D Graphics — Collision & Picking** — `Collisions`, `ClearCollisions`, `EntityType`, `GetEntityType`, `EntityRadius`, `EntityBox`, `EntityCollided`, `CountCollisions`, the `Collision…` queries, `LinePick`, `EntityPick`, `EntityPickMode`, the `Picked…` queries, `EntityVisible`

**Networking** — TCP streams, servers, timeouts and hostname lookup; UDP and DirectPlay are not implemented.

**3D Graphics — Brushes** — `CreateBrush`, `LoadBrush`, `FreeBrush`, `BrushColor`, `BrushAlpha`, `BrushShininess`, `BrushTexture`, `BrushBlend`, `BrushFX`, `GetBrushTexture`

**3D Graphics — Meshes** — `CreateMesh`, `LoadMesh`, `LoadAnimMesh`, `LoaderMatrix`, `CreateCube`, `CreateSphere`, `CreateCylinder`, `CreateCone`, `CopyMesh`, `AddMesh`, `FlipMesh`, `PaintMesh`, `LightMesh`, `FitMesh`, `ScaleMesh`, `RotateMesh`, `PositionMesh`, `UpdateNormals`, `MeshWidth`, `MeshHeight`, `MeshDepth`, `MeshesIntersect`, `CountSurfaces`

**3D Graphics — Surfaces & Vertices** — `CreateSurface`, `GetSurface`, `FindSurface`, `ClearSurface`, `PaintSurface`, `GetSurfaceBrush`, `AddVertex`, `AddTriangle`, `CountVertices`, `CountTriangles`, `TriangleVertex`, `VertexCoords`, `VertexNormal`, `VertexColor`, `VertexTexCoords`, `VertexX`, `VertexY`, `VertexZ`, `VertexNX`, `VertexNY`, `VertexNZ`, `VertexRed`, `VertexGreen`, `VertexBlue`, `VertexAlpha`, `VertexU`, `VertexV`, `VertexW`

**3D Graphics — Maths** — `TFormPoint`, `TFormVector`, `TFormNormal`, `TFormedX`, `TFormedY`, `TFormedZ`

### Compiler & Tooling
- **One-step build**: `blitzcc myfile.bb` → transpile to C++ → compile → `myfile.exe`
- **GCC-compatible error format**: `file:line:col: error: message` (parseable by any IDE)
- **Exit codes**: 0 = success, 1 = parse error, 2 = compile error
- **`-k` / `+k`**: dumps all known built-in names / signatures (Blitz3D IDE compatible)
- **`BLITZPATH`** env var: fallback toolchain root for non-standard installs

### BLTZNXT Extensions to `Graphics`

`Graphics width, height, depth, mode` supports two BLTZNXT-specific modes beyond the Blitz3D originals:

| Mode | Behaviour |
|------|-----------|
| `5` | **Windowed, scaled, resizable.** Physical window opens at `width×2 / height×2`. All drawing uses the logical `width × height` grid — game coordinates need no changes. Drag or maximise the window and SDL3 scales the content automatically, letterboxing to preserve the aspect ratio. |
| `6` | Fullscreen + vsync. Same logical-presentation scaling as mode 1, with vsync enabled. |

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

## Repository Layout

This repository holds two deliberately independent subprojects:

```
bltznxt/
  src/compiler/   ← the compiler and its header-only runtime
  bin/ tests/ examples/ libs/ tools/
  CMakeLists.txt  build_windows.bat  build_linux.sh
  ide/            ← BLTZNXT IDE (Electron + Vite + Monaco)
```

**They have no dependency on each other, and that is a rule, not an accident.**

The only contract between them is the `blitzcc` command line — a process boundary:
arguments in, stdout/stderr and an exit code out. Concretely:

- The IDE never includes, links against, or reads anything under `src/compiler/`.
- The IDE never hard-codes a path to `bin/blitzcc.exe`. The compiler location is
  configuration, resolved in this order: IDE setting → `BLITZPATH` → `PATH` →
  optionally `../bin/blitzcc.exe` as a developer convenience. The IDE runs against
  any installed BlitzNext, and starts fine with no compiler present at all.
- The IDE never freezes the built-in command list into its own source. It calls
  `blitzcc +k` at runtime, so autocomplete stays correct against a compiler that is
  newer than the IDE. Copying `kCommands[]` into the IDE would create a silent,
  versioned coupling — don't.
- Each side builds on its own: CMake / `build_windows.bat` for the compiler, npm for
  the IDE. Neither build script references the other. They version independently.

The compiler is IDE-agnostic by design and predates the IDE: the GCC-style error
format (`file:line:col: error: message`), the exit-code contract, and the `-k` / `+k`
flags exist precisely so that *any* editor can drive it.

---

## Architecture

BlitzNext is a transpiler: preprocess, lex, parse, check, emit C++17, hand the
result to g++. The entire compiler fits in `src/compiler/`:

```
src/compiler/
  blitzcc.cpp       ← entry point, CLI, build orchestration
  lexer.h           ← case-insensitive tokenizer
  preprocessor.h    ← #Include handling
  sourcemap.h       ← stream line → (file, line), so diagnostics survive Include
  token.h           ← token types
  ast.h             ← AST node definitions
  parser.h          ← recursive-descent parser
  semant.h          ← semantic pass: symbol tables, types, arity
  commands.h        ← built-in command table (generated, see scripts/)
  emitter.h         ← C++17 code generator (Visitor)
  bb_runtime.h      ← core runtime (types, data, I/O)
  bb_math.h         ← math functions
  bb_string.h       ← string functions
  bb_system.h       ← time, system, process
  bb_file.h         ← file I/O
  bb_socket.h       ← TCP streams and hostname lookup
  bb_bank.h         ← memory banks
  bb_sdl.h          ← SDL3 window & event loop
  bb_input.h        ← keyboard, mouse, joystick
  bb_sound.h        ← audio playback
  bb_sound3d.h      ← 3D positional audio
  bb_graphics2d.h   ← 2D graphics (window, buffer, color, shapes, text, fonts)
  bb_image.h        ← image loading, drawing, manipulation, pixel buffer (M44–M46b)
  bb_gl_ctx.h       ← OpenGL 3.3 Core loader (60 fn pointers via SDL_GL_GetProcAddress)
  bb_entity_core.h  ← entity handle system, scene graph, transforms, hierarchy (3D-03–05)
  bb_camera.h       ← camera entity, view/projection matrices (3D-06)
  bb_shader.h       ← three built-in GLSL 3.3 shaders (unlit, textured, lit) (3D-07)
  bb_mesh_core.h    ← VAO/VBO/EBO upload + draw, interleaved vertex format (3D-08)
  bb_mesh.h         ← mesh entity, primitive generators, RenderWorld pass (3D-09)
  bb_graphics3d.h   ← Graphics3D, UpdateWorld, RenderWorld, scene globals (3D-01–09)
  bb_gfxmode.h      ← graphics driver and mode enumeration (3D-00)
  bb_texture.h      ← textures (3D-11)
  bb_light.h        ← lights (3D-12)
  bb_loader.h       ← LoadMesh / LoadAnimMesh, .3ds loader (3D-13)
  bb_loader_x.h     ← DirectX .x loader, text and binary (3D-13)
  bb_brush.h        ← brushes (3D-15)
  bb_surface.h      ← surfaces, vertices, triangles (3D-15)
  bb_sprite.h       ← sprites and view modes (3D-16)
  bb_mirror.h       ← reflected scene passes (3D-16)
  bb_collision.h    ← collisions, line/entity picking and visibility (3D-17–18)
  suggest.h         ← "did you mean …?" for unknown names
```

The runtime is **header-only** — the generated `.cpp` file `#include`s only what it needs, then gets compiled by the bundled MinGW g++.

---

## Roadmap Overview

Milestones 6–46 (language, runtime, 2D) are implemented, with compatibility defects tracked separately. The 3D work is tracked in [ROADMAP3D.md](ROADMAP3D.md); several milestones contain both implemented and missing commands. See [roadmap.md](roadmap.md) for the 2D detail.

Where the 3D engine is heading — one modern material model, per-pixel lighting, shadows, render passes, `Vec2`/`Vec3`/`Vec4` vectors in the language and shaders written in Blitz syntax — is laid out in the design document [ENGINE_DESIGN.md](ENGINE_DESIGN.md) (German, draft, nothing of it implemented yet).

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
| K — 2D Graphics | Window, buffer, color, shapes, text, fonts, images, pixel buffer | ✓ Done |
| L — 3D Foundation | GL context, UpdateWorld/RenderWorld, entity system, camera (3D-01–06) | ✓ Done |
| 3D-07 – 3D-12 | Shaders, geometry buffers, primitives, appearance, textures, lighting | ✓ Done |
| 3D-13, 3D-15 | `.x`/`.3ds` mesh loading, brushes and surfaces available; remaining loader work in the roadmap | Partial / implemented subsets |
| 3D-16 | Sprites and mirrors available; `CreatePlane` missing | Partial |
| 3D-17 | Line/entity picking available; camera picking, projection and fog missing | Partial |
| 3D-18 | Collision methods, responses and result queries | Implemented, reference-tested |
| 3D-14, 3D-19 – 3D-23 | OBJ loader, animation, remaining 3D maths, terrain, MD2/BSP | Remaining roadmap work |

---

## Building from Source

```bat
build_windows.bat
```

This script downloads MinGW and SDL3 on first run, then builds the compiler via CMake. Subsequent runs skip the download if the toolchain is already present.

BlitzNext currently runs on **Windows only**. `build_linux.sh` exists, but the compiler still
depends on the Windows API and the bundled MinGW toolchain, so a Linux build does not work yet
(see [KNOWN_ISSUES.md](KNOWN_ISSUES.md)).

---

## Running Tests

```bash
bash tests/run_tests.sh
```

The suite contains **258 tests: 163 positive and 95 negative**. Positive tests with an
`.expected` file are executed and their stdout is compared; the others are compile-only.
Negative tests require a nonzero compiler exit status and, where an `.expected_err` exists,
an exact diagnostic match. The current runner does not enforce exit code 1 specifically,
check the executed program's exit status, or impose timeouts.

Where expected values are identified as reference measurements, they come from Blitz3D 11.8.
The primary game test supplements this suite; it does not replace coverage of features absent
from blox-n-balls.

---

## Developer Log

See [DEVLOG.md](DEVLOG.md) for a full session-by-session changelog.

---

## The Blitz3D Source as a Reference

Blitz3D was released as open source under the zlib/libpng license and is archived at
[blitz-research/blitz3d](https://github.com/blitz-research/blitz3d). BlitzNext consults it as a
**behavioural reference**: when a question comes up about what the language actually does, the answer
is read out of the original compiler rather than guessed at.

That distinction matters, because guessing turned out to be expensive. A review that reconstructed
the language from intuition alone got several rules wrong in ways that produced silently incorrect
programs — and got two *bug reports* wrong as well, describing correct behaviour as broken. Reading
the original settled each of them in minutes:

| Question | Answer in `blitz-research/blitz3d` |
|----------|-----------------------------------|
| Operator precedence | `compiler/parser.cpp`: primary → unary → `^` → `*` `/` `Mod` → `Shl` `Shr` `Sar` → `+` `-` → comparisons → `And` `Or` `Xor` → `Not` |
| Is `^` right-associative? | No — left-associative, so `2^3^2` is 64 |
| Do `And` and `Or` have separate levels? | No — one level, evaluated left to right |
| Are `a$` and `a%` two variables? | No — the name alone identifies the variable; a contradicting tag is a `Variable type mismatch` error (`compiler/varnode.cpp`) |
| Is `"text" + n` an error? | No — a `+` with a string on either side makes the whole expression a string and converts the other side (`compiler/exprnode.cpp`) |
| Is an undeclared variable an error? | No — it is created on first use (the `//ugly auto decl!` branch in `varnode.cpp`) |
| What are the argument-count errors called? | `Too many parameters` / `Not enough parameters` (`ExprSeqNode::castTo`) |

**No source code is copied.** The rules above were read and re-implemented; `src/compiler/semant.h`
names the original file for each rule it enforces, so any of them can be checked against the source.
The command table in `src/compiler/commands.h` is generated from BlitzNext's *own* runtime headers by
`scripts/gen_commands.py`, not from Blitz3D's — it has to describe what this runtime accepts, which is
not always the same set.

### Checking against a running Blitz3D

Reading the source answers what the language *means*. A Blitz3D installation answers what it
*does* — and the two are not always the same thing when the reading is mine. If Blitz3D is
installed locally, `scripts/compare_reference.sh` runs every program in `tests/` and
`examples/` through both compilers with `-c` and reports only the verdict and the message:

```
bash scripts/compare_reference.sh [path/to/Blitz3D]     # or set $BLITZ3D_HOME
```

Nothing is executed, so it takes seconds. It is a finding list, not a failure list: a `neg_*.bb`
that pins a deliberate extra diagnostic belongs in the "we reject, it accepts" column, and
commands from this project's SDL layer show up as `Function 'x' not found` and are counted
separately as a library difference rather than a language one.

Its first run found six divergences that source-reading had missed, and two wrong assumptions in
tests written that same day — both derived from the original compiler's source, both plausible,
both wrong. What it cannot do is compare program *output*: a Blitz3D program draws into its own
window rather than writing to stdout.

### Where the reference ends: rendering

BlitzNext does not rebuild Blitz3D's picture; it implements what the 3D commands **mean**. The
original is the reference for everything a program can **observe or rely on** — geometry, vertex
and triangle counts, normals, winding, transforms, hierarchy, picks, collisions, which entities and
cameras are drawn, what a texture contains, and every value a command returns. Those must match
exactly and are measured against a running Blitz3D.

The *shading* is not bound to Direct3D 7. Gouraud shading, per-vertex specular and unbounded
`range/distance` attenuation were the limits of a 1999 fixed pipeline, not choices the authors of
Blitz3D programs made. BlitzNext keeps the role of every parameter — `EntityColor` is the diffuse
colour, `LightRange` is how far a light reaches, `EntityFX 1` ignores lighting, negative light
colours darken — but will compute lighting in a modern way, per pixel. There will be no compatible
lighting mode; today's renderer still approximates the old per-vertex lighting and is being replaced.
An old scene may therefore look darker or brighter than it did in 2002; that is intended and can be
corrected in the program with light colours, ranges or `AmbientLight`. Colours read back from the
rendered image with `ReadPixel` are not guaranteed to match Blitz3D.

The full guideline, including the table of parameter roles, is in [ROADMAP3D.md](ROADMAP3D.md) under
"Richtlinie: Was exakt stimmen muss und was besser werden darf".

The zlib license permits far more than this. The acknowledgement is here because the work deserves it:
a language design that is still worth reading twenty-five years later, and a compiler whose structure
makes whole classes of mistake impossible — which is a lesson this project keeps relearning.

---
## Third-Party Libraries

BlitzNext bundles the following open-source libraries. Their source files are included in `src/thirdparty/` and `libs/`.

| Library | Author | License | Purpose |
|---------|--------|---------|---------|
| [SDL3](https://libsdl.org) | Sam Lantinga & contributors | zlib | Window, renderer, audio device, events |
| [SDL3_ttf](https://github.com/libsdl-org/SDL_ttf) | Sam Lantinga & contributors | zlib | TrueType font rendering |
| [stb_image](https://github.com/nothings/stb) | Sean Barrett | Public Domain / MIT | PNG, JPEG, BMP, TGA image loading |
| [stb_image_write](https://github.com/nothings/stb) | Sean Barrett | Public Domain | Image saving (PNG, BMP, TGA) |
| [stb_vorbis](https://github.com/nothings/stb) | Sean Barrett | Public Domain / MIT | OGG Vorbis audio decoding |
| [dr_mp3](https://github.com/mackron/dr_libs) | David Reid | Public Domain / MIT-0 | MP3 audio decoding |
| [MinGW-w64](https://www.mingw-w64.org) | Various | GCC Runtime Exception + LGPL | C++ toolchain (bundled, downloaded at build time) |

The SDL3 and SDL3_ttf zlib licenses require that the license text is preserved in source and binary distributions and that the libraries are not misrepresented as original work. The stb libraries and dr_mp3 are public domain — no attribution is legally required, though it is given here as a matter of courtesy.

License texts for SDL3 and SDL3_ttf are included in `libs/sd3/LICENSE.txt` and `libs/sdl3_ttf/LICENSE.txt`. License statements for the stb libraries and dr_mp3 are embedded at the end of each respective header file.

---

## A Note on Authorship

BlitzNext was conceived and directed by Farbfinsternis, who had the excellent idea that someone should modernise a beloved programming language and the equally excellent follow-up idea that *someone* did not necessarily have to mean *him, entirely, alone*.

The lexer, the parser, the abstract syntax tree, the C++17 code generator, the SDL3 audio pipeline, 283 runtime functions, and several hundred deeply specific opinions about reserved identifiers were largely produced by Claude, a large language model made by Anthropic, who is available around the clock, harbours no grudges about being asked to rewrite the same function four times, and finds the phrase "undefined behaviour" quietly alarming.

This collaboration produces, if you stop and think about it, a rather peculiar situation: a compiler for a programming language created by one person has been substantially written by a different kind of entity entirely, at the direction of a third person, for the benefit of anyone who happens to have a `.bb` file from 2003 gathering dust on a hard drive. The universe, Mark Sibly once observed (almost certainly), is under no obligation to make sense.

It works, though. That part is not peculiar at all.

*Co-authored with [Claude](https://claude.ai) — because the alternative was typing it all by hand.*
