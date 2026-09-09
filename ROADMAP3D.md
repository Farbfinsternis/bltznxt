# BlitzNext 3D Roadmap

Detaillierter Implementierungsplan für Phase L–U (M47–M70) der Hauptroadmap.
Technologie: **OpenGL 3.3 Core Profile** + SDL3.

---

## Architektur-Übersicht

```
bb_graphics2d.h   ←→   SDL_Renderer   (2D: Plot, Line, Rect, Text, Image…)
bb_graphics3d.h   ←→   OpenGL 3.3     (3D: Meshes, Cameras, Lights…)
bb_sdl.h          ←→   SDL3 Window    (gemeinsames Fenster, Events, Flip)
```

### 2D + 3D Koexistenz

Blitz3D erlaubt beide Modi gleichzeitig. Das typische Pattern ist:

```blitzbasic
While Not KeyDown(1)
    UpdateWorld
    RenderWorld          ; 3D-Szene in GL-Backbuffer
    Text 0,0,"Score: "+s ; 2D-HUD über der 3D-Szene
    Flip
Wend
```

**Implementierungsstrategie:**

1. `Graphics3D` erstellt das Fenster mit `SDL_WINDOW_OPENGL`-Flag und initialisiert
   einen OpenGL 3.3 Core Context via `SDL_GL_CreateContext`.
   Der `SDL_Renderer` wird weiterhin auf demselben Fenster erstellt —
   SDL3 unterstützt SDL_Renderer + raw-GL auf demselben Window.

2. **`RenderWorld`**:
   - `SDL_RenderFlush(bb_renderer_)` → flush pending 2D-Hintergrund-Draws
   - `SDL_GL_MakeCurrent(bb_window_, bb_gl_ctx_)` → GL aktiv
   - `glViewport` / `glClear` gemäß CameraClsMode
   - Alle sichtbaren Entities rendern (Meshes, Sprites…)
   - GL-State danach in SDL3-kompatiblen Zustand zurücksetzen

3. **2D nach RenderWorld** (z. B. HUD): SDL_Renderer-Calls funktionieren wie immer.

4. **`Flip`**: erkennt `bb_gl_active_`-Flag →
   - `SDL_RenderFlush` für finale 2D-Draws
   - `SDL_GL_SwapWindow(bb_window_)` statt `SDL_RenderPresent`

5. **`CameraClsMode(cam, False, True)`** = kein `GL_COLOR_BUFFER_BIT` clearen →
   2D-Hintergrund bleibt sichtbar, nur Z-Buffer wird gecleart.

### OpenGL Loader

GLAD2 (OpenGL 3.3 Core, single-header) wird unter
`src/thirdparty/glad/` eingebettet.
Kein externes Linking nötig — GLAD generiert reine C-Includes.

### Entity-System

Identisches Handle-Pattern wie Images/Files:
```
int handle → Entity* (std::unordered_map)
```
Entities sind polymorph: `PivotEntity`, `MeshEntity`, `CameraEntity`,
`LightEntity`, `SpriteEntity` erben alle von `bb_Entity_`.
Handles starten bei 1, 0 = ungültig.

### Shader-Strategie

Drei eingebettete GLSL-330-Shader als String-Literale in `bb_shader.h`:

| Shader     | Wann aktiv                                     |
|------------|------------------------------------------------|
| `unlit`    | EntityFX-Bit 2 gesetzt oder kein Licht in Szene |
| `textured` | Textur gebunden, kein Lighting                 |
| `lit`      | Standard — Phong, max. 8 Lichtquellen          |

---

### Rendering-Pipeline & Backend-Abstraktion

#### Aktuell: Forward Rendering

Der initiale Renderer ist **Forward Rendering** — ein Draw-Call pro Mesh,
Lighting vollständig im Fragment-Shader berechnet.

**Vorteile für den Einstieg:**
- Direkt korrekt für Blitz3D-Parität (Blitz3D selbst war Fixed-Function Forward)
- Transparenz (Sprites, Glas, Partikel) funktioniert out-of-the-box
- Einfache Implementierung, wenig Overhead

**Grenzen:**
- SSAO, SSR und echte Volumetrics sind **fundamentale Deferred-Techniken** —
  sie benötigen einen G-Buffer (Position + Normal + Material in Screen-Space).
  In Forward lassen sie sich nicht sauber ohne einen separaten Geometry-Pre-Pass
  implementieren, der im Grunde Deferred nachträglich hinzufügt.
- Viele Lichtquellen werden teuer (O(Meshes × Lichter))

#### Erweiterungspfad: Deferred Rendering

Um spätere Techniken wie **SSAO, SSR, PBR, Schatten, Volumetrics** zu ermöglichen,
wird der Renderer von Anfang an hinter einer **austauschbaren Backend-Schicht**
implementiert:

```
bb_RenderWorld()
    └── bb_renderer_backend_  (Funktionspointer-Struct, austauschbar)
            ├── ForwardRenderer   ← jetzt implementieren (3D-07 bis 3D-09)
            └── DeferredRenderer  ← später nachrüsten, ohne Entity/Camera/Light-System anzufassen
```

**`bb_renderer_backend_`** ist ein einfaches Struct mit Funktionspointern
(kein virtueller Dispatch, kein Overhead):

```cpp
struct bb_RenderBackend_ {
  void (*init)();
  void (*begin_frame)(bb_CameraEntity_*);
  void (*submit)(bb_Entity_*);
  void (*end_frame)();
  void (*shutdown)();
};

inline bb_RenderBackend_* bb_render_backend_ = nullptr; // gesetzt von Graphics3D
```

`bb_RenderWorld()` iteriert nur über sichtbare Entities und ruft
`bb_render_backend_->submit(entity)` auf — es kennt keinerlei Rendering-Details.

#### Technik-Kompatibilität

| Technik          | Forward | Deferred | Pfad |
|------------------|---------|----------|------|
| Phong Lighting   | ✓       | ✓        | Jetzt (3D-12) |
| PBR              | ✓       | ✓        | Forward-Shader-Upgrade (kein Backend-Wechsel nötig) |
| Shadow Maps      | ✓       | ✓        | Zusätzlicher Pre-Pass, funktioniert in beiden |
| SSAO             | ✗       | ✓        | Erfordert Deferred-Backend + G-Buffer |
| SSR              | ✗       | ✓        | Erfordert Deferred-Backend + G-Buffer |
| Volumetrics      | △       | ✓        | Screen-Space-Variante in Forward möglich, echte Volumetrics in Deferred |

**PBR** ist kein Architekturthema — nur ein anderes Lighting-Modell im Shader.
Der Wechsel von Phong → PBR erfordert neue BB-Befehle (`EntityMetallic`,
`EntityRoughness`, `EntityEmissive`) als BLTZNXT-Extension, aber keinen
Backend-Wechsel.

**SSAO und SSR** erfordern das Deferred-Backend. Wenn diese gewünscht werden,
wird ein neues `bb_deferred_renderer_.cpp` eingehängt — alle anderen Systeme
(Entity, Camera, Light, Collision, Animation) bleiben unverändert.

---

## Milestones

---

### 3D-01 · GLAD & OpenGL Context ✓ COMPLETE
*Dateien: `bb_gl_ctx.h` (neu), `bb_sdl.h`, `bb_graphics2d.h`, `bb_runtime.h`, `blitzcc.cpp`*

> **Abweichung vom Plan:** GLAD nicht nötig — SDL3 liefert `SDL_opengl_glext.h`
> mit allen `PFNGL*`-Typedefs. Eigener Inline-Loader (`BB_GL_DECL` / `BB_GL_LOAD`
> Makros) lädt 60 GL-3.3-Core-Funktionen direkt via `SDL_GL_GetProcAddress`.

- [x] `bb_gl_ctx.h`: GL-Typen, Konstanten, Funktionspointer-Deklarationen (60 Fns),
      `bb_gl_load_()`, `bb_gl_quit_()`
- [x] `bb_gl_ctx_` (`SDL_GLContext`) und `bb_gl_active_` (`bool`) in `bb_sdl.h`
- [x] `bb_Graphics3D(w, h, depth, mode)`:
      - SDL3-Fenster mit `SDL_WINDOW_OPENGL`
      - `SDL_GL_SetAttribute` für Core 3.3, Double-Buffer, Depth 24
      - `SDL_GL_CreateContext` → `bb_gl_ctx_`
      - `bb_gl_load_()` lädt alle Funktionspointer
      - `SDL_CreateRenderer` auf demselben Fenster (für 2D-Koexistenz)
      - `bb_gfx_width_/height_/depth_` gesetzt
      - GL-Info (Vendor, Renderer, Version) auf stderr
      - `bb_gl_active_ = true`
- [x] `bb_Flip` in `bb_graphics2d.h`: wenn `bb_gl_active_` →
      `SDL_FlushRenderer` + `SDL_GL_SwapWindow` statt `SDL_RenderPresent`
- [x] `bb_gl_quit_hook_` in `bb_sdl.h`; korrekte Quit-Reihenfolge:
      Renderer → GL-Context → Window
- [x] `-lopengl32` in `blitzcc.cpp` compile-Befehl
- [x] `Graphics3D`, `UpdateWorld`, `RenderWorld` u. a. in `kCommands[]`
- **Test:** `tests/test_3d01_context.bb` ✓
  ```
  [GL] Vendor:   NVIDIA Corporation
  [GL] Renderer: NVIDIA GeForce RTX 2080/PCIe/SSE2
  [GL] Version:  3.3.0 NVIDIA 591.86
  ```

---

### 3D-02 · UpdateWorld / RenderWorld / TrisRendered ✓ COMPLETE
*Dateien: `bb_graphics3d.h` (neu), `bb_gl_ctx.h`, `bb_runtime.h`, `blitzcc.cpp`*

- [x] `bb_graphics3d.h` erstellt; `bb_runtime.h` includet es statt `bb_gl_ctx.h` direkt
- [x] `bb_tris_rendered_` (int, global, reset am Anfang von RenderWorld)
- [x] `bb_UpdateWorld()`: stub — transform propagation kommt 3D-04
- [x] `bb_RenderWorld()`:
      - `SDL_FlushRenderer(bb_renderer_)` — 2D-Pre-Draws flushen
      - `SDL_GL_MakeCurrent(bb_window_, bb_gl_ctx_)`
      - `glViewport(0, 0, bb_gfx_width_, bb_gfx_height_)` (Camera-Viewport in 3D-06)
      - `glClearColor` + `glClearDepth` + `glClear` gemäß `bb_cam_cls_color_/zbuf_`
- [x] `bb_ClearWorld()`, `bb_CaptureWorld()`: Stubs
- [x] `bb_TrisRendered()` → `bb_tris_rendered_`
- [x] `bb_CameraClsMode(cam, cls_color, cls_zbuf)` — globaler State (per-cam in 3D-06)
- [x] `bb_CameraClsColor(cam, r, g, b)` — globaler State
- [x] `bb_Dither/WBuffer/AntiAlias/HWMultiTex`: Stubs
- [x] `bb_Wireframe(on)`: `glPolygonMode(GL_FRONT_AND_BACK, GL_LINE/GL_FILL)`
- [x] `bb_AmbientLight(r,g,b)`: globaler State (Shader-Upload in 3D-12)
- [x] `CameraClsMode`, `CameraClsColor` in `kCommands[]`
- **Test:** `tests/test_3d02_renderworld.bb` ✓
  — Fenster öffnet mit dunkelblauem GL-Hintergrund, TrisRendered=0 im Fenster

---

### 3D-03 · Entity Handle System & Pivot ✓ COMPLETE
*Dateien: `bb_entity_core.h` (neu), `bb_graphics3d.h`*

- [x] `bb_Entity_`-Basisstruct:
      ```cpp
      struct bb_Entity_ {
        int      handle;
        std::string name;
        int      parent = 0;          // handle, 0 = root
        std::vector<int> children;
        bool     visible = true;
        int      order   = 0;         // render order
        // transform (local)
        float px=0,py=0,pz=0;        // position
        float rx=0,ry=0,rz=0;        // rotation (Euler, degrees)
        float sx=1,sy=1,sz=1;        // scale
        // world matrix (computed by UpdateWorld)
        float world[16];             // column-major 4×4
        virtual ~bb_Entity_() = default;
        virtual EntityKind kind() const = 0;
      };
      ```
- [x] Handle-Map: `std::unordered_map<int, std::unique_ptr<bb_Entity_>> bb_entities_`
- [x] `bb_entity_next_id_` counter
- [x] Helper `bb_entity_get_(int h) → bb_Entity_*` (nullptr wenn nicht gefunden)
- [x] `bb_CreatePivot(int parent=0)` → handle
- [x] `bb_FreeEntity(int h)` → rekursiv alle Kinder freigeben, aus Parent-`children` entfernen
- [x] `bb_HideEntity(int h)`, `bb_ShowEntity(int h)`
- [x] `bb_EntityName(int h, const bbString&)` (setter), `bb_EntityName_(int h)` → bbString
- **Test:** `tests/test_3d03_pivot.bb` ✓
  ```blitzbasic
  Graphics3D 800,600
  Local p = CreatePivot()
  HideEntity p
  ShowEntity p
  FreeEntity p
  Print "OK"
  ```

---

### 3D-04 · Transform System & Scene Graph ✓ COMPLETE
*Dateien: `bb_entity_core.h`, `bb_graphics3d.h`*

- [x] Matrix-Helpers in `bb_entity_core.h` (kein Extern-Dependency):
      `mat4_identity`, `mat4_mul`, `mat4_translate`, `mat4_rotateX/Y/Z`,
      `mat4_scale`, `mat4_from_euler` (YXZ-Reihenfolge wie Blitz3D)
- [x] `bb_UpdateWorld()` implementiert: BFS/DFS von Root-Entities,
      `world = parent.world × local_matrix`
- [x] `bb_PositionEntity(h, x, y, z, glob=0)`
- [x] `bb_MoveEntity(h, dx, dy, dz)` — lokal relativ zur Entity-Orientierung
- [x] `bb_TranslateEntity(h, dx, dy, dz, glob=0)` — global oder lokal
- [x] `bb_RotateEntity(h, rx, ry, rz, glob=0)`
- [x] `bb_TurnEntity(h, rx, ry, rz, glob=0)` — relativ drehen
- [x] `bb_ScaleEntity(h, sx, sy, sz, glob=0)`
- [x] `bb_PointEntity(h, target, roll=0)` — Entity zu `target` ausrichten
- [x] `bb_AlignToVector(h, nx, ny, nz, axis, rate=1)` — weiche Ausrichtung
- [x] `bb_EntityX/Y/Z(h, glob=0)`, `bb_EntityRoll/Yaw/Pitch(h, glob=0)` — Queries
- [x] `bb_EntityDistance(h1, h2)` → float
- [x] `bb_ResetEntity(h)` — Position/Rotation/Scale auf Default zurück
- **Test:** `tests/test_3d04_transform.bb` ✓
  ```blitzbasic
  Graphics3D 800,600
  Local p = CreatePivot()
  PositionEntity p, 1.0, 2.0, 3.0
  UpdateWorld
  Print EntityX(p, True)   ; → 1.0
  Print EntityY(p, True)   ; → 2.0
  FreeEntity p
  ```

---

### 3D-05 · Entity Hierarchy ✓ COMPLETE
*Dateien: `bb_entity_core.h`*

- [x] `bb_EntityParent(h, parent, glob=0)` — re-parenten, Weltkoordinaten optional erhalten
- [x] `bb_GetParent(h)` → handle (0 = kein Parent)
- [x] `bb_FindChild(h, name$)` → handle (0 = nicht gefunden), rekursive Suche
- [x] `bb_CountChildren(h)` → int
- [x] `bb_GetChild(h, index)` → handle (1-basiert)
- [x] `bb_EntityOrder(h, order)` — niedrigere Werte zuerst rendern
- [x] `bb_EntityClass(h)` → bbString (`"Pivot"`, `"Mesh"`, `"Camera"`, `"Light"`, `"Sprite"`)
- **Test:** `tests/test_3d05_hierarchy.bb` ✓
  ```blitzbasic
  Graphics3D 800,600
  Local parent = CreatePivot()
  Local child  = CreatePivot()
  EntityParent child, parent
  Print CountChildren(parent)  ; → 1
  Print GetParent(child) = parent  ; → 1 (True)
  FreeEntity parent
  ```

---

### 3D-06 · Camera Entity ✓ COMPLETE
*Dateien: `bb_camera.h` (neu), `bb_graphics3d.h`*

- [x] `bb_CameraEntity_` erbt von `bb_Entity_`:
      `projMode`, `near`, `far`, `fov`, `vpX/Y/W/H`, `clsColor`, `clsDepth`,
      `clsR/G/B` (Clear-Farbe)
- [x] `bb_CreateCamera(int parent=0)` → handle
- [x] `bb_CameraRange(h, near, far)`
- [x] `bb_CameraZoom(h, zoom)` → passt FOV an: `fov = 2 * atan(1/zoom) * RAD_TO_DEG`
- [x] `bb_CameraProjMode(h, mode)` — 1=Perspektive (Default), 2=Ortho
- [x] `bb_CameraViewport(h, x, y, w, h)` — Viewport in Pixeln; 0,0,w,h = ganzes Fenster
- [x] `bb_CameraClsMode(h, cls_color, cls_zbuf)`:
      - `cls_color=True`: `glClear(GL_COLOR_BUFFER_BIT)` vor 3D-Render
      - `cls_color=False`: kein Color-Clear → 2D-Hintergrund bleibt sichtbar
      - `cls_zbuf=True`: `glClear(GL_DEPTH_BUFFER_BIT)` (immer empfohlen)
- [x] `bb_CameraClsColor(h, r, g, b)` → setzt `glClearColor`
- [x] View-Matrix aus Camera-World-Transform (inverse der World-Matrix)
- [x] Projection-Matrix (Perspektive: glm-freie eigene Impl., Inline-Math)
- [x] `RenderWorld` nutzt alle sichtbaren Cameras (sortiert nach order); mehrere Cameras = mehrere Passes
- **Note:** Aspect-Ratio-Bugfix: `aspect = vw/vh` (Breite/Höhe, konventionell)
- **Test:** `tests/test_3d06_camera.bb` ✓
  ```blitzbasic
  Graphics3D 800,600,32,1
  Local cam = CreateCamera()
  CameraRange cam, 1, 1000
  CameraClsColor cam, 20, 20, 80
  UpdateWorld : RenderWorld : Flip
  WaitKey
  ```

---

### 3D-07 · Shader Infrastructure ✓ COMPLETE
*Dateien: `bb_shader.h` (neu), `bb_graphics3d.h`, `bb_sdl.h`*

- [x] `bb_Shader_` struct: `GLuint program`; Uniform-Location-Cache (`unordered_map<string,GLint>`)
- [x] `bb_shader_compile_(vert_src, frag_src)` → `bb_Shader_*` oder nullptr + stderr
- [x] `bb_shader_uniform_i/f/v3/v4/mat4/mat3(shader, name, val)` + Array-Varianten `_iv/_fv/_v3v`
- [x] Drei eingebettete Shader als `constexpr char*`:
      - **`BB_GLSL_UNLIT`**: nur `u_color`, keine Lights — `a_pos` only
      - **`BB_GLSL_TEXTURED`**: `sampler2D u_tex` + `u_color`, keine Lights — `a_pos + a_uv`
      - **`BB_GLSL_LIT`**: Blinn-Phong, `u_light_pos/color/range/type[8]`, `u_ambient`,
        `u_shininess`; fällt auf Ambient-only zurück wenn `u_light_count == 0`
- [x] `bb_shaders_init_()` lazy in `bb_RenderWorld()` beim ersten Aufruf ausgeführt
- [x] `bb_shader_active_` pointer; `bb_shader_bind_(s)` — kein redundanter `glUseProgram`-Call
- [x] `bb_shader_quit_hook_` in `bb_sdl.h`; Cleanup vor GL-Context-Destroy (glDeleteProgram)
- **Vertex-Attrib-Layout** (abgestimmt auf 3D-08 Mesh-Format):
  `loc 0=a_pos, loc 1=a_normal, loc 2=a_uv, loc 3=a_color`
- **Test:** intern — validiert durch 3D-09 (Primitive Meshes)

---

### 3D-08 · Geometry Buffers (VAO/VBO) ✓ COMPLETE
*Dateien: `bb_mesh_core.h` (neu)*

- [x] Vertex-Format (interleaved, 11 floats pro Vertex):
      `{x, y, z,  nx, ny, nz,  u, v,  r, g, b}` — Alpha separat als Uniform
- [x] `bb_MeshData_` struct: `vertices`, `indices`, `vao/vbo/ebo` (GLuint),
      `dirty` flag, `triCount`
- [x] `bb_mesh_upload_(MeshData*)` → VAO/VBO/EBO auf GPU laden/aktualisieren
- [x] `bb_mesh_draw_(MeshData*, shader, mvp_mat4, color, tex, view_pos)` → `glDrawElements`
- [x] Cleanup: `bb_mesh_free_gpu_(MeshData*)`
- [x] In `bb_graphics3d.h` eingebunden (`#include "bb_mesh_core.h"`)
- **Test:** intern — validiert durch 3D-09 (Primitive Meshes)

---

### 3D-09 · Primitive Meshes ✓ COMPLETE
*Dateien: `bb_mesh.h` (neu), `bb_graphics3d.h`*

- [x] `bb_MeshEntity_` erbt von `bb_Entity_`; hat `std::vector<bb_MeshData_>`
      (eine Surface = ein Draw-Call); Destruktor ruft `bb_mesh_free_gpu_` auf
- [x] Generator-Funktionen (geben `bb_MeshData_` zurück):
      - `bb_gen_cube_()` — 6 Quads, Flat-Normals nach außen
      - `bb_gen_sphere_(int segs)` — UV-Sphere mit Smooth-Normals
      - `bb_gen_cylinder_(int segs, bool open)` — Smooth-Normals auf Mantel, Flat-Caps
      - `bb_gen_cone_(int segs, bool open)` — geneigte Normals auf Mantel, Flat-Cap
- [x] `bb_CreateCube(int parent=0)` → handle
- [x] `bb_CreateSphere(int segs=8, int parent=0)` → handle
- [x] `bb_CreateCylinder(int segs=8, int open=0, int parent=0)` → handle
- [x] `bb_CreateCone(int segs=8, int open=0, int parent=0)` → handle
- [x] `bb_MeshWidth/Height/Depth(h)` → float (AABB der CPU-Geometrie)
- [x] `RenderWorld` zeichnet alle sichtbaren `MeshEntity_` mit `UNLIT`-Shader
      via `bb_render_meshes_(shader, view, proj)` im Kamera-Pass
- [x] Befehle in `blitzcc.cpp` registriert (CreateCube, CreateSphere, etc.)
- **Test:** `tests/test_3d09_primitives.bb` ✓ (kompiliert)
  ```blitzbasic
  Graphics3D 800,600,32,1
  Local cam  = CreateCamera()
  PositionEntity cam, 0, 0, -5
  Local cube = CreateCube()
  UpdateWorld
  While Not KeyDown(1)
    TurnEntity cube, 0, 1, 0
    UpdateWorld : RenderWorld : Flip
  Wend
  ```

---

### 3D-00 · Grafikmodus- und Treiberaufzaehlung ✓ COMPLETE
*Datei: `bb_gfxmode.h` (neu)*

Nachtraeglich aufgenommen (2026-09-08): Dieser Block stand nicht auf dem Plan,
weil er kein Rendering betrifft — er blockierte aber **30 der 130
Installationsbeispiele**, weil die gemeinsame `start.bb` der mak-, halo-,
AGore-, Skully- und Richard_Betson-Beispiele damit beginnt. Sieben Dateien
uebersetzen allein dadurch vollstaendig.

- [x] `bb_GfxMode_` + einmalige, zwischengespeicherte Aufzaehlung ueber
      `SDL_GetFullscreenDisplayModes()`; Duplikate nach (Breite, Hoehe, Tiefe)
      fallen weg, weil Blitz3D keine Bildwiederholrate in dieser Liste fuehrt
- [x] `bb_CountGfxModes3D()`, `bb_CountGfxModes()` — dieselbe Liste: auf
      heutiger Hardware ist jeder Modus 3D-faehig
- [x] `bb_GfxModeWidth/Height/Depth(mode)` — **1-basiert**
- [x] `bb_GfxModeExists(w,h,depth)`
- [x] `bb_Windowed3D()` → 1 (der GL-Kontext haengt nicht am Vollbild)
- [x] `bb_CountGfxDrivers()`, `bb_GfxDriverName$(driver)` — ebenfalls 1-basiert
- [x] `bb_SetGfxDriver(driver)` — merkt den Wert; SDL3 waehlt den Videotreiber
      beim Initialisieren, ein echter Wechsel findet nicht statt

**Die 1-Basierung ist aus dem tatsaechlichen Gebrauch abgelesen**, nicht
geraten: `For k=1 To CountGfxModes3D()` und
`Input$("Display driver (1-"+CountGfxDrivers()+"):")` stehen so in den
Beispielen. Ein Index ausserhalb des Bereichs liefert 0 bzw. den leeren String
statt zu stuerzen.

- **Signaturvergleich:** `scripts/compare_commands.py` gegen `blitzcc +k` meldet
  fuer alle zehn Befehle keine Abweichung in Stelligkeit, Grenzen oder
  Rueckgabetyp.
- **Test:** `tests/test_3dgfx_modes.bb` — prueft **Invarianten statt Zahlen**
  (1-Basierung, Bereichsgrenzen, Vertraeglichkeit der Abfragen), weil Modusliste
  und Treibernamen vom Rechner abhaengen.

---

### 3D-10 · Entity Appearance
*Dateien: `bb_entity_core.h`, `bb_mesh.h`*

- [ ] `bb_Entity_` um `alpha`, `colorR/G/B`, `shininess`, `blend`, `fx` erweitern
- [ ] `bb_EntityAlpha(h, a)` — 0.0–1.0; alpha < 1.0 → Blend-Mode aktivieren
- [ ] `bb_EntityColor(h, r, g, b)` — 0–255 (Blitz3D-Konvention)
- [ ] `bb_EntityShininess(h, s)` — 0.0–1.0 für Specular
- [ ] `bb_EntityBlend(h, mode)` — 1=Normal, 2=Additive, 3=Multiply
- [ ] `bb_EntityFX(h, fx)` — Bitmask: Bit1=Fullbright, Bit2=Vertex-Color, Bit4=Flat, Bit8=No-Fog
- [ ] `bb_EntityAutoFade(h, near, far)` — Alpha-Fade mit Kamera-Distanz
- [ ] Blend-Mode in `bb_mesh_draw_()` umsetzen:
      - Normal: `glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)`
      - Additive: `glBlendFunc(GL_SRC_ALPHA, GL_ONE)`
      - Multiply: `glBlendFunc(GL_DST_COLOR, GL_ZERO)`
- **Test:** `tests/test_3d10_appearance.bb`
  ```blitzbasic
  Graphics3D 800,600,32,1
  Local cam  = CreateCamera()
  PositionEntity cam, 0,0,-5
  Local cube = CreateCube()
  EntityColor cube, 255, 0, 0
  EntityAlpha cube, 0.7
  UpdateWorld : RenderWorld : Flip : WaitKey
  ```

---

### 3D-11 · Texture Loading & Application ✓ COMPLETE
*Dateien: `bb_texture.h` (neu), `bb_shader.h`, `bb_mesh.h`*

`EntityTexture` steht in **54**, `LoadTexture` in **48** der 130 mitgelieferten
Beispielprogramme — beides haeufiger als `CreateLight` (39), das 3D-12
ausgeloest hat. Ohne Texturen bleibt jede dieser Szenen einfarbig.

- [x] `bb_Texture_`: `w/h`, `flags`, `name`, `blend`, `coords`, UV-Zustand und
      ein `bb_TexFrame_` je Frame (GL-Objekt + RGBA-Kopie fuer den Re-Upload)
- [x] Handle-Tabelle `std::unordered_map<int, std::shared_ptr<bb_Texture_>>`
- [x] `bb_LoadTexture(file$, flags=1)`, `bb_LoadAnimTexture(...)`,
      `bb_CreateTexture(w, h, flags=1, frames=1)`, `bb_FreeTexture`
- [x] `bb_TextureWidth/Height/Name`, `bb_ActiveTextures`, `bb_HWTexUnits`
- [x] `bb_EntityTexture(entity, tex, frame=0, index=0)` — acht Lagen laut Doku
- [x] `bb_TextureBlend`, `bb_TextureCoords`
- [x] `bb_ScaleTexture`, `bb_PositionTexture`, `bb_RotateTexture`
- [x] `bb_TextureFilter`, `bb_ClearTextureFilters` — Vorgabeliste `"",1+8`,
      von `Graphics3D` wiederhergestellt
- [x] Shader: gemeinsamer Texturbaustein fuer TEXTURED und LIT, bis zu vier
      Lagen mit eigener UV-Matrix, Blendmodus und Flags
- [x] `bb_texture_quit_` in der Quit-Kette vor dem Shader-Hook
- [ ] `bb_TextureBuffer` — liefert 0 mit Diagnose; braucht 2D-Zeichnen in
      Texturen (12 Beispieldateien)
- [ ] `bb_SetCubeFace`/`bb_SetCubeMode`, Flags 64/128 (Umgebungskarten) — Stubs

**Die Flags aus dem Entwurf oben waren falsch.** Der Entwurf las "Bit0
Mipmaps, Bit1 Clamp, Bit2 Nearest"; `help/commands/3d_commands/CreateTexture.htm`
fuehrt stattdessen `1 Color`, `2 Alpha`, `4 Masked`, `8 Mipmapped`,
`16 Clamp U`, `32 Clamp V`, `64 Sphere`, `128 Cube`, `256 VRAM`,
`512 High-Color`. Mit dem Entwurf waere jedes geladene Bild falsch behandelt
worden. Dazu die Vorgabe der Filterliste `TextureFilter "",1+8`: jede geladene
Textur ist mipmapped, auch wenn `LoadTexture` nur Flag 1 sieht.

**Die UV-Transformation ist am laufenden Original ausgemessen, nicht geraten.**
Ein Testprogramm im Original zeichnet eine Flaeche mit bekannter Textur und
liest die Bildzeile mit `ReadPixel` zurueck — die Antwort ist damit eine Zahl
und keine Einschaetzung:

    u' = ( cos a * u - sin a * v ) / u_scale - u_offset
    v' = ( sin a * u + cos a * v ) / v_scale - v_offset

Drei Dinge, die ohne diese Messung falsch geworden waeren: `ScaleTexture`
**teilt** die Koordinaten (`2,2` zeigt einen Ausschnitt, nicht zwei Kacheln),
`PositionTexture` **zieht ab**, und `RotateTexture` dreht um den **Ursprung**,
nicht um die Mitte — bei 90 und 180 Grad ununterscheidbar, bei 45 nicht.
Reihenfolge: Drehung, Skalierung, Verschiebung.

**FreeTexture** nimmt die Textur nur aus der Handle-Tabelle. Laut Doku
verlieren bereits texturierte Entities ihre Textur *nicht*; genau das leistet
der gemeinsame Besitz ueber `shared_ptr`.

**Offen und bewusst nicht behauptet:** Blendmodus 4 (Dot3) faellt auf Multiply
zurueck, weil eine Lichtrichtung im Tangentenraum fehlt. Lagen ab Index 4
werden gespeichert, aber nicht gemischt — `HWTexUnits()` meldet deshalb 4.
Transparente Flaechen werden nicht sortiert; das gehoert zu `EntityBlend`
(3D-10).

- **Signaturvergleich:** alle 20 Befehle gegen `blitzcc +k` — keine Abweichung.
- **Gleichwertigkeitsprobe:** dieselben acht Faelle (Skalierung, Verschiebung,
  Drehung um 45 und 90 Grad, Drehung mit Skalierung) durch unsere Runtime
  gerendert und mit `glReadPixels` zurueckgelesen; alle acht stimmen mit der am
  Original gemessenen Formel ueberein.
- **Test:** `tests/test_3d11_texture.bb` mit `tests/assets/` — sichert die
  sprachsichtbaren Zusagen zu (Handles, Groessen, absoluter Name,
  Framezerlegung, die Regel von FreeTexture, ein texturiertes Bild ohne
  Absturz). Das *Aussehen* sichert er nicht zu; dafuer steht die Messung oben.

---

### 3D-12 · Lighting ✓ COMPLETE
*Dateien: `bb_light.h` (neu), `bb_shader.h`, `bb_graphics3d.h`*

CreateLight war mit **39 betroffenen Beispieldateien der haeufigste fehlende
Einzelbefehl**. Der LIT-Shader konnte bereits acht Lichter; hinzugekommen sind
die Sprachseite, das Einsammeln im Renderpass und Spotlichter im Shader.

- [x] `bb_LightEntity_` erbt von `bb_Entity_`: `type`, `colR/G/B`, `range`,
      `inner`/`outer`
- [x] `bb_CreateLight(int type=1, int parent=0)` → handle
- [x] `bb_LightColor(h, r#, g#, b#)` — 0-255, **negative Werte verdunkeln**
      ("negative lighting" laut Doku), deshalb wird nicht geklemmt
- [x] `bb_LightRange(h, range#)` — Vorgabe **1000.0**
- [x] `bb_LightConeAngles(h, inner#, outer#)` — Vorgabe **0,90**
- [x] `bb_AmbientLight(r#, g#, b#)` — Vorgabe **127,127,127**; war vorher `int`
      und wich damit von der Originalsignatur ab
- [x] `RenderWorld` sammelt bis zu 8 sichtbare Lichter, waehlt bei mindestens
      einem den LIT-Shader und laedt die Uniforms hoch
- [x] Spotlichter im LIT-Shader ergaenzt (`u_light_dir`, `u_light_cos_inner`,
      `u_light_cos_outer`) — sonst haette `CreateLight(3)` still wie ein
      Punktlicht gerendert

**Aus der mitgelieferten Doku, nicht geraten** (`help/commands/3d_commands/`):
Die Vorgabe ist **1 = directional**, nicht point, und die Nummerierung beginnt
bei 1, waehrend der Shader intern ab 0 zaehlt. Ein Richtungslicht hat
"infinite position and infinite range"; seine Richtung kommt aus der Rotation
(die Beispiele richten es mit `RotateEntity` aus), im Shader also die
Gegenrichtung der +Z-Blickachse. Ein Licht mit Elternknoten entsteht laut Doku
trotzdem bei 0,0,0 — der Test prueft genau das.

**Offen und bewusst nicht behauptet:** ob Blitz3D die Kegelwinkel als vollen
Oeffnungswinkel oder als Halbwinkel versteht. Hier ist der volle Winkel
angenommen; entscheiden laesst sich das nur an einem laufenden
Original-Renderer.

- **Signaturvergleich:** `compare_commands.py` gegen `blitzcc +k` — keine
  Abweichung. Zusaetzlich von Hand die Parametertypen geprueft, die das
  Werkzeug **nicht** vergleicht; `AmbientLight` war dort abweichend und ist
  korrigiert.
- **Test:** `tests/test_3d12_lighting.bb` — sichert die sprachsichtbaren
  Zusagen zu (Handles, Entity-Klasse, Elternbindung, Positionsregel,
  negatives Licht, beleuchtetes Bild ohne Absturz). Das **Aussehen** ist
  ausdruecklich nicht zugesichert; dafuer waere ein Bildvergleich noetig.

---

### 3D-13 · Mesh Loading (.b3d)
*Dateien: `bb_loader.h` (neu)*

- [ ] Blitz3D `.b3d`-Format-Parser (Chunk-basiert: NODE, MESH, VRTS, TRIS, TEXS, BRUS, ANIM, KEYS, BONE)
- [ ] `bb_LoadMesh(path$, parent=0)` → handle; erkennt Extension `.b3d` / `.obj`
- [ ] `bb_LoadAnimMesh(path$, parent=0)` → handle mit Animations-Daten
- [ ] `bb_CopyMesh(h, parent=0)` → deep copy (neue GPU-Buffer)
- [ ] `bb_AddMesh(src, dst)` — Surfaces von `src` zu `dst` hinzufügen
- [ ] `bb_FlipMesh(h)` — Normals und Winding umkehren
- [ ] `bb_PaintMesh(h, brush)` — Brush auf alle Surfaces anwenden (stub bis Brush-System)
- [ ] `bb_LightMesh(h, r,g,b, range, x,y,z)` — Vertex-Colors backen
- [ ] `bb_FitMesh(h, x,y,z, w,h,d, uniform)`, `bb_ScaleMesh`, `bb_RotateMesh`, `bb_PositionMesh`
- [ ] `bb_UpdateNormals(h)` — Smooth-Normals neu berechnen
- [ ] `bb_MeshesIntersect(m1, m2)` → bool (AABB-Vortest + Triangle-Intersection)

---

### 3D-14 · OBJ-Loader (Fallback)
*Dateien: `bb_loader.h`*

- [ ] Minimaler `.obj` + `.mtl`-Parser (Positionen, Normals, UVs, Materialien)
- [ ] `bb_LoadMesh` erkennt `.obj`-Extension automatisch
- [ ] `.mtl` Diffuse-Textur → `bb_LoadTexture` intern aufrufen
- **Test:** `tests/test_3d14_obj.bb`
  ```blitzbasic
  Graphics3D 800,600,32,1
  Local cam = CreateCamera()
  PositionEntity cam, 0,1,-5
  Local m   = LoadMesh("tests/assets/teapot.obj")
  UpdateWorld : RenderWorld : Flip : WaitKey
  ```

---

### 3D-15 · Surface API (Prozedurale Meshes)
*Dateien: `bb_surface.h` (neu)*

- [ ] `bb_Surface_` ist ein Wrapper-Handle auf `bb_MeshData_` innerhalb einer `MeshEntity_`
- [ ] `bb_CreateSurface(mesh, brush=0)` → surface handle
- [ ] `bb_FindSurface(mesh, brush)` → surface handle (erste passende)
- [ ] `bb_AddVertex(surf, x,y,z, u=0,v=0,w=1)` → vertex index
- [ ] `bb_AddTriangle(surf, v0, v1, v2)` → triangle index
- [ ] `bb_VertexCoords(surf, idx, x, y, z)` — setter
- [ ] `bb_VertexNormal(surf, idx, nx, ny, nz)` — setter
- [ ] `bb_VertexTexCoords(surf, idx, u, v, w=1, set=0)` — setter
- [ ] `bb_VertexColor(surf, idx, r, g, b, a=1)` — setter
- [ ] `bb_VertexX/Y/Z/NX/NY/NZ/U/V/W(surf, idx)` — Getter → float
- [ ] `bb_TriangleVertex(surf, tri, corner)` → vertex index
- [ ] `bb_CountSurfaces(mesh)`, `bb_CountVertices(surf)`, `bb_CountTriangles(surf)`
- [ ] `bb_GetSurface(mesh, index)` → surface handle (1-basiert)
- [ ] `bb_PaintSurface(surf, brush)`, `bb_ClearSurface(surf, verts=1, tris=1)`
- [ ] Upload beim nächsten `UpdateWorld` (dirty flag)
- **Test:** `tests/test_3d15_surface.bb` — baut Triangle-Mesh aus einzelnen Vertices

---

### 3D-16 · Sprites
*Dateien: `bb_sprite.h` (neu)*

- [ ] `bb_SpriteEntity_` erbt von `bb_Entity_`:
      `texHandle`, `rotAngle`, `scaleX/Y`, `handleX/Y`, `viewMode`
      (viewMode: 1=Billboard, 2=Faced, 3=Fixed, 4=Free)
- [ ] `bb_CreateSprite(parent=0)` → handle
- [ ] `bb_LoadSprite(path$, flags=1, parent=0)` → handle + Textur auto-laden
- [ ] `bb_RotateSprite(h, angle)`, `bb_ScaleSprite(h, sx, sy)`
- [ ] `bb_HandleSprite(h, hx, hy)` — Pivot-Offset
- [ ] `bb_SpriteViewMode(h, mode)` — Billboard-Verhalten
- [ ] Billboard in `RenderWorld`: Quad immer zur Camera orientieren
      (Cylindrical Billboard für ViewMode 2, Free für 3)
- [ ] `bb_CreatePlane(segs=1, parent=0)` — flaches unendliches Mesh (riesige Plane)
- [ ] `bb_CreateMirror(parent=0)` — Stub (reflektierende Plane, FBO-basiert, aufwendig)
- **Test:** `tests/test_3d16_sprite.bb`
  ```blitzbasic
  Graphics3D 800,600,32,1
  Local cam = CreateCamera()
  PositionEntity cam, 0,0,-5
  Local s   = CreateSprite()
  EntityTexture s, LoadTexture("tests/assets/star.png")
  ScaleSprite s, 2, 2
  UpdateWorld : RenderWorld : Flip : WaitKey
  ```

---

### 3D-17 · Kamera: Fog & Picking
*Dateien: `bb_camera.h`*

- [ ] `bb_CameraFogMode(cam, mode)` — 0=Off, 1=Linear, 2=Exponential
- [ ] `bb_CameraFogRange(cam, near, far)`, `bb_CameraFogColor(cam, r, g, b)`
      → als GLSL-Uniform an `lit`-Shader übergeben
- [ ] `bb_CameraProject(cam, x, y, z)` → projiziert 3D auf 2D-Screen-Koordinaten
- [ ] `bb_ProjectedX/Y/Z()` — letzte Projektion
- [ ] `bb_CameraPick(cam, sx, sy)` → entity handle (Ray-Cast via Depth-Buffer-Read +
      Unprojection; Trefferkandidaten via AABB, dann Triangle-Test)
- [ ] `bb_PickedX/Y/Z()`, `bb_PickedNX/NY/NZ()`, `bb_PickedEntity()`
- [ ] `bb_PickedSurface()`, `bb_PickedTriangle()`, `bb_PickedTime()` (0–1 Rayparameter)
- [ ] `bb_EntityInView(entity, cam)` → bool (Frustum-Culling AABB-Test)
- **Test:** `tests/test_3d17_fog.bb`

---

### 3D-18 · Kollisionssystem
*Dateien: `bb_collision.h` (neu)*

- [ ] `bb_Entity_` um `colType`, `colRadius`, `colBox (x,y,z,w,h,d)`, `colPickMode` erweitern
- [ ] `bb_EntityRadius(h, xr, yr=0)`, `bb_EntityBox(h, x,y,z,w,h,d)`
- [ ] `bb_EntityType(h, type, recurse=0)`, `bb_EntityPickMode(h, mode, obscure=1)`
- [ ] `bb_GetEntityType(h)` → int
- [ ] `bb_Collisions(typeA, typeB, method, response)` — registriert Kollisionsregel
      (method: 1=Sphere-Sphere, 2=Sphere-Poly, 3=Box-Box; response: 1=Stop, 2=Slide, 3=Bounce)
- [ ] `bb_ClearCollisions()` — löscht Kollisionsregeln
- [ ] Kollisions-Detection in `UpdateWorld` (nach Transform-Propagation)
- [ ] `bb_EntityCollided(h, type)` → handle des ersten Kollisions-Partners (0 = keiner)
- [ ] `bb_CountCollisions(h)` → int
- [ ] `bb_CollisionX/Y/Z(h, idx)`, `bb_CollisionNX/NY/NZ(h, idx)` → Punkt & Normal
- [ ] `bb_CollisionTime(h, idx)`, `bb_CollisionEntity(h, idx)`, `bb_CollisionSurface(h, idx)`, `bb_CollisionTriangle(h, idx)`
- **Test:** `tests/test_3d18_collision.bb`
  ```blitzbasic
  Graphics3D 800,600,32,1
  Local cam    = CreateCamera()
  Local player = CreateSphere()
  Local wall   = CreateCube()
  EntityType player, 1
  EntityType wall,   2
  EntityRadius player, 0.5
  Collisions 1, 2, 2, 2
  PositionEntity wall, 0, 0, 5
  While Not KeyDown(1)
    MoveEntity player, 0, 0, 0.1
    UpdateWorld : RenderWorld : Flip
  Wend
  ```

---

### 3D-19 · Animation
*Dateien: `bb_animation.h` (neu)*

- [ ] `bb_AnimData_` struct: `length`, `keys` (frame → {pos,rot,scale})
- [ ] `bb_Entity_` um `animData[]` (Sequenzen), `animSeq`, `animTime`, `animSpeed`,
      `animMode`, `animating` erweitern
- [ ] `bb_Animate(h, mode, speed=1, seq=0, transition=0)`
      (mode: 0=Stop, 1=Loop, 2=Ping-Pong, 3=One-Shot)
- [ ] `bb_SetAnimTime(h, time, seq=0)`
- [ ] `bb_AnimSeq(h)`, `bb_AnimLength(h, seq=0)`, `bb_AnimTime(h)`, `bb_Animating(h)`
- [ ] `bb_AddAnimSeq(h, length)` → seq index
- [ ] `bb_ExtractAnimSeq(h, first, last, seq)` → seq index
- [ ] `bb_SetAnimKey(h, frame, pos=1, rot=1, scale=1)`
- [ ] `bb_LoadAnimSeq(h, path$)` → seq index (lädt Keyframes aus .b3d)
- [ ] Animation-Interpolation in `UpdateWorld` (Lerp Pos/Scale, Slerp Rot)
- **Test:** `tests/test_3d19_animation.bb`

---

### 3D-20 · Brush System
*Dateien: `bb_brush.h` (neu)*

- [ ] `bb_Brush_` struct: `colR/G/B`, `alpha`, `shininess`, `blend`, `fx`,
      `textures[8]` (tex-handle pro Slot)
- [ ] `bb_CreateBrush(r=255, g=255, b=255)` → handle
- [ ] `bb_LoadBrush(path$, flags=1, su=1, sv=1)` → handle (Textur auto-laden)
- [ ] `bb_FreeBrush(h)`
- [ ] `bb_BrushColor/Alpha/Shininess/Blend/FX(h, ...)`
- [ ] `bb_BrushTexture(h, tex, frame=0, index=0)`
- [ ] `bb_PaintEntity(entity, brush)` — kopiert Brush-Properties auf Entity
- [ ] `bb_GetEntityBrush(entity)` → neuer Brush mit Entity-Properties (Caller muss freeen)
- [ ] `bb_GetSurfaceBrush(surf)` → entsprechend für Surface
- **Test:** `tests/test_3d20_brush.bb`
  ```blitzbasic
  Local b = CreateBrush(255, 0, 0)
  BrushAlpha b, 0.5
  PaintEntity cube, b
  FreeBrush b
  ```

---

### 3D-21 · 3D Math Utilities
*Dateien: `bb_3dmath.h` (neu)*

- [ ] `bb_VectorDistance(x1,y1,z1, x2,y2,z2)` → float
- [ ] `bb_VectorYaw(dx,dy,dz)` → degrees
- [ ] `bb_VectorPitch(dx,dy,dz)` → degrees
- [ ] `bb_TFormPoint(x,y,z, src, dst)` — transformiert Punkt von src-Space in dst-Space
      (0 = Weltkoordinaten)
- [ ] `bb_TFormVector(x,y,z, src, dst)` — ohne Translation
- [ ] `bb_TFormNormal(x,y,z, src, dst)` — normalisierter Vektor
- [ ] `bb_TFormedX/Y/Z()` — Ergebnis des letzten TForm-Calls
- [ ] `bb_GetMatElement(entity, row, col)` → float (Welt-Matrix-Element)
- **Test:** `tests/test_3d21_math.bb`
  ```blitzbasic
  Print VectorDistance(0,0,0, 3,4,0)  ; → 5.0
  TFormPoint 1,0,0, 0, 0
  Print TFormedX()  ; → 1.0
  ```

---

### 3D-22 · Terrain
*Dateien: `bb_terrain.h` (neu)*

- [ ] `bb_TerrainEntity_` erbt von `bb_Entity_`:
      Heightmap-Daten (float[size×size]), GPU-Mesh (Triangle-Strip oder Indexed)
- [ ] `bb_CreateTerrain(size, parent=0)` → handle (size = Anzahl Vertices pro Seite, 2^n)
- [ ] `bb_LoadTerrain(path$, parent=0)` → lädt PNG/BMP als Heightmap via stb_image
- [ ] `bb_TerrainSize(h)` → int
- [ ] `bb_TerrainHeight(h, x, z)` → float (interpoliert)
- [ ] `bb_ModifyTerrain(h, x, z, height, realtime=0)` — Vertex-Height setzen;
      `realtime=1` → sofortige GPU-Aktualisierung
- [ ] `bb_TerrainDetail(h, detail, morph=0)` — LOD-Stufe (Stub für Morph)
- [ ] `bb_TerrainShading(h, on)` — Simple Slope-Shading
- [ ] `bb_TerrainX/Y/Z(h, x, height, z)` → Weltkoordinaten
- **Test:** `tests/test_3d22_terrain.bb`
  ```blitzbasic
  Graphics3D 800,600,32,1
  Local cam = CreateCamera()
  PositionEntity cam, 0, 50, -50
  RotateEntity cam, 45, 0, 0
  Local t = LoadTerrain("tests/assets/heightmap.png")
  UpdateWorld : RenderWorld : Flip : WaitKey
  ```

---

### 3D-23 · MD2 & BSP Loaders
*Dateien: `bb_loader.h`*

- [ ] MD2-Format-Parser (`bb_LoadMD2`): Vertex-Keyframes, UV-Mapping, Triangle-List
- [ ] `bb_AnimateMD2(entity, mode, speed, first, last, transition)`
- [ ] `bb_MD2AnimTime(entity)`, `bb_MD2AnimLength(entity)`, `bb_MD2Animating(entity)`
- [ ] BSP-Loader: Quake-III-Format (da Blitz3D BSP auf Q3 basiert)
- [ ] `bb_LoadBSP(path$, grav=0, light_gamma=1, ambient=0)` → entity handle
- [ ] `bb_BSPAmbientLight(r, g, b)`, `bb_BSPLighting(entity, on)`
- **Test:** `tests/test_3d23_md2.bb`

---

## Implementierungs-Reihenfolge (kritischer Pfad)

```
3D-01 → 3D-02 → 3D-03 → 3D-04 → 3D-05
                    ↓
              3D-06 (Camera)
                    ↓
              3D-07 (Shader)
                    ↓
              3D-08 (Geo Buffers)
                    ↓
              3D-09 (Primitives) ← ERSTER SICHTBARER 3D-OUTPUT
                    ↓
         ┌──────────┴──────────┐
    3D-10 (Appearance)   3D-11 (Textures)
         └──────────┬──────────┘
                    ↓
              3D-12 (Lighting)   ← Szene visuell vollständig
                    ↓
         ┌──────────┴──────────┐
    3D-13 (b3d Loader)   3D-15 (Surface API)
    3D-14 (OBJ Loader)   3D-16 (Sprites)
         └──────────┬──────────┘
                    ↓
    3D-17 (Fog/Pick) · 3D-18 (Collision) · 3D-19 (Animation)
                    ↓
    3D-20 (Brush) · 3D-21 (Math) · 3D-22 (Terrain) · 3D-23 (MD2/BSP)
```

**3D-01 bis 3D-09** sind der harte Kern — ohne sie läuft gar nichts.
Ab 3D-09 ist die Engine spielbar; alle weiteren Milestones sind
unabhängig parallelisierbar.

---

## Mapping zu Haupt-Roadmap (roadmap.md)

| Haupt-Roadmap | Dieser Plan          |
|---------------|----------------------|
| M47           | 3D-01, 3D-02         |
| M48           | 3D-06 (teilweise)    |
| M49           | 3D-11                |
| M50           | 3D-11 (Transforms)   |
| M51           | 3D-20                |
| M52           | 3D-09                |
| M53           | 3D-13, 3D-14         |
| M54           | 3D-15                |
| M55           | 3D-15                |
| M56           | 3D-22                |
| M57           | 3D-23                |
| M58           | 3D-03, 3D-05         |
| M59           | 3D-04                |
| M60           | 3D-10                |
| M61           | 3D-05                |
| M62           | 3D-06                |
| M63           | 3D-17                |
| M64           | 3D-12                |
| M65           | 3D-16                |
| M66–M67       | 3D-18                |
| M68–M69       | 3D-19                |
| M70           | 3D-21                |
