# Known Issues

BlitzNext is meant to replace Blitz3D: an old program must compile unchanged, must not crash
or stop halfway, and must stay playable. Small differences in looks, rounding or timing are
accepted. This page lists where that promise is **not yet** kept — programs that are rejected
although Blitz3D accepts them, programs that run but work differently, and commands that
exist but do nothing — and, separately, the differences that are known and accepted.

Every entry below has been reproduced against a running **Blitz3D 11.8** and checked
against the official source code at
[blitz-research/blitz3d](https://github.com/blitz-research/blitz3d). The `BUG-nn` numbers
refer to the project's internal tracker, so fixes can be found in [DEVLOG.md](DEVLOG.md)
and the commit history.

*Last updated: 2026-09-26 — 18 open bugs.*

If an old program breaks and the cause is not listed here, please open an issue with a
minimal `.bb` file and a description of what happens in both.

**Contents**

- [Intentional differences](#intentional-differences)
- [Accepted differences](#accepted-differences)
- [Missing commands](#missing-commands)
- [Silently different results](#silently-different-results) — read this first
- [Language and compiler](#language-and-compiler)
- [Data, Read and Restore](#data-read-and-restore)
- [Runtime library](#runtime-library)
- [2D graphics](#2d-graphics)
- [3D graphics](#3d-graphics)
- [Platform and tooling](#platform-and-tooling)

---

## Intentional differences

These are not bugs but decisions, and they will stay:

- **3D lighting is not a copy of Direct3D 7.** BlitzNext keeps what every lighting and
  material command means — `EntityColor` is the diffuse colour, `LightRange` how far a light
  reaches, `EntityFX 1` ignores lighting, negative light colours darken — but the lighting
  itself is being rebuilt to be computed per pixel with modern formulas. Old scenes can look
  darker, brighter or smoother than they did. If a scene is too dark or too bright, adjust
  `AmbientLight`, the light colours or `LightRange` in the program.
  Where BlitzNext lights a model correctly and Blitz3D did not, BlitzNext stays correct. One
  example: an object scaled unevenly with `ScaleEntity` (say `1.5,0.5,1`) is shaded
  differently, because Direct3D 7 stretched its normals along with the object. (BUG-177)
- **Colours read back from the rendered 3D image are not guaranteed.** `ReadPixel` or
  `CopyRect` after `RenderWorld` return BlitzNext's shading, not Direct3D 7's. What is in the
  image — geometry, visibility, texture contents — does match.

- **In a window, a loop with `Flip` can run faster than in Blitz3D.** Neither waits for the
  display there — Blitz3D's `Flip` does not sync in windowed mode on today's Windows — but
  Blitz3D still pays for its DirectDraw blit (about 6.5 ms a frame, measured: 164 frames per
  second on a 60 Hz monitor), while BlitzNext presents almost for free. Programs that move a
  fixed step per frame therefore run faster here; time them with `MilliSecs()`. In
  fullscreen both wait for the display with `Flip`/`Flip 1` and neither waits with `Flip 0`.
  (BUG-178)

- **Userlibs (`.decls`) are not supported, and will not be.** Blitz3D extends its command set with
  32-bit Windows DLLs declared in `userlibs/*.decls`; the interface prescribes `_stdcall` and hands
  the DLL raw addresses of banks and objects with no size and no type. BlitzNext leaves that out.
  Most of what people used it for was teaching Blitz3D what it could not do — up to whole rendering
  engines — and that is work BlitzNext does itself. A replacement that runs on Windows, Linux and
  macOS is planned for later; it will not be compatible with `.decls`, so programs that rely on a
  userlib need changes. A program that calls such a function is told which `.decls` file declares
  it, rather than that the command is unknown. The reasoning is in [VISION.md](VISION.md).

- **Entity commands always check their handles, as Blitz3D's debug mode does.** A freed or
  never-created entity, or one of the wrong kind, ends the program with Blitz3D's debug
  message — `Entity does not exist`, `Entity is not a camera`, `Parent entity does not exist`,
  `Collision index out of range` — on stderr. Blitz3D checks only in debug mode; a release
  build there crashes on handle 0 ("Memory access violation"), reads a freed entity's old
  values and lets `CameraZoom` on a cube write into foreign memory. A program that only ran
  by that accident stops here with the message. (BUG-170)
- **Fields of `Null` or a deleted object are always checked, as in Blitz3D's debug mode.**
  `p\x` on such a reference ends the program with `Object does not exist` on stderr, and so
  do `After Null` and `Before Null`. A Blitz3D release build crashes there. (BUG-104)

The full reasoning is in [ROADMAP3D.md](ROADMAP3D.md), section
"Richtlinie: Was exakt stimmen muss und was besser werden darf".

---

## Accepted differences

These differ from Blitz3D, but they do not stop an old program from working, so they are not
counted as bugs and are not planned to be fixed:

- **Last digits and rounding.** `ASin`, `ACos` and `Exp` can differ in the last digit
  (BUG-163). After several turns, `EntityRoll` may report -180 where Blitz3D reports 180 —
  the same orientation — and angles can differ in the last digits, because Blitz3D stores
  rotations as quaternions (BUG-150).
- **Order of evaluation.** A call that deletes or changes an object may run after the rest of
  the expression: in `F(w) + (w = Null)`, where `F` deletes `w`, the comparison can see `w`
  before the call. Blitz3D does not fix the order either. Put such a call into its own
  statement. (BUG-180)
- **Looks that the new renderer will change anyway.** `UpdateNormals` does not merge vertices
  at the same position, so a cube stays faceted where Blitz3D rounds its corners (BUG-134).
  Spherical environment mapping (texture flag 64) is drawn as a normal texture (BUG-130).
- **Legacy model formats.** `.3ds` models number their vertices differently and have
  normalised normals (BUG-135); some `.x` models list their surfaces in a different order
  (BUG-146); texture paths with a directory part inside `.x` files are resolved relative to
  the model (BUG-76).
- **The window title** is "BLTZNXT" when the program sets no `AppTitle`; Blitz3D leaves it
  empty. (BUG-147)
- **`CallDLL`** does nothing and returns 0 — part of the decision not to support userlibs
  (see above). (BUG-123)
- **Invalid programs that Blitz3D rejects may be accepted.** A program that ran in Blitz3D
  never hits these: division by a constant zero (BUG-75), a constant index outside a fixed
  array (BUG-78), an object assigned to an untagged variable (part of BUG-80), `Global x`
  after `x` has been used (BUG-81), two functions with the same name (BUG-138), the same
  `Local` declared twice (BUG-161). String functions with an invalid position or length
  (`Mid(s, 0)`) continue with a guessed value where Blitz3D stops with an error (BUG-125).

---

## Missing commands

104 of Blitz3D's commands (not counting language keywords) are not available yet. A
program that uses one of them is rejected with `unknown function or command`. `blitzcc -k` lists
everything that is available.

| Area | Missing |
|------|---------|
| Networking | all UDP and DirectPlay commands (`CreateUDPStream`, `SendUDPMsg`, `HostNetGame`, `SendNetMsg`, …) and `CopyStream`; TCP is available |
| Picking and projection | `CameraPick`, `CameraProject`, `ProjectedX/Y/Z`, `EntityInView` |
| Animation | `Animate`, `SetAnimTime`, `AnimTime`, `Animating`, `AnimSeq`, `AnimLength`, `LoadAnimSeq`, `AddAnimSeq`, `ExtractAnimSeq`, `SetAnimKey` |
| Terrain, BSP | `CreateTerrain`, `LoadTerrain`, `ModifyTerrain`, `TerrainHeight`, …, `LoadBSP`, `BSPAmbientLight`, `BSPLighting` |
| Planes | `CreatePlane` |
| Camera fog | `CameraFogMode`, `CameraFogRange`, `CameraFogColor` |
| 3D maths | `VectorYaw`, `VectorPitch`, `DeltaYaw`, `DeltaPitch`, `GetMatElement`, `TFormFilter` |
| Movies | `OpenMovie`, `DrawMovie`, `CloseMovie`, `MovieWidth`, `MovieHeight`, `MoviePlaying` |
| Other | `RectsOverlap`, `ResizeImage`, `TFormImage`, `VWait`, `ScanLine`, `GraphicsBuffer`, `BufferDirty`, `Stop`, `MouseWait`, `JoyWait`, the gamma commands, `CreateListener`, `EmitSound`, `MeshCullBox`, `Stats3D`, `RuntimeStats`, a few graphics-driver queries and joystick axis variants |

---

## Silently different results

These compile and run without any message, but compute something else than Blitz3D. They
are the most likely reason for an old program to behave strangely.

- **Numbers with a leading zero are read as octal.** `Print 010` prints `8`; `08` does not
  compile at all. *Workaround:* remove leading zeros. (BUG-98)
- **`WriteString` and `ReadString` use a different file format.** Blitz3D writes a 4-byte
  length followed by the characters; BlitzNext writes the characters followed by a zero
  byte. Files written by Blitz3D programs — save games, level data — are read incorrectly.
  *Workaround:* write the length with `WriteInt` and the characters with `WriteByte`, and
  read them back the same way; this format is identical in both. (BUG-117)
- **A local variable declared inside a block ends with the block.** In Blitz3D a `Local`
  belongs to the whole function. With a global `x`, a function that declares `Local x = 2`
  inside an `If` block and prints `x` after the `EndIf` prints `2` in Blitz3D and the
  global's value here. If the local has a different type than the global, the program is
  rejected.
  *Workaround:* give locals names that differ from globals. (BUG-100)

---

## Language and compiler

- **`Include` is handled line by line instead of as a statement.**
  - Code after an `Include` on the same line (`Include "a.bb" : Print "x"`) is dropped.
  - An `Include` after other statements on the same line is rejected.
  - The same file written in different case (`helper.bb`, `HELPER.BB`) is included twice.

  *Workaround:* put each `Include` on its own line and spell file names consistently. (BUG-94)
- **The type check treats an untagged variable as having the type of its first value.** In
  Blitz3D a variable created without a tag is always an integer. Valid programs are
  therefore rejected here: `x = 1.5` followed by a use of `x%` is reported as a type
  mismatch, and `x = "12" : Print x - 1` (which prints `11` in Blitz3D) as arithmetic on a
  string. *Workaround:* tag the variable where it first appears (`x% = …`). (BUG-80)
- **`Dim` inside an `If`, `While`, `For` or `Select` block of the main program** fails in the
  C++ compiler. *Workaround:* move the `Dim` to the top level. (WEAK-05)
- **`Not` applied to a string** (`Not "0"`) fails in the C++ compiler. (BUG-108)
- **An element of a fixed array as `For` counter** (`For v[0] = 1 To 3`) is rejected, and so
  is a chain of fields (`For a\b\c = …`). *Workaround:* count in a plain variable and assign
  it inside the loop. (BUG-160)
- **Integer literals wider than 32 bits** (`2147483648`, `$100000000`) are rejected, a
  decimal one only by the C++ compiler; Blitz3D wraps them around (`Print 2147483648`
  prints `-2147483648`). (BUG-11)
- **An empty source file** is rejected with "could not read file". (BUG-110)
- **An unclosed string literal** (`Print "abc` without the closing quote) is rejected.
  Blitz3D accepts it — and drops the last character. (WEAK-12)
- **Some errors are reported by the C++ compiler** instead of with a Blitz-style message,
  pointing into generated code. Every such case listed on this page is a bug. (WEAK-14)

---


## Data, Read and Restore

- **`Restore` inside a function** cannot reach a label in the main program; it is rejected
  as an undefined label. (BUG-87)

---

## Runtime library

- **`SystemProperty`** always returns an empty string (Blitz3D returns e.g. `"Intel"` for
  `"cpu"`). (BUG-122)
- **`ShowPointer` and `HidePointer`** have no effect. (BUG-124)
- `WriteString`/`ReadString`: see [Silently different results](#silently-different-results).

---

## 2D graphics

- **`ImagesCollide` and `ImageRectCollide`** compare bounding rectangles instead of visible
  pixels, and ignore the frame argument. Masked or transparent areas count as a hit.
  *Workaround:* none yet for pixel-accurate tests. (BUG-119)
- **Font heights differ.** The default font is 8 pixels high instead of 13, and a font loaded
  with `LoadFont(name, 40)` reports `FontHeight` 45 instead of 40. Text laid out with
  `FontHeight` or `StringWidth` ends up spaced differently. (BUG-139)

---

## 3D graphics

The 3D layer is under active development — see [ROADMAP3D.md](ROADMAP3D.md). Besides the
[missing commands](#missing-commands):

- **`CreateTexture` does not round sizes up to powers of two.** Blitz3D reports
  `TextureWidth` 32 for `CreateTexture(30,20)`; BlitzNext reports 30. Drawing into the texture
  buffer covers a different part of the surface. (BUG-143)
- **Changing the graphics mode keeps images and custom loader matrices.** Blitz3D frees all
  images and resets `LoaderMatrix` on `Graphics`, `Graphics3D` and `EndGraphics`. Programs
  written for Blitz3D reload their images after a mode change anyway. (BUG-140)

---

## Platform and tooling

- **Windows only.** `build_linux.sh` exists, but the compiler currently depends on the
  Windows API and the bundled MinGW toolchain; a Linux build does not work. (WEAK-24)
- **The IDE smoke test** (`ide/test/smoke.js`) reports two failures because its test file
  for the C++-error path is now caught earlier by the compiler. The IDE itself is not
  affected. (BUG-121)
- **Compiled programs open a console window** next to the graphics window, showing
  OpenGL diagnostics. Blitz3D programs have none. (BUG-175)
