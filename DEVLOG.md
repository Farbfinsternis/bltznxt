# BlitzNext Developer Log

## v0.5.5 - "MD2 models, transparent bitmaps, more samples running" (2026-09-19)

Three days after v0.5.0. More of the samples that ship with Blitz3D now compile unmodified and
look the same as in Blitz3D: AGore/BirdDemo, mak/dragon and the GCUK `animation.bb` tutorial
through the new MD2 support, and birdie/Mirror, mak/flag, mak/primitives, mak/multicam,
birdie/lodBalls and si/matrix through the rendering fixes. Every fix was measured against a
running Blitz3D 11.8. The entries below this one describe each step; this is the overview.

**3D**
- New: sprites (3D-16), mirrors, collisions and line/entity picking (3D-17, 3D-18), MD2 models
  with animation (3D-23).
- Rendering as in Blitz3D: triangle winding for all meshes, sphere/cylinder/cone vertex tables,
  several cameras with viewports and render order, cameras with an empty viewport, the half-pixel
  offset, hidden parents, copies of hidden entities, translucency and the depth buffer,
  drawing order of see-through objects, and lighting limited before the texture is applied.
- Entities: `TurnEntity`, `TranslateEntity`, world rotation under scaled parents, `EntityParent`
  keeping the world position, `ClearWorld` flags.

**2D and runtime**
- Drawing into image and texture buffers, `CopyRect` between all buffers, loaded images masked
  with black, a graphics mode change resets the drawing state.
- The same random numbers as Blitz3D, floats printed like Blitz3D, math functions as the x87
  computes them, runtime errors end with Blitz3D's message.
- Fullscreen scales the program's resolution, mouse movement is not lost around `MoveMouse`.
- TCP streams and hostname lookup, `DebugLog`.

**Compiler**
- Float-to-int conversion rounds everywhere, float literals are floats, `Abs`/`Sgn` keep their
  type, division by a constant power of two, field assignments convert to the field type.
- `Gosub` keeps a return stack, jumps into `Case` branches work, `Read` uses the declared type
  and stops with "Out of data", `Include` paths are relative to the main file.

**Documentation**
- `KNOWN_ISSUES.md` lists 47 open deviations, each reproduced against Blitz3D. 104 of
  Blitz3D's commands are still missing; the command table has 463 entries.

---

## 2026-09-24 — Deleted objects behave as in Blitz3D (BUG-104, BUG-103)

`Delete p` used to free the object at once and set only `p` to `Null`. Every other reference —
`q = p` beforehand, a field, a list — kept pointing at freed memory: `q = Null` was false, and
reading `q\x` read garbage or crashed. Common in games, where one object remembers another as a
target.

Blitz3D counts references (`bbruntime/basic.cpp`). Every variable, field and array element
holding an object counts, and so does the type's list. `Delete` releases the fields, marks the
object as deleted and gives up the list's reference; the object itself stays, still linked
into the list, until the last reference is gone. A deleted object compares equal to `Null`, and
`For Each`, `First`, `Last`, `After` and `Before` skip it.

BlitzNext now does the same. Object variables, fields, arrays, parameters and return values
are a small counted reference type (`bb_ref`, in `bb_object.h`) instead of raw pointers; the
generated list functions skip deleted objects and free an object with its last reference.
Accessing a field of `Null` or of a deleted object stops with `Object does not exist`, as in
Blitz3D's debug mode — a release build there crashes (see "Intentional differences").

This also fixes BUG-103: `For Each` now finds the next object *after* the loop body, from
whatever the loop variable holds then, just as `_bbObjEachNext` does. Changing the variable in
the body continues from there, and deleting the current object in the body is safe because the
variable still holds it.

Measured against Blitz3D: aliases, two deleted objects compared, lists with deleted objects,
`First`/`Last`/`After`/`Before`, deleting twice, deleting inside `For Each` (the current object
and its successor), `Insert` of a deleted object and `Delete` of a parameter — all ten cases
match, where the old code crashed from the third case on. The three error messages match
Blitz3D's debug mode. Tests `test_bug104_objekt_geloescht` and `test_bug104_feld_geloescht`.

While measuring, BUG-180 turned up: the left-to-right evaluation promised since BUG-29 does not
see a call that deletes or changes an object, so in `F(w) + (w = Null)` the comparison can run
first. Listed in KNOWN_ISSUES with a workaround.

---

## 2026-09-24 — Screenshots and screen recording see the 3D fullscreen (BUG-171)

In 3D fullscreen, the Windows screenshot tool captured an old frame (in blox-n-balls the loading
screen while the menu was showing), and OBS's display capture stayed black. Window capture
worked, and so did everything with Blitz3D's real mode change.

The cause is the graphics driver: when an OpenGL window's drawing area covers the monitor
exactly, the NVIDIA driver sends its frames straight to the display, past the Windows
compositor. Anything that captures the screen through the compositor then keeps seeing whatever
it got last. Measured by capturing the screen while a program changes colour every four
seconds: our 3D fullscreen stayed on the first colour, windowed 3D and the 2D fullscreen
(which SDL presents through Direct3D) did not. A drawing area one pixel larger than the monitor
is enough to stop the bypass; a layered window, a one-pixel frame outside the drawing area and
an invisible window on top are not.

`Graphics3D` in fullscreen now makes the window one pixel wider than the monitor on each side
of the axis that has the black bars (800×600 on 1920×1080: 1922×1080 at x = −1). The picture
is centred on that axis and scaled by the other one, so it stays exactly where it was.
Measured before and after at 800×600 and 1280×400: picture edges, 2D and 3D drawing and the
mouse position after `MoveMouse` are identical. The window keeps its size across a focus
change, and `Flip 1` still waits for the display in fullscreen (60 fps), `Flip 0` does not.
Checked with OBS's display capture on blox-n-balls. There is no automated test, since the suite
does not open fullscreen windows.

---

## 2026-09-24 — Sprites appear in mirrors (BUG-168)

A sprite above a `CreateMirror` plane had no reflection. The mirror pass draws the scene with a
camera reflected at the mirror plane and flips back-face culling. A sprite that faces the camera
takes its axes from that reflected camera, so its quad comes out with the *normal* winding in
the picture again and fell victim to the flipped culling. Blitz3D's `Sprite::render` handles
this by building the two triangles the other way round when the render context is reflected
(`0,2,1` and `0,3,2`); `bb_sprite_build_` now does the same, for every view mode, as the original.

The old code was wrong in both directions: camera-facing sprites had no reflection, while fixed
sprites (`SpriteViewMode 2`) turned away from the camera showed one they should not have.
Measured against Blitz3D with all four view modes, turned and rolled sprites, alpha and a
hidden mirror: pixel counts match, apart from 1–2 edge pixels on slanted edges. Test
`tests/test_bug168_sprite_spiegel.bb`, `.expected` produced in Blitz3D.

---

## 2026-09-23 — LoadBuffer scales the file to the buffer (BUG-179)

`LoadBuffer` worked only on a locked buffer, and then replaced its contents *and size* with the
file — an image even changed its `ImageWidth`. Blitz3D's `bbLoadBuffer` needs no lock: it loads
the file, scales it to the buffer with `tformCanvas` and copies it in opaquely at 0,0.

The scaling follows `tformCanvas` with `TFormFilter` on: the centre of every target pixel is
mapped back into the file and mixed bilinearly from four neighbours. Measured in Blitz3D, a
neighbour outside the file counts as black — clamping to the edge instead gets 371 of 777 pixels
wrong in a 4×3 → 37×21 case. Three more rules came out of the measurement: the buffer's origin
is set to 0,0 and stays there, its viewport clips the copy, and on a *locked* buffer `LoadBuffer`
returns 1 but changes nothing, not even after `UnlockBuffer`.

Results were compared through `SaveBuffer`, which since BUG-167 writes byte-identical files:
the same size into the back buffer, 4×3 into an image of 37×21, 64×64 into a 16×16 texture,
origin, viewport, mask colour and a missing file all match byte for byte. A 10×10 → 37×37 case
matches except for 2 of 1369 channel values that fall exactly on .5, where Blitz3D rounds once
down and once up; none of 256 rounding variants — float, double or 64-bit at each step,
computed exactly — reproduces both, so it is left there.

Two older faults surfaced on the way. `LockBuffer` on the screen read only the active viewport,
because SDL reads the viewport it is given, and `UnlockBuffer` wrote back into it; both now
cover the whole buffer. And a freshly locked `CreateImage` or a never-drawn back buffer read
as `$00000000`, where Blitz3D, which has no alpha channel there, gives `$FF000000`.

Test: `tests/test_bug179_loadbuffer.bb` with `tests/assets/farben4x3.bmp`, expected output
generated in Blitz3D.

---

## 2026-09-23 — SaveBuffer and SaveImage write Blitz3D's BMP (BUG-167)

Pressing F12 in blox-n-balls saved no screenshot. `SaveBuffer` worked only on a buffer that was
locked with `LockBuffer`, and then wrote PNG even when the name ended in `.bmp`; `SaveImage` wrote
PNG as well.

In Blitz3D both go through one function, `saveCanvas` in `bbgraphics.cpp`: it locks the buffer
itself and always writes an uncompressed 24-bit BMP — a 54-byte header with only type, sizes,
offset, width, height, planes and bit depth set, rows from the bottom up, each padded to four
bytes — whatever the file is called. BlitzNext now writes exactly that. The contents come from
the existing lock if there is one, so pixels written with `WritePixelFast` are included, or else
from a short lock of its own that leaves the buffer as it was.

Measured with the back buffer unlocked, under a `.png` name and locked, the front buffer after
`Flip`, a 37×21 image (row padding), `SaveImage` on frame 2 of an animation, a texture buffer and
a folder that does not exist: all nine files are byte-identical to Blitz3D's, and the return
values match. Before, three files were written, all PNG. `stb_image_write` is no longer needed.

Reading Blitz3D's `LoadBuffer` next to it shows the same kind of problem there: it needs no lock
and scales the file to the buffer, while ours needs a lock and takes over the file's size. That is
BUG-179, fixed in the entry above.

Test: `tests/test_bug167_savebuffer.bb` reads the files back (size, header fields, a sum over
every byte), expected output generated in Blitz3D.

---

## 2026-09-23 — Entity commands check their handles (BUG-170)

`EntityX(0)` returned 0 here, and so did every other entity command given a handle that was
never created, already freed, or of the wrong kind — a missing `Global` in a function went
unnoticed. Blitz3D behaves in two ways, measured with both builds:

| Case | Blitz3D release | Blitz3D debug | BlitzNext before |
|---|---|---|---|
| `EntityX(0)` | "Memory access violation" | `Entity does not exist` | 0 |
| `EntityX` of a freed cube | its old value, 5.0 | `Entity does not exist` | 0 |
| `CameraZoom` on a cube | nothing visible, writes into the cube | `Entity is not a camera` | ignored |

The release column is whatever the memory happens to hold, and not something to copy. Entity
commands now always check the way Blitz3D's debug mode does (`debugEntity`, `debugModel`,
`debugMesh`, `debugCamera`, `debugLight`, `debugSprite`, `debugMD2`, `debugParent`,
`debugColl` in `bbblitz3d.cpp`) and stop with its message on stderr:

```
Runtime Error: Entity is not a camera
```

All 111 of the 145 checked commands that BlitzNext has are covered. Handle 0 stays allowed where
Blitz3D allows it: as "no parent" and as world space in `TFormPoint`, `TFormVector` and
`TFormNormal`. A matrix of 24 calls — every kind of check, 0 allowed and not — gives the same
message at the same point as Blitz3D's debug mode in all 24.

The test runner can now compare a test's stderr too: a `.expected_stderr` file next to the test
holds the message, without the `[GL]`/`[shader]` diagnostic lines. Tests:
`test_bug170_entity_frei.bb`, `test_bug170_kein_model.bb`, `test_bug170_parent.bb`.

---

## 2026-09-23 — Objects outside the camera's view are skipped (BUG-174)

Blitz3D tests every model against the camera's view before drawing it and skips what lies
outside: a mesh by its bounding box, a sprite by its four corners, an MD2 model by its box.
BlitzNext did this only for MD2 and drew everything else. The picture was the same, but
`TrisRendered` counted the hidden objects too — in the BirdDemo 4064 triangles where Blitz3D
reports 4040 and 4052 — and the work was done for nothing.

The rule comes from `frustum.cpp`: an object is dropped only when all its points lie outside
the *same* plane of the view; a point exactly on a plane counts as inside, so a box just off a
corner of the view still counts. The view is the one `Camera::getFrustum` builds — near and far
from `CameraRange`, half width `near/zoom`, half height scaled by the viewport's aspect — and it
does not depend on the projection: an orthographic camera culls with the perspective view.
Something next to the camera that the orthographic picture would show is therefore dropped
in Blitz3D, and now here as well.

The bounding box lives with the mesh data that copies share and is recomputed whenever the
geometry changes. `ClearSurface` and `AddMesh` did not mark a change until now, so the collision
tree also stayed stale after them; both do now.

Measured in Blitz3D with 249 values (edges, a corner, near and far plane, rotated and scaled
objects, a turned camera, zoom, a narrow viewport, a child of a pivot, orthographic projection,
geometry edited after drawing, sprites): all equal, 45 of them differed before. The BirdDemo
now reports Blitz3D's counts at all nine measuring points.
Test: `tests/test_bug174_sichtkegel.bb`, 99 lines generated in Blitz3D.

---

## 2026-09-22 — Userlibs are out of scope, and the compiler says so

Blitz3D extends its command set with 32-bit Windows DLLs declared in `userlibs/*.decls`. The
interface prescribes `_stdcall` and hands the DLL raw addresses of banks and objects with no size
and no type. BlitzNext will not support it.

The reason is the purpose, not the difficulty. Userlibs were mostly used to teach Blitz3D what it
could not do — whole rendering engines were bolted on that way — and those capabilities are what
BlitzNext builds itself. With the use case gone, a Windows-only 32-bit interface has no place in an
engine meant to run on three platforms. A replacement is planned for the phase after compatibility:
one loader over `.dll`/`.so`/`.dylib`, a stable C ABI, checked arguments instead of bare pointers,
and plugins that announce themselves with a version. It will not be compatible with `.decls`.

Until now such a program failed with a plain `unknown function or command` — true, and useless. The
compiler now reads the declared names out of `userlibs/*.decls` and names the file instead:

```
game.bb:12:5: error: 'MessageBoxTest' is declared in userlibs/testlib.decls - userlibs are not supported, see KNOWN_ISSUES.md
```

Only the names are read, and only when a call matches nothing else; the ordinary "did you mean"
path is untouched. The declarations are parsed as `UserLibs.txt` describes them: `.lib` lines and
comments are skipped, a type tag (`%`, `#`, `$`) between the name and the bracket is ignored, and a
decorated name after the colon does not matter here.

---

## 2026-09-22 — A pixel centred exactly on a right edge is left out (BUG-176)

Direct3D 7 leaves out a pixel whose centre lies exactly on the right edge of a triangle;
OpenGL, with its origin at the bottom, decides the other way. Since BUG-152 the picture is
shifted half a pixel right and down to put pixel centres where Direct3D has them, and
vertically that shift is deliberately 1/256 of a pixel short so that no edge can land exactly
on a centre. Horizontally it was exactly half a pixel, so edges did land there.

The case needs an edge running through pixel centres, which is why squares and diagonals
aligned to pixel *boundaries* showed nothing. Reconstructed from the MD2 test, whose camera
carries a small offset to avoid it: a right triangle at (-5,-5) (5,-5) (-5,5), camera at
(0,0,-10), 200×200 window, its hypotenuse a 45° diagonal through the pixel centres. Blitz3D
draws row 60 as 50-59, BlitzNext drew 50-60 — every row from 52 to 148, and the topmost row
belonged here entirely while Blitz3D left it out.

Horizontally the shift is now 1/2 − 1/256 of a pixel as well. The reconstructed case matches
Blitz3D afterwards and `test_bug152_raster.bb` stays green. Sweeping the same triangle through
91 rotations moves closer to Blitz3D too: 60 of 91 angles differed, now 44, and the summed
difference of the row ends falls from 729 to 405 pixels. What remains is ordinary subpixel
rasterisation on slanted edges (NVIDIA resolves 8 bits), not edges on pixel centres.

Test: `tests/test_bug176_fuellregel.bb`, 54 rows measured in Blitz3D, 50 of them wrong before.

---

## 2026-09-22 — `Flip` waits for the display only in fullscreen (BUG-178)

A program in a window ran at the monitor's refresh rate here and much faster in Blitz3D. Before
changing anything, Blitz3D was measured again, 300 frames per run on a 60 Hz monitor:

| | `Flip 1` | `Flip` | `Flip 0` |
|---|---:|---:|---:|
| 2D window 320×240 | 164 fps | 149 fps | 8824 fps |
| 3D window 320×240 | 149 fps | 138 fps | 9677 fps |
| 2D fullscreen 640×480 | 60.6 fps | 60.7 fps | 2679 fps |
| 3D fullscreen 640×480 | 60.6 fps | 60.7 fps | 2679 fps |

So Blitz3D's `Flip` never waits in a window, not even with `Flip 1` — its DirectDraw blit does
not sync on today's Windows — while in fullscreen it waits exactly as documented. `Flip` now
follows that: the swap interval is 0 in a window whatever the argument says, and in fullscreen
`Flip`/`Flip 1` sync while `Flip 0` does not. The same four programs measured here afterwards:
window 12000 fps (2D) and 6250 fps (3D), fullscreen 60.0 and 60.9 fps with `Flip 1`, 4348 and
2083 fps with `Flip 0`.

In a window BlitzNext is now *faster* than Blitz3D rather than equal to it: Blitz3D still pays
about 6.5 ms a frame for its blit, which is what caps it near 150 fps, and that cap is a
property of its renderer, not of the language. Programs that move a fixed step per frame have
to time themselves with `MilliSecs()` — which is what Blitz3D programs have always done. The
case that started this, `tutorials/GCUK_Tuts/animation.bb`, now runs at 662 frames per second
instead of 57 and ends in the same place.

While fixing it, a second defect surfaced: the remembered swap interval survived a new window,
so a freshly created renderer — which starts without vsync — could keep it, because the
remembered value already matched and setting it was skipped. It is reset in `Graphics`,
`Graphics3D` and `EndGraphics` now.

Test: `tests/test_bug178_flip_fenster.bb`, in a 2D and a 3D window: `Flip 1` must not take longer
than `Flip 0`, and both must stay under the time the refresh rate would impose. Fullscreen stays
out of the suite because it would switch the screen during a run; it was measured by hand
against Blitz3D.

---

## 2026-09-19 — MD2 models (3D-23)

`LoadMD2`, `AnimateMD2`, `MD2AnimTime`, `MD2AnimLength` and `MD2Animating` are available. The
format is old, but Blitz3D programs use it: six models ship with Blitz3D alone, among them the
birds of the BirdDemo. The implementation follows Blitz3D's own MD2 code: the axes are swapped
the same way, vertices that share an index and a texture coordinate are merged in the same
order, and frames are blended linearly, positions and normals alike. Animation time advances in
`UpdateWorld` by speed × elapsed, only for visible models, with the same loop, ping-pong and
one-shot rules. Transitions freeze the current pose and blend towards the new animation. A copy
shares the model but starts unanimated, and a model outside the view is neither drawn nor
counted. Measured against Blitz3D: the animation time in 30 situations is identical, the
Gargoyle rendered unlit matches to a hundredth of a colour step, and the unmodified BirdDemo
now runs with both birds, with the same `TrisRendered` at every step and the picture differing
by less than one colour step on average. Two smaller findings came out of it: a pixel whose
centre lies exactly on a right edge is drawn here and not in Blitz3D (BUG-176), and objects
scaled unevenly are lit differently (BUG-177, to be decided under the lighting guideline).
Blitz3D itself crashes when a one-shot animation ends on the last frame of the file; BlitzNext
stays on that frame instead.

---

## 2026-09-19 — Light is limited before the texture, BirdDemo now matches (BUG-173)

The BirdDemo from the Blitz3D samples still needs `LoadMD2`, which we do not have yet, so it
was measured with the two birds replaced by cubes. It renders fixed steps of its camera flight
and averages every 8×8 block of the picture. The camera matched Blitz3D at every step, but
sunlit rock and grass were about 13 % too bright in red and green, while blue was the same. The
demo's ambient light plus its yellowish sun adds up to more than 1 in red and green. Direct3D's
fixed pipeline limits the lit colour to 1 first and multiplies the texture into it afterwards;
we multiplied the texture in before limiting. With the order changed, the average block differs
by less than half a colour step, and no block differs by more than 12. Two new open items came
out of the comparison: objects outside the view are still drawn and counted by `TrisRendered`
(BUG-174), and compiled programs open a console window (BUG-175).

---

## 2026-09-19 — Loaded images are masked with black (BUG-172)

At the end of a level, blox-n-balls showed its "next stage" banner on a black box. The game loads
the banner with `LoadImage` and never calls `MaskImage`. In Blitz3D a loaded image is masked
with black right away: all 9040 black pixels of the banner stay transparent, in 16-bit as in
32-bit mode, while nearly black pixels are drawn. We only masked after an explicit `MaskImage`.
Loading now masks black. Measuring the details showed that a mask in Blitz3D is a colour key
and nothing more, so three smaller things changed with it: a second `MaskImage` with another
colour makes the old one visible again, `ReadPixelFast` on a masked pixel returns `FF000000`,
and black written with `WritePixelFast` becomes transparent. The banner now looks as it does
in the original.

---

## 2026-09-19 — See-through surfaces no longer hide what comes after (BUG-169)

The sparks in the blox-n-balls main menu were missing. A single spark measured exactly like
Blitz3D, so the game itself had to be measured. We rendered each menu frame once with and once
without the sparks: in Blitz3D they changed about 4500 pixels, here none, although all 84 sparks
were in the right place. Next to them, at exactly the same depth, the menu has two large sprites
with `EntityAlpha 0.05`. Blitz3D draws every surface that does not blend with "replace" in a
separate pass that tests the depth buffer but does not write to it. We did write to it, so when
the nearly invisible sprite came first, every spark behind it failed the depth test. Surfaces that
blend now leave the depth buffer alone. While measuring this, the order of see-through objects at
the same distance turned out to be arbitrary here. Blitz3D keeps them in a priority queue, and
the order in which it resolves a tie is now reproduced, the same one we already use for cameras.
The mixed colour of two to seven overlapping sprites, and of a parent/child chain, now matches
Blitz3D. The sparks look as they do in the original.

---

## 2026-09-19 — A camera with an empty viewport draws nothing (BUG-166)

In the blox-n-balls main menu we rendered twice as many triangles as Blitz3D. The game hides
its game camera in the menu with `CameraViewport cam,0,0,0,0`. We treated a viewport of width
or height 0 as "the whole window", so that camera cleared the screen and drew the whole level
behind the menu. In Blitz3D such a camera neither clears nor draws, whatever its order among
the other cameras. While measuring this, a second difference showed up: `CreateCamera` takes
the current 2D `Viewport` as its viewport, and the full graphics size only when none is set.
Both now match the original, measured pixel by pixel and with `TrisRendered`. Suite 272/272.

---

## 2026-09-18 — Mouse movement that arrived too late (BUG-165)

The paddle in blox-n-balls did not move with the mouse. A log inside the game showed
`MouseXSpeed` returning 0 in 2308 of 2310 frames. The game reads the speed at the top of its
loop and puts the mouse back in the middle with `MoveMouse` after `RenderWorld`. Our runtime also
collects input events during `RenderWorld` and `Flip`, so the movement arrived after the read,
and `MoveMouse` overwrote it before the program ever saw it. In Blitz3D, DirectInput buffers the
movement and the next read applies it, even after `MoveMouse`. `MoveMouse` now carries the
unread movement over, including the fraction below one pixel that gets lost when the image is
scaled to a larger screen, and `FlushMouse` no longer drops movement, only button state, as in
Blitz3D. Measured with synthetic mouse input: the game's loop order went from 0 to 200 frames
with movement. The paddle now moves smoothly. Suite 271/271.

---

## 2026-09-18 — Nested Gosub, found in blox-n-balls (BUG-106)

blox-n-balls froze while loading; Windows marked the window as not responding. Attaching gdb to
the frozen process showed the main thread in our Gosub return table. The game calls
`Gosub start`, and `.start` itself calls `Gosub closedoors`. We kept only one return address,
so after `closedoors` the `Return` of `.start` jumped back behind `Gosub closedoors` again,
forever. Blitz3D compiles `Gosub` to a real `call` and `Return` to `ret`; we now keep a stack of
return addresses. Nesting and recursion (5000 deep) match the original, and a `Return` without
an open `Gosub` ends the program normally, as it does there. The game now gets past loading into
the first level. Suite 271/271.

---

## 2026-09-18 — Math functions as the x87 computes them (BUG-163)

`Sin(30) - 0.5` is `1.26184e-008` in Blitz3D, and `Sin(30) = 0.5` is false. The reason is the
FPU: Blitz3D runs it in 24-bit precision mode, so every arithmetic step rounds to a float, but
the transcendental instructions compute in full precision and their result stays unrounded in
the register, even across the return from `bbSin`. The runtime now models exactly that: `Sin`,
`Cos`, `Tan`, `Log`, `Log10` and `Exp` return a register value (`long double`, which is the
80-bit x87 format with MinGW). Any calculation with it yields a float, comparisons and integer
rounding use the full value, and assignment rounds. `ATan`, `ATan2`, `ASin`, `ACos` and `Sqr`
round, because their last step already rounds in Blitz3D.

A prototype that just returned doubles fixed the single values but made chains like
`Sqr(r) * Sqr(r) - r` worse, so it was not taken. Of 1360 measured values 598 differed before
and 23 do now; those are `ASin`, `ACos` and `Exp` in the last digit, which the old C runtime
computes with reduced precision itself (left open by decision). Suite 270/270.

---

## 2026-09-18 — Runtime exceptions end with a message (BUG-164)

An integer division by zero crashed the program without a word, and output that was still
buffered was lost with it. Hardware exceptions now end the program the way Blitz3D's
`seTranslator` does, with "Integer divide by zero", "Memory access violation" (for example a
field of a `Null` object), "Illegal instruction", "Stack overflow!" or "Unknown runtime
exception", written to stderr after the output has been flushed. Blitz3D shows the same texts in
a dialog. The two integer exceptions need a handler that runs first: otherwise the MinGW
runtime handles them itself, and `-2147483648 / -1` even retried the instruction forever, so
the program hung. Suite 268/268.

---

## 2026-09-18 — Dividing by a constant power of two (BUG-162)

Blitz3D's code generator turns an integer division by a constant power of two into an
arithmetic shift (`munchArith` in `codegen_x86.cpp`). For negative numbers a shift rounds
down instead of towards zero, so with `i = -33`, `i / 16` is `-3` in Blitz3D. We now do the
same, under the same conditions: the divisor is a constant `1 << k` (including `$80000000`)
and the left side is not a constant, because two constants are folded first and truncate as
usual. 27 cases measured, all equal, and the 2000 random expressions from BUG-97 now all match.
Suite 266/266.

---

## 2026-09-18 — Float literals are floats (BUG-97)

A float literal like `0.1` went into the generated C++ as a double, and so every calculation
involving a literal ran in double precision. The visible result: `a# = 0.1 : Print (a = 0.1)`
printed 0, because a float was compared with a double. `^` had the same problem through
`std::pow`. Literals are now written as floats, rounded the way Blitz3D rounds them, and `^`
returns a float like Blitz3D's `__bbFPow`. 2000 random expressions compared bit by bit with
the original: 139 differed before, now one does, and that one is a Blitz3D quirk of its own.
Integer division of a variable by a constant power of two is compiled as a bit shift there,
so `-33 / 16` becomes `-3` (BUG-162). Also noted: `ATan`, `ASin` and `Exp` differ in the last
bit because Blitz3D computes them in double (BUG-163), and an integer division by zero at run
time crashes here without a message (BUG-164). Suite 265/265.

---

## 2026-09-18 — `Abs` and `Sgn` keep their operand's type (BUG-96)

`Abs` and `Sgn` are operators in Blitz3D, not functions: the result has the type of the
operand. `Abs(3)/2` is `1` and `Sgn(2.0)/2` is `0.5`; here `Abs` always returned a float and
`Sgn` always an integer. The runtime now has both versions and the type check gives the call
the operand's type. A string operand is rejected as in Blitz3D instead of being converted.
29 cases measured, all equal, and the two tests that showed the deviation after the float
format fix now match the original completely. Suite 264/264.

---

## 2026-09-18 — Floats print like Blitz3D (BUG-68)

A float turned into text now looks exactly as in Blitz3D: `2.0` instead of `2`, `1234570.0`
instead of `1.23457e+06`, `1.e+008`, `1.23e-004`, `NaN`, `Infinity`. The runtime rebuilds
`ftoa()` from the Blitz3D source: six significant digits, at least one decimal, and below
0.001 or from 10^9 on the exponent format of Microsoft's `_gcvt` with its three-digit exponent.
One detail only showed up in a sweep over 3013 random float bit patterns: Microsoft's `_ecvt`
rounds an exact tie away from zero (38854.25 becomes `38854.3`), `printf` rounds it to even. We
now round ourselves. All 3013 values and 69 hand-picked ones match the original. `Print` and
`Write` of a float and `Read` of a float into a string use the same path; before, they still
wrote C++'s own format.

Thirteen tests had our old format in their expected output; each was checked against the
original and updated. The correct format makes one older deviation visible: `Abs(-3)` now
prints `3.0` because our `Abs` always returns a float (BUG-96, next). Suite 262/262.

---

## 2026-09-18 — `Read` into declared variables, and "Out of data" (BUG-85)

A plain `Read x` took its type from the tag written at the `Read`, and without one that was
an integer: after `Local x#` it read 1 instead of 1.5, after `Local s$` it stored a control
character instead of the string. It now uses the declared type, the same way an assignment
does, for locals, globals, parameters and lists alike. Reading past the last `Data` value now
stops the program with "Out of data", as Blitz3D does; before, it printed a warning and went
on with 0. Blitz3D shows the message in a dialog; here it goes to stderr, like every other
runtime error so far (the dialog question is BUG-125). 14 cases measured, all equal. Suite
261/261.

---

## 2026-09-18 — Float to int rounds everywhere (BUG-95, BUG-109)

Converting a float to an integer now rounds the way Blitz3D does, to the nearest integer and
to the even one at .5 (the x87 default): `Int(2.5)` is 2, `Int(3.5)` is 4, `x% = 1.9` is 2.
Until now only conditions and array indexes rounded; assignments, parameters, `Return`,
defaults, fields and `Int()` cut the fraction off. `For` converts its start, bound and step to
the counter's type (`For i = 1 To 1.9` runs twice, `Step 3.5` counts 0, 4, 8, `For i = "1" To 2`
now compiles), a float `Case` matches an integer `Select` after rounding, and assigning to an
array element converts to the element type. `Read` is the one exception and keeps truncating,
as in Blitz3D. A string `For` counter is now reported like Blitz3D does instead of failing in
the C++ compiler.

53 cases measured against the original, all equal. Three older tests had our truncation
written into their expected output; they now expect the original's values. Found on the way:
`For v[0] = …` is rejected (BUG-160) and a second `Local` of the same name is not reported
(BUG-161). Suite 260/260.

---

## 2026-09-18 — TCP, and what blox-n-balls needed besides (BUG-157, BUG-158, BUG-159, BUG-23)

The TCP commands are in: `OpenTCPStream`, `CreateTCPServer`, `AcceptTCPStream`, the close
commands, `TCPStreamIP/Port`, `TCPTimeouts`, `DottedIP`, `CountHostIPs` and `HostIP`. A TCP
stream is a stream like a file, so all the `Read…`/`Write…` commands, `ReadAvail` and `Eof` now
go through one path that serves files and sockets alike. Fourteen cases, measured against
Blitz3D with a server and a client in the same program, give the same results. UDP and
DirectPlay are still missing.

blox-n-balls needed three more things. Assigning to a field did not convert the value to the
field's type, so `list\player = Str(...)` into an integer field broke the C++ build (BUG-157).
A `Gosub` inside a `Case` returns by jumping back into that branch, and C++ does not allow a
jump over the initialisation of the `Select` temporary (BUG-158). `Select` now works as in
Blitz3D: it first finds the matching case in a block of its own, then runs that case's body.
Only an uninitialised `int` with the case number is left in scope, and a jump may cross it.
This also fixes the last open case of BUG-23, a `Goto` into a `Case` from outside.

In fullscreen the game's 800×600 image sat in the top left corner of the screen and clicks
landed in the wrong place (BUG-159). It is now scaled with borders, and the mouse is mapped
back to game coordinates. `MoveMouse` does the reverse mapping. `MouseXSpeed`/`MouseYSpeed` now
work as in `bbinput.cpp`: they return the distance from the last call or the last `MoveMouse`.
Before, the jump made by `MoveMouse` itself counted as movement, which works against the usual
mouse-look loop. After `Graphics` the mouse starts at 0,0, as in Blitz3D. Measured in a window
and in fullscreen against the original. Suite 258/258.

---

## 2026-09-17 — Mirrors, and what they uncovered (3D-16, BUG-155, BUG-156)

`CreateMirror` works. A mirror has no geometry at all: before the scene itself, Blitz3D draws it
once per visible mirror with the camera reflected in the mirror's XZ plane and the triangle winding
reversed. A half-transparent floor laid over the mirror is part of that reflected pass too, which
is why it blends twice. Twelve cases measured against Blitz3D, from `EntityClass` and the doubled
`TrisRendered` to a hidden mirror, two mirrors and a mirror parented to a pivot.

That last case exposed `EntityParent`: its third parameter defaults to 1 in Blitz3D, so a call
without it keeps the entity's world position, while we kept the local one and the entity jumped.
Decomposing the new local matrix also has to orthogonalise it the way `matrixQuat` does, or the
roll comes out 23.5 degrees off under an unevenly scaled parent (BUG-155). And the depth test:
Direct3D 7 compares with less-or-equal, OpenGL with less, so anything drawn twice at exactly the
same depth was dropped here (BUG-156). What blox-n-balls still misses: the TCP commands.
Suite 254/254.

---

## 2026-09-17 — Collisions and picking (3D-18, 3D-17, BUG-154)

The heart of a Blitz3D game runs now. `Collisions` rules with all three methods (sphere, mesh
triangles, box) and all responses, `EntityType`, `EntityRadius`, `EntityBox`, `EntityCollided`,
`CountCollisions`, `CollisionX/Y/Z`, `CollisionNX/NY/NZ`, `CollisionTime/Entity/Surface/Triangle`,
plus `LinePick`, `EntityPick`, `EntityPickMode`, the `Picked…` queries and `EntityVisible`. It is
a translation of `collision.cpp` and `World::collide`, down to the order of the steps, because
every one of those return values is observable. Two details only measurement could show: Blitz3D
works through the entities one after another, so a collision registered on both partners is wiped
again when the later one clears its own list, and the box method normalises the axes first, so a
scaled entity keeps its box. The triangle tree of the original is rebuilt as well - it decides
which of several equally close triangles wins, which is what `CollisionTriangle` reports.

`ResetEntity` turned out to reset position, rotation and scale here; in Blitz3D it only tells the
collision system to start from here (BUG-154). 55 cases measured against Blitz3D across three
tests. What blox-n-balls still misses: `CreateMirror` and the TCP commands. Suite 252/252.

---

## 2026-09-17 — Sprites (3D-16)

`CreateSprite`, `LoadSprite`, `RotateSprite`, `ScaleSprite`, `HandleSprite` and `SpriteViewMode`
work. A sprite is a 2x2 square rebuilt for every camera: view mode 1 takes the camera's rotation
(its own rotation and `ScaleEntity` do nothing), 2 its own, 3 stays upright with the camera's
forward axis, 4 follows only the camera's yaw. `LoadSprite` picks its blend from the texture
flags: masked draws solid, alpha blends, everything else adds. 25 cases measured in Blitz3D -
extents on screen, handle, rotation, all four view modes under a tilted, turned and rolled camera,
texture orientation, copies and a scaled parent - match pixel for pixel. The first of the commands
blox-n-balls needs beyond DebugLog. Suite 249/249. 45 open bugs.

---

## 2026-09-17 — Half a pixel (BUG-153 and BUG-152)

Two older faults that the sprite measurements uncovered. The alpha a texture gets when its image
has no alpha channel was the weighted luminance; Blitz3D takes the plain average `(R+G+B)/3`, and
the mask flag sets alpha 0 on black instead of leaving the channel alone (BUG-153, measured over
`TextureBuffer` for 16 colours and flags 1 to 7). And every edge that does not fall on a pixel
boundary sat half a pixel too far left and up, because Direct3D 7 puts the pixel centre on whole
coordinates and OpenGL on .5 (BUG-152); each camera's projection now carries that half pixel -
vertically a little less, since a tie on the pixel centre is decided by the fill rule, which runs
the other way round in GL. All 20 tests that read pixels stay green. Suite 249/249.

---

## 2026-09-17 — DebugLog

The first of the commands blox-n-balls still needs. In Blitz3D, `DebugLog` writes to the IDE
debugger's log; a standalone program shows nothing and keeps running. There is no debugger here,
so the line goes to stderr, next to runtime errors, and stdout stays exactly as in Blitz3D
(measured with text, an integer, a float expression and a variable). Suite 249/249.

---

## 2026-09-17 — Include paths are relative to the main file (BUG-151)

A Blitz3D game from 2003 with 26 source files stopped at its first nested include:
`includes\action.bb` includes `"includes\multiball.bb"`, and we looked for it in
`includes\includes\`. Blitz3D changes into the main file's directory before it parses, so every
include, from any file, is relative to that directory; an include relative to the including file
is rejected. A missing include was also only a warning here and compilation went on into follow-up
errors. Both now behave like Blitz3D, including the error position just past the closing quote.
What stops the game now are missing commands only: sprites, collisions, picking, `CreateMirror`,
TCP and `DebugLog`. Suite 245/245. 45 open bugs.

---

## 2026-09-17 — World rotation without the parents' scale (BUG-149)

Blitz3D keeps a world rotation apart from the world matrix: the product of the rotations from the
root down, with no scale in it. Five commands use it: the global `EntityPitch/Yaw/Roll`,
`RotateEntity` and `TurnEntity` with the global flag, `PointEntity` and `AlignToVector`. We read
angles and axes from the world matrix, whose columns are no longer perpendicular once a parent is
scaled unevenly, so four of them went wrong under such a parent. `PointEntity` was wrong under any
parent: it wrote the world angles as local angles. All five now go through one helper that builds
the world rotation from the chain of rotations, and one that sets it back into the parent's space.
Nine cases measured in Blitz3D under a rotated, unevenly scaled parent and grandparent match
exactly; seven of them failed before. Suite 243/243. 45 open bugs.

---

## 2026-09-17 — TurnEntity turns around the entity's own axes (BUG-148)

`TurnEntity` added its angles to the entity's pitch, yaw and roll. That is right for a single call
from rest and wrong for anything after it: a plane that rolls and then pitches never changed its
heading. Blitz3D multiplies rotations: locally the turn is applied after the current rotation,
globally before the world rotation and then brought back into the parent's space, without the
parents' scale. Ten cases measured in Blitz3D, from two turns to 600 steps of turning and moving,
match to three decimals. Jet Tails now flies the same path: after 60 frames the jet is at
(13,-15,9) here and (13,-16,9) in Blitz3D, the rest being `Int` truncating (BUG-95). Two findings
remain open: global angles under an unevenly scaled parent (BUG-149), and angles stored as Euler
angles instead of quaternions, which shows as -180 instead of 180 and in last digits (BUG-150).
Suite 242/242. 46 open bugs.

---

## 2026-09-17 — The same random numbers as Blitz3D (BUG-116)

`Rnd`, `Rand`, `SeedRnd` and `RndSeed` used the C library's `rand()`. They now follow Blitz3D's
own generator: Park-Miller with multiplier 48271, starting state `$1234`, 16 bits per draw. The
signatures are the original ones, `Rnd(from, to=0)` and `Rand(from, to=1)`, so `Rnd(4)` is
`4-4r` and `Rand(-10)` lies between -10 and 1. `SeedRnd 0` becomes 1, `RndSeed()` returns the
current state. The arithmetic has to be single precision: with double, `Rand(1,100000000)` was
off by up to 4. 18 cases measured in Blitz3D match exactly, including the 32-bit overflow of
`Rand(0,2147483647)` and the state after 100000 draws; the old milestone test now prints the same
lines as Blitz3D too. Jet Tails gets the same random numbers now but still flies differently:
`TurnEntity` adds Euler angles instead of turning around the entity's own axes (BUG-148).
Suite 241/241. 45 open bugs.

---

## 2026-09-17 — TranslateEntity ignores the entity's own rotation (BUG-145)

`TranslateEntity` without the global flag rotated the offset by the entity's own rotation, which is
what `MoveEntity` does. Blitz3D adds it unchanged in the parent's axes: a pivot turned 90 degrees
and translated by 0,0,1 ends at (0,0,1), not (-1,0,0); a child still follows its parent's rotation.
The local branch now just adds the offset. Seven cases measured in Blitz3D, including `MoveEntity`
as the counter-check, match exactly; three of them failed before. In Jet Tails the camera now
follows the jet instead of drifting away. The flight path still differs, because `Rnd` produces a
different sequence (BUG-116). Suite 240/240. 45 open bugs.

---

## 2026-09-17 — The demos again

The demos compared on 2026-09-16 were recorded again in both systems, in windowed copies with the
start menu of the `mak` demos advancing on its own. The mirror, the flag, the primitives, the three
camera views, the lights demo's white text, the LOD sphere and the closed matrix terrain now show
the same picture as Blitz3D, apart from shading, font metrics (BUG-139) and the window title. The
jet in Jet Tails is still missing: its camera follows with `TranslateEntity`, which moves along the
entity's own rotation here instead of the parent's axes, so the camera drifts away (BUG-145). The
same demo shows a different surface order in a `.x` model (BUG-146), and windows without
`AppTitle` are titled "BLTZNXT" instead of staying empty (BUG-147). 46 open bugs.

---

## 2026-09-17 — Translucency in the default blend mode (BUG-142)

`EntityAlpha 0.5` drew a solid cube. The renderer switched blending on but only set the blend
function when the brush's blend mode differed from the last one it had set — and it started from 0,
which is exactly the default brush's value. So the function was never set, or the previous object's
stayed: after a multiply-blended cube, `BrushAlpha`, vertex alpha with `EntityFX 32` and alpha
textures came out black. The blend function is now always set after blending is switched on, and
blend mode 0 mixes like 1. Ten cases measured in Blitz3D — alpha, add, multiply, brush alpha, vertex
alpha, an alpha texture — now match exactly; the old code missed four of them. Suite 239/239.
43 open bugs.

---

## 2026-09-17 — The first RenderWorld after loading an image (BUG-144)

A 3D program that loaded or created an image before its first `RenderWorld` read only white from
the screen afterwards. The cause was the order inside `RenderWorld`: it compiled its shaders on the
first call before making its own GL context current. Creating an SDL texture for an image makes the
2D renderer's context current, so the shaders ended up there. Shaders are now compiled after the
context switch. `tests/test_bug144_render_nach_2d.bb` loads, creates and draws an image before the
first render and checks `ReadPixel`, `LockBuffer` and `CopyRect` against Blitz3D. Suite 238/238.
44 open bugs.

---

## 2026-09-17 — CopyRect (BUG-118)

`CopyRect` accepted all arguments and did nothing. In Blitz3D it is a blit without mask between any
two buffers, defaulting to the current buffer for both — BlitzNext defaulted to the back buffer. It
now uses the canvas layer: image and texture buffers copy directly; the screen is read back once as
a source, and as a destination the changed rectangle is drawn back without blending. The source
image's handle offsets the copy and the destination's origin and viewport apply, as measured.
`tests/test_bug118_copyrect.bb` covers image, texture and back buffer as source and destination,
with clipping, origin and viewport — nine lines measured in Blitz3D, all failing on the old runtime.
Suite 237/237.

Measuring it turned up an older problem: once `CreateImage` has been called in 3D mode, reading the
screen returns white (BUG-144). Still 45 open bugs.

---

## 2026-09-17 — Drawing into textures (BUG-127)

`TextureBuffer` returned 0, so a texture drawn with `Rect`, `Text` or pixel commands stayed black —
the checkerboard objects in the `primitives` and `multicam` demos were invisible. Texture buffers now
build on the canvas layer from BUG-141: every frame of a texture has its own buffer, 2D commands,
`LockBuffer` and the pixel commands work on the texture's pixel copy, and the texture is uploaded
before the next `RenderWorld`. Measured in Blitz3D and matched: a fresh texture reads `$FF000000`,
changes show up in the next render even after the texture was already used, loaded textures can be
painted, and written alpha survives only in textures with an alpha channel — flag 4, or flags 1 and
2 together, as Blitz3D's `texture.cpp` implies. Two new tests have 23 lines measured in Blitz3D, all
of which fail on the old runtime. Suite 236/236.

Two new entries came out of it: translucent objects are drawn opaque because the default blend mode
never sets a blend function (BUG-142), and `CreateTexture` does not round to powers of two
(BUG-143). 45 open bugs.

---

## 2026-09-17 — Drawing into image buffers (BUG-141)

Starting on texture buffers (BUG-127) showed the deeper problem: every 2D command ignored
`SetBuffer` and drew to the screen, so nothing could be drawn into an image either. In Blitz3D each
buffer is a canvas with its own origin, viewport, handle and mask colour. BlitzNext now keeps that
per buffer: when an image buffer is active, `Cls`, `Plot`, `Line`, `Rect`, `Oval`, `Text`,
`DrawImage`, `DrawBlock`, `TileImage` and their variants draw into the image's pixel copy, using the
algorithms of Blitz3D's `gxcanvas.cpp` — line clipping and Bresenham, the oval's float formulas,
blitting with handle, mask and clipping, tiling. `SetBuffer` resets origin and viewport, `ReadPixel`
and `WritePixel` respect them, and the image is uploaded again only when it is next drawn to the
screen. The screen path stays on SDL for now; texture buffers and `CopyRect` build on this next.

Nine pixel tables and nine further values were measured in Blitz3D; all match, and the two new
tests differ from the old runtime in 147 of 210 lines. On the way, `DrawBlock` on the screen now
draws masked pixels like Blitz3D. Suite 234/234. Still 44 open bugs.

---

## 2026-09-17 — ClearWorld respects its flags, mode changes clear the world (BUG-120)

`ClearWorld` removed every entity whatever its three flags said, and left brushes and textures
alone. It now frees entities, brushes and textures only when asked; entities painted with a freed
brush or texture keep their look, as in Blitz3D, because they hold their own copies. Blitz3D also
closes the 3D scene on every `Graphics`, `Graphics3D` and `EndGraphics` with `ClearWorld 1,1,1` —
here old cameras and cubes kept rendering after a mode change. That now happens too, before the old
GL context goes away. `tests/test_bug120_clearworld.bb` has eight lines measured in Blitz3D; four
fail on the old runtime. Suite 232/232.

The source shows two more things a mode change does in Blitz3D — free all images and reset
`LoaderMatrix` — that BlitzNext does not do yet (BUG-140, not measured). Still 44 open bugs.

---

## 2026-09-17 — A graphics mode change resets the drawing state (BUG-131)

After `Graphics3D`, text kept the colour set before the mode change — visible in the Blitz3D demos,
whose start menu draws a blue URL just before switching modes. Blitz3D resets more than the colour:
`graphics()` sets the drawing colour to white, the clear colour to black, the default font and the
buffer of the new mode (back buffer for `Graphics3D`, front buffer for `Graphics`), which also
drops origin and viewport; `EndGraphics` does the same. All of that now happens here too, including
a previously selected image buffer, which used to stay active. `tests/test_bug131_graphics_reset.bb`
compares against Blitz3D after `Graphics3D`, `Graphics` and `EndGraphics`; four of its five lines
fail on the old runtime. Suite 231/231.

The measurement also showed that font heights differ — 8 instead of 13 for the default font, 45
instead of 40 for Arial at size 40 (BUG-139). Still 44 open bugs.

---

## 2026-09-17 — Several cameras render as in Blitz3D (BUG-129)

A second camera with its own viewport seemed not to render. In fact every camera cleared the
whole screen, because `glClear` ignores the viewport, and the cameras came in an arbitrary order,
so the last one wiped out the others. Each camera now clears only its viewport (scissor test),
and cameras render in Blitz3D's order: collected in scene-tree order, then taken from a priority
queue by `EntityOrder`, higher first — the reverse of what BlitzNext did. For equal orders the
sequence follows the heap mechanics of Blitz3D's STL rather than creation order (five equal
cameras render 0, 2, 4, 1, 3); it was measured pairwise for 2 to 10 cameras, twelve random order
assignments and cameras in a hierarchy, and is rebuilt exactly. `tests/test_bug129_kameras.bb`
has 36 lines measured in Blitz3D; 34 fail on the old runtime. Suite 230/230.

While building that test, a helper function ended up in it twice: Blitz3D rejects that with
`duplicate identifier`, BlitzNext did not notice (BUG-138). Still 44 open bugs.

---

## 2026-09-17 — RenderWorld without a camera draws nothing (BUG-137)

Without an active camera, `RenderWorld` cleared the back buffer using a made-up fallback state.
Blitz3D draws nothing at all in that case: whatever was there — a `Cls`, 2D drawing, the previous
frame — stays. The same holds for a camera switched off with `CameraProjMode 0`, which here kept
rendering in perspective. `RenderWorld` now returns when no camera is active, cameras in
projection mode 0 are skipped, and the fallback state behind `CameraClsColor`/`CameraClsMode` on
an invalid handle is gone. `tests/test_bug137_ohne_kamera.bb` has six lines measured in Blitz3D;
four fail on the old runtime. Suite 229/229. 44 open bugs.

---

## 2026-09-17 — Hiding a parent hides its children (BUG-136)

`HideEntity` only switched off the entity itself; its children kept rendering. In Blitz3D the
whole branch disappears, over any number of levels and for meshes, lights and cameras alike — a
light under a hidden pivot no longer lights, a camera under a hidden pivot no longer renders, and
`TrisRendered` does not count what is hidden that way. An entity attached later with
`EntityParent` to a hidden one disappears too. The child's own flag is untouched: showing the child
alone does not bring it back, and a child hidden by itself stays hidden when its parent is shown
again. Rendering now walks up the parent chain before drawing a mesh, using a light or rendering
from a camera. `tests/test_bug136_hide_kinder.bb` has 20 lines measured in Blitz3D; eight fail on
the old runtime. Suite 228/228. 45 open bugs.

---

## 2026-09-17 — Copies are always visible (BUG-128)

`CopyEntity` of a hidden entity returned a hidden copy, so the common pattern of loading a
template, hiding it and showing copies displayed nothing (the Jet Tails and lodBalls demos). In
Blitz3D 11.8 every copy is visible, and so is every child copied along with it, even one that was
hidden in the template — although the published source copies the visibility flag. The copy is
now always visible. `tests/test_bug128_copyentity_sichtbar.bb` covers eight cases measured in
Blitz3D; six of them fail on the old runtime. Suite 227/227.

Two more differences turned up while measuring: hiding a parent does not hide its children here
(BUG-136), and `RenderWorld` without a visible camera clears the back buffer, where Blitz3D leaves
it untouched (BUG-137). 46 open bugs.

---

## 2026-09-17 — Sphere, cylinder and cone as in Blitz3D (BUG-69, BUG-93, BUG-133)

`CreateSphere`, `CreateCylinder` and `CreateCone` now follow `MeshUtil::createSphere`,
`createCylinder` and `createCone` from the Blitz3D source line by line, with the same float
constants and the same rotation maths. The sphere has one pole vertex per segment and shares its
ring vertices (151 vertices and 224 triangles at the default 8 segments, was 576 and 288);
cylinder and cone keep the side and the caps in separate surfaces, and the cone's side normals
are horizontal, as in Blitz3D. `tests/test_bug69_primitive.bb` prints every vertex and triangle of
nine shapes; its 644 expected lines come from Blitz3D and match exactly.

Found along the way: the second parameter of `CreateCylinder` and `CreateCone` was called `open`
with default 0 here, but is `solid` with default 1 in Blitz3D, so `CreateCylinder(8,0)` had caps
here and none in Blitz3D. It is `solid` now. One lighting test (BUG-92) checks its outermost
silhouette point only for being lit: with the correct mesh it reads 153 instead of Direct3D 7's
137, which is shading, not geometry. Suite 226/226.

---

## 2026-09-17 — Triangles face the right way (BUG-126)

The renderer declared counter-clockwise triangles as front faces. Blitz3D shows the side the
cross product `(b-a)x(c-a)` points to in its left-handed coordinates, which on screen is
clockwise — and the view matrix only mirrors z, so clockwise stays clockwise. `RenderWorld` now
sets `glFrontFace(GL_CW)`. The compensations that had grown around the wrong setting are gone:
cube and sphere use Blitz3D's triangle order again (`TriangleVertex` on a cube returns 0,1,2
and 0,2,3, all twelve triangles as in Blitz3D), cylinder and cone are reversed, and
`UpdateNormals` computes `(b-a)x(c-a)`. The `.3ds` loader keeps its index swap.

Visible effects: meshes built with `AddTriangle` (flags, mirrors, terrain grids) appear, loaded
models no longer show their inside walls, and loaded models without normals are lit from the
correct side. `tests/test_bug126_umlauf.bb` checks both orders for `AddTriangle`, `.x` and
`.3ds`, all four primitives from outside and inside, and light from front and back; every value
was measured in Blitz3D, and 10 of its 20 lines fail on the old runtime. Suite 225/225.

---

## 2026-09-17 — Bug list checked against the engine draft, winding measured

The open bug list was compared with `ENGINE_DESIGN.md`, and the draft now follows from it: the
order in section 9 has ten steps. Winding (BUG-126) comes before the primitive meshes, 3D
behaviour tests before restructuring the renderer, and a handful of language bugs (float to int
conversion, float literals, local shadowing, implicit variable types, `Str(float)`) plus a typed
constant evaluator before vectors, because vectors build on exactly those paths. Sphere mapping
(BUG-130) moves into the material model step. `TextureBuffer`/`CopyRect` are split: behaviour
now, storage (CPU copy or GPU target) together with render targets. A twelfth open question
covers vectors and Blitz's implicit rules (`Const`, `Data`, default parameters, `Handle`).

Measuring the winding in both systems showed BUG-126 is wider than recorded. A single triangle
as `.x`, `.3ds` and `AddTriangle`, each in both orders: Blitz3D shows the side the cross product
`(b-a)x(c-a)` points to, BlitzNext the other — for every mesh, loaded ones included. Geometry
and the `.3ds` index swap agree; only the renderer culls the wrong side. Closed loaded models
look right from outside because the inside of the far wall is drawn; with the camera inside a
crate, Blitz3D shows nothing and BlitzNext the inner walls. The primitives and the cross
product in `UpdateNormals` are reversed to compensate, so loaded models without normals get
inward normals. Two side findings became BUG-134 (`UpdateNormals` in Blitz3D merges vertices at
the same position) and BUG-135 (`.3ds` vertex numbering and unnormalised normals). 49 open bugs.

---

## 2026-09-16 — Engine design draft

`ENGINE_DESIGN.md` collects where the 3D engine is heading, as a draft with every point marked
as decided, proposed or open. Nothing in it is implemented.

Decided so far: vectors become part of the language as `Vec2`, `Vec3` and `Vec4`, always float
(`Vec3#` is allowed, `Vec3%` and `Vec3$` are errors), values rather than objects, and reserved
names — a program that defines them itself has to rename. All surfaces go through one modern
material model to which the old parameters are mapped; there is no separate legacy shader path.
Shadows are on by default. Shaders will eventually be written in a Blitz dialect in their own
files and translated to GLSL.

Proposed: render passes as explicit commands in the program's own main loop
(`CreateRenderTarget`, `CameraRenderTarget`, `LoadShader`, `RenderPass`), shadows switchable
globally, per light and per entity, vector commands prefixed `Vec` because `Dot`, `Cross` and
`Length` appear in Blitz3D samples, and Forward+ before a deferred renderer. Eleven questions are
listed as open.

A local, unpublished plan from 2026-09-15 with three render modes (Legacy, Enhanced, Modern) is
superseded and deleted; the draft lists what was taken over from it.

---

## 2026-09-16 — The 3D bugs sorted by the new rendering guideline

Every 3D entry in the bug list was checked against the rule "observable values exact, shading
free". Where it was unclear which side an entry falls on, it was measured.

- **Cone and cylinder are geometry bugs.** BUG-93 described the cone as "lit differently".
  Reading the vertex table in both systems shows why: Blitz3D builds `CreateCone(8)` as two
  surfaces (side 17 vertices / 8 triangles with horizontal side normals, base 8 / 6), BlitzNext
  as one surface with 41 vertices, 16 triangles and normals tilted by 45 degrees. The cylinder,
  whose lighting had looked close, differs the same way: two surfaces (18/16 and 16/12) against
  one with 66 vertices and 32 triangles. BUG-93 is rewritten as a geometry bug, the cylinder is
  BUG-133.
- **BUG-66 and BUG-91** stay fixed, but their per-vertex Direct3D 7 lighting is now a
  transitional state rather than a target.
- **BUG-92** stays valid: which side of the sphere is visible is geometry.
- **BUG-130** (sphere mapping) stays a bug because flag 64 does nothing at all, but the look no
  longer has to match Direct3D 7.
- **BUG-132** (`fakelight`) is closed without a bug: every observable part measured equal, the
  rest is shading.
- **WEAK-25** is new: `test_bug66_shininess`, `test_bug91_punktlicht` and
  `test_bug92_kugel_licht` pin Direct3D 7 brightness values and have to be rewritten together
  with the modern lighting. `test_bug63_backbuffer` is a borderline case that depends only on
  the ambient light.

`KNOWN_ISSUES.md` now describes the cone and cylinder as mesh differences and no longer lists
the `fakelight` lighting as a bug.

---

## 2026-09-16 — Rendering follows meaning, not Direct3D 7

The rendering guideline from 2026-09-15 planned two lighting modes: a compatible one that
reproduces Direct3D 7 and a modern one. That plan is dropped. BlitzNext implements what the
3D commands mean and computes lighting in a modern way; there will be no compatible mode.

What a program can observe or rely on still has to match Blitz3D exactly: geometry, winding,
transforms, picks, collisions, return values, which entities and cameras are drawn, and what a
texture contains. Shading does not: lighting model, attenuation, specular and per-vertex versus
per-pixel are free. Old scenes may look darker or brighter than they did; that is accepted and
can be corrected in the program. Colours read back with `ReadPixel` after `RenderWorld` are not
guaranteed.

So that "meaning" is not a matter of taste, the guideline in `ROADMAP3D.md` now has a table of
parameter roles taken from the Blitz3D command help (`EntityColor` is the diffuse colour,
`LightRange` a cutoff that the help itself calls "very approximate", negative light colours
darken, the `EntityFX`, `EntityBlend`, `TextureBlend` and texture flag values). The per-vertex
lighting built for BUG-66 and BUG-91 is a transitional state. README and `KNOWN_ISSUES.md`
("Intentional differences") say this openly.

---

## 2026-09-16 — Seven more known issues from the Blitz3D demos

Running 17 of the 3D demos that ship with Blitz3D side by side in both systems showed that
only five look the same. The differences were narrowed down with small probe programs that
read pixels after `RenderWorld` in both systems, and filed as BUG-126 to BUG-132: meshes
built with `AddTriangle` have their winding reversed, drawing into `TextureBuffer` has no
effect, a copy of a hidden entity stays hidden, a second camera with a viewport does not
render, sphere mapping (texture flag 64) is ignored, `Graphics3D` keeps the previous drawing
colour, and the lighting in the `fakelight` demo differs for a reason not yet found.

`KNOWN_ISSUES.md` lists them in the 3D section, with the two workarounds that were checked
in both systems (`ShowEntity` on copies, `Color 255,255,255` after `Graphics3D`). The README
now reports 47 open bugs.

---

## v0.5.0 - "Measured against the original" (2026-09-16)

The first version since v0.4.3 (2026-09-03). The theme of these two weeks: behaviour is
no longer guessed but taken from the Blitz3D source and measured against a running
Blitz3D 11.8. The entries below this one describe each step; this is the overview.

**Compiler**
- A semantic pass between parser and emitter: types, arity, fields, labels, with
  "did you mean …?" for commands, types and fields (WEAK-14).
- Diagnostics point to the right file and line after `Include` (WEAK-13, BUG-14).
- The built-in command table is generated from the runtime headers (WEAK-17).
- Grammar and precedence follow the reference: case-insensitive identifiers, `Not`,
  `Shl`/`Shr`/`Sar`, `^` with a sign, reserved words (`Pi`, `Abs`, `Int`, …), `For Each`,
  `Before`/`After`, the `Type` body, block closers, jump targets and duplicate labels.
- New language features: parameter defaults (BUG-49), fixed arrays and object fields in
  types (BUG-59, BUG-60), `Dim` arrays of objects (BUG-52), `Read` with a list of targets
  (BUG-85), `Null` as a type of its own (BUG-45).
- Conversions at assignments, in conditions and array indexes, and in `And`/`Or`/shifts
  follow Blitz3D (BUG-53, BUG-54, BUG-61, BUG-79).

**Runtime**
- 3D: graphics mode enumeration (3D-00), entity appearance (3D-10), textures (3D-11),
  lights (3D-12), `.3ds` and `.x` loading (3D-13), brushes and the surface API (3D-15),
  `CopyEntity`, `TFormPoint`/`TFormVector`/`TFormNormal`.
- Measured against the original: primitive vertex tables and winding, rotations and
  angle getters, `AlignToVector`, `PointEntity`, per-vertex point/spot lighting and
  specular, one shared back buffer for 2D and 3D.
- Runtime library: `Int()`/`Float()` of strings, `WriteLine` with CRLF, RLE-compressed
  BMP, and the string functions `Hex`, `Bin`, `Asc`, `RSet`, `Mid`, `Trim`.

**Documentation**
- `KNOWN_ISSUES.md` lists the 40 known deviations from Blitz3D, each reproduced against
  the original, with workarounds.
- README and roadmap report measured numbers: 380 of Blitz3D's 540 commands plus 27
  BlitzNext additions.

Test suite: 224 tests.

---

## 2026-09-16 — Known issues are public, README and roadmap brought up to date

The bug list is an internal working document and stays out of the repository, which
left users with no way to see what does not work yet. `KNOWN_ISSUES.md` fills that gap:
all 40 open bugs plus the user-visible weaknesses, grouped by area, each reproduced
against Blitz3D 11.8, with a workaround where one exists. A section of its own lists the
deviations that compile and run silently, since those are the hardest to find.

Two workarounds were measured before they went in. On the way it turned out that BUG-95
is wider than its title: every float-to-integer conversion truncates, including a plain
`x% = 1.9`; only conditions and array indexes round.

The README described an older state. It now links the known issues from the status line,
reports 380 of Blitz3D's 540 commands (compared with `blitzcc -k` of both compilers,
keywords excluded) plus 27 BlitzNext additions, lists the 3D commands that exist
(textures, brushes, meshes, surfaces, lights, graphics modes), marks `Include`,
`Goto/Gosub`, `Data/Read` and `Const` as working with deviations, states that the Linux
build does not work (WEAK-24), and fixes the clone URL. The reference to `Buglist.md`,
which is not in the repository, is gone.

The "Command Parity Progress" checklist in `roadmap.md` was generated against
`blitzcc -k` instead of being edited by hand: 27 lines were complete but still unticked,
51 of 62 are ticked now. Partially available lines name what is missing, and `Handle` /
`Object` have a line of their own.

---

## 2026-09-16 — Trim removes every invisible character at the ends (BUG-115)

`Trim` only removed space, tab, CR and LF. `bbTrim` tests both ends with
`isgraph()`, so in the ASCII range everything from 0 to 32 and 127 goes,
including NUL, vertical tab and form feed. `Len(Trim(Chr(11)+"x"+Chr(12)))`
was 3 here and is 1 in the original. All 128 ASCII values were measured
against the original.

Bytes from 128 up cannot be matched: the original passes a signed `char` to
`isgraph()` and reads outside its table. Three runs of the same program kept
34, 36 and 37 of those 128 bytes, and most umlauts were cut off. Decided
together: they count as visible and stay, so `Trim("Ärger")` keeps its `Ä`.

`bb_Trim` now walks both ends with that rule. New test: `test_bug115_trim`.
The ASCII part is identical in the original and here; the line for bytes from
128 up records the decision and is not compared with the original.
Runtime only, the emitted C++ is unchanged. Full suite: 224 passed, 0 failed.

---

## 2026-09-16 — Mid with a negative length returns the rest (BUG-114)

`Mid("abcd",2,-1)` returned an empty string, the original returns `bcd`.
`bbMid` takes `substr(o)` for every negative length, and -1 is the default
for the omitted argument, so an explicit -1 has to behave like the
two-argument form. Only a length of 0 gives an empty string. Measured against
the original: lengths -1, -5, 0, 10 and 1, and start positions beyond the end.

`bb_Mid` with three arguments now hands a negative length to the
two-argument form.

New test: `test_bug114_mid_negativ`, identical in the original and here.
`test_m19_strings` also matches the original. Runtime only, the emitted C++ is
unchanged. Full suite: 223 passed, 0 failed.

---

## 2026-09-16 — RSet keeps the end of a string that is too long (BUG-113)

`RSet("abcdef",3)` returned `abc`, the original returns `def`. `bbRSet` cuts
with `substr(size-n)`, so a right-aligned column keeps its last characters;
only `LSet` keeps the start. Padding was already right. Measured against the
original: `RSet` of `"abcdef",3` / `"abc",3` / `"ab",5` / `"abc",0` / `"",2`
and the same forms for `LSet`.

New test: `test_bug113_rset`, identical in the original and here.
`test_m20_strings2` had pinned `RSet("Truncated",5)` as `Trunc`; it is now
`cated`, and the whole test matches the original. Runtime only, the emitted
C++ is unchanged. Full suite: 222 passed, 0 failed.

---

## 2026-09-16 — Asc of an empty string is -1 (BUG-112)

`Asc("")` returned 0, the original returns -1 (`bbAsc`:
`s->size() ? (*s)[0] & 255 : -1`). With 0, an empty string could not be told
apart from `Chr(0)`. Measured against the original: `""` -1, `Chr(0)` 0,
`Chr(200)` 200, `Chr(256)` 0, `Chr(-1)` 255.

New test: `test_bug112_asc_leer`, identical in the original and here, including
a loop that reads a string character by character until `Asc` returns -1.
Runtime only, the emitted C++ is unchanged. Full suite: 221 passed, 0 failed.

---

## 2026-09-16 — Hex and Bin keep their leading zeros (BUG-111)

`Hex(1)` returned `1`, the original returns `00000001`. `bbHex` and `bbBin`
fill a fixed buffer digit by digit, so the result is always 8 hex or 32
binary digits, uppercase, no prefix. Measured against the original for 0, 1,
255, -1, -255, $80000000 and 2147483647.

`bb_Hex` now formats with `%08X` and `bb_Bin` fills 32 digits from the right.

New test: `test_bug111_hex_bin`, identical in the original and here.
`test_m20_strings2` had pinned the old output (`FF`, `101`, `0xFF`); its
expectation is updated and now checked against the original, except for the
RSet line, which is BUG-113. Runtime only, the emitted C++ is unchanged.
Full suite: 220 passed, 0 failed.

---

## 2026-09-15 — The sphere faces the right way (BUG-92)

A sphere under a point light stayed black. Measured against the original, it
also stayed black under a directional light, while a cube in the same place
was lit correctly in both. So the cause was the sphere, not the light.

Its normals point outwards, but its triangles used the original's winding.
Because our renderer mirrors z in the view matrix, that winding is the back
face here, which `bb_gen_cube_` already documents and compensates for. The
sphere never got that treatment: its front faces were culled, and what showed
was the inside of the far half, with normals facing away from the light.

`bb_gen_sphere_` now winds each quad as (0,2,1)/(0,3,2), like the cube. The
sphere is lit and within a few steps of the original. It cannot match exactly
yet: its tessellation differs (BUG-69), and directional light is computed per
pixel. The largest remaining difference, 10, is on the silhouette edge.

New test: `test_bug92_kugel_licht`. It checks three setups point by point
against the original's values with a tolerance of ±12, and states why. Against
the previous runtime all three fail, reading 0 everywhere. Full suite: 219
passed, 0 failed.

Checking the other primitives on the way: the cylinder is within a few steps
of the original. The cone differs clearly in brightness and on one side;
filed as BUG-93.

---

## 2026-09-15 — Point and spot lights: range/distance, per vertex (BUG-91)

Point lights attenuated with `1 - distance/range`, per pixel. `gxLight` sets
`dvAttenuation1 = 1/range` and leaves the other factors at 0, so Direct3D
divides by `distance/range`. That gives `range/distance`, unbounded, per
vertex.

Three setups, measured against the original at `F:\dev\Blitz3D`:

- a cube under a point light at LightRange 20 / 5 / 3.5 / 2:
  original 255 / 136 / 95 / 55, we gave 85 / 40 / 14 / 0;
- a large flat wall with a point light close in front: the original is 16
  across the whole face, since only the four distant corners are lit and
  interpolated;
- the same wall under a spot light: 0 everywhere, since all corners lie
  outside the cone.

Fixing only the formula per pixel gave 167 / 117 / 67 on the cube and bright
hotspots (255) on the wall where the original has none. Computing point and
spot diffuse per vertex matched all 18 values exactly. The decision taken is
point and spot per vertex, directional per pixel as before.

The LIT vertex shader now computes point/spot diffuse (with the spot cone) and
all specular in one loop. The fragment shader handles directional lights only
and adds the interpolated point/spot term.

New test: `test_bug91_punktlicht`, expected output taken from the original.
`test_bug66_shininess` is unchanged. Full suite: 218 passed, 0 failed.

---

## 2026-09-15 — EntityShininess: specular per vertex (BUG-66)

The entry had the original's numbers for a grey cube under a front light:
64 / 93 / 91 / 75 with rising shininess, where we gave 128 at the centre. It
also asked for a decision first: move all lighting to per-vertex, or keep the
difference.

The reference sets the material in `gxruntime/gxscene.cpp`: specular strength
`min(s,1)`, power `s*128`. `gxLight` keeps its specular colour at white;
`LightColor` only writes the diffuse colour. The fixed D3D7 pipeline lights per
vertex and adds specular after the texture stages.

Measured against the original at `F:\dev\Blitz3D`, the face is equally bright
at its centre and halfway to the edge — Gouraud. At a cube corner `N·H` is
0.9758, and `64 + 255·min(s,1)·0.9758^min(s·128,128)` gives exactly
64 / 93 / 91 / 75 / 75 for s = 0 / 0.25 / 0.5 / 1 / 2. That s=2 looks like s=1
shows the exponent is capped at 128.

My first recommendation, fixing the formula but staying per-pixel, was wrong:
per pixel the face centre hits the exact mirror direction, which would have
given 128 / 191 / 255. The decision taken is specular per vertex, diffuse per
pixel as before.

The LIT vertex shader now computes specular with the original's material and
passes it on interpolated and flat (for EntityFX 4). The fragment shader adds
it after colour and texture.

New test: `test_bug66_shininess` — five shininess levels, a red light on a grey
cube (155,27,27: green and blue are the white specular), and flat shading. Its
expected output is the original's, character for character. Full suite: 217
passed, 0 failed.

Found on the way and filed: point lights attenuate with `1 - d/r` where the
original uses `r/d` (BUG-91), and a sphere under a point light stays black
(BUG-92).

---

## 2026-09-15 — 2D and 3D share one back buffer (BUG-63)

`LockBuffer BackBuffer()` after `RenderWorld` read 0 everywhere. Measured
against the original, now available again at `F:\dev\Blitz3D`, with the same
program writing its readings to a file. The original returned the clear
colour, the cube (127,0,0) and the 2D drawing on top, locked or not. We
returned only the 2D points when locked, 0 when not, and `GetColor` returned
black.

The cause turned out larger than the entry assumed. `Graphics3D` created the 2D
renderer with SDL's default backend, which on Windows is direct3d11: a separate
buffer next to the GL back buffer. A diagnostic build with `glReadPixels` found
the original's exact 3D values in the GL buffer and none of the 2D drawing.
Since `Flip` presents only the GL buffer, **no 2D drawing was ever on screen in
a `Graphics3D` program**. Window-only screenshots confirm it: the original
shows the rectangle and text over the cube, we showed only the cube.

`bb_Graphics3D` now asks for the OpenGL renderer backend. It has its own
context but draws into the same back buffer as `RenderWorld`, so both land in
command order and `SDL_RenderReadPixels` sees both. While it is created, the GL
attributes are set to 2.1; without that, SDL recreates the window (measured: a
different HWND). Because the 2D renderer leaves its own context current,
`bb_gl_use_()` restores ours before every GL call outside `RenderWorld`: mesh
and texture release, `Wireframe`, shader cleanup, and the swap in `Flip`.

Found on the way: `ReadPixel` and `WritePixel` did nothing without
`LockBuffer`. In the original they lock by themselves, and now they do here
too; on the screen as a single pixel.

All nine readings of the measurement program now match the original.

New test: `test_bug63_backbuffer`. It runs three frames mixing 2D drawing,
texture loading, `FreeEntity`/`FreeTexture` after 2D, and `Wireframe`. Its
expected output is the original's, character for character. Built against the
previous runtime it reads black for the cube and the background. Full suite:
216 passed, 0 failed.

---

## 2026-09-15 — compare_commands.py compares parameter types (BUG-62)

The tool compared arity, return type and the order of parameter names, but
stripped the type suffixes before comparing. Since BUG-53 an argument converts
silently to the type our table lists, so a `%` where the original has `#` drops
the fraction without any message.

First, the original is back: the Blitz3D installation moved to
`F:\dev\Blitz3D`. `blitzcc +k` runs there and lists 536 commands.

The tool now reads the types on both sides (the name suffix in the original,
int when there is none; the type character in `commands.h`, "any" when there is
none) and compares them position by position. Two new categories: type
mismatch, and parameters we leave untyped. The first run found exactly the
three cases the manual check had found, plus `Print` and `Write` as untyped.

Those three are fixed in the runtime: `CameraClsColor` takes floats, as
`bbCameraClsColor` does (`r*ctof`), and `ChannelPitch` and `SoundPitch` take an
int pitch. After regenerating `commands.h` the tool reports no type
mismatches.

Regenerating turned up drift: `commands.h` was not the generated state.
`CaseEq` and `ToNum` from BUG-79 would have become Blitz commands, and `ToInt`
and `ToFloat` already were. All four are emitter helpers and are now in
`gen_commands.py`'s skip list. `gen_commands.py --check` is clean.

No runtime test: the 3D image cannot be read back by pixel (BUG-63), and pitch
is not observable without an audio device.

Validation: emitted C++ compared over 223 programs against `1de424e`: 123
byte-identical, 96 rejected by both with the same diagnostic, 4 changed. Those
four call the three commands, and only the argument conversion at the call
differs. Full suite: 215 passed, 0 failed.

---

## 2026-09-15 — EndType, EndFunction and EndSelect are not keywords (BUG-89)

The keyword table in `compiler/toker.cpp` has `EndIf` and `ElseIf` as one
word, but `End Type`, `End Function` and `End Select` only as two. In one word
those three are identifiers there. Our `isKeyword()` listed them as aliases, so
`Type V … EndType` was accepted.

The aliases are removed. The two-word forms are still merged into `ENDTYPE`
and friends by the step from BUG-57. The one-word forms now behave as in the
reference: inside a Type body they are `Expected 'Field' or 'End Type'`, and
elsewhere they parse as a call, so the open block is reported at end of file.
To keep that from being a riddle, the message names the one-word form and
where it stood: `('EndFunction' at file:4:1 is not a keyword in Blitz3D -
write 'End Function')`. The names are now usable as variables, as in the
original.

New tests: `test_bug89_end_zweiwort` (rejected by the previous compiler),
`neg_bug89_endtype_ein_wort`, `neg_bug89_endfunction_ein_wort`,
`neg_bug89_endselect_ein_wort`.

Validation: emitted C++ compared over 223 programs against `5323f62`: 126
byte-identical, 93 rejected by both with the same diagnostic, the four
differences being the new tests. Full suite: 215 passed, 0 failed.

---

## 2026-09-15 — The For Each counter is an ordinary variable (BUG-90)

`For q.P = Each P … Next` followed by `q = First P` passed the frontend and
failed in g++. The emitter declared the counter inside the C++ loop
(`auto *var_q`), and `hoistLocals()` left it out on purpose. The later untagged
assignment then hoisted an int of the same name.

The same shape hid a worse problem. A counter that already existed —
`Local q.P`, a `Global`, a parameter — was shadowed by the inner declaration
and never written. After `Exit` it kept its old value; the test program, minus
its first case, crashes with the previous compiler.

In the reference, `ForEachNode::translate` passes the variable itself to
`__bbObjEachFirst`/`__bbObjEachNext`, which store into it: `Null` after the
last pass, the current object after `Exit`.

`collectLocals()` now reports the counter like any other variable of the body,
with the loop's object type, and `visit(ForEachStmt*)` writes to it:
`for (var_q = head; var_q; var_q = next)`, with the successor taken before the
body so `Delete q` stays safe. `foreachVars_`, `writtenNames_` and
`addWritten()` existed only for the old exception and are gone.

New test: `test_bug90_each_zaehler`.

Validation: emitted C++ compared over 218 programs against `9aaf7e5`: 115
byte-identical, 93 rejected by both with the same diagnostic, 10 changed. All
ten use `For Each`, and in all ten only the loop head and the counter's
declaration change. `examples/asteroids` built in full. Full suite: 211
passed, 0 failed.

---

## 2026-09-15 — Stray and missing block closers (BUG-58)

The entry listed four accepted programs: `EndIf` without `If`, `Wend` without
`While`, an extra `EndIf`, and a function without `End Function`. Measuring
around them found much more:

- every stray closer was dropped silently: `EndIf`, `Wend`, `Next`, `Else`,
  `Forever`, `Default`, `End Select`, `End Function`, `End Type`, including
  inside a different block;
- without `End Function` the rest of the file became part of the function;
- inside `Select`, a statement before the first `Case` vanished, and a `Case`
  after `Default` was accepted;
- any other keyword at the start of a statement (`Then`, `Field`, `Pi`) was
  skipped;
- **silent wrong result:** the function body and the Case block counted a bare
  `End` as their closer. `End` inside a function ended the *function*, and the
  rest of its body ran as main program. Behind that, the emitter wrote `End` as
  `bbEnd(); return 0;`, which inside a function only returns.

In `compiler/parser.cpp`, `parseStmtSeq()` stops at any token that cannot
start a statement, and the caller demands its closer through `exp()`. `exp()`
checks the token it finds first, so a stray `Next` is `'Next' without 'For'`
wherever it stands.

The parser now does the same. `strayCloser()` reports the twelve closers in
the reference's wording and consumes them, from `parseBlock()`, the top level
and `parseStatement()`. `Function` inside a block is reported once, with a hint
when `End Function` is missing. The function body ends only at
`End Function`. `parseSelect()` follows the reference's structure, and `End`
is always the program end. The emitter writes `End` inside a function as
`bbEnd(); std::exit(0);`.

New tests: `test_bug58_bloecke` (built with the previous compiler it prints
both `FEHLER` lines) and eight `neg_bug58_*`. `neg_bug35_assign_pi` now reports
one error at `Pi` instead of two follow-ups.

Validation: emitted C++ compared over 218 programs against `7eea23b`: 124
byte-identical, 84 rejected by both with the same diagnostic, the ten
differences being the nine new tests and the changed `neg_bug35_assign_pi`
message. No existing valid program changes. Full suite: 210 passed, 0 failed.

---

## 2026-09-15 — And, Or and the shifts convert to int (BUG-54)

`If q$ <> "yes" Or "no" Then` was rejected with
`Operator cannot be applied to strings`. The original accepts it, and the
question was what it computes. There is no boolean context: in
`compiler/exprnode.cpp`, `And`, `Or`, `Xor`, `Shl`, `Shr` and `Sar` are
`BinExprNode`, whose `semant` casts both sides to int. A string goes through
`__bbStrToInt` (`atoi` when constant), a float is rounded. The line above is
`(q$ <> "yes") Or 0`.

Our `binary()` applied the string rule of `ArithExprNode` before it reached
these six operators. The emitter never converted their operands either, so
`x# And 3` passed the frontend and failed in g++ with
`invalid operands of types 'float' and 'int'`.

`binary()` now handles the six before arithmetic: an object or Blitz array is
an illegal conversion, anything else gives int. The emitter wraps both
operands in `bb_IntegerContext()`, the same conversion conditions and indices
use since BUG-61. `bb_IntegerContext(int)` is now `constexpr`, because the
helper also appears in `Const A = 1 Or 2` and in array sizes.

New tests: `test_bug54_bitops_wandeln` (rejected three times by the previous
compiler), `neg_bug54_objekt`.

Validation: emitted C++ compared over 207 programs against `9d57710`: 97
byte-identical, 84 rejected by both with the same diagnostic, 26 changed. All
26 differ only by the wrapping: with `bb_IntegerContext`, parentheses and
whitespace removed, old and new are identical. `examples/asteroids` built in
full. Full suite: 201 passed, 0 failed.

---

## 2026-09-15 — Dim arrays of objects (BUG-52)

`Dim feld.Punkt(3)` failed at `Expected '('`. In `compiler/parser.cpp`,
`parseArrayDecl()` reads the tag with `parseTypeTag()`, which includes
`.Ident`, and `parseVar()` reads the `\`/`[` chain after an array element just
as after a variable.

Reading the tag was one line. Three more things were missing behind it:

- `feld(i)\x = 1` and `Read feld(i)\x` failed to parse. The statement parser
  only knew the chain after a plain name, and had it copied twice. It is now
  `parseChainAssign()`, shared by assignment and `Read`, and both Dim-element
  branches call it when `\` or `[` follows the `)`.
- `Delete feld(0)` compiled to `feld.at(0) = nullptr`: the emitter could not
  tell the element's type, so the object stayed in its list and `For Each`
  still counted it — a silent wrong result. The emitter now tracks
  `dimObjectTypes_`, and `getExprTypeName()` answers for `ArrayAccess`, which
  fixes `Delete`, `Insert`, `After` and `Before` on elements.
- `Dim feld.Nirgends(3)` reported no unknown type. `semant.h` now checks it.

Assignment, comparison and field checks on elements already went through
`arrays_` and needed no change.

Found on the way: a `For Each` variable reassigned after its loop is emitted
as an int, and g++ fails. Filed as BUG-90.

New tests: `test_bug52_dim_objekte` (rejected by the previous compiler at line
13), `neg_bug52_falscher_typ`, `neg_bug52_typ_fehlt`.

Validation: emitted C++ compared over 207 programs against `aa314ec`: 122
byte-identical, 82 rejected by both with the same diagnostic, the three
differences being the new tests. Full suite: 199 passed, 0 failed.

---

## 2026-09-15 — Null has its own type (BUG-45)

`parsePrimary()` turned `Null` into the integer literal 0, so the semantic pass
never saw an object: `Select Null` went through, and so did everything else
where an integer stands in for an object. In the reference,
`Type::null_type` is a `StructType("Null")` (`compiler/type.cpp`): it converts
to every object type and every object type converts to it, but it never meets
int, float or string. `NullNode` is not a `ConstNode`.

`LiteralExpr` now carries `isNull`. The emitter still writes 0; the semantic
pass gives it the type `NUL`. `castable()` models `canCastTo` and replaces the
rule in `checkAssign()` that let any integer onto an object. That rule existed
only because Null arrived as 0. The consumers follow `exprnode.cpp` and
`stmtnode.cpp`:

- assignment, parameters and `Return`: Null onto an object only; `p.T = 0`
  and `x% = Null` are rejected, and a new untagged variable assigned Null is an
  int, as in the reference;
- comparison: with an object on either side only `=` and `<>`, and both sides
  must convert to the non-Null side's type (`If p = 0`, `p.A = q.B`);
- `Not x` is `x = 0` in the reference, so `If Not p` is an error; arithmetic and
  unary operators reject objects and Null; `If Null` is an illegal conversion;
- `Select Null`, `After Null` and `Before Null` get the reference's messages;
  `Function F(p.T = Null)` is not a constant default (measured earlier).

Two findings on the way: `Delete 5` was not reported (`Can't delete
non-Newtype`), and `Delete Null`, which is valid, emitted `0 = nullptr;` and
failed in g++. It now emits nothing. `Insert` checks for two objects of the
same type. The `Not` node carries its column.

Left open: `Print Null` passes, because `Print` has an untyped parameter in
`commands.h`. A new untagged variable assigned an object still becomes an
object, where the reference makes it an int.

New tests: `test_bug45_null` (the previous compiler fails it in g++) and nine
`neg_bug45_*`.

Validation: emitted C++ compared over 204 programs against `9295df0`: 121
byte-identical, 73 rejected by both with the same diagnostic, the ten
differences being the new tests. No existing program is affected by the
stricter rules. Full suite: 196 passed, 0 failed.

---

## 2026-09-15 — The Type body reads like the reference (BUG-43)

`Type Vec2 : Field dx#, dy# : End Type` is rejected at the first `:`, matching
the original's `14:11`. In `compiler/parser.cpp`, `parseStructDecl()` skips
only line ends between the name, the `Field` lines and `End Type`; anything
else is `Expecting 'Field' or 'End Type'`.

The colon was the smallest part. `parseTypeDecl()` skipped `:` along with line
ends and **skipped any unknown token in the body**. Accepted before the fix:
`Local y` or `Print` inside a Type (silently dropped), a Type with no
`End Type` at end of file, a bare `End` closing the Type, `End  Type` with two
spaces, and `Type` inside a function or an `If`.

The parser now follows the reference's structure and reports one error, then
reads on to that Type's `End Type`. At end of file the message adds
`- 'Type X' is not closed`. `semant.h` reports `Type` outside the top level of
the main program in the style of `Global` and `Const` (BUG-17); `TypeDecl` now
carries its column. `Field x = 5` is still accepted, as in the reference, and
the initializer is read and discarded.

Found on the way: the reference has `EndIf` as one word, but not `EndType`,
`EndFunction` or `EndSelect`. Our lexer accepts all three. Filed as BUG-89.

`test_type` and `test_type_instances` now write the Type over several lines;
emitted C++ unchanged. New tests: `test_bug43_type_body` and six
`neg_bug43_*`.

Validation: emitted C++ compared over 194 programs against `84caf47`: 121
byte-identical, 67 rejected by both with the same diagnostic, the six
differences being the new negative tests. Full suite: 186 passed, 0 failed.

---

## 2026-09-15 — Jump targets without a dot, and duplicate labels (BUG-42)

`Goto .done` and `Gosub .done` are no longer accepted. In the original
`compiler/parser.cpp`, `case GOTO` and `case GOSUB` read the target with
`parseIdent()`; only the label definition `.done` has a dot. The error is
reported at the dot, matching the original's `10:6` for the old
`test_goto.bb`, and the name behind it is consumed so no `Undefined label`
follows.

Three findings in the same place:

- A `.` without a name was silently skipped. It now reports
  `Expected label name after '.'`.
- `case RESTORE` only takes an `IDENT`. Anything else is a `Restore` without a
  target, and `.d` after it is the next statement: a label definition. We
  swallowed the dot and jumped to `d`. `parseRestore()` now does what the
  original does, so `Restore .d` with no other `.d` resets to the start.
- Duplicate labels in one scope went through the frontend and failed in g++.
  `sammleLabels()` now reports
  `Duplicate label 'x' (first defined at file:line:col)` at the second
  definition, as `LabelNode::semant` does in `compiler/stmtnode.cpp`. For
  `Restore .d` the reference points straight at the cause. `LabelStmt` now
  carries its column.

New tests: `test_bug42_labels` (rejected by the previous compiler with
`Undefined label 'zweite'`), `neg_bug42_goto_dot`, `neg_bug42_gosub_dot`,
`neg_bug42_dot_ohne_name`, `neg_bug42_label_doppelt`, `neg_bug42_restore_dot`
(all five accepted by the previous frontend).

Validation: emitted C++ compared over 187 programs: 119 byte-identical, 62
rejected by both with the same diagnostic, the six differences being the new
tests. Full suite: 179 passed, 0 failed.

---

## 2026-09-15 — Not only at the start of an expression (BUG-41)

`a And Not b` is no longer accepted, and neither is `Not Not a`. In the
original `compiler/parser.cpp`, `parseExpr()` is the only place that checks for
`NOT`, and it reads the operand with `parseExpr1( false )` — the And/Or level,
not another `parseExpr()`. Anywhere else `Not` reaches `parsePrimary()` and
fails with `Expecting expression`. It stays valid at the start of every
expression, including parentheses and argument lists: `a And (Not b)`,
`F(Not x)`.

Our `parseNot()` between `parseLogical()` and `parseComparison()` accepted `Not`
before every And/Or operand; a comment called that deliberately harmless. It is
removed. A misplaced `Not` now reports
`'Not' is only allowed at the start of an expression; put it in parentheses: (Not x)`
and consumes its operand, so no second message follows. The position matches
the original: the previous `test_bug2526_precedence.bb` is rejected at `11:13`,
as measured against V11.8 on 2026-09-07.

`test_bug2526_precedence` and `test_fixes` now put that `Not` in parentheses,
with unchanged output. New negative tests: `neg_bug41_not_after_and`,
`neg_bug41_not_not`.

Validation: emitted C++ compared over 181 programs: 119 byte-identical, 60
rejected by both with the same diagnostic, the two differences being the new
negative tests. Full suite: 173 passed, 0 failed.

---

## 2026-09-15 — Before and After bind like a sign (BUG-88)

`Before` and `After` read a full expression as their operand, so
`If After p = Null` became `After (p = Null)`: the front end accepted it and g++
failed on `(p == 0)->__next__`. In the original `compiler/parser.cpp` both are
cases of `parseUniExpr()` and read their operand with `parseUniExpr( false )`.
The parser now uses `parseUnary()` there, and both nodes carry their column.

The semantic pass also lacked the operand check. Following
`AfterNode::semant`/`BeforeNode::semant` in `compiler/exprnode.cpp`, a known
non-object type or a whole fixed array now reports
`'After' must be used with a custom type object` at the keyword. The original's
separate message for `Null` is not reachable while `Null` is an integer literal
(BUG-45); `After Null` is rejected with the general message.

New tests: `test_bug88_before_after` (15 output lines: comparisons, `And`,
nesting, forward and backward list loops; the previous compiler fails on it in
g++), `neg_bug88_after_number`, `neg_bug88_before_array`.

Validation: emitted C++ of the old and new compiler compared over 179
programs: 118 byte-identical (including `test_m16_iteration`, which uses
`Before`/`After` throughout), 58 rejected by both with the same diagnostic, the
three differences being the new tests. Full suite: 171 passed, 0 failed.
Messages and positions come from the reference source; no run against the
original was possible.

---

## 2026-09-15 — Field access only after a variable (BUG-40)

`(First Node)\val` and `F()\val` are no longer accepted. In the original
`compiler/parser.cpp`, the `\` and `[` postfix loop lives only in `parseVar()`,
which `parsePrimary()` reaches solely from an identifier that is not a function
call. After a parenthesised expression, a call, `First`/`Last`/`New` or a
literal, the `\` is left over and the statement ends there. `parsePostfix()`
now enters its loop only for a `VarExpr` or `ArrayAccess`.

The diagnostic keeps our parser wording (`unexpected token '\'`); its position
matches the original: the previous `test_bug09_paren_expr.bb` is now rejected
at `28:19`, the exact spot measured against V11.8 on 2026-09-07.

Three tests used the form as expected behaviour (`test_bug09_paren_expr`,
`test_bug12_case_insensitive`, `test_m16_iteration`). They were rewritten to
valid Blitz3D with unchanged output. Two negative tests were added.

Validation: emitted C++ of the old and new compiler compared over all 176
programs in `tests/` and `examples/`: 118 byte-identical, 56 rejected by both
with the same diagnostic, the only two differences being the new negative
tests. Full suite: 168 passed, 0 failed; `test_m16_iteration` (compile-only)
was run by hand and still prints 30 and 10. A run against the original was not
possible, as `G:\dev\Blitz3D` no longer exists after the move to `F:`.

Side finding, not fixed: `Before`/`After` parse a full expression instead of a
unary operand, so `If After p = Null` becomes `After (p = Null)` and fails only
in g++ (BUG-88).

---

## 2026-09-14 — Integer conversions in conditions and array indices (BUG-61)

Conditions (`If`/`ElseIf`, `While`, `Until`), array indices (dynamic arrays and
fixed arrays, including fields), and `Dim` bounds now explicitly convert to
integer. Numeric strings use their integer prefix; floats round to the nearest
integer with ties to even. For example, `If "0.6"` is false while `If 0.6` is
true; array index `"1.9"` selects element 1 and `1.9` selects element 2.
Objects and whole fixed arrays in these contexts produce a semantic diagnostic.

The rules were checked in the original Blitz3D compiler's `stmtnode.cpp`,
`varnode.cpp` and `exprnode.cpp`, then measured against the installed V11.8.
The new regression covers 37 output values, including loop re-evaluation,
index side effects, and `Read`/`For` targets. Its outputs match the original
using file output; three rejection cases were also checked against V11.8.

Validation: compiler rebuilt successfully. The full suite reported 165 passed
and one output mismatch caused by LF line endings in the new expected file
(the existing Windows fixtures use CRLF). After correcting that file format,
all four new tests passed a focused recheck, including exact diagnostics and
exit code 1 for negative tests. All 162 pre-existing tests passed unchanged.
The command-generator check has pre-existing differences from `commands.h`;
the new helper is excluded, and the scanned public signatures match HEAD.

The conversion helper is limited to these contexts. Existing float conversion
differences elsewhere and string `And`/`Or` (BUG-54) remain separate work.

---

## v0.4.3 - "3D Shader, Geometry Buffers & Primitive Meshes" (2026-03-10)

**Files touched:** `src/compiler/bb_shader.h` (new), `src/compiler/bb_mesh_core.h` (new),
`src/compiler/bb_mesh.h` (new), `src/compiler/bb_graphics3d.h`, `src/compiler/bb_sdl.h`,
`src/compiler/blitzcc.cpp`, `README.md`, `ROADMAP3D.md`, `tests/`

Completes 3D-07 through 3D-09 — the rendering half of the 3D foundation. After v0.4.2
the scene graph existed but nothing reached the screen; this release closes that gap.
`CreateCube` now produces visible geometry.

**3D-07 · Shader Infrastructure**
- `bb_shader.h`: `bb_Shader_` wraps a linked GL program plus a uniform-location cache
  (`std::unordered_map<std::string, GLint>`), so repeated `glGetUniformLocation` calls
  cost one hash lookup instead of a driver round-trip.
- `bb_shader_compile_(vert, frag)` → `bb_Shader_*` or `nullptr`; `bb_shader_check_()`
  prints the GL info log to stderr on compile or link failure.
- Three GLSL 3.30 Core shaders embedded as raw string literals:
  - **UNLIT** — solid `u_color`, no lighting
  - **TEXTURED** — `sampler2D` × `u_color`, no lighting
  - **LIT** — Blinn-Phong, up to 8 lights, optional texture; degrades to ambient-only
    when `u_light_count == 0`, so it is safe to bind before lights exist (3D-12)
- Light types in the LIT shader: 0 = directional (`u_light_pos` is a direction),
  1 = point (`u_light_pos` is a world position, `u_light_range` the falloff radius).
  Colour uniforms are pre-normalised to [0,1] by the caller. Normals are transformed
  with `mat3(u_model)` — correct for rotation and uniform scale.
- `bb_shaders_init_()` runs lazily on the first `RenderWorld` call, not at startup:
  a GL context must be current, and text-mode programs must never touch GL.
- `bb_shader_bind_()` skips a redundant `glUseProgram` when the program is already active.
- `bb_shader_quit_hook_` registered in `bb_sdl.h`; programs are deleted before the
  GL context is destroyed.

**3D-08 · Geometry Buffers (VAO/VBO/EBO)**
- `bb_mesh_core.h`: `bb_MeshData_` holds CPU-side vertices and indices, the three GL
  handles, a `dirty` flag and `triCount`.
- Interleaved vertex format, 11 floats per vertex, stride 44 bytes:
  position (0), normal (12), texcoord (24), colour (32). Attribute locations
  0–3 match `bb_shader.h` exactly — the two files are a matched pair and must be
  changed together.
- `bb_mesh_upload_()` creates or refreshes VAO/VBO/EBO; `bb_mesh_draw_()` re-uploads
  when dirty, sets MVP, model matrix, colour, texture and view position, then calls
  `glDrawElements`; `bb_mesh_free_gpu_()` releases GPU objects but leaves the CPU
  geometry intact, so a mesh can be re-uploaded after a context loss.

**3D-09 · Primitive Meshes**
- `bb_mesh.h`: `bb_MeshEntity_` extends `bb_Entity_` with a `std::vector<bb_MeshData_>`
  — one surface per draw call. The destructor frees the GPU buffers.
- Four generators, all unit-sized around the origin so `ScaleEntity` behaves predictably:
  - `bb_gen_cube_()` — 6 quads, flat outward normals
  - `bb_gen_sphere_(segs)` — UV sphere, smooth normals
  - `bb_gen_cylinder_(segs, open)` — smooth normals on the mantle, flat caps
  - `bb_gen_cone_(segs, open)` — slanted mantle normals, flat cap
- Commands: `CreateCube`, `CreateSphere`, `CreateCylinder`, `CreateCone`,
  `MeshWidth/Height/Depth` (AABB of the CPU geometry).
- `bb_render_meshes_(shader, view, proj)` is called from `bb_RenderWorld()` inside the
  per-camera loop after the view and projection matrices are built. It currently binds
  the UNLIT shader for every mesh; the TEXTURED and LIT paths activate with 3D-11 and 3D-12.
- `glEnable(GL_DEPTH_TEST)` moved into the render pass.

**`kCommands[]`:** 55 new 3D entries registered across v0.4.1–v0.4.3 (scene, camera,
entity, transform, hierarchy and mesh commands) — these feed `-k` / `+k` and therefore
IDE autocomplete.

**Version string:** `blitzcc.cpp` carried a hard-coded `v0.4.0` in two places while the
documentation had already moved to v0.4.2. Both replaced by a single
`static constexpr const char *kVersion` — closes BUG-02 / REFACTOR R12.

**Tests:** `tests/test_3d09_primitives.bb` (rotating cube). Full suite: 51 passed,
0 failed, 1 skipped (`test_m16_iteration`, known pre-existing parser bug).

---

## v0.4.2 - "3D Entity System & Camera" (2026-03-10)

**Files touched:** `src/compiler/bb_entity_core.h` (new), `src/compiler/bb_camera.h` (new),
`src/compiler/bb_graphics3d.h`, `src/compiler/bb_sdl.h`, `ROADMAP3D.md`, `tests/`

Implements the core 3D scene graph (3D-03 through 3D-06) on top of the GL infrastructure
from v0.4.1. Entities, transforms, hierarchy, and camera are now fully operational.
Also fixes a perspective projection bug found during code review.

**3D-03 · Entity Handle System & Pivot**
- `bb_entity_core.h`: polymorphic `bb_Entity_` base with handle map
  (`std::unordered_map<int, std::unique_ptr<bb_Entity_>>`), counter, `bb_entity_get_()`.
- `bb_PivotEntity_` as first concrete entity type.
- `bb_CreatePivot(parent=0)`, `bb_FreeEntity(h)` (recursive child teardown),
  `bb_HideEntity`, `bb_ShowEntity`, `bb_NameEntity`, `bb_EntityName`.
- `bb_entity_quit_hook_` registered at startup; `bb_sdl_quit_()` calls it before GL teardown.

**3D-04 · Transform System & Scene Graph**
- Column-major 4×4 matrix helpers: `mat4_identity_`, `mat4_mul_`, `mat4_make_translate_`,
  `mat4_make_scale_`, `mat4_make_euler_YXZ_` (analytically derived — YXZ Blitz3D convention),
  `mat4_inverse_` (Mesa GLU algorithm), `mat4_extract_euler_YXZ_`, `mat4_xform_pt_`.
- `bb_entity_update_all_()`: DFS from all root entities — `world = parent.world × local_TRS`.
- `bb_PositionEntity`, `bb_MoveEntity`, `bb_TranslateEntity` (local + world-space),
  `bb_RotateEntity`, `bb_TurnEntity`, `bb_ScaleEntity`, `bb_PointEntity`, `bb_AlignToVector`,
  `bb_ResetEntity`.
- Queries: `bb_EntityX/Y/Z`, `bb_EntityPitch/Yaw/Roll`, `bb_EntityDistance`.

**3D-05 · Entity Hierarchy**
- `bb_EntityParent(h, new_parent, glob=0)`: re-parents with optional world-coord preservation
  (decomposes new local matrix from `inv(new_parent.world) × old_world`).
- `bb_GetParent`, `bb_CountChildren`, `bb_GetChild` (1-based), `bb_FindChild` (recursive DFS),
  `bb_EntityOrder`, `bb_EntityClass`.

**3D-06 · Camera Entity**
- `bb_CameraEntity_` with `projMode`, `near_/far_`, `zoom`, per-camera viewport and cls state,
  `view[16]` + `proj[16]` computed each frame in RenderWorld.
- `bb_CreateCamera`, `bb_CameraRange`, `bb_CameraZoom`, `bb_CameraProjMode`,
  `bb_CameraViewport`, `bb_CameraClsMode`, `bb_CameraClsColor`.
- `bb_RenderWorld()` upgraded: collects all visible cameras (sorted by `order`), flips
  Blitz3D-to-GL viewport Y, builds per-camera view + projection matrices each frame.
- Global fallback clear state retained for programs without a camera entity (3D-02 compat).

**Bugfix: perspective projection aspect ratio**
- `aspect = vw/vh` (width ÷ height) instead of `vh/vw`.
  Previously VFOV ≈ 106° on 800×600 with zoom=1 (wider than HFOV). Now VFOV ≈ 74° (correct).

**Tests:** `test_3d03_pivot.bb`, `test_3d04_transform.bb`, `test_3d05_hierarchy.bb`,
`test_3d06_camera.bb` — all pass.

---

## v0.4.1 - "3D Foundation: OpenGL Context & Scene Control" (2026-03-09)

**Files touched:** `src/compiler/bb_gl_ctx.h` (new), `src/compiler/bb_graphics3d.h` (new),
`src/compiler/bb_sdl.h`, `src/compiler/bb_graphics2d.h`, `src/compiler/bb_runtime.h`,
`src/compiler/blitzcc.cpp`, `ROADMAP3D.md` (new), `tests/`

Kicks off Phase L — 3D graphics. Establishes the OpenGL 3.3 Core infrastructure
and wires it cleanly into the existing SDL3 + 2D pipeline.

**3D-01 · OpenGL Context Bootstrap**
- `bb_gl_ctx.h`: self-contained GL 3.3 Core loader — 60 function pointers declared via
  `BB_GL_DECL` macro, loaded at runtime via `SDL_GL_GetProcAddress`. No GLAD needed:
  SDL3 ships `SDL_opengl_glext.h` with all `PFNGL*` typedefs.
- `bb_Graphics3D(w,h,depth,mode)`: creates SDL3 window with `SDL_WINDOW_OPENGL`,
  requests Core 3.3 + 24-bit depth buffer, creates GL context, loads all function
  pointers, creates SDL_Renderer on the same window for future 2D coexistence.
- `bb_Flip()` updated: detects `bb_gl_active_` → `SDL_FlushRenderer` +
  `SDL_GL_SwapWindow` instead of `SDL_RenderPresent`.
- Quit hook wired into `bb_sdl_quit_()` with correct order: Renderer → GL context → Window.
- `-lopengl32` added to the generated compile command.

**3D-02 · UpdateWorld / RenderWorld / Scene State**
- `bb_graphics3d.h`: main 3D coordination header; `bb_runtime.h` now includes this
  instead of `bb_gl_ctx.h` directly.
- `bb_RenderWorld()`: `SDL_FlushRenderer` → `SDL_GL_MakeCurrent` → `glViewport` →
  `glClearColor/Depth/glClear` according to `CameraClsMode` state.
- `bb_CameraClsMode(cam, cls_color, cls_zbuf)` and `bb_CameraClsColor(cam, r, g, b)`:
  global state now, per-camera fields arrive in 3D-06.
- `bb_Wireframe(on)`: `glPolygonMode(GL_FRONT_AND_BACK, GL_LINE/GL_FILL)`.
- `bb_AmbientLight(r,g,b)`, `bb_TrisRendered()`, scene-level stubs all in place.
- `Graphics3D`, `UpdateWorld`, `RenderWorld`, `CameraClsMode`, `CameraClsColor` and
  friends added to `kCommands[]`.

**Architecture decisions documented in `ROADMAP3D.md`:**
- Forward Rendering initially; render-backend abstraction (`bb_RenderBackend_` struct
  with function pointers) allows Deferred Renderer to be swapped in later without
  touching Entity/Camera/Light systems.
- 2D + 3D coexistence strategy: SDL_Renderer for 2D, raw GL for 3D, synced via
  `SDL_FlushRenderer` before each GL render pass.
- 23 granular milestones (3D-01 through 3D-23) replacing the 7 coarse milestones
  in the main roadmap.

**Test results:**
```
[GL] Vendor:   NVIDIA Corporation
[GL] Renderer: NVIDIA GeForce RTX 2080/PCIe/SSE2
[GL] Version:  3.3.0 NVIDIA 591.86
```
Window opens with GL-cleared background, no crash, correct quit sequence.

---

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

### Nachtrag (2026-09-08, BUG-46): fuehrender Punkt im Float-Literal

`.5` ist gueltiges Blitz3D und war die groesste Einzelursache im Beispielbestand.
Der Dispatch in `tokenize()` schickte jedes `.` nach `lexOperator()`, `lexNumber()`
wurde nur bei einer fuehrenden Ziffer betreten — `Local a# = .5` scheiterte an
`unexpected token '.'`.

Die Regel wurde am laufenden Original gemessen, nicht aus dem Referenzquelltext
abgeleitet, und sie ist einfacher als vermutet: **rein lexikalisch und kontextfrei.**
Eine Ziffer hinter dem Punkt beginnt immer eine Zahl. Vier Messungen gegen
Blitz3D 11.8 zeigen, dass das Original wirklich keinen Kontext heranzieht:

- `Print .5` faltet zu `"0.5"`, `Print 5.` zu `"5.0"` — beide Punktstellungen erlaubt.
- `Goto .5` scheitert mit `Expecting identifier`. Selbst dort, wo ausschliesslich ein
  Label stehen kann, liest das Original die Zahl.
- `a.5` und `Dim a.5(2)` scheitern mit `Expecting end-of-file` bzw. `Expecting '('`.
  Auch direkt hinter einem Bezeichner wird der Punkt nicht mehr als Type-Tag gelesen.
- `1..5` zerfaellt in `1.` und `.5` und wird abgelehnt — genau ein Punkt je Zahl.

Deshalb genuegt ein Zeichen Vorschau im Dispatch: `.` gefolgt von einer Ziffer geht
nach `lexNumber()` (das den fuehrenden Punkt bereits konnte), alles andere bleibt
`OPERATOR "."` und erreicht die Label- und Type-Tag-Pfade im Parser unveraendert.
Das ist dieselbe Bauform wie die bestehenden Rueckfaelle bei `$` und `%`, die schon
so zwischen Hex-/Binaerliteral und String-/Integer-Tag unterscheiden.

**Wirkung, gemessen ueber die 130 Installationsdateien:** von 124 vom Original
akzeptierten Dateien hatten 71 einen `.`-Fehler, 29 davon haben jetzt keinen mehr.
Nur fuenf wechseln ganz die Fehlerklasse (89 → 84 echte Sprachfehler), weil ein
erster Parserfehler die folgenden verdeckt — die meisten dieser Dateien haben neben
`.5` noch andere Blocker. Die verbleibenden 42 `.`-Fehler sind ausnahmslos BUG-47
(`x.T = New T`), an der Quellzeile geprueft.

**Absicherung:** Ablehnungsvergleich ueber alle 119 `tests/**/*.bb` und
`examples/**/*.bb` ergab 0 geaenderte Urteile; volle Suite 113 passed, 0 failed.

Nicht Teil des Fixes: `Print .5+.5` gibt bei uns `1` aus, im Original `1.0`. Das ist
die Stringformatierung ganzzahliger Floatwerte, eine eigene offene Abweichung; der
Positivtest vermeidet solche Werte deshalb.

### Nachtrag (2026-09-08, BUG-47): Type-Tag an der Zuweisung

`p.T = New T` ohne vorheriges `Local` ist gueltiges Blitz3D und war nach BUG-46 die
groesste verbliebene Einzelursache — **alle 42 restlichen `.`-Fehler im
Installationsbestand waren dieser Fall.** Der Zuweisungszweig las nur `#/%/$`.

Die Sichtbarkeitsregel ist die Stelle, an der man sich hier verrennen kann, und sie
wurde am Assembler des Originals abgelesen statt vermutet: **das Tag oeffnet keinen
neuen Gueltigkeitsbereich.** Innerhalb einer Funktion schreibt `p.T = New T` in das
*globale* Slot, wenn ein `Global p.T` existiert — im ASM `mov [esp],_vp` gefolgt von
`__bbObjStore`, nicht `[ebp-N]`. Ohne passendes Global entsteht dort eine lokale.
Und eine getaggte Zuweisung im Hauptteil ist ihrerseits in Funktionen *nicht*
sichtbar (`Variable must be a Type`), ist also eine lokale Variable von `main` wie
jede implizite Variable auch. Das Tag liefert also nur den Typ einer noch nicht
existierenden Variablen; ansonsten bindet der Name ganz gewoehnlich.

Genau deshalb war im Analyzer **keine Zeile** noetig: der `AssignStmt`-Zweig macht
bereits `lookup()` und deklariert nur bei Fehlschlag, und `fromHint()` kannte `.T`
schon. Dieselbe Regel gilt seit BUG-38 am `For`-Zaehler. Im Parser kam der Tag-Zweig
dazu (konsumieren-und-`expect`, dieselbe Bauform wie dort), im Emitter eine Zeile:
`visit(AssignStmt)` traegt den Objekttyp jetzt wie `visit(VarDecl)` in
`varObjectTypes` ein — sonst faende ein spaeteres `Delete p` den Typ nicht und liefe
in den „type indeterminate"-Pfad.

Weiter am Original gemessen und im Test festgehalten: das Tag ist auch **vor dem
Feldtrenner** erlaubt (`p.T\v = 1`), dasselbe Tag darf mehrfach stehen, ein spaeterer
Verzicht darauf ebenso, zwei verschiedene Tags sind `Variable type mismatch` (unsere
Meldung steht auf derselben Position und traegt denselben Text), ein unbekannter Typ
ist `Type "q" not found`.

**Wirkung ueber die 130 Installationsdateien:** `.`-Fehler 42 → 19, echte
Sprachfehler 84 → 80, vollstaendig uebersetzende Dateien **22 → 24**, keine
Regression. Die 19 verbliebenen Punkt-Fehler sind an der Quellzeile geprueft und
gehoeren zu genau zwei anderen Ursachen: 14 zu Objektparametern `Function F(p.T)`
(neu als BUG-56 notiert, entspricht A-11 bei Astra) und 5 zu BUG-52 `Dim a.T(n)`.

**Absicherung:** Ablehnungsvergleich ueber alle 121 `tests/**` und `examples/**`
ergab 0 geaenderte Urteile; volle Suite 115 passed, 0 failed.

Nebenbefund, als BUG-55 notiert statt hier mitgenommen: `checkAssign()` prueft
Objekttypen ueberhaupt nicht — `Local p.T = New U` wird schon vor dieser Aenderung
angenommen, das Original lehnt ab. Vorbestehend und mit weiterer Reichweite
(`Local`, `Global`, Zuweisung), deshalb ein eigener Eintrag.

### Nachtrag (2026-09-08, BUG-48): `Then` ist optional — und der einzeilige Rumpf war zu kurz

Nach BUG-46 und BUG-47 war das die mit Abstand groesste Einzelursache: **60 der 124
vom Original akzeptierten Installationsdateien** meldeten `Expected ENDIF`.

Am Original gemessen ergaben sich **zwei** Regeln, und die zweite war die
unangenehmere:

**`Then` entscheidet gar nichts.** Es ist in beiden Formen optional. Was die Form
bestimmt, ist das Token direkt nach der Bedingung (und nach einem etwaigen `Then`):
Zeilenumbruch oder Doppelpunkt beginnen die Blockform, alles andere die einzeilige.
Genau deshalb verlangt `If a=1 : Print "x"` ein `EndIf`, `If a=1 Print "x"` dagegen
nicht — beides gemessen. Unser Parser hatte die Entscheidung zusaetzlich an `hasThen`
gehaengt; das faellt weg.

**Der einzeilige Rumpf laeuft bis zum Zeilenende, Doppelpunkte eingeschlossen.** Das
war ein zweiter, vorbestehender Defekt und ein *stilles Falschergebnis*: der Parser
las genau eine Anweisung, der Rest der Zeile lief unbedingt. Mit `a=0` gab
`If a=1 Then Print "x" : Print "y"` bei uns `y` aus, im Original nichts. Im Assembler
des Originals ueberspringt ein einziger bedingter Sprung beide Prints, und bei
`If a=1 P"x" Else P"y" : P"z"` enthaelt der Else-Zweig ebenfalls beide Anweisungen.
Haette man nur `Then` optional gemacht, waere dieser Fehler auf alle neu angenommenen
Programme ausgeweitet worden — die beiden Teile gehoeren zusammen.

Der neue `parseSingleLineBody()` liest also bis zum Zeilenumbruch, nimmt Doppelpunkte
als Trenner und laesst Blockabschluesse fuer den Aufrufer stehen. **`END` steht
bewusst nicht in dieser Liste:** ein blosses `End` ist die Programmende-Anweisung und
ein zulaessiger einzeiliger Rumpf — das Original nimmt `If a=1 End` an und legt
`_fend` in den bedingten Zweig. Auf `ElseIf` rekursiert der einzeilige Zweig wie die
Referenz und kehrt sofort zurueck, damit das verschachtelte If seine eigene Form
waehlt und hier kein `EndIf` erwartet wird. Zum Abschluss steht eine
Zeilenendepruefung; ohne sie wurde ein `EndIf` auf derselben Zeile stillschweigend
verschluckt und `If a=1 EndIf` faelschlich angenommen.

**Wirkung ueber die 130 Installationsdateien:** Dateien mit `Expected ENDIF`
**60 → 23**, echte Sprachfehler **80 → 64**, und **16 Dateien haben jetzt gar keinen
Sprachfehler mehr**. Keine Regression.

Von den 23 Resten haengen **20 an einem einzigen `Else If`** in
`samples/mak/start.bb`, das die `mak`-Beispiele reihum inkludieren — als BUG-57
notiert (entspricht A-29 bei Astra). Der Rest ist von frueheren Fehlern derselben
Datei verdeckt, etwa `lmesh.bb` mit festen Feldarrays.

**Absicherung:** Ablehnungsvergleich ueber alle 123 `tests/**` und `examples/**`
ergab 0 geaenderte Urteile; volle Suite 117 passed, 0 failed.

### Nachtrag (2026-09-08, BUG-57): `Else If` — und der Zwischenraum zaehlt

Ein einziges `Else If` in `samples/mak/start.bb` blockierte **20 der 23** Dateien,
die nach BUG-48 noch `Expected ENDIF` meldeten: die `mak`-Beispiele inkludieren
diese Datei reihum. Unser `kEndMerge` fasste nur `End X` zusammen.

Beim Messen kam heraus, dass der **Zwischenraum bedeutungstragend ist** — genau die
Frage, die Astra bei A-29 offengelassen hatte. Das Original fasst **nur bei genau
einem Leerzeichen** zusammen:

```
Else If      (ein Leerzeichen)  -> ein Token ELSEIF, braucht ein EndIf
Else  If     (zwei Leerzeichen) -> Else + verschachteltes If, braucht zwei
Else<TAB>If                     -> ebenso verschachtelt
Else <NL> If                    -> ebenso verschachtelt
```

Alle vier am Original gemessen, jeweils in beiden Varianten (ein bzw. zwei `EndIf`).
Dieselbe Regel gilt fuer die vier `End X`-Abschluesse: `End  If`, `End<TAB>If` und
`End  Function` lehnt das Original ab.

Damit war es wie bei BUG-48 wieder ein Zweiteiler. Unsere Zusammenfassung verlangte
nur „gleiche Zeile" und war damit zu lax — ein vorbestehender Defekt, der fuer
`End X` bloss zu grosszuegig war. Fuer `Else If` waere er schlimmer gewesen:
`Else  If` waere faelschlich zu `ElseIf` geworden, also **falsche Blockstruktur**
statt nur einer zusaetzlichen Annahme. Die laxe Regel zu uebernehmen war deshalb
keine Option.

Eine Falle steckt in der Umsetzung: **ein Tabulator zaehlt in unserem Lexer wie ein
Leerzeichen genau eine Spalte.** Eine reine Spaltenrechnung (`zweites Token beginnt
eine Spalte hinter dem ersten`) haette `End<TAB>If` durchgelassen — der erste Anlauf
tat das auch. Jetzt werden die Zeilenanfaenge einmal vorberechnet und der Abstand am
Quelltext selbst gelesen. Er muss vor der Zusammenfassung gelesen werden, weil das
zusammengefasste Token kuerzer ist als der Quelltext, aus dem es entstand.

Vorher geprueft: weder der Projektbestand noch die 130 Installationsdateien
enthalten unregelmaessigen Zwischenraum in diesen Formen, die strengere Regel bricht
also nichts. `Else If` mit einem Leerzeichen kommt im Installationsbestand 26-mal vor.

**Wirkung:** Dateien mit `Expected ENDIF` **23 → 1**; der Rest (`lmesh.bb`) scheitert
zuerst an festen Feldarrays. 22 Dateien kommen an der If-Kette vorbei, 15 davon
stossen jetzt nur noch auf unbekannte 3D-Befehle. Keine Regression, 0 geaenderte
Urteile ueber alle 125 Projektdateien.

**Die Serie BUG-46/47/48/57 zusammen**, gemessen ueber die 124 vom Original
akzeptierten Installationsdateien:

| Stand | Sprachfehler | Dateien mit Sprachfehler |
|---|---:|---:|
| vorher      | 1407 | 89 |
| nach BUG-46 |  872 | 84 |
| nach BUG-47 |  799 | 80 |
| nach BUG-48 |  438 | 64 |
| nach BUG-57 |  408 | 63 |

Als BUG-58 notiert statt hier mitgenommen: uebrig gebliebene Blockabschluesse
werden still geschluckt. `EndIf` ohne `If`, `Wend` ohne `While`, ein ueberzaehliges
`EndIf` und eine Funktion ohne `End Function` nehmen wir alle vier an, das Original
lehnt alle vier ab. Vorbestehend und unabhaengig von dieser Aenderung — die
Gegenproben enthalten kein `Else If`.

### Nachtrag (2026-09-08, BUG-51): `Delete Each <Typ>`

Die Bugliste notierte „1 der 49 Befunde". Gemessen sind es **13 Vorkommen** —
wieder ueber das geteilte `samples/mak/start.bb` (`Delete Each GfxMode`), dazu
`functions.bb`, `flares.bb`, `Main.bb`, `insectoids.bb`. Alle 13
`unexpected token 'EACH'`-Fehler im Bestand waren diese Form, keine andere.

Am Original gemessen: die Form uebersetzt nach `__bbObjDeleteEach` und verlangt
einen **Typnamen, keinen Ausdruck**. `Delete Each Q` (unbekannt) und
`Delete Each p` (Objektvariable) werden beide mit `Specified name is not a
NewType name` abgelehnt. Eine leere Liste ist zulaessig, in einer Funktion ist die
Form erlaubt.

`DeleteStmt` traegt jetzt entweder ein Objekt oder einen `eachTypeName`. Der
Analyzer prueft den Namen mit dem vorhandenen `knownType()`, also derselben
Meldung samt Vorschlag wie bei `For ... = Each`. Der Emitter leert die Liste ueber
ihren Kopf:

```cpp
while (bb_t_head_) bb_t_Delete(bb_t_head_);
```

`bb_T_Delete` haengt den Knoten aus der Liste aus, der Kopf rueckt also nach — ein
eigener Zeiger auf das naechste Element waere ueberfluessig und im Fehlerfall
gefaehrlich.

**Der neue Zustand im AST verlangte eine Rundum-Pruefung** (RED-06: dieselbe
Knotenart wird an mehreren Stellen von Hand durchlaufen). `DeleteStmt` wird
ausser im Emitter noch zweimal angefasst: `blitzcc.cpp:165` reicht
`del->object.get()` an `collectCallsExpr()`, das auf `nullptr` prueft, und
`emitter.h:961` ist ein reiner Typtest. Keine Stelle dereferenziert das jetzt
moeglicherweise leere `object`.

**Wirkung:** Dateien mit `unexpected token 'EACH'` **13 → 0**, keine Regression,
0 geaenderte Urteile ueber alle 127 Projektdateien.

**Eine Warnung zur Kennzahl:** die Gesamtzahl der Sprachfehler steigt dabei von
408 auf 420. Das ist kein Rueckschritt — der Parser kommt jetzt an `Delete Each`
vorbei und findet in denselben Dateien bisher verdeckte Fehler
(`Expected parameter name (got '.')` steigt von 10 auf 13, das ist BUG-56). Die
Sprachfehlerzahl ist damit **kein monotones Fortschrittsmass**; belastbar sind die
Zahl der Dateien mit Sprachfehlern und das Verschwinden der jeweiligen Fehlerform.

### Nachtrag (2026-09-08, BUG-56): Objektparameter `Function F(p.T)`

15 Dateien, nach BUG-51 die groesste verbliebene Sprachursache. An Parametern
wurden nur `%/#/$` gelesen, obwohl der Rueckgabetyp drei Zeilen darueber `.T`
schon konnte.

**Die Lebensdauerfrage aus A-11 ist beantwortet, und die Antwort war guenstig.**
Astra warnte, der Referenzcompiler behandle Objektparameter anders als
gewoehnliche Objektvariablen. Das stimmt — der Unterschied ist, dass ein
Objektparameter **nicht referenzgezaehlt** wird. Im Assembler des Originals liest
der Rumpf ihn direkt aus dem Stack-Slot, und `p = New T` im Rumpf schreibt mit
einem schlichten `mov [ebp+20],eax` zurueck, ohne `_bbObjStore`/`_bbObjRelease`,
die eine lokale oder globale Objektvariable dort sehr wohl durchlaeuft. Er ist
also ein gewoehnlicher Wertparameter, und der Aufrufer sieht eine Zuweisung im
Rumpf nicht. Ein roher Zeiger als C++-Parameter bildet das exakt ab — unser
Modell zaehlt ohnehin nirgends Referenzen. Zu bauen war deshalb nichts.

Weiter gemessen: gemischte Parameterlisten sind erlaubt, ein Objektparameter darf
zurueckgegeben und geloescht werden, `Null` ist ein zulaessiges Argument, ein
Vorgabewert ist es nicht (`F(p.T=Null)` → `Expression must be constant`).

**Fehlerbehandlung eigens nachgebessert.** Ein `expect()` an dieser Stelle haette
bei `Function F(p.)` das folgende `)` mitkonsumiert, die Parameterschleife waere
bis zum Dateiende gelaufen und haette acht Folgefehler an eine einzige Ursache
gehaengt. Jetzt wird gemeldet und die Liste abgebrochen: genau ein Fehler, auf
derselben Position, auf der auch das Original meldet.

**Ein stilles Falschergebnis kam mit heraus.** `varObjectTypes` war nicht an den
Funktionsrumpf gebunden — ein Objekttyp aus einer Funktion blieb danach im
Hauptteil stehen. Gemessen an einer Funktion mit `Local p.T` und einem spaeteren
`p = First U : Delete p` im Hauptteil emittierte der Compiler
`bb_t_Delete(var_p)`: der **falsche Listen-Helfer fuer ein U-Objekt**, ohne jede
Warnung. Die Karte wird jetzt wie `declaredVars` um den Rumpf gesichert und
wiederhergestellt. Vorbestehend — `hoistLocals()` trug Locals schon vorher ein —
aber BUG-56 haette Parameter hinzugefuegt und das Leck verbreitert, deshalb hier
mitbehoben statt notiert.

Dieser Fall bekommt bewusst **keinen Test**: die richtige Ausgabe waere ein
geloeschtes U-Objekt, unser Ergebnis ist die Ersatzhandlung aus A-19. Ein Test
haette jenen Defekt als Sollverhalten festgeschrieben.

**Wirkung:** Dateien mit `Expected parameter name (got '.')` **15 → 0**,
Sprachfehler 420 → 390, Dateien mit Sprachfehlern 63 → 61. Keine Regression,
0 geaenderte Urteile ueber alle 129 Projektdateien. Die drei haeufigsten ersten
Fehlermeldungen im Installationsbestand sind jetzt **allesamt unbekannte
Befehle** — die Blockade ist dort ueberwiegend keine Sprachfrage mehr.

### Nachtrag (2026-09-08, BUG-53): implizite Umwandlung an Zuweisungsgrenzen

Die groesste Einzelursache im Beispielbestand: **31 der 61 Befunde**, fast immer
`Text 10,20,punkte` mit einem Integer als String-Parameter. Die alte Notiz „1 von
49" taeuschte — erst nachdem die sechs vorigen Fixes den Parser weitergebracht
hatten, wurde die wahre Groesse sichtbar.

Am Original gemessen ergab sich eine vollstaendig **symmetrische** Matrix:
Integer, Float und String wandeln in allen sechs Richtungen, an allen vier
Stellen mit bekanntem Zieltyp — Zuweisung, `Local`/`Global` mit Initialisierung,
Parameter, `Return`. Objekte wandeln nie. Zahl→String faltet der Originalcompiler
zur Uebersetzungszeit, String→Zahl ist ein Laufzeitaufruf und folgt `atoi`/`atof`:
`"12abc"` → 12, `"abc"` → 0, ohne Fehler.

**Die Diagnose bloss zu entfernen waere die schlechtere Loesung gewesen, und das
ist gemessen, nicht vermutet.** Nach der reinen Lockerung in `semant.h`
uebersetzte `Local s$ = 42` anstandslos und gab `*` aus — C++ nahm die 42 als
Zeichencode. Aus einer lauten Ablehnung waere ein stilles Falschergebnis
geworden. Astra hatte in A-02 genau davor gewarnt.

Der Kniff, der das ohne typisierte Zwischendarstellung loest: **der Emitter kennt
den Zieltyp, aber nicht den Typ des Quellausdrucks.** Also sind die neuen Helfer
`bb_Str`, `bb_ToInt` und `bb_ToFloat` ueber alle Quelltypen ueberladen,
Durchreicher eingeschlossen. Der Emitter darf bedenkenlos wrappen, die
C++-Ueberladungsaufloesung entscheidet, ob ueberhaupt etwas passiert — dieselbe
Bauform wie die vorhandenen `operator+`-Ueberladungen fuer `"Score: " + n`.

Die Zieltypen kommen aus der Deklaration (`varHints_`, analog zu
`varObjectTypes`), aus dem Rueckgabetag der laufenden Funktion und an
Aufrufstellen aus `userFuncDecls_` bzw. der erzeugten Befehlstabelle. `Text`
steht dort als `x%,y%,s$,...`; heraus kommt
`bb_Text(10, 20, bb_Str(var_punkte))`. Ein Parameter ohne Typ in der Tabelle
(`Print` hat `val?`) heisst ausdruecklich „beliebig" und wird nicht gewandelt.

`bb_ToInt(float)` schneidet weiterhin ab, wie der erzeugte Code es bisher tat.
Dass Blitz3D rundet, ist eine eigene Abweichung (A-03) und haette hier nur den
Befund verwischt.

**Zwei Negativtests waren gar keine.** `neg_weak17_builtin_return.bb`
(`Local s$ = Len("abc")`) und `neg_weak17_builtin_types.bb` (`Sin("x")`) werden
vom Original angenommen; sie hielten unsere eigene zu strenge Regel fest, nicht
Blitz3D. Astra hatte beide benannt. Sie sind entfernt und als Faelle in den
Positivtest gewandert.

**BUG-55 faellt mit ab.** Die beiden alten Zweige von `checkAssign()` waren
ohnehin falsch; an ihre Stelle trat die Pruefung, die wirklich noetig ist — ist
eine Seite ein Objekt, muessen beide dasselbe Objekt sein. Damit lehnen alle
sechs vorher ungeprueften Stellen ab wie das Original. Zwei Grenzfaelle brauchten
Sorgfalt: `p.T = Null` bleibt zulaessig, und ein unbekannter Typname darf keine
zweite Meldung erzeugen — ohne diese Abgrenzung schlugen zwei bestehende
Diagnosetests fehl, genau wofuer sie da sind.

**Wirkung:** Dateien mit `cannot assign` **32 → 0**. Dateien mit Sprachfehlern
**61 → 36**, Sprachfehler gesamt **390 → 306**. Der groesste Einzelsprung der
Serie, keine Regression, volle Suite 123 passed.

Nicht Teil des Fixes und als BUG-61 notiert: Vergleich, Bedingung, Arrayindex und
Schleifengrenze. Dort lauern zwei am Original gemessene Ueberraschungen — der
**Vergleich** wandelt die Zahl zum String (`"2" > 10` ist wahr, `"abc" = 0`
falsch), die **Bedingung** dagegen den String zur Zahl (`If "x"` ist falsch, nicht
„nicht leer = wahr"). Wer das verwechselt, bekommt das Gegenteil heraus.

### Nachtrag (2026-09-08, BUG-49): Vorgabewerte fuer Funktionsparameter

Acht Dateien. Am Original ergaben sich zwei Regeln, die man nicht raten sollte:

**Pflicht ist alles bis zum LETZTEN Parameter ohne Vorgabe.** `F(a=1,b)` wird
angenommen, verlangt aber beide Argumente — `F(1)` ist dort
`Not enough parameters`. Ebenso verlangt `F(a,b=2,c)` alle drei. Eine Vorgabe vor
einem Parameter ohne Vorgabe ist also erlaubt, aber nie weglassbar.

**Der Vorgabewert muss ein konstanter Ausdruck sein.** Literal, Vorzeichen,
`1+1`, `1 Shl 2`, ein `Const` und die reservierten `True`/`False`/`Pi` gehen
durch; eine Variable und `Len("abc")` nicht.

Die erste Regel loest ein Problem, das zunaechst nach einer Sackgasse aussah:
C++ erlaubt Vorgabeargumente nur am Ende, Blitz3D aber auch davor. Weil ein
Parameter mit Vorgabe, dem einer ohne folgt, ohnehin nie weggelassen werden
kann, darf der Emitter die Vorgabe dort schlicht **weglassen** — die Bedeutung
bleibt identisch, und die Stelligkeitspruefung im Analyzer haelt den Rest. Die
Vorgaben stehen ausserdem nur in der Vorwaertsdeklaration, weil C++ sie je
Funktion genau einmal erlaubt.

`FunctionDecl::params` ist dafuer von `pair<name,hint>` auf eine `Param`-Struktur
mit `defaultValue` umgestellt; sechs Bindungsstellen zogen nach. Ein zweiter,
parallel gefuehrter Vektor waere die fehleranfaellige Variante gewesen.
`FuncInfo` traegt jetzt `required` neben `params.size()` — `checkArity()` nahm
beide Zahlen ohnehin schon getrennt entgegen.

**Eine Reihenfolgefalle kam beim Messen heraus.** Die Konstantenpruefung stand
zuerst in `collect()`; dort ist `constNames_` aber noch unvollstaendig, weshalb
ein `Const` *hinter* der benutzenden Funktion faelschlich abgelehnt wurde. Das
Original nimmt diese Reihenfolge an. Die Pruefung sitzt jetzt in
`checkFunctions()`, das nach dem Sammeln laeuft.

Ausgewertet wird nichts: fuer die Diagnose genuegt die Form des Ausdrucks, und
der Emitter reicht ihn als C++-Vorgabeargument weiter, wo keine Konstante
verlangt ist. Ein eigener Konstantenauswerter (A-07) war nicht noetig.

**Wirkung:** Dateien mit `Expected parameter name (got '=')` **8 → 0**, Dateien
mit Sprachfehlern 36 → 30, Sprachfehler gesamt 306 → 271. Keine Regression,
volle Suite 126 passed.

### Nachtrag (2026-09-08, BUG-50): `=>`, `=<` und `><`

Blitz3D kennt die drei Vergleichsoperatoren in beiden Reihenfolgen. Der Eintrag
nannte urspruenglich nur `=>` und `=<`; **`><` fehlte ebenfalls** — Astra hatte
die dritte Form benannt, hier ist sie gemessen bestaetigt.

Am Original jeweils in beide Richtungen belegt, damit nicht nur die Annahme,
sondern auch die Bedeutung feststeht: `5 => 3` → 1 und `3 => 5` → 0 (also `>=`),
`5 =< 3` → 0 und `3 =< 5` → 1 (also `<=`), `5 >< 3` → 1 und `3 >< 3` → 0 (also
`<>`). Ein Zwischenraum ist nicht erlaubt, und weitere Aliase gibt es nicht.

Der Fix sind drei Zeichenpaare mehr in der Zwei-Zeichen-Erkennung von
`lexOperator()`, danach auf die kanonische Schreibweise normalisiert. Parser und
Emitter bleiben unveraendert und muessen nur eine Form kennen. Die
Zwischenraumregel ergibt sich hier von selbst, weil nur unmittelbar benachbarte
Zeichen zusammengefasst werden — anders als bei BUG-57, wo genau das eigens
geprueft werden musste.

Der Preis der Normalisierung, bewusst in Kauf genommen: eine Diagnose nennt
`>=`, wo die Quelle `=>` schreibt. Die Alternative waere gewesen, beide
Schreibweisen durch Parser und Emitter zu fuehren.

Drei Gegenproben stellen sicher, dass die gewoehnliche Zuweisung nicht leidet
(`x = 5`, `y = -3`, `If x = 5`) — das `=` darf durch die neuen Paare nicht
verloren gehen.

**Wirkung:** Dateien mit `unexpected token '>'` **5 → 0**, Dateien mit
Sprachfehlern 30 → 25, Sprachfehler gesamt 271 → 231. Keine Regression.

### Nachtrag (2026-09-08, 3D-00): Grafikmodus- und Treiberaufzaehlung

Nach zehn Sprachfixes war die Sprache nicht mehr der Engpass: 74 der 124 vom
Original akzeptierten Beispieldateien scheiterten nur noch an unbekannten
Befehlen. Die Messung ueber alle Vorkommen ergab **131 verschiedene fehlende
Befehle in 1527 Vorkommen** — und einen Block, den der 3D-Meilensteinplan gar
nicht fuehrt, weil er kein Rendering betrifft.

`CountGfxModes3D`, `GfxModeWidth/Height/Depth`, `GfxModeExists`, `Windowed3D`,
`CountGfxDrivers`, `GfxDriverName`, `SetGfxDriver`: eine reine Abfrage-API, die
**30 Beispieldateien** blockierte, weil die gemeinsame `start.bb` der mak-,
halo-, AGore-, Skully- und Richard_Betson-Beispiele damit beginnt. Sieben
Dateien uebersetzen allein dadurch vollstaendig — die Messung hatte genau diese
sieben vorhergesagt, und genau sie sind es geworden.

**Die 1-Basierung ist abgelesen, nicht geraten.** Die Signaturen liefert
`blitzcc +k` exakt, die Indexbasis aber nicht. Sie steht im tatsaechlichen
Gebrauch:

```blitzbasic
For k=1 To CountGfxModes3D()
  Print k+":"+GfxModeWidth(k)+","+GfxModeHeight(k)
Next
driver = Input$( "Display driver (1-"+CountGfxDrivers()+"):" )
```

Ein Index ausserhalb des Bereichs liefert 0 bzw. den leeren String statt zu
stuerzen. Blitz3D meldet dort einen Laufzeitfehler; der stille Nullwert ist die
vorsichtigere Wahl, solange dessen genaue Form nicht gemessen ist.

Zwei bewusste Abweichungen stehen im Header: `CountGfxModes()` liefert dieselbe
Liste wie `CountGfxModes3D()` (auf heutiger Hardware ist jeder Modus 3D-faehig),
und `SetGfxDriver` merkt den Wert nur — SDL3 waehlt den Videotreiber beim
Initialisieren, ein echter Wechsel findet nicht statt.

**Verifikation ohne Bildvergleich.** Fuer Rendering-Befehle taugt der bisherige
Weg (Werte gegen das Original) nicht. Hier greifen stattdessen zwei andere
Instrumente: `scripts/compare_commands.py` gegen `blitzcc +k` meldet fuer alle
zehn Befehle **keine Abweichung** in Stelligkeit, Grenzen und Rueckgabetyp, und
der Test prueft **Invarianten statt Zahlen** — 1-Basierung, Bereichsgrenzen,
Vertraeglichkeit der Abfragen untereinander. Eine feste `.expected` mit "21
Modi" waere auf jedem anderen Rechner falsch.

**Wirkung:** vollstaendig uebersetzende Dateien **25 → 32**, unbekannte Befehle
131 → 121 verschieden und 1527 → 1243 Vorkommen. Keine Regression.

### Nachtrag (2026-09-08, 3D-12): Lichter

`CreateLight` war mit **39 betroffenen Beispieldateien der haeufigste fehlende
Einzelbefehl** ueberhaupt. Der LIT-Shader konnte laut 3D-07 bereits acht
Lichter — gefehlt haben die Sprachseite, das Einsammeln im Renderpass und, wie
sich zeigte, die Spotlichter.

**Die mitgelieferte Dokumentation hat mich vor drei Fehlgriffen bewahrt.**
`help/commands/3d_commands/` liegt im Installationsverzeichnis und ist fuer
Rendering-Befehle das, was `blitzcc +k` fuer Signaturen ist:

- Die Vorgabe von `CreateLight()` ist **1 = directional**, nicht point. Der
  eigene Roadmap-Entwurf hatte im Strukturkommentar „1=point" stehen — das
  waere fuer die 65 Aufrufe von `CreateLight()` ohne Argument der falsche
  Lichttyp gewesen.
- Die Nummerierung beginnt bei **1** (1/2/3), waehrend der Shader intern ab 0
  zaehlt. Ohne Umsetzung waere jeder Typ um eins verschoben.
- `LightRange` hat die Vorgabe **1000.0**, `LightConeAngles` **0,90**, und
  `AmbientLight` **127,127,127** — eine Szene ohne `AmbientLight` ist im
  Original also mittelgrau beleuchtet, nicht schwarz. Unsere Vorgabe war 0.

Dazu: `LightColor` erlaubt ausdruecklich **negative Werte** ("negative
lighting" fuer Schatteneffekte), deshalb wird dort nicht geklemmt — erst das
Endergebnis im Shader wird begrenzt.

**Spotlichter habe ich im Shader ergaenzt statt sie als Punktlicht zu
rendern.** Zehn Beispieldateien rufen `CreateLight(3)`; ohne Kegelrechnung
haetten sie ein sichtbar falsches Bild ergeben, ohne dass irgendetwas meldet —
genau die Klasse stiller Falschergebnisse, die diese Sitzung durchgehend
vermieden hat. Neu sind `u_light_dir`, `u_light_cos_inner` und
`u_light_cos_outer`; der Vergleich laeuft ueber den Kosinus des Halbwinkels,
damit im Fragment keine Trigonometrie noetig ist.

Ein Richtungslicht bezieht seine Richtung aus der Rotation (die Beispiele
richten es mit `RotateEntity` aus). Entities blicken nach +Z, die dritte Spalte
der Weltmatrix ist also die Vorwaertsachse; der Shader will die Richtung *zum*
Licht, somit deren Gegenrichtung.

**Ein Nebenbefund: das Signaturwerkzeug ist blinder als gedacht.**
`compare_commands.py` vergleicht Stelligkeit, Rueckgabetyp und
Parameter**namen** — die Parameter**typen** nicht, es streift `#$%` vor dem
Vergleich ab. Ein Handabgleich ueber alle 320 gemeinsamen Befehle fand vier
Abweichungen, die das Werkzeug nicht sehen kann. `AmbientLight` (wir `%,%,%`
gegen `#,#,#`) ist hier mitbehoben; die drei uebrigen stehen als BUG-62. Seit
BUG-53 wandeln Argumente still an der Parametergrenze um, ein falsch
gefuehrter Typ schneidet also lautlos ab.

**Wirkung:** vollstaendig uebersetzende Beispieldateien 32 → 34, unbekannte
Befehle 1243 → 1187 Vorkommen. Nur zwei Dateien mehr, weil die meisten der 39
zusaetzlich Texturen brauchen — `EntityTexture` (33), `EntityAlpha` (23),
`LoadTexture` (22) sind jetzt die Spitze, also 3D-10 und 3D-11.

### Nachtrag (2026-09-09, 3D-11): Texturen

`EntityTexture` steht in **54**, `LoadTexture` in **48** der 130 mitgelieferten
Beispielprogramme — beides haeufiger als `CreateLight` (39), das den vorigen
Schritt ausgeloest hat. Neu ist `bb_texture.h` mit allen 20 Texturbefehlen; der
Shader mischt bis zu vier Lagen.

**Der eigene Roadmap-Entwurf hatte die Flags erfunden.** Dort stand „Bit0
Mipmaps, Bit1 Clamp, Bit2 Nearest". `help/commands/3d_commands/CreateTexture.htm`
fuehrt `1 Color, 2 Alpha, 4 Masked, 8 Mipmapped, 16 Clamp U, 32 Clamp V,
64 Sphere, 128 Cube, 256 VRAM, 512 High-Color` — mit dem Entwurf waere jedes
geladene Bild falsch behandelt worden, und zwar still. Dazu die Vorgabe der
Filterliste `TextureFilter "",1+8`: jede geladene Textur ist mipmapped, auch
wenn `LoadTexture` nur Flag 1 sieht, und `Graphics3D` stellt diese Vorgabe
wieder her.

**Die UV-Transformation habe ich am laufenden Original ausgemessen statt sie
aus der Doku abzuleiten — die Doku sagt dazu naemlich nichts.** `ScaleTexture`,
`PositionTexture` und `RotateTexture` beschreibt sie nur als „scales a
texture", „positions a texture", „rotates a texture". Die Richtung steht
nirgends, und alle drei sind in den Beispielen haeufig (`ScaleTexture` allein in
34 Dateien).

Das Messverfahren ist dabei der Punkt: ein Blitz3D-Programm schreibt nicht auf
stdout, also **kann man seine Ausgabe nicht vergleichen** (so steht es auch in
der Absicherungsnotiz). Ein Bild anzuschauen waere eine Einschaetzung, keine
Messung. Der Ausweg: das Testprogramm zeichnet eine Flaeche mit bekannter
Textur (linke Haelfte schwarz, rechte weiss — bzw. vier Quadranten in vier
Farben), liest die Bildzeile nach `RenderWorld` mit `ReadPixel` zurueck,
klassifiziert jeden Punkt zu einem Buchstaben und schreibt die Zeile mit
`WriteFile` in eine Textdatei. Damit ist die Antwort eine Zeichenkette, die man
gegen eine Vorhersage haelt:

    u' = ( cos a * u - sin a * v ) / u_scale - u_offset
    v' = ( sin a * u + cos a * v ) / v_scale - v_offset

Drei Befunde, die eine naheliegende Umsetzung allesamt verfehlt haette:

- **`ScaleTexture` teilt.** `ScaleTexture t,2,2` liefert eine durchgehend
  schwarze Flaeche — es wird nur `u` von 0 bis 0.5 abgetastet, die Textur wirkt
  doppelt so gross. Die naheliegende Multiplikation haette zwei Kacheln
  gezeigt, also genau das Gegenteil.
- **`PositionTexture` zieht ab.** Bei `0.25,0` erscheint zuerst die *rechte*
  Haelfte des Bildes; mit einer Addition waere es die linke gewesen.
- **`RotateTexture` dreht um den Ursprung, nicht um die Mitte.** Bei 90 und 180
  Grad sind beide Lesarten ununterscheidbar (die Differenz ist genau eine ganze
  Kachel und faellt beim Wiederholen weg) — erst 45 Grad trennt sie. Genau hier
  haette eine Stichprobe mit „schoenen" Winkeln das Falsche bestaetigt.

Die Reihenfolge ist Drehung, dann Skalierung, dann Verschiebung. Auch das ist
gemessen: `RotateTexture 90` zusammen mit `ScaleTexture 0.5,1` ergibt ein
anderes Bild, je nachdem welche Operation zuerst wirkt, und nur eine der beiden
Anordnungen deckt sich mit dem Original.

**Gegenprobe in unsere Richtung.** Dieselben acht Faelle laufen anschliessend
durch *unsere* Runtime, zurueckgelesen mit `glReadPixels`, und werden gegen die
oben gemessene Formel gerechnet — acht von acht stimmen ueberein. Der Umweg ist
noetig, weil `ReadPixel` bei uns den SDL-Renderer liest und nicht den
GL-Framebuffer; das Testprogramm sah nur Schwarz. Das ist ein eigener Befund,
kein Fehler dieser Aenderung: `LockBuffer BackBuffer()` liefert nach
`RenderWorld` nicht das gerenderte Bild.

**Was die Doku sonst noch verhindert hat:** `FreeTexture` sagt ausdruecklich
„entities already textured with it will not lose the texture". Eine Handle-Map
mit `unique_ptr` haette die Textur mitgerissen. Mit `shared_ptr` haelt die
Entity ihre eigene Referenz; das GL-Objekt stirbt mit der letzten, und der Test
prueft genau diese Regel.

**Nicht eingeloest und auch nicht behauptet:** `TextureBuffer` liefert 0 mit
einer Meldung, statt ein erfundenes Pufferhandle zurueckzugeben — das haette
still in ein fremdes Bild gezeichnet. Blendmodus 4 (Dot3) faellt auf Multiply
zurueck, weil die Lichtrichtung im Tangentenraum fehlt. Lagen ab Index 4 werden
gespeichert, aber nicht gemischt, deshalb meldet `HWTexUnits()` 4 und nicht 8.

Ein Nebeneffekt der Umstellung: ohne Licht zeichnet `RenderWorld` jetzt mit dem
TEXTURED- statt dem UNLIT-Shader. Bei `u_tex_count == 0` bleibt darin genau
`u_color` uebrig, das Bild ist also unveraendert — der Texturteil ist als
gemeinsamer GLSL-Baustein einmal vorhanden und wird in beide Shader
einkopiert, damit TEXTURED und LIT nicht auseinanderlaufen koennen.

**Wirkung:** vollstaendig uebersetzende Beispieldateien **34 → 36**, unbekannte
Befehle **138 → 124** verschieden und **2392 → 2057** Vorkommen; 14 Befehle
sind ganz verschwunden, keiner neu dazugekommen. (Gezaehlt ueber `samples`,
`tutorials` und `Games` mit `-c`; die Zahlen der frueheren Eintraege stammen aus
einer anderen Zaehlweise und sind mit diesen nicht direkt vergleichbar.)

**Abgesichert** nach der ueblichen Kette: Emittatvergleich alt/neu ueber 89
Programme (88 identisch, 0 abweichend, 1 nur vom alten Compiler abgelehnt —
der neue Test, also die Gegenprobe), Diagnosevergleich ueber 44 Negativtests
(44 gleich), `compare_reference.sh` gegen das Original ohne neue Abweichung,
und alle nicht blockierenden 3D-Tests neu gebaut und gelaufen.

---

### Nachtrag (2026-09-09, 3D-10): Aussehen der Entities

`EntityAlpha` steht in **35**, `EntityFX` in **29**, `EntityColor` in **25** der
130 mitgelieferten Beispielprogramme. Neu sind die sechs Befehle, die
Zeichenreihenfolge und die Rueckseitenentfernung.

**Der eigene Roadmap-Entwurf hatte `EntityBlend` 2 und 3 vertauscht** — dort
stand „1=Normal, 2=Additive, 3=Multiply", `EntityBlend.htm` fuehrt
`1 Alpha, 2 Multiply, 3 Add`. Jeder Laserstrahl und jedes Feuer waere
multiplikativ gezeichnet worden (also unsichtbar dunkel) und jeder Schatten
additiv. Das ist innerhalb von drei Schritten das dritte Mal, dass der eigene
Entwurf eine Zahlentabelle erfunden hat, die in der mitgelieferten Doku
danebensteht — nach den Texturflags (3D-11) und dem Lichttyp (3D-12).

**Diesmal habe ich nicht nur nachgeschlagen, sondern durchgerechnet.** 38
Faelle im laufenden Original gemessen, wieder ueber `ReadPixel` in eine
Textdatei; 34 davon durch unsere Runtime nachgestellt und mit `glReadPixels`
zurueckgelesen. **34 von 34 Bildpunkten stimmen zeichengenau ueberein.** Zwei
Befunde haetten sich anders nicht zeigen koennen:

**Geklemmt wird nach der Multiplikation mit der Entityfarbe, nicht davor.**
Farbe 255,128,0, ein volles Licht, Umgebungslicht 64,32,16 — das Original
liefert **255,144,0**. Der Gruenanteil steigt also *ueber* die eingestellten
128 hinaus, weil erst `(Licht + Umgebung) * Farbe` gerechnet und dann geklemmt
wird. Unser Shader klemmte das Licht zuerst und haette 128 geliefert. Die
Differenz ist klein genug, um beim Hinsehen durchzugehen, und gross genug, um
jede helle Szene anders aussehen zu lassen.

**Eine Szene ohne Licht ist mittelgrau, nicht weiss.** Ein weisser Wuerfel ohne
jedes Licht und ohne `AmbientLight` kommt im Original als **127,127,127**
heraus — genau die Vorgabe von `AmbientLight`, die 3D-12 schon aus der Doku
uebernommen hatte. Bei uns war er weiss, weil ohne Licht der TEXTURED-Shader
zeichnete und der das Umgebungslicht gar nicht kennt. Netze zeichnet jetzt
immer der LIT-Shader; mit `AmbientLight 0,0,0` ist der Wuerfel schwarz, mit
`255,255,255` weiss, dazwischen linear — alle drei gemessen.

Weiter gemessen und uebernommen: full-bright ignoriert **auch** das
Umgebungslicht; Rueckseiten werden entfernt (eine Kamera im Inneren eines
Wuerfels sieht dort den Hintergrund, mit `EntityFX 16` die Innenseiten);
`EntityAutoFade` rechnet `alpha = (far − Abstand) / (far − near)` mit dem
Abstand zum **Ursprung** des Entity (bei `5,10` gemessen: 1.0, 0.8, 0.6, 0.4,
0.2, 0 an den Ganzzahlpunkten); `EntityOrder > 0` zeichnet zuerst und damit
hinter allem, `< 0` zuletzt und damit vor allem.

**Drei Nebenbefunde, alle mit Zahlen belegt und als Fehler festgehalten:**

- **BUG-64: `RenderWorld : Flip` verliert den Befehl.** Unser Parser liest
  einen Bezeichner vor einem Doppelpunkt als Sprungmarke; `UpdateWorld :
  RenderWorld` erzeugt zweimal `lbl_updateworld:` und **keinen einzigen
  Aufruf**. Aufgefallen ist es nur, weil g++ ueber die doppelte Marke
  stolperte — bei einem einzigen Vorkommen waere der Befehl lautlos
  verschwunden. Das Original kennt die Schreibweise gar nicht: `meinlabel:`
  ergibt dort `Function 'meinlabel' not found`, Sprungmarken sind
  ausschliesslich `.name`. Acht der 130 Beispielprogramme sind betroffen. Der
  eigene Test umgeht die Form bewusst und sagt im Kopf, warum.
  **`compare_samples.sh` kann diese Klasse nicht sehen**, weil dort nur das
  Frontend laeuft und das Frontend die Programme annimmt.
- **BUG-65: die Texturkoordinaten von `CreateCube` sind gespiegelt.** Derselbe
  Aufbau in beiden Systemen, Textur mit schwarzer linker Haelfte: das Original
  zeigt links schwarz, wir links rot. Die Geometrie stimmt — ein Wuerfel bei
  `x=+2` erscheint in beiden Systemen rechts —, es ist die UV-Belegung der
  Flaeche.
- **BUG-66: `EntityShininess` rechnet je Bildpunkt, das Original je Vertex.**
  Im Original wird der Glanzpunkt mit **steigendem** Shininess *dunkler*
  (64 → 93 → 91 → 75 bei 0, 0.25, 0.5, 1), obwohl die Flaeche genau in die
  Spiegelrichtung zeigt. Das geht nur, wenn je Vertex gerechnet und
  interpoliert wird. Bis zu diesem Schritt tat `EntityShininess` bei uns
  ueberhaupt nichts, weil `u_shininess` nie hochgeladen wurde.

**Wirkung:** unbekannte Befehle **124 → 118** verschieden und **2057 → 1814**
Vorkommen; sechs Befehle sind verschwunden, keiner neu. Vollstaendig
uebersetzende Beispieldateien bleiben bei 36 — die 35 Dateien mit `EntityAlpha`
brauchen zusaetzlich Netze aus Dateien und das Brush-System.

**Abgesichert:** Emittatvergleich alt/neu ueber 90 Programme (89 identisch, 0
abweichend, 1 nur vom alten Compiler abgelehnt — der neue Test, also die
Gegenprobe), 44 Negativtests mit gleicher Diagnose, `compare_reference.sh`
ohne neue Abweichung, alle nicht blockierenden 3D-Tests neu gebaut und
gelaufen.

---

### Nachtrag (2026-09-09, BUG-64): der verschluckte Befehl vor dem Doppelpunkt

Beim Schreiben des 3D-10-Tests scheiterte `UpdateWorld : RenderWorld` an einer
Meldung von g++: `duplicate label 'lbl_updateworld'`. Der Parser las einen
**Bezeichner vor einem Doppelpunkt als Sprungmarke** — aus zwei Befehlen wurden
zwei gleichnamige Marken und **kein einziger Aufruf**.

**Das Schlimme daran ist nicht der Fehler, sondern wie er sich zeigte.** Die
Meldung kam nur, weil dieselbe Marke zweimal in einer Funktion stand. Bei einem
einzigen Vorkommen uebersetzte das Programm anstandslos und der Befehl war weg.
`compare_samples.sh` konnte diese Klasse nie sehen: dort laeuft nur das
Frontend, und das Frontend nahm die Programme an.

**Das Original kennt die Schreibweise gar nicht.** Gemessen: `meinlabel:` in
eigener Zeile ergibt dort `Function 'meinlabel' not found` — der Bezeichner ist
ein Aufruf, der Doppelpunkt der Anweisungstrenner. Sprungmarken schreibt
Blitz3D ausschliesslich als `.name`; `.meinlabel` mit `Goto meinlabel` wird
angenommen. Der Zweig im Parser war also von Anfang an eine Zutat, die es in
der Sprache nicht gibt — eingebaut in Milestone 11 und im DEVLOG damals sogar
ausdruecklich als „colon-label" vermerkt.

Der Zweig ist ersatzlos entfernt. Ein Bezeichner vor `:` faellt jetzt in den
Aufrufpfad, dessen Argumentschleife ohnehin am `:` endet — ein parameterloser
Befehl vor dem Trenner ergibt damit einen Aufruf ohne Argumente.

**Was das an echtem Fremdcode bewirkt.** Acht der 130 mitgelieferten
Beispielprogramme enthalten die Form; fuenf davon kommen bei uns durchs
Frontend, und bei allen fuenf faellt die falsche Marke weg. Das schoenste
Beispiel steht in `samples/AGore/start.bb`:

```blitzbasic
If cnt=0 Print "No 3D Graphics modes detected.":WaitKey:End
```

Vorher wurde `WaitKey` verschluckt — die Fehlermeldung waere also unlesbar
durchgeblitzt und das Programm sofort beendet, was genau wie ein Absturz
aussieht. Jetzt steht `bb_WaitKey();` im Emittat.

**`tests/test_goto.bb` hat die Fehlannahme gedeckt.** Die Datei pruefte die
Doppelpunktform ausdruecklich („Test 1: Goto + colon-label on same line") und
stand deshalb schon laenger in der Fundliste von `compare_reference.sh` — dort
allerdings wegen `Goto .done`, sodass der eigentliche Grund nie auffiel. Sie
ist jetzt auf `.label` umgeschrieben und damit gueltiges Blitz3D; ihre
`.expected` bleibt unveraendert, das Verhalten ist also dasselbe.

**Nicht mitgeaendert:** `Goto .label` und `Gosub .label` nehmen wir weiterhin
an, obwohl das Original sie mit `Expecting identifier` ablehnt. Das ist „wir
laxer" — eine Entscheidung, kein stilles Falschergebnis, und der neue
`test_goto.bb` sagt das im Kopf.

**Abgesichert:** Emittatvergleich alt/neu ueber 91 Programme — **90 identisch,
1 abweichend**, und das eine ist der neue Test. 44 Negativtests mit woertlich
gleicher Diagnose. Alle **71 `.expected` und 33 `.expected_err` gruen**.
`compare_reference.sh`: „wir nehmen an, das Original lehnt ab" **8 → 7**,
gleiches Urteil **123 → 125**. Gegenprobe: `tests/test_bug64_colon_call.bb`
scheitert mit dem alten Compiler und wird vom Original angenommen.

---

### Nachtrag (2026-09-09, 3D-13, Teil 1): die Meshbefehle ohne Loader

**Die Ueberschrift dieses Roadmap-Punktes hiess „Mesh Loading (.b3d)" — und
das war die falsche Datei.** Vor dem ersten Handgriff gezaehlt: die 32
Beispielprogramme mit `LoadMesh`/`LoadAnimMesh` laden **39-mal `.x` und
20-mal `.3ds`**, und **kein einziges Mal `.b3d`**. In der ganzen
Blitz3D-Installation liegen 51 `.x`, 38 `.3ds` und 6 `.md2` — aber **null**
`.b3d`. Ein `.b3d`-Loader waere gegen keine einzige echte Datei pruefbar
gewesen; ich haette meine Testdaten selbst erzeugen muessen und damit genau
das gemessen, was ich vorher hineingeschrieben habe.

Das ist der vierte Fall in vier Schritten, in dem der eigene Roadmap-Entwurf
etwas behauptet, was an der Installation nachprüfbar nicht stimmt — nach den
Texturflags, dem Lichttyp und den EntityBlend-Modi jetzt das Dateiformat.
**Die Tabellen und Ueberschriften in `ROADMAP3D.md` sind Entwuerfe, keine
Referenz.**

Nach Ruecksprache: `.3ds` zuerst (starrer Chunk-Walk, 38 Dateien zum
Gegenpruefen), und in diesem Schritt erst einmal die Haelfte von 3D-13, die
gar keinen Loader braucht — `ScaleMesh` (22 Dateien), `FlipMesh` (19),
`CreateMesh` (17), `UpdateNormals` (12), `FitMesh` (8), dazu `RotateMesh`,
`PositionMesh`, `AddMesh`, `CopyMesh`, `CountSurfaces`, `LightMesh`,
`MeshesIntersect` und `PaintMesh`. Diese Befehle arbeiten auf den Vertices,
nicht auf der Transformation der Entity, und sind mit den vorhandenen
Primitiven heute schon messbar.

**Gemessen statt abgeleitet — und diesmal liefert die Sprache selbst Zahlen.**
`MeshWidth/Height/Depth` gibt es in beiden Systemen, die Ausmasse nach jedem
Eingriff sind also direkt vergleichbar, ohne den Umweg ueber Bildpunkte. Wo
es doch aufs Bild ankam (FlipMesh, LightMesh, die Lage nach FitMesh), kam
`ReadPixel` bzw. `glReadPixels` dazu. **48 von 48 vergleichbaren Faellen
stimmen zeichengenau ueberein.**

Was die Messung entschieden hat:

- **`ScaleMesh` ist kumulativ.** Zweimal `2` ergibt den achtfachen Wuerfel
  (2.0 → 4.0 → 8.0), nicht den doppelten. „Scales all vertices by the
  specified scaling factors" laesst beides zu.
- **`FitMesh` setzt die Mindestecke der Box auf `x,y,z`,** nicht deren Mitte.
  `FitMesh m,0,0,0,2,2,2` legt den Wuerfel auf [0,2]³ — im Bild steht er
  danach rechts oben, nicht in der Mitte. Mit `uniform` gilt der **kleinste**
  der drei Faktoren: ein 2×2×2-Wuerfel in eine Box 4×2×6 gepasst bleibt
  2×2×2.
- **`FlipMesh` kehrt auch die Normalen um.** Die Doku spricht nur von
  Dreiecken („Flips all the triangles in a mesh"). Mit abgeschalteter
  Rueckseitenentfernung wird die vorher weisse Flaeche danach schwarz — das
  geht nur, wenn die Normale mitkippt.
- **`LightMesh`** war der interessanteste Fall. Die Doku sagt nur „performs a
  'fake' lighting operation" und nennt kein Gesetz. Eine Reihe ueber die
  Reichweiten 1 bis 12 ergab 69, 139, 208, dann 255 — perfekt linear in der
  Reichweite. Zusammen mit dem Abstand (3.3166 vom Licht zur Flaeche) und dem
  Kosinus zwischen Normale und Lichtrichtung (0.9045) loest sich das zu

      Vertexfarbe += Farbe · (range / Abstand) · max(N·L, 0)

  auf: 255 · (1/3.3166) · 0.9045 = 69.6 → **69**. Die Gegenprobe mit einem
  ganz anderen Aufbau (Licht bei z=−12, Reichweite 10) sagt 228.1 → **228**,
  gemessen 228. Ohne Reichweite — oder mit Reichweite 0 — wird gleichmaessig
  addiert, ohne Abstand und ohne N·L; genau deshalb funktioniert das in der
  Doku empfohlene `LightMesh mesh,-255,-255,-255` als Ruecksetzer.
- **Die Vertexfarbe liegt im Original als Byte vor.** Bei Reichweite 1 steht
  dort 69 und nicht 70 — abgeschnitten, nicht gerundet. Ohne diese
  Quantisierung wichen fuenf Messpunkte um genau eins ab; mit ihr stimmen
  alle. Es lohnt sich, solche Einser ernst zu nehmen: sie sind der Unterschied
  zwischen „ungefaehr richtig" und „nachgerechnet".
- **`AddMesh` fasst in die vorhandene Flaeche zusammen**, die Flaechenzahl
  bleibt 1, und die Quelle bleibt erhalten. Ein zweites `AddMesh` mit
  versetztem Netz wuchs die Zielbreite von 10 auf 30 — die Geometrie kommt
  also wirklich dazu und wird nicht ersetzt.

**Offen und bewusst nicht behauptet:** `ScaleMesh` laesst die Normalen in
Ruhe. Die Doku nennt `UpdateNormals` ausdruecklich als das Mittel, sie nach
solchen Eingriffen richtigzustellen — eine automatische Korrektur waere eine
Zutat, die das Original nicht hat.

**Ein Nebenbefund beim Testschreiben:** `If MeshWidth(m) = 6` schlaegt nach
einer Drehung fehl, obwohl `Print MeshWidth(m)` „6" ausgibt. `cos(90)` ist im
Gleitkomma nicht genau 0, und `%g` versteckt den Rest. Der Test vergleicht
dort jetzt mit einer Toleranz und sagt im Kommentar, warum — sonst haette
irgendwann jemand die Zeile fuer kaputt gehalten statt fuer genau.

**Wirkung:** vollstaendig uebersetzende Beispieldateien **36 → 37**,
unbekannte Befehle **118 → 109** verschieden und **1814 → 1691** Vorkommen;
neun Befehle sind verschwunden, keiner neu.

**Abgesichert:** Emittatvergleich alt/neu ueber 92 Programme (91 identisch, 0
abweichend, 1 nur vom alten Compiler abgelehnt — der neue Test), 44
Negativtests mit gleicher Diagnose, alle **72 `.expected` gruen**,
`compare_reference.sh` ohne neue Abweichung (gleiches Urteil 125 → 126). Das
Original nimmt die neue Testdatei an.

---

### Nachtrag (2026-09-09, 3D-13, Teil 2): der .3ds-Loader

`bb_loader.h` liest jetzt `.3ds`. Das Format ist ein Baum aus Chunks mit
2 Byte Kennung und 4 Byte Laenge — die Struktur ist in einem Nachmittag
gelesen. **Interessant war nicht das Parsen, sondern alles, was danach kommt:
sechs Entscheidungen, die man plausibel anders trifft und die sich nie von
selbst melden.** Jede einzelne ist am laufenden Original ausgemessen.

**1. Die Achsen tauschen y und z.** 3D Studio ist rechtshaendig mit z nach
oben, Blitz3D linkshaendig mit y nach oben. `MeshWidth/Height/Depth` gibt es
in beiden Systemen, also liess sich das an zehn Dateien direkt ablesen:
`fighter.3ds` misst roh 372.47 / 529.94 / 152.39 und wird als
372.472 / 152.386 / 529.937 gemeldet.

**2. Das lokale Koordinatensystem wird nicht angewandt.** Sechs der zehn
Dateien haben in Chunk 0x4160 keine Einheitsmatrix — `wcrate1.3ds` traegt
eine Skalierung von 13.583, `fighter.3ds` eine von 0.257, `rock.3DS` eine
Drehung um rund 6 Grad. Die gemeldeten Ausmasse entsprechen trotzdem genau
den **rohen** Vertexkoordinaten. Wer die Matrix anwendet, macht die Kiste um
das Dreizehnfache zu gross.

**3. Der Drehpunkt aus dem Keyframe-Abschnitt wird abgezogen — und er steht
in lokalen Einheiten.** Das war der Befund, der am laengsten gedauert hat.
Die Kiste stand bei uns 20 Einheiten zu hoch, und die Silhouette zeigte es
sofort; die Ausmasse dagegen nicht, denn eine Verschiebung aendert an
`MeshWidth` nichts. Die naheliegende Erklaerung — "das Original zentriert das
Netz" — ist **falsch**: `rock.3DS` hat ein AABB-Zentrum von (−9.7, −18.2,
6.0) und wird nicht verschoben, ebenso `solid01.3ds` und die vier Teile von
`rocket.3ds`. Was sie gemeinsam haben, ist ein Drehpunkt von 0.

Richtig ist: Verschiebung = Achsenmatrix · Drehpunkt + Ursprung. Fuer
`wcrate1.3ds` ergibt das 13.583 · (0.016, −0.032, 1.484) + (0.085, 0.152,
0.03) = (0.301, −0.279, 20.188) — und genau (0.301, −0.279, 20.188) ist das
AABB-Zentrum dieser Datei. Bei `fighter.3ds` genauso: 0.257 · (0, 0, 140.607)
+ (0, 1.603, 0.111) = (0, 1.603, 36.247) gegen ein Zentrum von (0, 1.603,
36.245). **Damit wird die Achsenmatrix doch gebraucht — nicht fuer die
Vertices, sondern um den Drehpunkt in dieselben Einheiten zu bringen.** Zwei
Befunde, die einander auf den ersten Blick widersprechen und zusammen erst
Sinn ergeben.

**4. Der Umlaufsinn kehrt sich um.** Der Achsentausch dreht die Haendigkeit,
also laeuft ein Dreieck, das in der Datei von aussen gegen den Uhrzeigersinn
liegt, danach mit dem Uhrzeigersinn. Ohne Vertauschen zweier Indizes zeigt
die Rueckseitenentfernung die **Rueckseite** des Modells. Bei einer
geschlossenen Kiste sieht die Silhouette dabei voellig unveraendert aus — es
faellt nur auf, weil die Helligkeit ueber die Flaeche seitenverkehrt verlief:
das Original von 68 nach 28, wir von 41 nach 64.

**5. Die v-Koordinate laeuft andersherum.** Gemessen, indem die Textur der
Kiste durch ein Bild aus vier farbigen Quadranten ersetzt wurde: das Original
zeigt die linke obere Ecke des Bildes an der linken oberen Ecke der Flaeche,
wir zeigten die untere. Danach stimmen alle vier Ecken.

**6. Die Materialfarbe gilt nur ohne Textur.** `rocket.3ds` hat vier
texturlose Materialien und erscheint im Original genau in deren Farben
(255,191,0 / 191,191,255 / 236,42,42 / 255,255,255). Die texturierte Kiste
dagegen traegt die Diffusfarbe 191,191,191 und kommt trotzdem mit 254 heraus.
Wer die Farbe immer anwendet, dunkelt jede texturierte Flaeche um 25 Prozent
ab — sichtbar, aber leicht fuer "so sieht das Modell eben aus" zu halten.

**Dazu zwei Regeln zur Aufteilung in Flaechen.** Zweiseitige Materialien
(0xA081) werden ohne Rueckseitenentfernung gezeichnet — bei `rocket.3ds`
sahen wir sonst durch die weisse Aussenhaut auf das blaue Innenteil, was sich
als 322 statt 196 blauen Bildpunkten zeigte. Und Flaechen entstehen je
Brush, wobei **gleiche Brushes zusammenfallen**: `ufo.3ds` hat drei
Materialien und meldet **zwei** Flaechen, solange die beiden Texturdateien
fehlen — dann sind zwei Brushes schlicht "weiss ohne Textur". Legt man die
Texturen daneben, meldet dieselbe Datei **drei**. Beide Faelle stimmen bei
uns, und der zweite ist die Gegenprobe zum ersten.

**Dafuer mussten die Texturen von der Entity an die Flaeche wandern.** Im
Original haengt das Aussehen am Brush, und ein Netz aus einer Datei hat
mehrere davon. `bb_MeshData_` hat jetzt einen `bb_Brush_` mit Farbe,
Deckkraft, Glanz, Zweiseitigkeit und Texturlagen; `EntityTexture` schreibt in
alle Flaechen, so wie das Original alle Brushes eines Netzes setzt. Das ist
zugleich die Grundlage fuer das Brush-System in 3D-15.

**Wie gut es stimmt.** Zehn echte Modelldateien aus der Installation, 443
Byte bis 15 kB, 2 bis 409 Dreiecke, 1 bis 4 Flaechen:

- `MeshWidth/Height/Depth`, `CountSurfaces` und `TrisRendered` sind bei allen
  zehn **gleich**. Der einzige Textunterschied ist die Zahlenausgabe: fuer
  `rock.3DS` druckt das Original 26.1755, wir 26.1754 — der genaue Wert ist
  26.1754479, **unsere Rundung ist die richtige**.
- Die Silhouetten decken sich: Begrenzungsrechtecke bis auf einen
  Rasterschritt von 4 px, belegte Rasterpunkte um 0 bis 3 von 150 bis 680.
  Der groesste Ausreisser (`wcrate1.3ds`, 400 gegen 440) verschwindet
  vollstaendig, sobald das Abtastraster um 2 px versetzt wird — die
  Kistenkante liegt genau auf den Rasterpunkten. Das ist eine Rundung im
  Messverfahren, kein Unterschied im Bild, und es lohnt sich, so etwas
  nachzupruefen statt es als "fast gleich" abzuhaken.
- Bei der texturierten und beleuchteten Kiste stimmen je nach Blickwinkel 10
  bis 16 von 25 Rasterpunkten auf den Kanal genau, die mittlere Abweichung
  liegt bei 2.6 bis 11.3 von 255. Der Rest geht auf Texturfilterung und
  darauf, dass das Original je Vertex beleuchtet und wir je Bildpunkt
  (BUG-66) — beides bekannt und festgehalten.

**Die Testdatei ist selbst erzeugt.** `tests/assets/test_box.3ds` entsteht
aus einem kleinen Schreiber: zwei Quader, zwei Materialien, in
3DS-Koordinaten 12×4×6. Damit stehen die erwarteten Zahlen fest, statt aus
einer fremden Datei abgelesen zu sein — und das Original meldet fuer dieselbe
Datei dieselben Werte (w=12, h=6, d=4, zwei Flaechen). Dazu eine bewusst
kaputte Datei mit richtiger Endung, deren Hauptchunk eine Laenge weit hinter
dem Dateiende angibt: der Parser muss dort in den Grenzpruefungen
haengenbleiben und 0 liefern.

**Nicht eingeloest und nicht behauptet:** `.x` (39 der 59 Ladevorgaenge in
den Beispielen, davon 36 von 51 Dateien im Textformat) und `.b3d`; Hierarchie
und Animation, weshalb `LoadAnimMesh` einmal meldet, dass es wie `LoadMesh`
laedt; Umgebungskarten.

**Wirkung:** vollstaendig uebersetzende Beispieldateien **37 → 40**,
unbekannte Befehle **109 → 107** verschieden und **1691 → 1647** Vorkommen.

**Abgesichert:** Emittatvergleich alt/neu ueber 93 Programme (92 identisch, 0
abweichend, 1 nur vom alten Compiler abgelehnt — der neue Test), 44
Negativtests mit gleicher Diagnose, alle **73 `.expected` gruen**,
`compare_reference.sh` ohne neue Abweichung (gleiches Urteil 126 → 127). Das
Original nimmt die neue Testdatei an.

---

### Nachtrag (2026-09-09, 3D-13): der Quelltext haette es schneller gesagt

**Blitz3D ist seit 2014 offen** (zlib-Lizenz, `blitz-research/blitz3d`), und
das README dieses Projekts nennt den Quelltext ausdruecklich als
Verhaltensreferenz: "when a question comes up about what the language actually
does, the answer is read out of the original compiler rather than guessed at".
Beim `.3ds`-Loader habe ich das nicht getan und die sechs Regeln stattdessen
aus Messungen rekonstruiert. Das hat gehalten — aber teuer, und an einer
Stelle nicht ganz.

**Zwei der sechs standen sogar in der mitgelieferten Doku**, eine Datei neben
`LoadMesh.htm`, die ich gelesen hatte. `LoaderMatrix.htm`:

```
LoaderMatrix "x",1,0,0,0,1,0,0,0,1    ; no change in coord system
LoaderMatrix "3ds",1,0,0,0,0,1,0,1,0  ; swap y/z axis'
```

Das ist der Achsentausch wortwoertlich — und der Umlaufsinn gleich mit, denn
eine Vertauschungsmatrix hat Determinante −1. Ich hatte `LoadMesh.htm` gelesen
und aufgehoert, statt das Verzeichnis nach "Loader" zu durchsuchen.

**Vier weitere bestaetigt der Quelltext, drei davon woertlich:**

| Befund | `blitz3d/loader_3ds.cpp` |
|---|---|
| v umkehren | `v.tex_coords[0][1]=1-uv[1];` |
| Farbe nur ohne Textur | `mat.setTexture(...); mat.setColor( Vector(1,1,1) );` |
| Umlaufsinn | `if( conv_tform.m.i.cross(conv_tform.m.j).dot(conv_tform.m.k)<0 ) flip_tris=true;` |
| Drehpunkt | `pivot=conv_tform*pivot; ... mesh->transform( -pivot );` |
| Flaechen je Brush | `map<Brush,Surf*> brush_map;` in `meshloader.cpp` |

**Und einer war falsch hergeleitet.** Zum lokalen Koordinatensystem (0x4160)
hatte ich aus den Messungen geschlossen: "wird ignoriert". Der Quelltext zeigt
etwas anderes — es wird zur Weltmatrix des Netzes, die Vertices werden mit dem
Kehrwert in den lokalen Raum geholt, und beim Einschmelzen kommt es wieder
heraus:

```cpp
mesh->setWorldTform( tform );
Transform inv_tform=-tform;
for( ... ) v.coords=inv_tform * v.coords;
```

Netto stehen die rohen Koordinaten da, mein Ergebnis stimmte also. Die
Verschiebung durch den Drehpunkt aber nicht ganz: ausmultipliziert ergibt die
Kette `v' = L·(v − M·pivot)`, wobei M **nur** der Dreh- und Skalenanteil von
0x4160 ist. Der Translationsanteil hebt sich zwischen Hin- und Rueckweg auf —
meine gemessene Fassung hatte ihn addiert. Auf den zehn Testdateien macht das
0.03 bis 0.15 Einheiten aus, an der Kamera ein Viertelpixel. **Meine
Silhouettenmessung konnte das nicht sehen, und sie hat es auch nicht
gesehen.** Es ist jetzt korrigiert.

**Was daraus folgt, ist keine neue Regel, sondern die vorhandene richtig
angewandt.** Das Absicherungsverfahren dieses Projekts sagt: der Quelltext
sagt, was gemeint ist, das laufende Original sagt, was herauskommt, und wo
beides zu haben ist, gilt die Messung. Richtig — aber das ist eine Regel fuer
den **Konflikt**, nicht fuer die Reihenfolge. Die Reihenfolge muss sein:
zuerst lesen, was dasteht, dann messen, ob es stimmt. Messen allein liefert
eine Formel, die zu den Datenpunkten passt; ob es *die* Formel ist, sagt nur
der Quelltext. Genau der Unterschied kostet hier einen Translationsanteil.

**Konkret hinzugekommen ist `LoaderMatrix`** — ein Befehl, den ich gar nicht
hatte. Der Achsentausch ist jetzt kein Sonderfall des `.3ds`-Lesers mehr,
sondern eine Matrix je Dateiendung mit den dokumentierten Vorgaben; der
Umlaufsinn folgt ihrer Determinante, so wie im Original. Ein Programm kann die
Matrix damit aendern, und der Test haelt beide Faelle fest: mit der
Einheitsmatrix wird aus 3DS 12×4×6 ein Netz von w=12, h=4, d=6, mit der
Vorgabe w=12, h=6, d=4.

**Gegenprobe nach der Korrektur:** dieselben zehn Modelldateien, diesmal bei
voller Aufloesung statt im 4-Pixel-Raster und mit weit gestellter
Kamerareichweite. Alle zehn Silhouetten stimmen auf ein Pixel im
Begrenzungsrechteck, die Pixelzahlen auf 0.3 bis 3 Prozent. Dabei fiel auch
auf, dass zwei fruehere Ausreisser Messfehler waren: `wcrate1.3ds` lag mit
seiner Kante genau auf den Rasterpunkten, und `fighter.3ds` ragte bei
Kameraabstand 1054 durch die voreingestellte hintere Schnittebene von 1000.
Beides verschwindet, sobald man richtig misst — **auch das Messverfahren
gehoert geprueft**, nicht nur das Ergebnis.

**Abgesichert:** Emittatvergleich gegen den Stand vor dieser Korrektur ueber
93 Programme (92 identisch, 1 nur vom aelteren Compiler abgelehnt — der um
`LoaderMatrix` erweiterte Test), 44 Negativtests mit gleicher Diagnose, alle
73 `.expected` gruen, `LoaderMatrix` gegen `blitzcc +k` ohne Abweichung.

---

### Nachtrag (2026-09-09, 3D-13, Teil 3): der .x-Leser

`.x` ist das haeufigste Modellformat der Beispiele: sie laden 36 verschiedene
Dateien, davon **28 im Textformat und 7 binaer**. Dieser Schritt bringt das
Textformat.

**Diesmal zuerst der Quelltext** — und der erste Blick hat gleich die Groesse
der Aufgabe verschoben: `blitz3d/loader_x.cpp` **parst .x gar nicht selbst**.
Es uebergibt die ganze syntaktische Schicht an `d3dxof.dll`
(`DirectXFileCreate`, `RegisterTemplates(D3DRM_XTEMPLATES)`,
`CreateEnumObject`) und laeuft danach nur den Objektbaum nach GUIDs ab. Zu
uebernehmen gibt es also die **Bedeutung**, nicht den Parser; den Tokenizer,
die Vorlagen, die Verweise und die geschachtelten Objekte muss man selbst
bauen. Das ist gut zu wissen, **bevor** man anfaengt.

**Was der Quelltext an Bedeutung hergab** — jedes davon eine Stelle, an der
ein selbstgeschriebener Leser plausibel danebengreift:

- `MeshTextureCoords` und `MeshNormals` gelten **nur, wenn ihre Anzahl genau
  der Vertexzahl entspricht** (`if( num_coords==num_verts )`), und liegen
  dann in Vertexreihenfolge vor. Der eigene Flaechenindex von `MeshNormals`
  wird gar nicht ausgewertet. Es ist also **nichts zu verschweissen** —
  anders als bei den meisten .x-Lesern, die Position, Normale und UV zu
  Tripeln zusammenfassen muessen.
- **Die v-Koordinate wird hier nicht gespiegelt.** Bei `.3ds` rechnet
  dasselbe Programm `1-uv[1]`, bei `.x` uebernimmt es `tu`/`tv` unveraendert.
  Wer die Regel vom einen Format aufs andere uebertraegt, dreht jede Textur
  um.
- Vielecke werden als **Faecher ab der ersten Ecke** zerlegt, und ob dabei
  die beiden hinteren Ecken tauschen, haengt an der Determinante der
  Loadermatrix — dieselbe Regel wie bei `.3ds`.
- `Material` ist Farbe plus Deckkraft, dann Glanz und zwei weitere Farben.
  Die **Deckkraft gilt nur, wenn sie ungleich 0 ist** (`if( data[3] )`), und
  ein Texturname setzt die Farbe auf weiss zurueck.
- `MeshMaterialList` fuehrt seine Materialien wahlweise inline **oder als
  Verweis** auf ein frueher benanntes `Material`.

**Eine Frage blieb offen und wurde gemessen:** ob `LoadMesh` die
Frame-Matrizen anwendet. Die Doku sagt nur, die Hierarchie werde „ignoriert",
und im Quelltext ist die Antwort ueber `setLocalTform`, das Einschmelzen und
`MeshLoader` verteilt. Zwei Rechnungen gegen das laufende Original
entscheiden es in einem Schritt: `plane.x` misst mit angewandten Frames
19.8346 x 4.4206 x 13.5502 und ohne sie 65.07 x 14.50 x 44.46 — das Original
meldet 19.8346 x 4.42056 x 13.5502. Ueber alle Textdateien gerechnet passt
die ganze Kette bei 80 Faellen, die blosse unmittelbare Matrix nur bei 74.
Die Matrix steht zeilenweise und wirkt auf Zeilenvektoren, das Kind vor dem
Elternteil.

**Der Abgleich lief dann ueber alle 36 Textdateien der Installation**, jede
in ihrem eigenen Verzeichnis, damit Texturen beidseitig gleich aufgeloest
werden. Der erste Durchgang: 17 von 36 gleich. Was die restlichen 19
auseinandergetrieben hat, war jedes Mal etwas anderes, und jedes Mal etwas,
das man am Bild nicht gesehen haette:

1. **Dieselbe Texturdatei bekam bei jedem Material ein neues Handle.** Damit
   unterschieden sich zwei sonst gleiche Brushes allein durch eine Zahl und
   wurden nicht zusammengefasst — `mak_robotic.x` meldete 38 Flaechen statt
   3. Das Original haelt dafuer einen Texturzwischenspeicher
   (`blitz3d/cachedtexture.cpp`). Ein gemeinsamer Zwischenspeicher je
   Ladevorgang hat 15 Dateien auf einen Schlag in Ordnung gebracht.
2. **Ein Verweis steht in eigenen Klammern: `{x3dc_0}`.** Wer die oeffnende
   Klammer nur ueberliest, laesst die schliessende das **umgebende** Objekt
   beenden — alles danach faellt weg. `ship.x` fand so ein Material statt
   vier, `747.X` verlor ein ganzes Netz. Das ist der Fehler, den man beim
   Selberschreiben eines Parsers macht und beim Benutzen von `d3dxof.dll`
   nicht machen kann.
3. **Namensregister nur fuer Materialien.** In `interior.X` heissen Frames
   und Materialien gleich (`x3dc_1`, `x3dc_2`, ...). Ein gemeinsames Register
   behaelt den ersten Treffer — das war der Frame, und die Materialverweise
   liefen ins Leere.
4. **Die Vorlage heisst dort `TextureFileName` mit grossem N**, in anderen
   Dateien `TextureFilename`. Das Original vergleicht GUIDs, dem ist die
   Schreibweise egal; buchstabengenau verglichen findet man in `interior.X`
   keine einzige Textur. Vorlagennamen werden jetzt ohne Ruecksicht auf
   Gross- und Kleinschreibung verglichen.

**Danach: 35 von 36 gleich.** Die letzte Abweichung war `plane.x` mit 3
statt 4 Flaechen — und die Ursache liegt nicht im Loader. `F15.bmp` ist ein
**RLE8-komprimiertes** BMP, das `stb_image` nicht liest; alle Materialien mit
fehlgeschlagener Textur fallen zu einem Brush zusammen. Mit einer
unkomprimiert gespeicherten Kopie derselben Textur meldet dieselbe Datei 4.
**Damit stimmen alle 36.** Die BMP-Luecke steht als BUG-67 — sie gehoert zu
3D-11, nicht hierher, und sie hat zwei Gesichter: sichtbar ein
unbeschriftetes Modell, unsichtbar eine falsche Flaechenaufteilung.

**Die Testdatei ist wieder selbst erzeugt** (`scripts/make_x_asset.py`): zwei
Vierecke, das zweite in einem Frame um +10 in x verschoben, zwei Materialien
— eines inline, eines als Verweis — und die Vorlage absichtlich als
`TextureFileName` geschrieben. Damit trifft sie genau die vier Stellen von
oben. Erwartet und gemessen: w=11, h=2, d=0, zwei Flaechen, vier Dreiecke,
im Original wie bei uns.

**Nicht eingeloest und nicht behauptet:** das Binaerformat (7 der 36
geladenen Dateien) — `LoadMesh` sagt das jetzt ausdruecklich, statt still 0
zu liefern. Ebenso Hierarchie und Animation sowie `.b3d`.

**Abgesichert:** Emittatvergleich ueber 93 Programme (92 identisch, 1 nur vom
aelteren Compiler abgelehnt — der erweiterte Test), 44 Negativtests mit
gleicher Diagnose, alle 73 `.expected` gruen. Die Zahlen ueber die
Beispielprogramme aendern sich nicht: `LoadMesh` gab es schon, neu ist, dass
36 weitere Modelldateien tatsaechlich laden.

---

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

### Korrektur (2026-09-07, BUG-38)
`For Each var.Type` war eine Erfindung dieses Projekts — Blitz3D kennt die Form nicht.
Der `FOR`-Zweig von `parseStmtSeq` liest erst die Variable, dann `=`, dann `EACH`; die
Schreibweise heisst dort `For var[.Type] = Each Type`. Der Parser nahm bis dahin
ausschliesslich die erfundene Form an und lehnte die Referenzform ab, weshalb kein
echtes Blitz3D-Programm mit `For Each` uebersetzte. Alles, was oben ueber den Emitter
und die loeschsichere Schleife steht, gilt unveraendert — nur die Schreibweise davor
ist eine andere. `parseForEach()` heisst jetzt `parseForEachOldForm()` und dient nur
noch dazu, die alte Form mit einer Meldung abzulehnen, die die richtige nennt.
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
  *(Die Doppelpunktform ist am 2026-09-09 mit BUG-64 wieder entfernt worden: das
  Original kennt sie nicht, und sie verschluckte den Befehl in `RenderWorld : Flip`.)*
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
