# BLTZNXT Roadmap to 1.0

This document tracks the progress towards a fully compatible, high-performance Blitz3D 1.0 reimplementation.

## Progress Notation
- `[x]` **Solid**: Command implemented, tested, and verified stable.
- `[-]` **Partially Implemented / Fragile**: Command exists but may have bugs, incomplete edge cases, or platform-specific issues.
- `[ ]` **Planned**: Command not yet implemented.
- `[R]` **Regression/Broken**: Was working, now broken. Requires immediate attention.

---

## 1. Compiler & Core Language Features

### 1.1 Core Components
- [x] Lexer (Tokenization)
- [x] Parser (AST Generation)
- [x] Semantic Analyzer (Type Checking)
- [x] Transpiler (B3D -> C++ Core)
- [x] Linker Integration (Clang support)
- [x] Source Location Tracking (Line/Col reporting)

### 1.2 Data Types & Structures
- [x] Integer `%`
- [x] Float `#`
- [x] String `$`
- [x] Arrays `Dim` / `Undim`
- [x] Custom Types `Type` / `Field`
- [x] **Task 1.2a**: Type Methods (`Method...End Method`)
- [x] Objects (Instance management)

### 1.3 Control Flow
- [x] `If...Then...Else...EndIf`
- [x] `For...Next`, `For...Each`
- [x] `While...Wend`, `Repeat...Until/Forever`
- [x] `Select...Case...Default`
- [x] `Function...End Function`
- [x] `Return`, `Exit`

---

## 2. Master Command Roadmap

### 2.1 System & Core Utilities
- [x] `End`
- [x] `Stop`
- [x] `AppTitle`
- [x] `RuntimeError`
- [ ] `ExecFile`
- [x] `Delay`
- [x] `MilliSecs`
- [x] `CommandLine`
- [x] `SystemProperty`
- [x] `GetEnv`
- [x] `SetEnv`
- [x] `CreateTimer`
- [x] `WaitTimer`
- [x] `FreeTimer`
- [x] `DebugLog`
- [ ] `RuntimeStats`

### 2.2 Math & Trigonometry
- [x] `Abs` (Fixed for Int/Float)
- [x] `Sgn`
- [x] `Mod`
- [x] `Sin`
- [x] `Cos`
- [x] `Tan`
- [x] `ASin`
- [x] `ACos`
- [x] `ATan`
- [x] `ATan2`
- [x] `Sqr`
- [x] `Floor`
- [x] `Ceil`
- [x] `Exp`
- [x] `Log`
- [x] `Log10`
- [x] `Rnd`
- [x] `Rand`
- [x] `SeedRnd`
- [x] `RndSeed`

### 2.3 String Manipulation
- [x] `String$`
- [x] `Left$`
- [x] `Right$`
- [x] `Replace$`
- [x] `Instr`
- [x] `Mid$`
- [x] `Upper$`
- [x] `Lower$`
- [x] `Trim$`
- [x] `LSet$`
- [x] `RSet$`
- [x] `Chr$`
- [x] `Asc`
- [x] `Len`
- [x] `Hex$`
- [x] `Bin$`
- [x] `CurrentDate$`
- [x] `CurrentTime$`

### 2.4 Filesystem & Streams
- [x] `OpenFile`
- [x] `ReadFile`
- [x] `WriteFile`
- [x] `CloseFile`
- [x] `FilePos`
- [x] `SeekFile`
- [x] `ReadDir`
- [x] `CloseDir`
- [x] `NextFile`
- [x] `CurrentDir`
- [x] `ChangeDir`
- [x] `CreateDir`
- [x] `DeleteDir`
- [x] `FileSize`
- [x] `FileType`
- [x] `CopyFile`
- [x] `DeleteFile`
- [x] `Eof`
- [ ] `ReadAvail`
- [x] `ReadByte` / `ReadShort` / `ReadInt` / `ReadFloat`
- [x] `ReadString$` / `ReadLine$`
- [x] `WriteByte` / `WriteShort` / `WriteInt` / `WriteFloat`
- [x] `WriteString$` / `WriteLine$`
- [ ] `CopyStream`

### 2.5 Memory Banks
- [x] `CreateBank`
- [x] `FreeBank`
- [x] `BankSize`
- [x] `ResizeBank`
- [ ] `CopyBank`
- [x] `PeekByte` / `PeekShort` / `PeekInt` / `PeekFloat`
- [x] `PokeByte` / `PokeShort` / `PokeInt` / `PokeFloat`
- [ ] `ReadBytes` / `WriteBytes`
- [ ] `CallDLL`

---

## 3. Graphics Subsystem

### 3.1 2D/3D Graphics Control
- [x] `Graphics` / `Graphics3D`
- [x] `EndGraphics`
- [ ] `GraphicsLost`
- [ ] `SetGamma` / `UpdateGamma`
- [ ] `GammaRed` / `GammaGreen` / `GammaBlue`
- [x] `FrontBuffer`
- [x] `BackBuffer`
- [ ] `ScanLine`
- [x] `VWait`
- [x] `Flip`
- [x] `GraphicsWidth`
- [x] `GraphicsHeight`
- [x] `GraphicsDepth`
- [x] `TotalVidMem`
- [x] `AvailVidMem`
- [x] `Origin`
- [x] `Viewport`

### 3.2 2D Rendering
- [x] `Color`
- [x] `GetColor`
- [x] `ColorRed` / `ColorGreen` / `ColorBlue`
- [x] `ClsColor`
- [x] `Cls`
- [x] `Plot`
- [x] `Rect`
- [x] `Oval`
- [x] `Line`
- [x] `Text`
- [x] `CopyRect`
- [x] `SetFont`
- [x] `LoadFont` / `FreeFont`
- [x] `FontWidth` / `FontHeight`
- [x] `StringWidth` / `StringHeight`

### 3.3 Images & Surfaces
- [x] `LoadImage`
- [x] `LoadAnimImage`
- [x] `CopyImage`
- [x] `CreateImage`
- [x] `FreeImage`
- [ ] `SaveImage`
- [ ] `GrabImage`
- [ ] `ImageBuffer`
- [x] `DrawImage` / `DrawBlock`
- [ ] `TileImage` / `TileBlock`
- [ ] `DrawImageRect` / `DrawBlockRect`
- [x] `MaskImage`
- [x] `HandleImage` / `MidHandle` / `AutoMidHandle`
- [x] `ImageWidth` / `ImageHeight`
- [x] `ImageXHandle` / `ImageYHandle`
- [ ] `ScaleImage` / `ResizeImage` / `RotateImage`
- [ ] `TFormImage` / `TFormFilter`
- [ ] `ImagesOverlap` / `ImagesCollide`
- [ ] `RectsOverlap`
- [ ] `ImageRectOverlap` / `ImageRectCollide`

### 3.4 Buffer Manipulation
- [x] `SetBuffer`
- [x] `GraphicsBuffer`
- [ ] `LoadBuffer` / `SaveBuffer`
- [ ] `BufferDirty`
- [ ] `LockBuffer` / `UnlockBuffer`
- [x] `ReadPixel` / `WritePixel`
- [ ] `ReadPixelFast` / `WritePixelFast`
- [ ] `CopyPixel` / `CopyPixelFast`

---

## 4. Input & Multimedia

### 4.1 Keyboard & Mouse
- [x] `KeyDown`
- [x] `KeyHit`
- [x] `GetKey`
- [x] `WaitKey`
- [x] `FlushKeys`
- [x] `MouseDown`
- [x] `MouseHit`
- [x] `GetMouse`
- [x] `WaitMouse`
- [x] `MouseX` / `MouseY` / `MouseZ`
- [x] `MouseXSpeed` / `MouseYSpeed` / `MouseZSpeed`
- [x] `FlushMouse`
- [x] `MoveMouse`
- [x] `HidePointer`
- [x] `ShowPointer`

### 4.2 Joysticks
- [ ] `JoyType`
- [ ] `JoyDown` / `JoyHit` / `GetJoy` / `WaitJoy`
- [ ] `JoyX` / `JoyY` / `JoyZ` / `JoyU` / `JoyV`
- [ ] `JoyPitch` / `JoyYaw` / `JoyRoll`
- [ ] `JoyHat`
- [ ] `JoyXDir` / `JoyYDir` / `JoyZDir`...
- [ ] `FlushJoy`

### 4.3 Audio
- [-] `LoadSound` / `FreeSound` (SDL_mixer optional)
- [-] `LoopSound` (Partial)
- [-] `SoundPitch` / `SoundVolume` / `SoundPan` (Partial)
- [-] `PlaySound`
- [ ] `PlayMusic`
- [ ] `PlayCDTrack`
- [-] `StopChannel` / `PauseChannel` / `ResumeChannel` (Partial)
- [ ] `ChannelPitch` / `ChannelVolume` / `ChannelPan`
- [ ] `ChannelPlaying`
- [ ] `Load3DSound`

---

## 5. 3D Engine (The "Blitz3D" Core)

### 5.1 Entities (General)
- [x] `CopyEntity`
- [x] `FreeEntity`
- [x] `HideEntity` / `ShowEntity`
- [x] `EntityParent`
- [x] `CountChildren` / `GetChild` / `FindChild`
- [x] `EntityX` / `EntityY` / `EntityZ`
- [x] `EntityPitch` / `EntityYaw` / `EntityRoll`
- [x] `EntityDistance`
- [x] `TFormPoint` / `TFormVector` / `TFormNormal`
- [x] `TFormedX` / `TFormedY` / `TFormedZ`
- [x] `PointEntity`
- [ ] `AlignToVector`
- [ ] `NameEntity` / `EntityName` / `EntityClass`

### 5.2 Transformation & Movement
- [x] `MoveEntity`
- [x] `TurnEntity`
- [x] `TranslateEntity`
- [x] `PositionEntity`
- [x] `ScaleEntity`
- [x] `RotateEntity`

### 5.3 Mesh Management
- [x] `CreateMesh`
- [x] `LoadMesh` / `LoadAnimMesh`
- [x] `CreateCube` / `CreateSphere` / `CreateCylinder` / `CreateCone`
- [x] `CopyMesh`
- [x] `ScaleMesh` / `RotateMesh` / `PositionMesh`
- [ ] `FitMesh`
- [ ] `FlipMesh`
- [ ] `PaintMesh`
- [ ] `AddMesh`
- [x] `UpdateNormals`
- [ ] `MeshWidth` / `MeshHeight` / `MeshDepth`

### 5.4 3D Surfaces
- [x] `CreateSurface`
- [x] `GetSurface` / `CountSurfaces`
- [ ] `FindSurface`
- [ ] `ClearSurface`
- [ ] `PaintSurface`
- [x] `AddVertex`
- [x] `AddTriangle`
- [x] `VertexCoords` / `VertexNormal` / `VertexColor` / `VertexTexCoords`
- [x] `CountVertices` / `CountTriangles`

### 5.5 Camera & Lighting
- [x] `CreateCamera`
- [x] `CameraZoom` / `CameraRange`
- [x] `CameraClsColor` / `CameraClsMode`
- [x] `CameraProjMode` / `CameraViewport`
- [x] `CameraFogColor` / `CameraFogRange` / `CameraFogMode`
- [x] `CameraProject` / `ProjectedX` / `ProjectedY` / `ProjectedZ`
- [x] `CreateLight`
- [x] `LightColor` / `LightRange` / `LightConeAngles`

### 5.6 Textures & Brushes
- [x] `LoadTexture` / `LoadAnimTexture` / `CreateTexture`
- [ ] `TextureBlend` / `TextureCoords`
- [ ] `ScaleTexture` / `RotateTexture` / `PositionTexture`
- [x] `CreateBrush` / `LoadBrush`
- [ ] `BrushColor` / `BrushAlpha` / `BrushShininess`
- [x] `BrushTexture`
- [x] `PaintEntity` / `EntityColor` / `EntityAlpha` / `EntityTexture`

### 5.7 Collision & Picking

#### 5.7.1 Collision System (Core)
- [x] `EntityType` / `EntityRadius` / `EntityBox` (Shape definitions)
- [x] `Collisions` (Interaction rules)
- [x] `UpdateWorld` (Sphere-Sphere & Sphere-Mesh implemented)
- [x] **Task 5.7.1a**: Sphere-Sphere collision detection & Stop response.
- [x] **Task 5.7.1b**: Basic Sliding collision response.
- [x] **Task 5.7.1c**: Sphere-Mesh collision detection.
- [x] **Task 5.7.1d**: Entity-to-Self collision support.
- [ ] **Task 5.7.1e**: Ellipsoid collision support (Multi-radius).
- [ ] **Task 5.7.1f**: Box-collision support (Method 3).

#### 5.7.2 Collision State & Queries
- [x] `ResetEntity` (Records prev-coordinates)
- [ ] **Task 5.7.2a**: Tracking previous entity positions for swept-testing.
- [ ] **Task 5.7.2b**: `EntityCollided()` and `CountCollisions()`.
- [x] **Task 5.7.2c**: `CollisionX/Y/Z/NX/NY/NZ` etc. (Result getters)
- [ ] **Task 5.7.2d**: Interpolation (`tween`) support.
- [ ] **Task 5.7.2e**: Spatial Partitioning optimization (Grid/Octree).
- [ ] **Task 5.7.2f**: `EntityCollided` and `CollisionTime` queries.

#### 5.7.3 Picking (Raycasting)
- [x] `EntityPickMode` (Visibility setup)
- [x] `LinePick` / `EntityPick` / `CameraPick` (Sphere-cap only)
- [x] `PickedX/Y/Z/Entity/Normal/Time` (Sphere-hit results)
- [ ] **Task 5.7.3a**: Ray-Mesh intersection (Triangle-level accuracy).
- [ ] **Task 5.7.3b**: `PickedSurface()` and `PickedTriangle()` detection.

### 5.8 Advanced Entities
- [x] `CreatePivot`
- [ ] `CreateSprite` / `LoadSprite`

### 5.9 Animation Engine
- [ ] **Task 5.9.1**: Hierarchical Bone Animation updates in `UpdateWorld`.
- [ ] **Task 5.9.2**: Vertex Animation (MD2) support.
- [ ] **Task 5.9.3**: `ExtractAnimSeq`, `Animate`, `Animating` commands.
- [ ] `LoadMD2` / `AnimateMD2`
- [ ] `LoadBSP`
- [ ] `CreateMirror`
- [x] `CreatePlane`
- [ ] `CreateTerrain` / `LoadTerrain` / `ModifyTerrain`

---

## 6. Stability & Regression Tracking

### Known Critical Bug Tracking
| Bug ID | Description | Impact | Status | Fix Version |
| :--- | :--- | :--- | :--- | :--- |
| CORE-001 | AST Memory Corruption in Parser | High (Crashes) | FIXED | 0.9.5 |
| GFX-002 | Null Pointer Deref in Graphics Init | High (Startup) | PATCHED | 0.9.1 |
| RT-003 | String Safety in Runtime Linker | Med (Corruption) | PATCHED | 0.9.2 |

### Stability Metrics
- **Verification Level**: All CORE modules are verified against `tests/smoke_test.bb`.
- **API Coverage**: Currently ~55% of total Blitz3D commands are implemented.
- **Transpiler Accuracy**: High. 1:1 mapping for major control flows.

---

---

## 7. The Granular Ladder to 1.0

To ensure steady progress within development quotas, we balance technical **Hardening** with visible **WOW-Effect** in smaller increments.

### 🛠️ v0.5.4: "The Foundation Update" [COMPLETED]
- **Hardening (P0)**: Final AST Memory Management fix (Node ownership refactor).
- **Core API**: Implementation of `MilliSecs()`, `Delay`, and `CommandLine`.
- **Filesystem**: Stream parity (`OpenFile`, `ReadFile`, `WriteFile`, `CloseFile`, `Eof`).

### 🔍 v0.5.5: "The Observation Update" [COMPLETED]
- **3D Features**: Implementation of `CameraProject`, `ProjectedX/Y/Z`.
- **Picking**: Basic Ray-Picking functionality (`LinePick`, `CameraPick`, `PickedX/Y/Z`).
- **Filesystem**: Directory manipulation (`ReadDir`, `NextFile`, `ChangeDir`).

### 💥 v0.5.6: "The Geometric Update" [COMPLETED]
- **Language**: **Type-Aware Name Mangling** (Resolves variable/function collisions).
- **3D Primitives**: Added `CreateCylinder`, `CreateCone`, `CreatePlane`.
- **Collisions**: Initialization logic (`EntityType`, `EntityRadius`, `EntityBox`, `Collisions`).
- **Hardening**: Signature mismatch resolution & transpiler build stability.

### 📈 v0.5.7: "The Timing & Layout Update" [COMPLETED]
- **Core API**: Implementation of `CreateTimer`, `WaitTimer`, and `FreeTimer`.
- **Graphics**: Fullscreen support in `Graphics3D` (`mode=1`) and Desktop Res detection.
- **Graphics**: Added `GraphicsWidth()`, `GraphicsHeight()`, `TotalVidMem()`, and `AvailVidMem()`.
- **Graphics**: Full Font support (`LoadFont`, `SetFont`, metrics).
- **System & IO**: Expanded `SystemProperty` (Modern paths) and full IO command parity.
- **Input**: Added `HidePointer` and `ShowPointer`.
- **Hardening**: Resolved `bbToString` ambiguity and filename encoding issues.

### 🚀 v0.6.0: "The Interaction Update" [COMPLETED]
- **3D Core**: Full Sliding Collision Response (`Collisions`, `UpdateWorld`).
- **Language**: **Type Methods** (`Method` / `End Method`) support.
- **WOW**: Terrain Engine Basics (`CreateTerrain`, `ModifyTerrain`).
- **WOW**: Basic animation loading (MD2/BSP loaders).

### 🎨 v0.7.0: "The Paradigms & Visuals Update"
- **Phase 7.1**: SDL_mixer integration for basic Audio.
- **Phase 7.2**: Advanced Texture support (CubeMaps in hardware).
- **Phase 7.3**: Multi-texturing support.

### 🏁 v1.0.0: "Final Release"
- Stable, documented, and **100% Core-Language compliant**.
- Final performance tuning and bug-squashing.

---

*Last Updated: February 21, 2026*
