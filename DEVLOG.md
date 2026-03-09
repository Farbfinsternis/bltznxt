# BlitzNext Developer Log

## v0.4.0 - "Bugfix & Hardening" (2026-03-09)

**Files touched:** `src/compiler/parser.h`, `src/compiler/emitter.h`,
`src/compiler/lexer.h`, `src/compiler/blitzcc.cpp`, `src/compiler/bb_runtime.h`,
`src/compiler/bb_math.h`, `src/compiler/bb_file.h`, `src/compiler/bb_bank.h`,
`CMakeLists.txt`, `Buglist.md`, `tests/`

This release works through the entire Buglist (WEAK-05 through WEAK-12 + one parser
bug discovered during the colon-separator analysis). No new milestones — pure quality.

### Parser: Dim forward-reference fix (WEAK-05)

`Dim`'d arrays are now recognised everywhere, even when the `Dim` statement
appears *after* the first use in the token stream (e.g. a function declared before
its array, or an include file ordered after the usage code).

- **Parser** (`parser.h`): new `preScanDims()` pre-pass — walks all tokens before
  parsing begins and registers every `Dim`-declared array name in `dimmedArrays`.
  Forward references are now correctly parsed as array accesses, not function calls.
- **Emitter** (`emitter.h`): new `collectDims()` — analogous to `collectGlobals()`.
  All top-level `Dim` arrays are forward-declared as empty `std::vector<T>` at
  **file scope** (visible to user functions). `visit(DimStmt*)` emits a re-assignment
  at the original position, preserving re-Dim semantics.

### Emitter: portable Gosub/Return (WEAK-06)

Removed GCC-only computed-goto extension (`&&label` / `goto *ptr`).
Replaced with a portable `int __gosub_ret__` variable + a `switch`-based dispatch
table emitted at the end of `main()`. Generated programs now compile with any
standard C++17 compiler.

### Emitter: array bounds checking (WEAK-07)

`[index]` → `.at(index)` in `visit(ArrayAccess*)` and `visit(ArrayAssignStmt*)`.
Out-of-bounds access now throws `std::out_of_range` with a clear message
(index + array size) instead of undefined behaviour. Applies to all dimensions.

### Runtime: resource cleanup on exit (WEAK-08)

Added `bb_file_quit_()` (`bb_file.h`) and `bb_bank_quit_()` (`bb_bank.h`).
Both called from `bbEnd()` before the SDL/audio shutdown, ensuring all open file
handles and bank allocations are released on normal program exit.

### CMake: remove spurious SDL3 linkage (WEAK-09)

`target_link_libraries(blitzcc PRIVATE SDL3::SDL3)` removed. `blitzcc` is a
source transpiler — it never calls SDL3 functions. `find_package(SDL3)` kept
(optional) for informational purposes only.

### Runtime: `bb_Int()` overload set hardened (WEAK-10)

`bb_Int(float)` → `bb_Int(double)` + new `bb_Int(int)` overload in `bb_math.h`.
Eliminates the real ambiguity that caused `bb_Int(3.9)` (double literal) to fail
to compile. Three unambiguous candidates: `bb_Int(double)`, `bb_Int(int)`,
`bb_Int(const bbString&)`.

### Runtime: `bb_DataVal` long-literal safety (WEAK-11)

Added `explicit bb_DataVal(long v)` constructor to `bb_runtime.h`. Prevents
theoretical ambiguity if the emitter ever produces `bb_DataVal(42L)`.

### Lexer: unclosed string is now a hard error (WEAK-12)

`lexString()` previously emitted a bare `std::cerr` warning (no filename, no
IDE-parseable format) and returned a partial token, letting the parser continue
silently. Now:
- GCC-format error: `file:line:col: error: unclosed string literal`
- `Lexer` tracks `lexErrors_`; `blitzcc.cpp` checks `lexer.hasErrors()` and
  returns exit code 1 immediately.
- New negative test: `tests/neg_unclosed_string.bb`.

### Parser: colon as statement separator — If/Else bug fixed

Colon (`:`) already worked as a statement separator in the main loop via
`skipNewlines()`. However, `If`/`Else` blocks with colons were mis-parsed:
`If x = 0 : Print "zero" : Else : Print "nonzero" : End If` produced wrong output
because the single-line-If detection checked only for `NEWLINE`, not for `:`.

**Fix** (`parser.h`): single-line form is now only taken when `THEN` is explicitly
present *and* the next token is neither `NEWLINE` nor `:`. Colon-separated
If/Else/End-If blocks are correctly handled as the multi-line block form.

---

## v0.3.9 - "Scaling + Branding" (2026-03-08)

**Files touched:** `src/compiler/bb_graphics2d.h`, `src/compiler/blitzcc.cpp`,
`examples/asteroids/asteroids.bb`

### New `Graphics` mode 5 — windowed scaled + resizable (`bb_graphics2d.h`)

Added **mode 5** as a BLTZNXT-specific extension to the `Graphics` command:

```
Graphics width, height, depth, 5
```

Opens a physical window at `width×2 / height×2` with `SDL_WINDOW_RESIZABLE`
set. `SDL_SetRenderLogicalPresentation` (letterbox) maps all drawing commands to
the logical `width × height` grid, so game coordinates require no changes.
Dragging or maximising the window causes SDL3 to scale the content automatically,
preserving the aspect ratio with black bars.

The default fallback window title (used when `AppTitle` is not called) was
renamed from "BlitzNext" to "BLTZNXT".

### Asteroids example — retro scaling demo + branding

- Reduced logical resolution from 800×600 to **400×300**; opened with mode 5 →
  800×600 physical window, freely resizable.
- Fixed asteroid polygon closure: factor `2.3` in `Sin(angle × 2.3)` gave a
  non-integer period (828° mod 360° = 108° ≠ 0), leaving a gap between the
  first and last vertex. Changed to integer factor `2` (period = 720° = 2×360°).
- All "BlitzNext" references in the example renamed to **BLTZNXT**.

---

## v0.3.8 - "Asteroids + Syntax + Renderer Fixes" (2026-03-08)

**Files touched:** `src/compiler/lexer.h`, `src/compiler/emitter.h`,
`src/compiler/bb_graphics2d.h`, `examples/asteroids/asteroids.bb` (new)

### New example — Asteroids clone

Added `examples/asteroids/asteroids.bb`: a self-contained Asteroids clone
(~450 lines) that exercises the full 2D graphics and input stack. Pure vector
graphics, no external assets. Features: main menu, three-tier asteroid
splitting, bullet collision, level scaling, hi-score tracking, and a game-over
screen.

### Bug fix — Two-word `End X` syntax (`lexer.h`)

Blitz3D uses both single-word (`EndIf`, `EndFunction`) and two-word (`End If`,
`End Function`) forms interchangeably. The parser only recognised the
single-word forms, so any program using `End If`, `End Function`, `End Type`,
or `End Select` would fail to parse.

Fixed with a post-tokenisation merge pass at the end of `Lexer::tokenize()`:
adjacent tokens `END` + `IF/FUNCTION/TYPE/SELECT` on the same line are
collapsed into a single compound token (`ENDIF`, `ENDFUNCTION`, etc.) before
the parser runs. All 42 test cases still pass.

### Bug fix — Bitmap font characters mirrored (`bb_graphics2d.h`)

The built-in 8×8 bitmap font stores glyph data LSB-first (bit 0 = leftmost
pixel). The renderer was extracting bits MSB-first (`0x80u >> col`), producing
horizontally mirrored glyphs. Fixed by switching to `1u << col`.

### Bug fix — Infinite loop on parameter assignment (`emitter.h`)

Function parameters were not pre-registered in `declaredVars`. The first
assignment to a parameter inside the function body was therefore emitted as a
new local declaration (e.g., `float var_v = ...`) instead of a plain assignment
(`var_v = ...`). This shadowed the parameter, leaving the original untouched and
causing any loop that modified a parameter to spin forever.

Fixed by saving/restoring `declaredVars` around each function body, seeding it
from `globalVarNames` (so global assignments inside functions remain plain
assignments), then pre-registering each parameter name before processing the
body.

---

## v0.3.7 - "Audio Formats + Fullscreen Scaling" (2026-02-24)

**Files touched:** `src/compiler/bb_sound.h`, `src/compiler/bb_graphics2d.h`,
`src/thirdparty/dr_libs/dr_mp3.h` (new), `src/thirdparty/stb/stb_vorbis.c` (new)

`PlayMusic` and `LoadSound` previously only accepted WAV files via `SDL_LoadWAV`.
Attempting to load MP3 or OGG returned 0 silently (no error, no sound).

### MP3 support — dr_mp3

Integrated **dr_mp3** (David Reid, public domain, single-header) at
`src/thirdparty/dr_libs/dr_mp3.h`. New internal `bb_load_mp3_()` decodes
MP3 → float32 PCM via `drmp3_open_file_and_read_pcm_frames_f32`; constructs an
SDL3 audio stream with `SDL_AUDIO_F32LE`. SDL3 handles sample-rate conversion
transparently at bind time.

### OGG Vorbis support — stb_vorbis

Integrated **stb_vorbis** (Sean Barrett, public domain) at
`src/thirdparty/stb/stb_vorbis.c`. New internal `bb_load_ogg_()` decodes
OGG → int16 PCM via `stb_vorbis_decode_filename`; constructs an SDL3 audio
stream with `SDL_AUDIO_S16LE`.

**Macro collision fix:** `stb_vorbis.c` leaks single-character macros
`L`, `C`, `R` (channel routing flags) into the TU. These collide with the
variable `L` inside `stb_image.h` (JPEG marker parser). Fixed by
`#undef L`, `#undef C`, `#undef R` immediately after the stb_vorbis include.

### Extension dispatch in `bb_LoadSound`

`bb_LoadSound` now detects the file extension (case-insensitive) and routes:
- `.mp3` → `bb_load_mp3_()`
- `.ogg` → `bb_load_ogg_()`
- everything else → `SDL_LoadWAV` (WAV, AIFF)

`bb_PlayMusic` is unchanged — it delegates to `bb_LoadSound`, so WAV / MP3 / OGG
all work transparently.

### Bug fix — Fullscreen resolution ignored (`bb_graphics2d.h`)

`Graphics 640,480,0,1` was opening the window at desktop resolution and ignoring
the requested 640×480. Root cause: in SDL3, `SDL_WINDOW_FULLSCREEN` does not
change the display mode — it creates a fullscreen window at the current desktop
resolution by default. A previous attempt to fix this via
`SDL_SetWindowFullscreenMode` was reverted because modern GPUs rarely expose
640×480 as a native display mode, making the approach unreliable.

**Fix — renderer-side logical presentation (Godot approach):**

- For fullscreen (mode 1, 6): window is created at desktop resolution (`0,0`).
- After renderer creation, `SDL_SetRenderLogicalPresentation(renderer, w, h,
  SDL_LOGICAL_PRESENTATION_LETTERBOX)` is called. SDL3 then maps all drawing
  commands from the virtual `w×h` coordinate space onto the physical screen,
  scaling up and letterboxing if the aspect ratio differs (e.g. 4:3 game on a
  16:9 monitor gets black bars left and right).
- `GraphicsWidth()` / `GraphicsHeight()` still return the requested values —
  game code is unaffected.
- No display mode change, no flicker, no dependency on driver-supported modes.

---

## v0.3.6 - "Phase K Complete: 2D Graphics" (2026-02-24)

Phase K (2D Graphics, Milestones 41–46b) completed in full. 42 of 66 milestones done.
**Files primarily touched:** `src/compiler/bb_graphics2d.h`, `src/compiler/bb_image.h`,
`src/compiler/bb_sdl.h`, `src/compiler/emitter.h`, `src/compiler/blitzcc.cpp`,
`src/compiler/ast.h`, `src/compiler/parser.h`, `src/compiler/bb_runtime.h`

---

### Milestone 41: Line & Shape Primitives

- `bb_Line(x1,y1,x2,y2)` — `SDL_RenderLine`, coords cast to float
- `bb_Rect(x,y,w,h,solid=1)` — `SDL_RenderFillRect` / `SDL_RenderRect`
- `bb_Oval(x,y,w,h,solid=1)` — filled: scanline half-chord; outline: parametric
  loop, `steps = max(16, ⌈2π·max(rx,ry)⌉ + 4)`, closed `SDL_RenderLines`
- `bb_Poly(x0,y0,x1,y1,x2,y2)` — hardware triangle via `SDL_RenderGeometry`

**Bug fix — Type/function name collision** (`emitter.h`): adding `bb_Rect()` to the
runtime caused `Type Rect` → `struct bb_Rect` to be shadowed by the function.
Fix: `hintToType()` returns `"struct bb_TypeName *"` (elaborated type specifier);
`emitTypeDecl()` uses `spname = "struct " + sname + " *"` throughout. Zero-cost,
standard-conforming, permanently prevents `bb_`-namespace collisions.

---

### Milestone 42: Text & Console Output

- `bb_Write<T>` — no-newline output via `std::cout << val << std::flush`
- `bb_Locate(x,y)` — ANSI cursor (`\x1b[row;colH`); no-op when not a TTY
- `bb_Text(x,y,s,cx=0,cy=0)` — pixel-perfect 8×8 bitmap font renderer;
  `static constexpr uint8_t bb_font8x8_[128][8]` baked into the header (CP437)

---

### Milestone 43: Font System + SDL3_ttf

- `bb_Font_` struct (`height`, `width`, `valid`, `ttf`); slot 0 = built-in 8×8 default
- `LoadFont` / `SetFont` / `FreeFont` / `FontWidth` / `FontHeight` / `StringWidth` / `StringHeight`
- **SDL3_ttf 3.2.2** integrated: `build_windows.bat` downloads it automatically;
  `blitzcc.cpp` detects presence → `-DBB_HAS_SDL3_TTF -I<inc>` + links `libSDL3_ttf.dll.a`
- `bb_find_font_file_()` resolves system font names (`%WINDIR%\Fonts\*.ttf`)
- `bb_Text()` dispatches to TTF path (`TTF_RenderText_Blended` → texture → render → free)
  or falls back to built-in bitmap
- API note: `TTF_GetStringSize(TTF_Font*, ...)` used — not `TTF_GetTextSize(TTF_Text*, ...)`

---

### Milestone 44: Image Loading & Drawing

New file `src/compiler/bb_image.h`; embedded **stb_image** (public domain, `src/thirdparty/stb/`)

- `bb_LoadImage(file)` — stb_image → RGBA32 surface → SDL_Texture; headless-safe
- `bb_CreateImage(w,h)` — `SDL_TEXTUREACCESS_TARGET` texture
- `bb_FreeImage`, `bb_ImageWidth`, `bb_ImageHeight`
- `bb_DrawImage`, `bb_DrawImageRect`, `bb_DrawBlock`, `bb_DrawBlockRect` — `SDL_RenderTexture`
- `bb_image_quit_hook_` registered before `bb_sdl_quit_()` (teardown order)

---

### Milestone 45: Image Manipulation

- `HandleImage` / `MidHandle` / `AutoMidHandle` / `ImageXHandle` / `ImageYHandle`
- `ScaleImage` / `RotateImage` / `MaskImage`
- `TileImage` / `TileBlock` / `DrawImageEllipse`
- `SaveImage` (stb_image_write, PNG)
- `ImagesOverlap` / `ImageRectOverlap` / `ImagesColl` / `ImageXColl` / `ImageYColl`

---

### Milestone 46: Pixel Buffer Access

- `bb_ImageBuffer(img)` → handle (`img+2`); `BackBuffer()=1`, `FrontBuffer()=2`
- `LockBuffer` / `UnlockBuffer` — in-memory RGBA pixel arrays; flush to SDL texture on unlock
- `ReadPixel` / `WritePixel` — bounds-checked ARGB; `*Fast` variants unchecked
- `CopyPixel` / `CopyPixelFast` — cross-buffer
- `LoadBuffer` / `SaveBuffer` — stb_image load; stb_image_write PNG save (no renderer needed)
- `BufferWidth` / `BufferHeight`
- `bb_Rgb(r,g,b)` → `(r<<16)|(g<<8)|b` colour helper added to `bb_graphics2d.h`

**Bug fix** (`emitter.h`): `AND`/`OR` emitted as `&&`/`||` (logical, 0 or 1) instead of
`&`/`|` (bitwise). Fixed in `mapOp()`.

---

### Milestone 46b: Animated Images & Image API Completion

Complete rewrite of `bb_image.h` for multi-frame support:

- `bb_FrameData_` — per-frame: `tex`, `pixels`, `handle_x/y`, `scale_x/y`, `rotation`
- `bb_Image_` → `{width, height, valid, std::vector<bb_FrameData_> frames}`
- All M44/M45/M46 functions gain optional `frame%=0` parameter
- Buffer handle: `ImageBuffer(img,frame) = (img-1) + frame*65536 + 3`
  (for `frame=0` equals old `img+2` — fully backward compatible)
- `bb_LoadAnimImage(file,fw,fh,first,count)` — sprite strip slicer
- `bb_GrabImage(h,x,y,frame)` — screen → image frame capture
- `bb_CopyImage(h)` — deep copy of all frames
- `bb_FlipImage` / `bb_MirrorImage` — vertical / horizontal pixel flip
- `bb_ImagesCollide` / `bb_ImageRectCollide` — AABB (pixel-perfect stub)

---

### Bug Fix: Toolchain path resolution (`blitzcc.cpp`)

`resolvePath()` failed when `blitzcc.exe` was invoked from a subdirectory.
`argv[0]` now sets `g_exeDir_`; search order: CWD → exe-dir → **exe-dir/..** → `../CWD` → `$BLITZPATH`.

---

### Bug Fix: Implicit global variable declarations (`ast.h`, `parser.h`, `emitter.h`)

Bare `x = value` without `Local`/`Global` (standard Blitz3D) caused "undeclared variable"
in the generated C++. `AssignStmt` gains `typeHint`; emitter auto-declares on first use.
`collectGlobals()` now also populates `declaredVars` to block re-declaration in function bodies.

---

### Bug Fix: `Graphics` mode parameter (`bb_graphics2d.h`)

Mode `2` (windowed) was incorrectly opening a fullscreen-desktop window.
Correct: 0/2/3 = windowed, 1/6 = fullscreen (6 also enables vsync).

---

### Bug Fix: `Flip(vblank)` — vsync ignored (`bb_graphics2d.h`)

`vblank` parameter was discarded. Now calls `SDL_SetRenderVSync(renderer, 1/0)` on change
(tracked in `bb_vsync_mode_`). `Flip` / `Flip 1` = vsync on; `Flip 0` = vsync off.

---

### Bug Fix: `WaitTimer` precision on Windows (`bb_runtime.h`, `blitzcc.cpp`)

Windows default timer resolution ~15.6 ms caused ±8 ms jitter at 60 Hz.
`bbInit()` calls `timeBeginPeriod(1)` (1 ms resolution); `bbEnd()` restores it.
Link command adds `-lwinmm`.

---

- **Tests:** 42 PASS, 0 FAIL ✓

---

## v0.2.7 - "Color & Pixel Primitives" (2026-02-23)

### Milestone 40: Color & Pixel Primitives

- **`bb_graphics2d.h` extended** — colour/pixel API appended after the M39 buffer/flip section

- **Draw colour state** — `bb_draw_r_/g_/b_` (`Uint8`, all 255 = white, matching Blitz3D's startup default); separate from the clear colour (`bb_cls_r_/g_/b_`, declared in M39)

- **`bb_Color(r, g, b)`** — stores clamped (0–255) values into `bb_draw_r_/g_/b_`; does not repaint anything already on screen

- **`bb_ClsColor(r, g, b)`** — updates `bb_cls_r_/g_/b_`; takes effect on the next `Cls()` call

- **`bb_ColorRed/Green/Blue()`** — return the three draw-colour bytes as `int`; pure state reads with no SDL side-effect

- **`bb_GetColor(x, y)`**:
  - Headless guard: returns 0 when `bb_renderer_` is null
  - `SDL_RenderReadPixels(renderer, &rect{x,y,1,1})` → `SDL_Surface*`
  - `SDL_ReadSurfacePixel(surf, 0, 0, &r, &g, &b, &a)` extracts RGBA bytes
  - `SDL_DestroySurface(surf)` frees the temporary surface
  - Overwrites `bb_draw_r_/g_/b_` so `ColorRed/Green/Blue()` immediately reflect the sampled pixel

- **`bb_Plot(x, y)`**:
  - Headless guard: no-op when `bb_renderer_` is null
  - `SDL_SetRenderDrawColor(renderer, bb_draw_r_, bb_draw_g_, bb_draw_b_, 255)`
  - `SDL_RenderPoint(renderer, (float)x, (float)y)`

- **`blitzcc.cpp`**: 7 new entries in `kCommands[]` (Color, ClsColor, ColorRed, ColorGreen, ColorBlue, GetColor, Plot); version bumped to v0.2.7

### Design Notes
`bb_draw_r_/g_/b_` defaults to white (255,255,255) to match Blitz3D behaviour where new programs draw white on a black background until `Color` is called. Separating draw colour from clear colour (introduced in M39) means `ClsColor` and `Color` can be set independently without interfering with each other. `GetColor` rewrites the draw-colour state rather than returning a packed integer — this matches Blitz3D's API where the caller reads back components via `ColorRed/Green/Blue`. `SDL_ReadSurfacePixel` is an SDL3-only function; no SDL2 equivalent. Clamping in `bb_Color` / `bb_ClsColor` mirrors Blitz3D: out-of-range values are silently clamped rather than raising an error.

### Verification (`tests/test_m40_color.bb`)
- `Color 200,100,50` → `ColorRed=200`, `ColorGreen=100`, `ColorBlue=50` ✓
- `ClsColor 10,20,30 : Cls` → `ClsColor OK`, `Cls OK` ✓ (no-op headless)
- `Color 300,-5,128` → clamped to `255/0/128` ✓
- `Plot 100,100` → `Plot OK` ✓ (no-op headless)
- `GetColor 100,100` → `GetColor OK` ✓ (no-op headless; draw colour unchanged)
- `DONE` ✓

---

## v0.2.6 - "Buffer & Flip" (2026-02-23)

### Milestone 39: Buffer & Flip

- **`bb_graphics2d.h` extended** — buffer/flip API added after the M38 graphics init section; `bb_runtime.h` already includes this file

- **Buffer handle constants**:
  - `BB_BACK_BUFFER_H = 1`, `BB_FRONT_BUFFER_H = 2` — integer tokens that match Blitz3D's `BackBuffer()` / `FrontBuffer()` return values
  - `bb_active_buffer_` — tracks the currently set buffer (default: back)

- **Clear color state** — `bb_cls_r_/g_/b_` (`Uint8`, all 0 = black) reserved for `ClsColor` (M40); declared here so M39's `Cls` uses them immediately

- **`bb_BackBuffer()`** → returns `BB_BACK_BUFFER_H` (1)

- **`bb_FrontBuffer()`** → returns `BB_FRONT_BUFFER_H` (2)

- **`bb_SetBuffer(buf)`** → stores in `bb_active_buffer_`; no renderer action needed (SDL3 always renders to the back buffer internally)

- **`bb_Cls()`**:
  - No-op when `bb_renderer_` is null (headless safe)
  - `SDL_SetRenderDrawColor(renderer, bb_cls_r_, bb_cls_g_, bb_cls_b_, 255)` then `SDL_RenderClear(renderer)`
  - Color sourced from `bb_cls_r_/g_/b_` so `ClsColor` (M40) takes effect immediately

- **`bb_Flip(vblank=1)`**:
  - No-op when `bb_renderer_` is null
  - `SDL_RenderPresent(renderer)` — swaps back/front buffers
  - Calls `bb_PollEvents()` to drain SDL events after every present; prevents window-not-responding freeze in game loops that only call `Flip`, not an explicit event pump
  - `vblank` accepted for API parity, not acted upon (SDL3 vsync is set at renderer creation)

- **`bb_CopyRect(sx, sy, sw, sh, dx, dy, srcbuf, dstbuf)`** — silent stub; compile-safe; full implementation deferred to M46 (render-to-texture)

- **`blitzcc.cpp`**: 6 new entries in `kCommands[]` (BackBuffer, FrontBuffer, SetBuffer, Cls, Flip, CopyRect); version bumped to v0.2.6

### Design Notes
SDL3's renderer always draws to an internal back buffer; `SDL_RenderPresent` is the flip. Blitz3D's `SetBuffer BackBuffer()` / `SetBuffer FrontBuffer()` are therefore bookkeeping only — the active buffer token is stored for programs that query it but has no effect on where SDL renders. `Flip` pumping `bb_PollEvents()` is essential: Blitz3D programs typically have `Flip` at the end of their game loop and nothing else that would drain the OS event queue; without the pump the window would become unresponsive within seconds.

### Verification (`tests/test_m39_buffer.bb`)
- `BackBuffer()` → `1` ✓
- `FrontBuffer()` → `2` ✓
- `SetBuffer BackBuffer()` → `SetBuffer OK` ✓
- `Cls` → `Cls OK` ✓ (no-op headless)
- `Flip` → `Flip OK` ✓ (no-op headless)
- `Flip 0` → `Flip 0 OK` ✓
- `CopyRect 0,0,100,100,200,200` → `CopyRect OK` ✓
- `DONE` ✓

---

## v0.2.5 - "Graphics Mode Init" (2026-02-23)

### Milestone 38: Graphics Mode Init

- **`bb_graphics2d.h` (new file)** — 2D graphics foundation; `bb_runtime.h` includes it after `bb_sound3d.h`

- **Global display state** — four inline ints `bb_gfx_width_`, `bb_gfx_height_`, `bb_gfx_depth_`, `bb_gfx_rate_` store the active display parameters; set unconditionally by `bb_Graphics()` so query functions work even on headless machines

- **`bb_Graphics(w, h, depth, mode)`**:
  - Calls `bb_sdl_ensure_()` then tears down any existing window/renderer before creating new ones
  - mode 0: windowed (`SDL_WindowFlags = 0`)
  - mode 1 / 6: fullscreen (`SDL_WINDOW_FULLSCREEN`); mode 6 also enables vsync via `SDL_SetRenderVSync`
  - mode 2: fullscreen-desktop — queries primary display via `SDL_GetCurrentDisplayMode` and overrides `w/h` with desktop size
  - `SDL_CreateWindow(title, w, h, flags)` — title sourced from `bb_app_title_` (set by `AppTitle`), defaults to `"BlitzNext"`
  - `SDL_CreateRenderer(window, nullptr)` — uses SDL3's default hardware renderer
  - Refresh rate queried post-creation via `SDL_GetDisplayForWindow` + `SDL_GetCurrentDisplayMode`; stored in `bb_gfx_rate_`
  - Silent on headless machines (SDL init failure or display unavailable)

- **`bb_EndGraphics()`** — destroys renderer then window; zeros all `bb_gfx_*` state; idempotent

- **Query functions** — `bb_GraphicsWidth/Height/Depth/Rate()` return the stored `bb_gfx_*` values

- **Memory stubs** — `bb_TotalVidMem()` / `bb_AvailVidMem()` return `512 * 1024 * 1024` (512 MB); SDL3 exposes no VRAM query API; value is large enough that BB programs checking available VRAM before loading assets always proceed

- **`bb_GraphicsMode(w, h, depth, rate)`** — re-enters Graphics at a different resolution; `rate` accepted for API parity, not used; delegates to `bb_Graphics(w, h, depth, 0)`

- **`blitzcc.cpp`**: 9 new entries in `kCommands[]` (Graphics, EndGraphics, GraphicsWidth, GraphicsHeight, GraphicsDepth, GraphicsRate, TotalVidMem, AvailVidMem, GraphicsMode); version bumped to v0.2.5

### Design Notes
`bb_Graphics()` stores requested dimensions before calling `bb_sdl_ensure_()`, so `GraphicsWidth()`/`GraphicsHeight()` always return sane values even when SDL fails to open a window (headless CI, no display). The `AppTitle` integration means calling `AppTitle "Game"` before `Graphics 800,600` correctly sets the window title. SDL3 `SDL_CreateRenderer` selects the best available hardware backend automatically; no flags are needed.

### Verification (`tests/test_m38_graphics.bb`)
- `GraphicsWidth()` → `800` ✓
- `GraphicsHeight()` → `600` ✓
- `GraphicsDepth()` → `32` ✓
- `GraphicsRate OK` ✓ (non-zero on a real display)
- `TotalVidMem OK` ✓ (512 MB stub)
- `AvailVidMem OK` ✓
- `GraphicsMode 640,480,32,0` → no crash ✓
- `EndGraphics OK` ✓
- `DONE` ✓

### Post-M38 Bug Fixes (v0.2.5 patch)

**Bug 1 — `AppTitle` does not update a live window title** *(`bb_system.h`, `bb_graphics2d.h`)*
`bb_AppTitle()` only stored the string; calling it after `Graphics()` left the OS window title unchanged.
Fix: added `bb_title_update_hook_` callback (same pattern as `bb_audio_update_hook_`). `bb_graphics2d.h` registers `bb_update_window_title_()` which calls `SDL_SetWindowTitle(bb_window_, title)` whenever a window is open.

**Bug 2 — `WaitKey` ignores the window when launched without a console** *(`bb_sdl.h`)*
When a Blitz3D program is launched by double-click or from an IDE (no console, but window open), `bb_stdin_is_console_()` returned false and `WaitKey` returned immediately — the window closed before the user could see it.
Fix: `bb_WaitKey()` now checks `bb_window_ != nullptr` first. When a window is open it always blocks on `SDL_WaitEvent`, regardless of stdin state.

**Bug 3 — `bb_stdin_is_console_()` treats Windows NUL as a real console** *(`bb_sdl.h`)*
`GetFileType()` returns `FILE_TYPE_CHAR` for both real consoles and the Windows `NUL` device (`/dev/null`). This caused test programs with `WaitKey` (e.g. `test_fixes.bb`) to block forever when run as `test.exe < /dev/null`.
Fix: replaced `GetFileType` with `GetConsoleMode` — it succeeds only for genuine interactive console handles and fails for `NUL`, pipes, and file redirections.

---

## v0.2.4 - "3D Sound" (2026-02-23)

### Milestone 37: 3D / Positional Sound

- **`bb_sound3d.h` (new file)** — 3D audio state layer on top of `bb_sound.h`; `bb_runtime.h` includes it after `bb_sound.h`

- **`bb_Load3DSound(file)`** — thin wrapper over `bb_LoadSound`; the returned handle works with `PlaySound`/`LoopSound` and is then positioned via `Channel3DPosition`. No separate sound bank needed.

- **Per-sound falloff: `bb_SoundRange(snd, inner, outer)`** — stores the full-volume inner radius and silence outer radius in `bb_snd3d_inner_[snd]` / `bb_snd3d_outer_[snd]`. Not applied at runtime yet; reserved for a future distance-attenuation pass.

- **Per-channel 3D state**:
  - `bb_Channel3DPosition(ch, x, y, z)` — stores world-space position in `bb_snd_chan3d_[ch]`
  - `bb_Channel3DVelocity(ch, vx, vy, vz)` — stores Doppler velocity (stub; stored only)
  - `bb_Chan3D_` struct added: `{x, y, z, vx, vy, vz}` with float defaults

- **Listener state** (global; one listener per program):
  - `bb_ListenerPosition(x, y, z)` → `bb_snd3d_lx_/y_/z_`
  - `bb_ListenerOrientation(fx, fy, fz, ux, uy, uz)` → forward + up vectors; default `-Z` forward, `+Y` up
  - `bb_ListenerVelocity(vx, vy, vz)` → Doppler velocity (stored)

- **`bb_WaitSound(ch)`** — blocks until `bb_ChannelPlaying(ch)` returns 0; polls every 10 ms via `SDL_Delay` + `bb_snd_update_()` so one-shot cleanup runs correctly. `ch=0` (or finished channel) returns immediately.

- **C++ aliases for digit-prefixed BB commands**:
  - `bb_3DSoundVolume(snd, vol)` → `bb_SoundVolume`
  - `bb_3DSoundPan(snd, pan)` → `bb_SoundPan`
  - `bb_3DChannelVolume(ch, vol)` → `bb_ChannelVolume`
  - `bb_3DChannelPan(ch, pan)` → `bb_ChannelPan`
  - BB-level exposure of `3D`-prefixed names requires a lexer extension (identifiers cannot start with a digit); deferred.

- **`blitzcc.cpp`**: 8 new entries in `kCommands[]` (Load3DSound, SoundRange, Channel3DPosition, Channel3DVelocity, ListenerPosition, ListenerOrientation, ListenerVelocity, WaitSound); version bumped to v0.2.4

### Design Notes
SDL3 has no native positional audio API. All 3D state (position, velocity, orientation) is stored in inline globals/arrays for forward compatibility. A future milestone can compute distance-based gain and stereo pan from listener↔source geometry and write the results via `SDL_SetAudioStreamGain` and a manual stereo-pan mixing pass. `WaitSound` deliberately uses `bb_snd_update_()` rather than sleeping blindly so that one-shot channels are cleaned up on schedule and looping channels continue to refill.

### Verification (`tests/test_m37_sound3d.bb`)
- `Load3DSound("boom.wav")` → `0` ✓ (headless / no file)
- `SoundRange 0, 1.0, 10.0` → safe no-op (snd=0 guard) ✓
- `WaitSound 0` → immediate return ✓
- `Channel3DPosition 0, …` / `Channel3DVelocity 0, …` → no crash ✓
- `ListenerPosition` / `ListenerOrientation` / `ListenerVelocity` → no crash ✓
- `DONE` ✓

---

## v0.2.3 - "Music & CD" (2026-02-23)

### Milestone 36: Music & CD

- **`bb_sound.h` extended**:
  - `bb_snd_music_snd_` / `bb_snd_music_ch_` — global slots tracking the active music track
  - `bb_PlayMusic(file)` — stops current music, loads WAV via `bb_LoadSound` + `bb_LoopSound`; returns channel handle; OGG/MP3 fail gracefully (SDL3 only handles WAV natively)
  - `bb_StopMusic()` — `bb_StopChannel(music_ch)` + `bb_FreeSound(music_snd)`; resets both globals to 0
  - `bb_MusicPlaying()` — delegates to `bb_ChannelPlaying(music_ch)`; auto-clears globals when channel finishes
  - `bb_PlayCDTrack(track)` — stub; logs `[runtime] PlayCDTrack: CD audio is not supported` to stderr; returns immediately
- **`blitzcc.cpp`**: 4 music entries added to `kCommands[]`; version bumped to v0.2.3
- **Test:** `tests/test_m36_music.bb` — headless: `PlayMusic` → 0 (no file/device), `MusicPlaying` → 0, `StopMusic` → no-op, `PlayCDTrack 1` → warning on stderr

---

## v0.2.2 - "Channel Control" (2026-02-23)

### Milestone 35: Channel Control

- **`bb_sound.h` extended**:
  - `bb_Sound_` gains `vol=1.0f`, `pan=0.0f`, `pitch=0.0f` — defaults propagated to new channels in `bb_play_sound_()`
  - `bb_Channel_` gains `paused=false`, `gain=1.0f`, `pan=0.0f` — tracks runtime per-channel state
  - `bb_play_sound_()` now calls `SDL_SetAudioStreamGain(s, vol)` and `SDL_SetAudioStreamFrequencyRatio(s, pitch/orig_freq)` when sound has non-default values; stores `gain`/`pan` in the new channel slot
  - `bb_snd_update_()` skips channels with `paused=true` (no refill, no cleanup while paused)

- **`bb_PauseChannel(ch)`** — `SDL_UnbindAudioStream`; `paused = true`; data remains in stream buffer, no data is consumed while unbound
- **`bb_ResumeChannel(ch)`** — `SDL_BindAudioStream(bb_snd_dev_, stream)`; `paused = false`; continues from buffer position (no seek needed)
- **`bb_ChannelPlaying(ch)`** — returns `stream != nullptr`; returns 1 for paused channels (they're still alive), 0 when slot is empty (finished one-shot or manually stopped)
- **`bb_ChannelVolume(ch, vol)`** — stores in `gain`; calls `SDL_SetAudioStreamGain` (skipped when paused, gain restored on resume isn't automatic — caller must re-set if needed)
- **`bb_ChannelPan(ch, pan)`** — stored in `pan` field; SDL3 has no per-stream stereo pan natively; full implementation deferred to audio-processing milestone
- **`bb_ChannelPitch(ch, hz)`** — `SDL_SetAudioStreamFrequencyRatio(stream, hz / src_spec.freq)`; resampling handled by SDL3 internally
- **`bb_SoundVolume/Pan/Pitch(snd, …)`** — set defaults on `bb_Sound_`; applied when `bb_PlaySound`/`bb_LoopSound` creates a new channel
- **`blitzcc.cpp`**: 9 channel-control entries added to `kCommands[]`

### Design Notes
Pause/resume via unbind/rebind is cleaner than gain=0 (gain=0 still lets SDL consume data, causing a paused one-shot to drain silently). Unbound streams preserve their queued data in the stream's internal buffer — rebinding resumes from the exact position. `ChannelVolume` during pause stores the gain but does not call `SDL_SetAudioStreamGain` (there's no stream bound to update); the gain is reapplied by the caller via `ChannelVolume` after `ResumeChannel` if needed.

### Verification (`tests/test_m35_channel.bb`)
- All 9 API functions: OK ✓
- `ChannelPlaying(0)` → `0` ✓
- `DONE` ✓

---

## v0.2.1 - "Sound Loading & Playback" (2026-02-23)

### Milestone 34: Sound Loading & Playback

- **`bb_sound.h` (new file)** — complete SDL3 audio system:
  - `bb_snd_dev_` (`SDL_AudioDeviceID`) + `bb_snd_spec_` — lazy audio device; opened with `SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr)`; device format retrieved via `SDL_GetAudioDeviceFormat`
  - `bb_snd_ensure_()` — calls `bb_sdl_ensure_()` then `SDL_InitSubSystem(SDL_INIT_AUDIO)` and opens the device; separate from graphics init so text-mode + graphics-only programs pay zero audio overhead
  - `bb_snd_quit_()` — unbinds+destroys all streams, SDL_frees all sound buffers, closes device, calls `SDL_QuitSubSystem(SDL_INIT_AUDIO)`
  - `bb_Sound_{data, len, spec}` — 64-slot bank; slots 1–63 used; data is SDL-allocated PCM
  - `bb_Channel_{stream, snd_id, looping}` — 32-slot bank; slots 1–31 used
  - `bb_LoadSound(file)` — `SDL_LoadWAV` → fills next free `bb_snd_sounds_` slot; returns 1-based handle
  - `bb_FreeSound(handle)` — stops dependent channels; `SDL_free`; zeros slot
  - `bb_play_sound_(snd, loop)` — internal: reuses finished one-shot slots; `SDL_CreateAudioStream(src_spec, device_spec)` + `SDL_BindAudioStream` + `SDL_PutAudioStreamData`; flushes if one-shot
  - `bb_PlaySound(snd)` → `bb_play_sound_(snd, false)` (flush = one-shot)
  - `bb_LoopSound(snd)` → `bb_play_sound_(snd, true)` (no flush = kept alive by refill)
  - `bb_StopChannel(ch)` — `SDL_UnbindAudioStream` + `SDL_DestroyAudioStream`; zeros slot
  - `bb_snd_update_()` — looping: refills when `SDL_GetAudioStreamQueued < sound_len`; one-shot: auto-destroys when queued == 0
  - `bb_snd_hook_reg_` — `inline const bool` that registers `bb_snd_update_` as `bb_audio_update_hook_` at startup (before main)

- **`bb_sdl.h` extended**:
  - `bb_audio_update_hook_` — `inline void (*)() = nullptr`; called by `bb_PollEvents()` after every SDL event drain; zero cost when audio not in use

- **`bb_runtime.h` updated**:
  - `#include "bb_sound.h"` added after bb_input.h
  - `bbEnd()` now calls `bb_snd_quit_()` before `bb_sdl_quit_()` to ensure audio teardown in correct order

- **`blitzcc.cpp`**: 5 sound entries added to `kCommands[]` (LoadSound, FreeSound, PlaySound, LoopSound, StopChannel)

### Design Notes
Audio uses SDL3 audio streams — one `SDL_AudioStream` per active channel, all bound to the same `SDL_AudioDeviceID`. SDL3 mixes bound streams automatically. Each stream is created with the sound's native format as source and the device's format as destination; SDL3 handles sample-rate conversion and format conversion transparently. Looping relies on periodic refill via `bb_PollEvents()` hook rather than a dedicated audio thread, which keeps the implementation simple and single-threaded.

### Verification (`tests/test_m34_sound.bb`)
- `LoadSound("beep.wav")` → `0` ✓ (no audio device in headless mode)
- `PlaySound(0)` → `0` ✓ (null handle → no-op)
- `StopChannel(0)` → no crash ✓
- `FreeSound(0)` → no crash ✓
- `DONE` ✓

---

## v0.2.0 - "Joystick Input" (2026-02-23)

### Milestone 33: Joystick Input

- **`bb_sdl.h` extended**:
  - `BB_JOY_MAX_PORTS=4`, `BB_JOY_MAX_BUTTONS=32`, `BB_JOY_BTN_QUEUE_CAP=8`
  - `bb_JoyPort_` struct — `handle`, `id`, `is_gamepad`; axes `x/y/z/u/v` (float -1..1); `hat` (int 0–8); `btn_down[32]`, `btn_hit[32]` (bool arrays); `btn_queue[8]` FIFO
  - `bb_joy_[4]` inline array of ports
  - `bb_joy_find_port_(SDL_JoystickID)` — O(4) scan returning port index or -1
  - `bb_sdl_hat_to_blitz_(Uint8)` — converts SDL bitmask hat → Blitz3D direction (switch on 9 values)
  - `bb_sdl_ensure_()` now inits `SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD` in addition to VIDEO/EVENTS
  - `bb_sdl_quit_()` closes all open joystick handles before SDL_Quit
  - `bb_sdl_process_event_()` extended: JOYSTICK_ADDED → open + fill port; JOYSTICK_REMOVED → close + zero port; AXIS_MOTION → normalize Sint16 to -1..1, map axis 0-4; HAT_MOTION → convert hat 0 only; BUTTON_DOWN → set held/hit, enqueue 1-based number; BUTTON_UP → clear held

- **`bb_input.h` extended** — joystick API:
  - `bb_JoyType(port)` → 0/1/2; pumps events then checks handle
  - `bb_JoyX/Y/Z/U/V(port)` → float; pumps + reads cached axis
  - `bb_JoyHat(port)` → int 0–8; pumps + reads cached hat
  - `bb_JoyDown(port, btn)` — 1-based; pumps, checks `btn_down[btn-1]`
  - `bb_JoyHit(port, btn)` — edge-triggered; reads+clears `btn_hit[btn-1]`
  - `bb_WaitJoy(port)` — ensures SDL, pumps, blocks via `SDL_WaitEvent` + `bb_sdl_process_event_()` until button FIFO non-empty; returns 1-based button
  - `bb_GetJoy(port)` — alias for `bb_WaitJoy`
  - `bb_FlushJoy(port)` — zeros held/hit arrays and FIFO; axis/hat preserved

- **`blitzcc.cpp`**: 12 joystick entries added to `kCommands[]`

### Design Note
All axis/hat/button state is event-driven (same pattern as keyboard/mouse). Headless programs (SDL not initialized) return 0/0.0f from all joystick queries with zero overhead. Axis normalization divides by 32767.0f and clamps to -1.0 (handles Sint16 minimum of -32768). Only hat 0 is tracked per port. WaitJoy correctly handles port-scoped queues separate from each other.

### Verification (`tests/test_m33_joystick.bb`)
- `JoyType(0)` → `0` ✓ (headless, no device)
- `JoyX(0)` → `0` ✓
- `JoyY(0)` → `0` ✓
- `JoyHat(0)` → `0` ✓
- `JoyDown(0,1)` → `0` ✓
- `JoyHit(0,1)` → `0` ✓
- `FlushJoy 0` → no crash ✓
- `DONE` ✓

---

## v0.1.9 - "Mouse Input" (2026-02-23)

### Milestone 32: Mouse Input

- **`bb_sdl.h` extended**:
  - `bb_mouse_x_`, `bb_mouse_y_` — cursor position (float; updated by MOUSE_MOTION)
  - `bb_mouse_z_` — scroll-wheel accumulator (positive = up; flipped direction normalised)
  - `bb_mouse_xrel_`, `bb_mouse_yrel_`, `bb_mouse_zrel_` — delta accumulators; reset on each speed read
  - `bb_mouse_down_[4]`, `bb_mouse_hit_[4]` — held/edge-triggered state (indices 1=left,2=right,3=middle)
  - `BB_MOUSE_QUEUE_CAP = 16`; `bb_mouse_queue_buf_[16]` FIFO of button numbers; `bb_mouse_queue_head_/tail_`
  - `bb_sdl_btn_to_blitz_(Uint8)` — maps SDL LEFT/RIGHT/MIDDLE → 1/2/3
  - `bb_sdl_process_event_()` extended: MOUSE_MOTION updates pos + xrel/yrel; BUTTON_DOWN sets hit/down + enqueues; BUTTON_UP clears down; MOUSE_WHEEL accumulates z/zrel

- **`bb_input.h` extended** — mouse API:
  - `bb_MouseX()`, `bb_MouseY()` — pump events, return `(int)bb_mouse_x_/y_`
  - `bb_MouseZ()` — pump events, return `(int)bb_mouse_z_`
  - `bb_MouseXSpeed()`, `bb_MouseYSpeed()`, `bb_MouseZSpeed()` — return int delta, reset accumulator to 0
  - `bb_MouseDown(btn)` — pump + read held flag; bounds-checked (1–3)
  - `bb_MouseHit(btn)` — pump + read+clear edge-triggered hit flag
  - `bb_WaitMouse()` — ensures SDL, pumps, blocks via `SDL_WaitEvent` + `bb_sdl_process_event_()` until queue non-empty; dequeues and returns button number
  - `bb_GetMouse()` — alias for `bb_WaitMouse()`
  - `bb_FlushMouse()` — zeros held/hit arrays, speed accumulators, queue head/tail; drains `SDL_EVENT_MOUSE_MOTION..MOUSE_WHEEL` via `SDL_FlushEvents`
  - `bb_MoveMouse(x,y)` — ensures SDL; `SDL_WarpMouseInWindow` when window exists, else `SDL_WarpMouseGlobal`

### Design Notes
All mouse functions pump SDL events before sampling (same pattern as keyboard), so game loops that poll `MouseDown` work without an explicit `PollEvents` call. In headless mode (`bb_sdl_initialized_ = false`) all positional and button queries return 0 with zero overhead. `bb_mouse_xrel_/yrel_/zrel_` accumulate between reads, so `MouseXSpeed()` correctly sums multiple motion events that fire within one game frame.

### Verification (`tests/test_m32_mouse.bb`)
- `MouseX()` → `0` ✓ (headless)
- `MouseY()` → `0` ✓
- `MouseZ()` → `0` ✓
- `MouseDown(1)` → `0` ✓
- `MouseHit(1)` → `0` ✓
- `FlushMouse` → no crash ✓
- `DONE` ✓

---

## v0.1.8 - "Keyboard Input" (2026-02-23)

### Milestone 31: Keyboard Input

- **`bb_input.h` (new file)** — keyboard API; `bb_runtime.h` includes it after `bb_sdl.h`
- **`bb_blitz_to_sdl_[256]`** — inline `std::array`; maps Blitz3D DIK code (1–255) → `SDL_Scancode`; covers full US QWERTY layout, F1–F12, numpad, cursor cluster, Home/End/PgUp/PgDn/Ins/Del, Win/App keys; built at startup with a lambda initializer
- **`bb_sdl_to_blitz_[512]`** — reverse map; inverted from `bb_blitz_to_sdl_` at startup; used by `bb_GetKey()` to convert queued scancodes to Blitz3D codes
- **`bb_KeyDown(code)`** — pumps SDL events (if `bb_sdl_initialized_`), then checks `bb_sdl_key_down_[sdl_sc]`; returns 0 safely in headless mode
- **`bb_KeyHit(code)`** — edge-triggered; reads `bb_sdl_key_hit_raw_[sdl_sc]` and clears it; pumps events first
- **`bb_GetKey()`** — ensures SDL, pumps, then blocks via `SDL_WaitEvent` + `bb_sdl_process_event_()` until a key is queued; dequeues and translates to Blitz3D code; returns 0 if SDL unavailable
- **`bb_FlushKeys()`** — zeros `bb_sdl_key_down_` and `bb_sdl_key_hit_raw_` arrays, resets queue head/tail, calls `SDL_FlushEvents(KEY_DOWN, KEY_UP)` if SDL is running
- **`bb_sdl.h` extended**:
  - `BB_KEY_QUEUE_CAP = 64` constant
  - `bb_sdl_key_down_[512]`, `bb_sdl_key_hit_raw_[512]` inline bool arrays
  - `bb_key_queue_buf_[64]` circular FIFO of `SDL_Scancode`; `bb_key_queue_head_/tail_`
  - `bb_sdl_process_event_(const SDL_Event&)` — handles QUIT, KEY_DOWN (edge + queue), KEY_UP; called by `bb_PollEvents()` and `bb_WaitKey()`

### Design Note
`bb_KeyDown` / `bb_KeyHit` call `bb_PollEvents()` automatically when SDL is initialized, so game loops like `While Not KeyHit(1) : ... : Wend` work correctly without an explicit event-pump call. In headless mode (SDL not initialized), both functions return 0 immediately — zero overhead, no SDL initialization side-effect.

### Verification (`tests/test_m31_keyboard.bb`)
- `KeyDown(1)` → `0` ✓ (headless, no key pressed)
- `KeyHit(1)` → `0` ✓
- `FlushKeys` → no crash ✓
- `DONE` ✓
- M29 (PeekByte/PokeInt) and M16 (Delete) regression tests recompiled and pass ✓

---

## v0.1.7 - "SDL3 Infrastructure" (2026-02-23)

### Milestone 30: SDL3 Init + Headless Event Loop

- **`bb_sdl.h` (new file)** — standalone SDL3 header; `bb_runtime.h` includes it
- **`bb_sdl_ensure_()`** — lazy SDL3 initializer (SDL_Init VIDEO+EVENTS); called on first use; guards against double-init
- **`bb_window_` / `bb_renderer_`** — inline globals, both `nullptr` until a graphics milestone creates them
- **`bb_PollEvents()`** — drains SDL event queue; handles `SDL_EVENT_QUIT` by calling `bb_sdl_quit_()` + `std::exit(0)`; placeholder comment for M31 key/mouse state
- **`bb_WaitKey()`** — redesigned for SDL:
  - Non-interactive stdin (piped / test runner) → returns immediately (no SDL, no block)
  - Interactive: calls `bb_sdl_ensure_()`, then `SDL_WaitEvent()` until `KEY_DOWN` or `QUIT`
  - SDL init failure fallback: `std::cin.get()`
- **`bb_sdl_quit_()`** — safely tears down renderer → window → SDL in order; idempotent
- **`bbEnd()` updated** — calls `bb_sdl_quit_()` before exit
- **`bbInit()` updated** — stores `argc/argv`; SDL deferred to first use (text-mode programs pay zero SDL overhead)
- **Compile step** — `SDL3` import lib (`libSDL3.dll.a`) linked automatically when present; `SDL3.dll` copied next to output executable
- **`bb_stdin_is_console_()`** — Windows: `GetFileType(GetStdHandle(STD_INPUT_HANDLE)) == FILE_TYPE_CHAR`; POSIX: `isatty(fileno(stdin))`

### Design Note
SDL3 is initialised **lazily** — text-mode programs (Print, File I/O, Data, etc.) never touch SDL at all. The first call to `bb_WaitKey()`, `bb_PollEvents()`, or a future graphics function triggers `bb_sdl_ensure_()`. This keeps non-graphical programs fast and dependency-free at runtime (no SDL3.dll needed unless the program actually uses it).

### Verification (`tests/test_m30_sdl.bb`)
- `Print "SDL OK"` → `SDL OK` ✓ (headless path: SDL never initialized, zero overhead)
- Binary links SDL3 import library and copies `SDL3.dll` next to exe ✓
- `bbEnd()` / `bb_sdl_quit_()` — no crash on clean exit ✓
- All prior tests (M29 and earlier) still pass ✓

---

## v0.1.6 - "Delete Fix" (2026-02-22)

### Bug Fix: Delete First / Delete Last No Longer Leaks or Corrupts the List

**Root cause**: `visit(DeleteStmt)` only handled the `VarExpr` case via a direct
`varObjectTypes` lookup. Any other expression (`FirstExpr`, `LastExpr`,
`BeforeExpr`, `AfterExpr`) hit the fallback that set the pointer to `nullptr`
but never called `bb_TypeName_Delete()` → memory leak + object remained in the
intrusive linked list (list corruption).

**Fix — `emitter.h`**:
1. `getExprTypeName()` extended with `BeforeExpr`/`AfterExpr` arms (recurse into
   `be->object` / `ae->object` to inherit the object type).
2. `visit(DeleteStmt)` rewritten to call `getExprTypeName()` — type now resolved
   for `VarExpr`, `FirstExpr`, `LastExpr`, `BeforeExpr`, `AfterExpr`.
3. `bb_TypeName_Delete(expr)` emitted directly; pointer passed as-is.
4. Local variable nulled only when object is a `VarExpr`.

**Generated C++ before / after** (`Delete First Node`):
```cpp
// before — fallback, broken:
__bb_Node_head__ = nullptr; // Delete (type unknown)

// after — correct:
bb_Node_Delete(__bb_Node_head__);
```

### Verification (`tests/test_delete.bb`)
- 4 nodes created; count = 4 ✓
- `Delete First Node` → count = 3; new first val = 20 ✓
- `Delete Last Node`  → count = 2; new last val  = 30 ✓
- `Delete b` (VarExpr) → count = 1; only remaining = 30 ✓
- Deletion-safe `For Each` + `Delete n` inside loop → count = 0 ✓

---

## v0.1.5 - "Global Scope Fix" (2026-02-22)

### Bug Fix: Global Variables Now Visible Inside Functions

**Root cause**: `visit(VarDecl)` previously emitted all variable declarations
(Local *and* Global) inside `main()`. User functions are emitted before `main()`,
so any variable declared Global was invisible to them → C++ compile error.

**Fix — two-pass approach in `emitter.h`**:
1. New `collectGlobals()` pass scans the full AST (including function bodies,
   since Blitz3D allows `Global` anywhere) for `VarDecl` nodes with
   `scope == GLOBAL` and emits them at C++ file scope, after type struct
   definitions and before user functions.
2. `visit(VarDecl)` modified: for `GLOBAL` scope, skip the declaration (already
   at file scope) and only emit the initializer assignment in the main body.

**Emit order** (guaranteed by `emit()`):
```
#include "bb_runtime.h"
[type struct + helpers]
[global var declarations]   ← NEW
[user function bodies]
int main(...) {
    [global initializer assignments]
    [main body statements]
}
```

**Parser fix (bonus)**: `Name()` in statement position was crashing. The
statement parser had no paren-call path — it fell into the whitespace-arg loop,
tried to parse `(` as an expression, then choked on `)`.
Fix: added explicit paren-call branch in `parseStatement()` before the
whitespace-arg loop.

### Verification (`tests/test_global.bb`)
- `Greet()` (reads `Global name$`) → `Hello, world!` ✓
- Three `Increment()` calls (mutate `Global counter%`) → `Counter: 3` ✓
- Two `AddScore()` calls (accumulate `Global score#`) → `Score: 4` ✓
- `name = "BlitzNext"` in main + `Greet()` → `Hello, BlitzNext!` ✓

---

## v0.1.4 - "String Transformation" (2026-02-22)

### Milestone 20: String Transformation & Encoding

- **`bb_Upper(s)` / `bb_Lower(s)`** — case conversion via `std::transform` with lambda `(unsigned char c)` to avoid UB on signed char
- **`bb_Trim(s)`** — strips `' '`, `\t`, `\r`, `\n` from both ends
- **`bb_LSet(s, n)`** — left-aligned pad/truncate; **`bb_RSet(s, n)`** — right-aligned pad/truncate
- **`bb_Chr(n)`** → single-char string; **`bb_Asc(s)`** → unsigned ASCII of first char, 0 for empty
- **`bb_Hex(n)`** → uppercase hex, no prefix; negatives treated as unsigned 32-bit
- **`bb_Bin(n)`** → binary string, no leading zeros, minimum `"0"`
- **`bb_String(s, n)`** → repeat string n times

### Verification
- `Upper("Hello World")` → `HELLO WORLD` ✓ / `Lower(...)` → `hello world` ✓
- `Trim("  hello  ")` → `hello` ✓
- `LSet("Hi", 8)` → `Hi      ` ✓ / `RSet("Hi", 8)` → `      Hi` ✓
- `Chr(65)` → `A` ✓ / `Asc("Hello")` → `72` ✓
- `Hex(255)` → `FF` ✓ / `Bin(255)` → `11111111` ✓
- `String("ab", 3)` → `ababab` ✓
- Regressions (M19, M17-M18, all prior tests) pass ✓

---

## v0.1.3 - "String Runtime" (2026-02-22)

### Milestone 19: String Extraction & Search

- **`bb_string.h` (new file)** — standalone string header; `bb_runtime.h` includes it
- **`bbString` typedef** moved from `bb_runtime.h` to `bb_string.h` (its natural home)
- **`bb_Left(s, n)`** — first n characters; **`bb_Right(s, n)`** — last n characters
- **`bb_Mid(s, pos)`** — from 1-based pos to end; **`bb_Mid(s, pos, n)`** — n chars at pos (two overloads)
- **`bb_Instr(s, sub)`** — 1-based index of first match, or 0; **`bb_Instr(s, sub, start)`** — search from pos
- **`bb_Replace(s, from, to)`** — replaces all occurrences (safe against empty `from`)
- **`bb_Str`, `bb_Int(bbString)`, `bb_Float`, `bb_Len`** — moved from `bb_runtime.h`
- **`bb_Str(double)`** — overload changed from `float` to `double` to resolve C++ double-literal ambiguity; `%g` format trims trailing zeros

### Design Note
`bb_string.h` has no dependency on `bb_math.h` or `bb_runtime.h` — self-contained (`<string>`, `<algorithm>`, `<cstdio>` only). Umbrella `bb_runtime.h` includes `bb_string.h` first (provides `bbString`), then `bb_math.h`.

### Verification
- `Print Left("Hello World", 5)` → `Hello` ✓
- `Print Right("Hello World", 5)` → `World` ✓
- `Print Mid("Hello World", 7)` → `World` ✓ / `Mid(..., 7, 3)` → `Wor` ✓
- `Print Instr("Hello World", "World")` → `7` ✓ / `Instr("abcabc","b",3)` → `5` ✓
- `Print Replace("Hello World", "World", "Blitz")` → `Hello Blitz` ✓
- `Print Str(3.14)` → `3.14` ✓ (no trailing zeros via `%g`)
- Regressions (M17 math, M18 random, all prior tests) still pass ✓

---

## v0.1.2 - "Random Numbers" (2026-02-22)

### Milestone 18: Random Number Functions

- **`bb_Rnd()`** — float in [0, 1); **`bb_Rnd(max)`** — float in [0, max); **`bb_Rnd(min, max)`** — float in [min, max)
- **`bb_Rand(max)`** — int in [1, max]; **`bb_Rand(min, max)`** — int in [min, max]
- **`bb_SeedRnd(seed)`** — calls `std::srand(seed)`; stores seed in inline global `__bb_rnd_seed__`
- **`bb_RndSeed()`** — returns `__bb_rnd_seed__` as int
- All overloads live in `bb_math.h`; no parser or emitter changes needed (standard `bb_` call convention)
- `inline` global for seed storage requires C++17 — consistent with rest of project

### Verification
- `SeedRnd 42 : Print RndSeed()` → `42` ✓
- `SeedRnd 1234 : Print RndSeed()` → `1234` ✓
- `Rnd()` → values in [0, 1) (e.g. 0.00186, 0.53167) ✓
- `Rnd(100.0)` → value in [0, 100) ✓
- `Rnd(10.0, 20.0)` → value in [10, 20) ✓
- `Rand(6)` → int in [1, 6] ✓
- `Rand(3, 9)` → int in [3, 9] ✓
- Same seed produces identical sequence (deterministic) ✓

---

## v0.1.1 - "Math Runtime" (2026-02-22)

### Milestone 17: Math Completeness — Trig Inverse & Utility

- **`bb_math.h` (new file)** — standalone math header; `bb_runtime.h` includes it; all math symbols now live in one place
- **`bb_Pi`** — `constexpr float` (3.14159265…); `Pi` as a Blitz3D identifier emits `bb_Pi` via special-case in `Emitter::visit(VarExpr*)`
- **`bb_ASin(x)`** → `asin(x)` in radians → degrees; **`bb_ACos(x)`**, **`bb_ATan(x)`** — same pattern
- **`bb_ATan2(y, x)`** → `atan2(y, x)` → degrees; matches Blitz3D argument order (y first)
- **`bb_Sgn(x)`** → `-1`, `0`, or `1` (sign of x)
- **`bb_Log10(x)`** → base-10 logarithm
- **`bb_Int(float)`** → truncate toward zero (C++ overload alongside existing `bb_Int(bbString)` for string-to-int)
- **Internal helpers** `_bb_d2r()` / `_bb_r2d()` keep trig conversion DRY without polluting global scope
- **`bb_runtime.h` refactored** — math block removed; `#include "bb_math.h"` added; `<cmath>` dependency moved to `bb_math.h`

### Design Note
`Pi` is a Blitz3D built-in constant (used without parentheses). Since the lexer tokenises it as `ID`, it becomes a `VarExpr` in the AST. The Emitter's `visit(VarExpr*)` special-cases `pi` (case-insensitive) → emits `bb_Pi` instead of `var_pi`. No parser changes needed.

### Verification
- `Print Pi` → `3.14159` ✓
- `Print ATan2(1.0, 1.0)` → `45` ✓
- `Print ASin(1.0)` → `90` ✓
- `Print ACos(1.0)` → `0` ✓
- `Print ATan(1.0)` → `45` ✓
- `Print Sgn(-42.5)` → `-1`, `Sgn(0)` → `0`, `Sgn(7.3)` → `1` ✓
- `Print Log10(100.0)` → `2`, `Log10(1000.0)` → `3` ✓
- `Print Int(3.9)` → `3`, `Int(-3.9)` → `-3` ✓
- Regressions (Floor, Ceil, Sqr, Abs, Sin, Cos, Tan, all prior tests) still pass ✓

---

## v0.1.0 - "Type Iteration" (2026-02-22)

### Milestone 16: Type Iteration — First, Last, Before, After, Insert, Each

- **6 new AST nodes** — `FirstExpr`, `LastExpr`, `BeforeExpr`, `AfterExpr`, `InsertStmt`, `ForEachStmt`
- **`ASTVisitor`** — 6 new pure-virtual `visit()` overloads added
- **Parser — `For Each`** — detected at statement level (peek-ahead after `FOR`); dispatched to `parseForEach()` which consumes `FOR EACH var.TypeName ... NEXT`
- **Parser — `Insert`** — `INSERT expr BEFORE/AFTER expr` → `InsertStmt`
- **Parser — `First/Last/Before/After`** — all handled in `parsePrimary()` as expressions; `First TypeName`/`Last TypeName` take a type name token; `Before`/`After` take a sub-expression
- **Emitter — `FirstExpr`** → `__bb_TypeName_head__`
- **Emitter — `LastExpr`** → `__bb_TypeName_tail__`
- **Emitter — `BeforeExpr`** → `(obj)->__prev__`
- **Emitter — `AfterExpr`** → `(obj)->__next__`
- **Emitter — `InsertStmt`** → `bb_TypeName_InsertBefore(obj, tgt)` / `bb_TypeName_InsertAfter(obj, tgt)`; type resolved via `getExprTypeName()` helper (checks `varObjectTypes` map and `FirstExpr`/`LastExpr` nodes)
- **Emitter — `ForEachStmt`** → deletion-safe while-loop: caches `__next__` before body runs; iteration variable auto-registered in `varObjectTypes` for nested field access
- **`emitTypeDecl()`** — now also emits `bb_T_Unlink()`, `bb_T_InsertBefore()`, `bb_T_InsertAfter()` helpers for each Type

### Design Note
`For Each` uses a deletion-safe loop pattern:
```cpp
auto *_fe_cur_p_ = __bb_TypeName_head__;
while (_fe_cur_p_) {
    auto *var_p = _fe_cur_p_;
    _fe_cur_p_ = _fe_cur_p_->__next__;  // cached before body
    // body
}
```
This allows `Delete p` inside the loop body without corrupting iteration, matching Blitz3D's semantics.

### Verification
- `For Each n.Node : Print n\val : Next` → `10  20  30` ✓
- `First Node`, `Last Node` → correct head/tail pointers ✓
- `After First Node`, `Before Last Node` → middle element ✓
- `Insert c Before b` → reordering `10 30 20` ✓
- `Insert a After b` → reordering `30 20 10` ✓
- `Delete b` then `For Each` → skips deleted element safely ✓
- `(First Node)\val`, `(Last Node)\val` → field access on First/Last ✓
- All prior tests (M14/M15 type instances, M12 data) still pass ✓

---

## v0.0.9 - "Type Instances + Field Access" (2026-02-22)

### Milestone 14: Type Instances — New, Delete

- **`NewExpr` AST node** — `typeName`; emits `bb_TypeName_New()`
- **`DeleteStmt` AST node** — `object` expr; emits `bb_TypeName_Delete(var); var = nullptr;`
- **`Local v.TypeName`** — `.TypeName` type annotation parsed in `parseVarDecl()` as `typeHint = ".TypeName"`; emitter maps to `bb_TypeName *var = nullptr;`
- **`emitTypeDecl()`** — emits C++17 struct with field members + intrusive doubly-linked list: `__next__`, `__prev__`, head/tail globals, `bb_T_New()`, `bb_T_Delete()`
- **Emitter** — TypeDecl emitted before function definitions; `typeNames` and `varObjectTypes` maps track types/instances for code-gen; TypeDecl excluded from main body loop

### Milestone 15: Type Field Access (`\` operator)

- **`FieldAccess` AST node** — `object` + `fieldName`; emits `obj->var_field`
- **`FieldAssignStmt` AST node** — `object` + `fieldName` + `value`; emits `obj->var_field = expr;`
- **`parsePostfix()`** — new parsing level between `parsePrimary()` and `parsePower()`; handles chained `\` access (`a\b\c`)
- **Statement context** — `var\field = expr` detected after type-hint consumption in identifier-led branch

### Design Note
Each `Type` declaration generates a standalone C++ struct `bb_T` with intrusive doubly-linked list pointers. `New` allocates and appends to the tail; `Delete` unlinks from the list and frees. This matches Blitz3D's reference semantics and prepares for `First`, `Last`, `Each` iteration (M16).

### Verification
- `Local v.Vec = New Vec : v\x = 10 : v\y = 20 : Print v\x + v\y` → `30` ✓
- Float fields, multiple instances, chained field writes all pass ✓
- `Delete v2` → frees instance, sets pointer to nullptr ✓
- All prior tests still pass ✓

---

## v0.0.8 - "Type Declaration (Struct Parsing)" (2026-02-22)

### Milestone 13: Type Declaration (Struct Parsing)

- **`TypeDecl` AST node** — `name` (string) + `std::vector<Field>` where `Field = {name, typeHint}`
- **`ASTVisitor`** — added `visit(TypeDecl*)` pure virtual
- **Parser** — `TYPE` keyword dispatched to `parseTypeDecl()`; multi-line and colon-separated forms supported; multiple fields per `Field` line (`Field left%, top%, right%, bottom%`); `End Type` and `EndType` both accepted; `TYPE` added to `END` secondary-keyword list
- **Emitter** — `visit(TypeDecl*)` no-op stub; full C++ struct emission deferred to M14

### Design Note
`Field` declarations support one or more comma-separated names on a single line, each with an optional type hint. A `TypeDecl` with no `Field` lines is also valid. The `End Type` terminator can appear on the same line as the last field (colon-separated) or on its own line.

### Verification
- `Type Player : Field x%, y% : End Type` → no crash ✓
- `Type Vec2 : Field dx!, dy! : End Type` → no crash ✓
- `Type Rect : Field left%, top%, right%, bottom% : End Type` → no crash ✓
- `Print "Types parsed OK"` → `Types parsed OK` ✓
- All prior tests still pass ✓

---

## v0.0.7 - "Data / Read / Restore" (2026-02-22)

### Milestone 12: Data, Read, Restore

- **`DataStmt` AST node** — `std::vector<Token> values`; supports signed numeric literals (`-3`, `-1.5`)
- **`ReadStmt` AST node** — `name` + `typeHint`; used by emitter for correct type cast + implicit declaration
- **`RestoreStmt` AST node** — optional `label` field; bare `Restore` resets to index 0
- **Parser** — `DATA`, `READ`, `RESTORE` keywords dispatched; `parseData()` handles signed literals and comma-separated values; `parseRead()` consumes optional type hint; `parseRestore()` accepts plain and dot-label forms
- **Runtime (`bb_runtime.h`)** — `bb_DataVal` tagged struct with `operator int/float/double/bbString()`; inline globals `__bb_data_pool__` + `__bb_data_idx__`; `bb_DataRead()` / `bb_DataRestore(size_t idx=0)`
- **Emitter** — `collectData()` recursive first-pass fills pool before any `Read`; `visit(DataStmt*)` is no-op; `visit(ReadStmt*)` auto-declares undeclared variables (Blitz3D implicit decl); `visit(RestoreStmt*)` emits `bb_DataRestore()`; `declaredVars` set prevents re-declaration

### Design Note
All `Data` statements across the entire program form **one flat sequential pool** — this is correct Blitz3D behaviour. `Read` advances a single pointer through this pool regardless of where the `Data` statement appears in source.

### Verification
- `Data 10,20,30 : Read a : Read b : Print a + b` → `30` ✓
- `Read c : Print c` → `30` (next item in pool) ✓
- `Read s$ : Read t$ : Print s$ + " " + t$` → `Hello World` ✓
- `Restore : Read x : Print x` → `10` (pool reset) ✓
- `Read n% : Read m% : Print n% + m%` → `30` (typed hint) ✓
- All prior tests still pass ✓

---

## v0.0.6 - "Goto / Gosub / Labels" (2026-02-22)

### Milestone 11: Goto, Gosub, Return (Legacy Flow)

- **`LabelStmt` AST node** — stores lowercase label name; handles both `.labelname` and `labelname:` syntax
- **`GotoStmt` AST node** — stores lowercase label name
- **`GosubStmt` AST node** — stores lowercase label name
- **Parser — `:` separator** — `skipNewlines()` now also skips `OPERATOR(":")` tokens; call-arg loop and `parseReturn()` stop at `:`
- **Parser — label detection** — at statement level: `OPERATOR(".")` + ID → dot-label; ID + `OPERATOR(":")` → colon-label
- **Parser — GOTO / GOSUB** — both accept `Goto label` and `Goto .label` forms
- **Emitter — LabelStmt** → `lbl_name:;` (null statement satisfies C++ label grammar)
- **Emitter — GotoStmt** → `goto lbl_name;`
- **Emitter — GosubStmt** → GCC computed-goto trick: `__gosub_ret__ = &&_gosub_ret_N_; goto lbl_X; _gosub_ret_N_:;`
- **Emitter — ReturnStmt** — new `inFunctionBody` flag: bare `Return` in main → `goto *__gosub_ret__`; in function → `return;`

### Verification
- `Goto skip : Print "SKIP" : skip: Print "OK"` → `OK` ✓
- `Goto .done : Print "SKIP2" : .done : Print "OK2"` → `OK2` ✓
- `Gosub greet` / `greet:` / `Return` → `Hello from Gosub` / `Back from Gosub` ✓
- All prior tests (arrays, consts, fixes) still pass ✓

---

## v0.0.5 - "Phase B Language Core" (2026-02-22)

### Milestone 10: Array Indexing

- **`DimStmt` AST node** — `name`, `typeHint`, `dims` (vector of ExprNodes for each dimension)
- **`ArrayAccess` AST node** — `name`, `indices` (vector) for expression-context reads
- **`ArrayAssignStmt` AST node** — `name`, `indices`, `value` for array element writes
- **Parser** — `parseDim()` registers array names in `dimmedArrays` set; disambiguates `name(i)` as `ArrayAccess` vs `CallExpr` in `parsePrimary()`; detects `name(i) = expr` as `ArrayAssignStmt` in statement context
- **Emitter** — `%`→`int`, `#`/`!`→`float`, `$`→`bbString`, no hint→`int`; 1D: `std::vector<T>(n+1)`; multi-dim: nested `std::vector`; Blitz3D size semantics: `Dim a(N)` → indices 0..N → C++ size N+1
- **`bb_runtime.h`** — added `#include <vector>`

### Verification
- `Dim arr%(5) : arr(2) = 42 : Print arr(2)` → `42` ✓
- `Dim scores#(3) : scores(0) = 1.5 : scores(1) = 2.5 : Print scores(0) + scores(1)` → `4` ✓
- `Dim grid%(3,3) : grid(1,1) = 99 : Print grid(1,1)` → `99` ✓
- Array in For loop: `vals(i) = i * 10 : Print vals(3)` → `30` ✓

---

### Milestone 9: Const Declarations

- **`ConstDecl` AST node** — `name`, `typeHint`, `value` (ExprNode)
- **Parser** — `Const name[hint] = expr`; multiple declarations per line via comma separator
- **Emitter** — `%`→`constexpr int`, `#`/`!`→`constexpr float`, no hint→`constexpr auto`, `$`→`const bbString`
- Constants use `var_` prefix for seamless `VarExpr` resolution (no parser changes needed at call sites)

### Verification
- `Const MaxHP% = 100` → `constexpr int var_MaxHP = 100;` ✓
- `Const Pi# = 3.14159` → `constexpr float var_Pi = 3.14159;` ✓
- `Const Greeting$ = "Hello"` → `const bbString var_Greeting = "Hello";` ✓
- `Const A% = 10, B% = 20` → two constexpr declarations ✓
- Full output: `100 / 3.14159 / Hello / 30 / HP is high` ✓

---

## v0.0.4 - "Structured Errors & CLI Parity" (2026-02-22)

### Milestone 7: Structured Error Reporting

- **GCC-compatible error format** — Parser now emits `filename:line:col: error: message` on stderr
- **Unified `error()` method** in `Parser` — replaces three separate `std::cerr` sites (`expect()`, `parseRepeat()`, `parsePrimary()`)
- **Filename propagation** — `parse()` accepts `const std::string& fname`; passed from `blitzcc.cpp` as `cfg.inputPath`
- **Error tracking** — `errorCount` member + `hasErrors()` method; pipeline aborts before emit when parse errors exist
- **Exit codes** — `transpile()` now returns `int`: 0 = success, 1 = parse error, 2 = compile error; `main()` forwards it
- **Compile-step error** also follows structured format: `filename:0:0: error: compilation failed`

### Milestone 8: CLI Parity

- **`-k` flag** — lists all 17 known built-in command names to stdout (one per line)
- **`+k` flag** — lists `name(signature)` per line; usable by IDEal for autocomplete
- **`kCommands[]` table** — static array of `{name, sig}` in `blitzcc.cpp`; single source of truth
- **`BLITZPATH` env var** — `resolvePath()` now checks `$BLITZPATH/rel` as a third fallback after CWD and `../`
- **`-release` flag** — accepted without error, ensures `debug = false` (compatibility with older IDEs that pass `-release`)
- **Version bump** — `-v` and help now report `v0.0.4`

### Verification
- `blitzcc -k` → 17 command names listed ✓
- `blitzcc +k` → `Print(value)`, `Sin(deg#)`, etc. ✓
- `blitzcc -release tests/test_fixes.bb` → exit 0 ✓
- `BLITZPATH=... blitzcc tests/hello.bb` → resolves toolchain via env var ✓
- `tests/bad.bb` → `tests/bad.bb:3:9: error: unexpected token 'THEN'`, exit 1 ✓
- `tests/test_fixes.bb` → `Success: … created.`, exit 0 ✓

---

## v0.0.3 - "Correctness & IDE Foundation" (2026-02-21)
A correctness overhaul that fixes critical compiler bugs, completes the core language, and lays the foundation for IDE integration.

### Language & Compiler Fixes (13 critical bugs resolved)
- **Assignments** (`x = expr`) now correctly emit `AssignStmt` instead of returning `nullptr`
- **Logical operators** `And`, `Or`, `Xor`, `Not`, `Mod`, `Shl`, `Shr`, `Sar` added to keyword table — were silently dropped before
- **`expect()`** now advances on mismatch; previously caused infinite loops on parse errors
- **`ElseIf` chains** fully implemented via recursive `parseIfTail()`; were fundamentally broken
- **`For...Step -1`** (negative step) fixed with block + ternary condition in emitter
- **Type hint `!`** (float) now correctly emits `float`; was falling through to `auto`
- **Function declarations** (`Function name%(param%)...End Function`) fully implemented with return type hint consumption
- **Preprocessor** `#Include` now resolves paths relative to the including file, not CWD; word boundary check prevents matching `INCLUDEFILES`

### New AST Nodes
- `AssignStmt` — explicit assignment statement
- `FunctionDecl` — user function with typed parameters
- `ReturnStmt` — bare or value-returning return
- `ExitStmt` — loop break
- `EndStmt` — program termination

### Runtime (bb_runtime.h)
- `bb_Print` upgraded to template (any printable type)
- Added: `bb_Input`, `bb_Str`, `bb_Int`, `bb_Float`, `bb_Len`
- Added math: `bb_Sin`, `bb_Cos`, `bb_Tan`, `bb_Sqr`, `bb_Abs`, `bb_Log`, `bb_Exp`, `bb_Floor`, `bb_Ceil`
- Added: `bb_Delay` (Windows/Linux platform-aware)

### Emitter (emitter.h)
- `indentLevel` tracking for properly nested output
- `inExprCtx` flag distinguishes `CallExpr` as statement vs. expression
- `userFunctions` set: first-pass collects declared function names; controls `bb_` prefix
- Operator mapping: `=`→`==`, `<>`→`!=`, `AND`→`&&`, `OR`→`||`, `MOD`→`%`, `SHL`→`<<`, `^`→`std::pow()`

### Cleanup
- Removed `commands.json` and all related scaffolding (`-k`/`+k` flags, `loadCommands()`, `listCommands()`, `nlohmann/json` dependency) — runtime is now the single source of truth for known commands
- `resolvePath()` helper in `blitzcc.cpp` replaces 4× duplicated path logic
- SDL3 linking made conditional on import library presence
- `-std=c++17` flag added to the g++ invocation

### IDE Foundation (Milestone 6)
- `Token` struct already carried `line` and `col` from the lexer
- Added `int line = 0` to `ASTNode` base class
- Parser now stores `token.line` on every constructed node (all statement and expression types)
- Parser errors already show `at line:col`; full IDE-compatible format follows in Milestone 7

### Roadmap
- Expanded from 12 coarse milestones to **70 atomic milestones** across 21 phases — each scoped to a single AI-session context window
- Parity table corrected: `Function`, `Return`, `True`, `False`, `Null`, `Include`, `Xor` operator now marked complete

### Verification
- `tests/test_fixes.bb` → `test_fixes.exe` ✓ (assignments, AND/NOT/OR, ElseIf, For Step -1, float hint, functions, single-line If)
- `tests/bad.bb` → parser errors with correct line numbers, no crash ✓

---

## v0.0.2 - "The Blitz3D Experience" (2026-02-21)
A major usability update that transforms the transpiler into a seamless build orchestrator.

### Features
- **One-Click Build Automation**: `blitzcc` now automatically invokes the MinGW toolchain.
- **Auto-Deployment**: Required runtime DLLs (like `SDL3.dll`) are automatically copied to the output folder.
- **Smart Output Placement**: Executables are created in the source directory by default, keeping the project root clean.
- **Ultra-Portable Linking**: Refined the linking strategy to bake `winpthread` statically into the executable.
- **Single-DLL Runtime**: Only `SDL3.dll` remains as an external dependency; `libwinpthread-1.dll` is no longer required.
- **Clutter Control**: Intermediate `.cpp` files are automatically cleaned up after successful compilation.

## v0.0.1 - Hello World Milestone (2026-02-21)
The first functional end-to-end transpilation pipeline.

### Milestone 1-5 Completion
- **Toolchain**: Robust `build_windows.bat` for automatic MinGW, CMake, and SDL3 setup.
- **Lexer**: Case-insensitive tokenization with support for keywords, identifiers, and literals.
- **Preprocessor**: Support for `#include` directives with circular inclusion protection.
- **Parser**: 
    - Full operator precedence matching Blitz3D.
    - Variable declarations (`Global`, `Local`, `Dim`) with type hints (#, %, !, $).
    - Control flow structures: `If/Else/EndIf`, `While/Wend`, `Repeat/Until/Forever`, `For/Next`, and `Select/Case`.
- **Emitter**: 
    - AST Visitor-based C++17 code generation.
    - Seamless integration with the local MinGW toolchain.
    - Automatic deployment of required runtime DLLs (`SDL3.dll`, `libstdc++`, etc.).

### Verification
- **Test Case**: `tests/hello.bb` successfully transpiles to `output.exe.cpp` and compiles to a native `hello_world.exe`.
- **Output**: "[DEBUG] blitzcc starting... / Hello from BlitzNext!"

---
*Devlog started by BlitzNext AI (Antigravity)*
