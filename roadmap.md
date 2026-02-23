# BlitzNext: Granular Roadmap to 100% Blitz3D Compatibility

Each milestone is scoped to fit within a single AI-session context window:
- Parser/AST/Emitter changes: max 2-3 new constructs per milestone
- Runtime functions: max ~15 new functions per header file
- Every milestone ends with a concrete `.bb` test that proves it works

---

## PHASE A — IDE & CLI Integration

### Milestone 6: Line Number Tracking ✓ COMPLETE
*Touch: `lexer.h`, `ast.h`*
- [x] Add `line` field to `Token` struct; Lexer increments line counter on `\n`
- [x] Add `line` field to `ASTNode` base class
- [x] Parser stores `token.line` when constructing every AST node
- **Test:** `blitzcc tests/bad.bb` → Parser errors show `at 3:9`, `at 6:18` ✓

### Milestone 7: Structured Error Reporting ✓ COMPLETE
*Touch: `parser.h`, `blitzcc.cpp`*
- [x] Parser `error()` method emits `filename:line:col: error: message` (GCC format) to stderr
- [x] Compile-step failure emits `filename:0:0: error: compilation failed`
- [x] Exit code 0 = success, 1 = parse error, 2 = compile error
- **Test:** `blitzcc tests/bad.bb` → `tests/bad.bb:3:9: error: unexpected token 'THEN'` ✓

### Milestone 8: CLI Parity with Original blitzcc ✓ COMPLETE
*Touch: `blitzcc.cpp`*
- [x] `-k` flag: dumps all known built-in command names to stdout (static `kCommands[]` table in `blitzcc.cpp`)
- [x] `+k` flag: dumps `name(signature)` per line for autocomplete use
- [x] `BLITZPATH` environment variable: checked as third fallback in `resolvePath()` after CWD and `../`
- [x] `-release` flag: accepted and ensures `debug = false` (IDE compatibility)
- **Test:** `blitzcc -k` → 17 commands listed; `blitzcc +k` → with signatures ✓

---

## PHASE B — Language Core Completion

### Milestone 9: Const Declarations ✓ COMPLETE
*Touch: `ast.h`, `parser.h`, `emitter.h`*
- [x] `ConstDecl` AST node (name, typeHint, value expression)
- [x] Parser: `Const name[hint] = expr` → `ConstDecl`; multiple on one line via comma
- [x] Emitter: `%`/`#`/`!` → `constexpr int/float/auto`; `$` → `const bbString`
- **Test:** `Const MaxHP% = 100 : Print MaxHP` → `100` ✓

### Milestone 10: Array Indexing (Dim → Emit) ✓ COMPLETE
*Touch: `ast.h`, `parser.h`, `emitter.h`, `bb_runtime.h`*
- [x] `DimStmt` AST node (name, typeHint, vector of size expressions)
- [x] `ArrayAccess` AST node (name, vector of index expressions)
- [x] `ArrayAssignStmt` AST node (name, indices, value)
- [x] Parser: `Dim name[hint](d1, d2, ...)` → `DimStmt`; registers name in `dimmedArrays` set
- [x] Parser: `name(i) = expr` → `ArrayAssignStmt` (distinguished from plain assignment)
- [x] Parser: `name(i)` in expression context → `ArrayAccess` (distinguished from function call)
- [x] Emitter: `Dim a%(5)` → `std::vector<int> var_a(5 + 1);`
- [x] Emitter: `Dim grid%(3,3)` → `std::vector<std::vector<int>> var_grid(3 + 1, std::vector<int>(3 + 1));`
- [x] `bb_runtime.h`: added `#include <vector>`
- **Test:** `Dim arr%(5) : arr(2) = 42 : Print arr(2)` → `42` ✓

### Milestone 11: Goto, Gosub, Return (Legacy Flow) ✓ COMPLETE
*Touch: `ast.h`, `parser.h`, `emitter.h`*
- [x] `LabelStmt` AST node (name); `GotoStmt`; `GosubStmt`; bare `ReturnStmt` (already exists)
- [x] Parser: `.labelname` or `labelname:` → `LabelStmt`; `Goto lbl` → `GotoStmt`; `Gosub lbl` → `GosubStmt`
- [x] `:` as statement separator (skipNewlines + call-arg loop both stop at `:`)
- [x] Emitter: `lbl_name:;` / `goto lbl_name;` / computed-goto Gosub trick (GCC `&&label`)
- [x] `inFunctionBody` flag: bare `Return` in main → `goto *__gosub_ret__`, in function → `return;`
- **Test:** `Goto skip : Print "SKIP" : skip: Print "OK"` → `OK` ✓

### Milestone 12: Data, Read, Restore ✓ COMPLETE
*Touch: `ast.h`, `parser.h`, `emitter.h`, `bb_runtime.h`*
- [x] `DataStmt` AST node (list of literals); `ReadStmt` (variable + typeHint); `RestoreStmt` (optional label)
- [x] Parser: `Data 1,2,"x"` → `DataStmt`; `Read v[hint]` → `ReadStmt`; `Restore [label]` → `RestoreStmt`
- [x] Emitter: `collectData()` first-pass fills `__bb_data_pool__` before any Read; `DataStmt` visit is no-op
- [x] `ReadStmt` auto-declares undeclared variables (Blitz3D implicit declaration); casts to correct type
- [x] `bb_DataVal` struct with conversion operators (int/float/bbString); `bb_DataRead()` / `bb_DataRestore()`
- [x] `declaredVars` set tracks declared variable names to avoid re-declaration
- **Test:** `Data 10,20,30 : Read a : Read b : Print a + b` → `30` ✓

### Milestone 13: Type Declaration (Struct Parsing) ✓ COMPLETE
*Touch: `ast.h`, `parser.h`, `emitter.h`*
- [x] `TypeDecl` AST node (name, `std::vector<Field>` where `Field = {name, typeHint}`)
- [x] Parser: `Type Name ... Field x% ... End Type` → `TypeDecl`; multi-line and colon-separated forms
- [x] Parser: multiple fields per `Field` line (`Field left%, top%, right%, bottom%`)
- [x] Parser: `End Type` / `EndType` both accepted; `END TYPE` added to secondary-keyword list
- [x] Emitter: `visit(TypeDecl*)` no-op stub (full emission in M14)
- **Test:** `Type Player : Field x%, y% : End Type : Print "Types parsed OK"` → `Types parsed OK` ✓

### Milestone 14: Type Instances — New, Delete ✓ COMPLETE
*Touch: `ast.h`, `parser.h`, `emitter.h`*
- [x] `NewExpr` AST node (`typeName`); `DeleteStmt` AST node (`object` expr)
- [x] Emitter: `TypeDecl` → C++ `struct bb_T` + `bb_T_New()` / `bb_T_Delete()` + intrusive doubly-linked list head/tail
- [x] `New TypeName` → `bb_TypeName_New()`; `Delete v` → `bb_TypeName_Delete(var_v); var_v = nullptr;`
- [x] `Local v.Vec` type annotation: typeHint = `".Vec"` → `bb_Vec *var_v = nullptr;`
- [x] TypeDecl emitted before functions; skipped in main body loop
- [x] `varObjectTypes` map tracks lowercase var name → TypeName for Delete code-gen
- **Test:** `Local v.Vec = New Vec : v\x = 10 : Print v\x : Delete v` → `10` ✓

### Milestone 15: Type Field Access (`\` operator) ✓ COMPLETE
*Touch: `ast.h`, `parser.h`, `emitter.h`*
- [x] `FieldAccess` AST node (`object` expr, `fieldName`); `FieldAssignStmt` AST node
- [x] Parser: `obj\field` in expression context via `parsePostfix()` (chained: `a\b\c`)
- [x] Parser: `var\field = expr` in statement context (detected after type-hint consumption)
- [x] Emitter: `FieldAccess` → `obj->var_field`; `FieldAssignStmt` → `obj->var_field = expr;`
- **Test:** `v\x = 1.5 : Print v\x` → `1.5` ✓ (combined with M14 test)

### Milestone 16: Type Iteration — First, Last, Before, After, Insert, Each ✓ COMPLETE
*Touch: `ast.h`, `parser.h`, `emitter.h`*
- [x] `FirstExpr`, `LastExpr`, `BeforeExpr`, `AfterExpr` AST nodes
- [x] `InsertStmt` (insert before/after)
- [x] `ForEachStmt` (For Each obj.Type ... Next)
- [x] Emitter: `InsertBefore`/`InsertAfter`/`Unlink` helpers; deletion-safe `For Each` loop
- **Test:** `For Each n.Node : Print n\val : Next` → `10 20 30` ✓

---

## PHASE C — Runtime: Math (bb_math.h)

### Milestone 17: Math Completeness — Trig Inverse & Utility ✓ COMPLETE
*Touch: `bb_math.h` (new file), `bb_runtime.h`, `emitter.h`*
- [x] `bb_ASin`, `bb_ACos`, `bb_ATan`, `bb_ATan2` (degrees in/out)
- [x] `bb_Sgn(x)` → -1/0/1
- [x] `bb_Log10(x)`
- [x] `bb_Pi` as `constexpr float`; `Pi` identifier emits `bb_Pi` in Emitter
- [x] `bb_Int(float)` → truncate to int (C++ overload alongside `bb_Int(bbString)`)
- [x] All existing math moved from `bb_runtime.h` to `bb_math.h`; `bb_runtime.h` includes it
- **Test:** `Print ATan2(1.0, 1.0)` → `45` ✓

### Milestone 18: Random Number Functions ✓ COMPLETE
*Touch: `bb_math.h`*
- [x] `bb_Rnd()` → float in [0,1); `bb_Rnd(max)` → [0,max); `bb_Rnd(min,max)` → [min,max)
- [x] `bb_Rand(max)` → int in [1,max]; `bb_Rand(min,max)` → int in [min,max]
- [x] `bb_SeedRnd(seed)` → `srand(seed)`; `__bb_rnd_seed__` tracks last seed
- [x] `bb_RndSeed()` → returns last seed as int
- **Test:** `SeedRnd 42 : Print RndSeed()` → `42`; deterministic sequence verified ✓

---

## PHASE D — Runtime: Strings (bb_string.h)

### Milestone 19: String Extraction & Search ✓ COMPLETE
*Touch: `bb_string.h` (new file)*
- [x] `bb_Left(s, n)`, `bb_Right(s, n)`, `bb_Mid(s, pos)`, `bb_Mid(s, pos, n)` — all 1-based
- [x] `bb_Instr(s, sub)` / `bb_Instr(s, sub, start)` → 1-based index or 0
- [x] `bb_Replace(s, from, to)` — replaces all occurrences
- [x] `bb_Len`, `bb_Str`, `bb_Int(bbString)`, `bb_Float` moved from `bb_runtime.h` to `bb_string.h`
- [x] `bbString` typedef moved to `bb_string.h`; `bb_runtime.h` now includes it
- [x] `bb_Str(double)` overload avoids C++ double-literal ambiguity
- **Test:** `Print Left("Hello World", 5)` → `Hello`; `Print Instr("abcabc","b",3)` → `5` ✓

### Milestone 20: String Transformation & Encoding ✓ COMPLETE
*Touch: `bb_string.h`*
- [x] `bb_Upper(s)`, `bb_Lower(s)` — via `std::transform` + lambda (avoids signed-char UB)
- [x] `bb_Trim(s)` — strips leading and trailing whitespace (`' '`, `\t`, `\r`, `\n`)
- [x] `bb_LSet(s, n)` — left-aligned, pad right with spaces or truncate; `bb_RSet(s, n)` — right-aligned
- [x] `bb_Chr(n)` → single-char string; `bb_Asc(s)` → unsigned ASCII of first char, 0 for empty
- [x] `bb_Hex(n)` → uppercase hex, no prefix, negative treated as unsigned 32-bit
- [x] `bb_Bin(n)` → binary string, no leading zeros (minimum `"0"`)
- [x] `bb_String(s, n)` → repeat string s n times
- **Test:** `Print Hex(255)` → `FF`; `Print Bin(255)` → `11111111`; `Print Upper("hello")` → `HELLO` ✓

---

## Retrospective Fixes (v0.1.5 – v0.1.6)

### Bug Fix: DeleteStmt Fallback — Delete First/Last ✓ FIXED (v0.1.6)
*Touch: `emitter.h`*
- [x] `getExprTypeName()` extended with `BeforeExpr`/`AfterExpr` (recurse into object to inherit type)
- [x] `visit(DeleteStmt)` rewritten to call `getExprTypeName()` — type now resolved for all common object expressions
- [x] `bb_TypeName_Delete(expr)` emitted in all resolved cases; fallback only for truly indeterminate types
- [x] Local variable nulled only for `VarExpr` (the only case with a named pointer to invalidate)
- **Test:** `Delete First Node` / `Delete Last Node` → list shrinks correctly, no leak ✓

### Bug Fix: Global Variable Scope ✓ FIXED (v0.1.5)
*Touch: `emitter.h`, `parser.h`*
- [x] `Global` VarDecl now emits at C++ **file scope** (before user functions) via new `collectGlobals()` first pass
- [x] `visit(VarDecl)` for GLOBAL: skips local declaration, only emits initializer assignment in main body
- [x] `collectGlobals()` recurses into `Program` wrappers and `FunctionDecl` bodies (Global can appear anywhere in BB)
- [x] **Parser bonus**: `Name()` call in statement position now works (paren-call branch added before whitespace-arg loop)
- **Test:** `Global counter% : Function Inc() : counter=counter+1 : End Function : Inc() : Inc() : Print counter` → `2` ✓

---

## PHASE E — Runtime: Time & System (bb_system.h)

### Milestone 21: Time Functions ✓ COMPLETE
*Touch: `bb_system.h` (new file)*
- [x] `bb_MilliSecs()` → ms since program start (`std::chrono::steady_clock`)
- [x] `bb_CurrentDate()` → `"DD Mon YYYY"` string (`strftime %d %b %Y`)
- [x] `bb_CurrentTime()` → `"HH:MM:SS"` string (`strftime %H:%M:%S`)
- [x] `bb_Delay` moved from `bb_runtime.h` to `bb_system.h`; `bb_runtime.h` now includes it
- **Test:** `MilliSecs OK / CurrentDate OK / CurrentTime OK / DONE` ✓

### Milestone 22: Timer Objects ✓ COMPLETE
*Touch: `bb_system.h`*
- [x] `bb_CreateTimer(hz)` → int handle; `std::chrono`-based tick period (1000/hz ms)
- [x] `bb_FreeTimer(handle)` → erases from global `bb_timers_` map
- [x] `bb_WaitTimer(handle)` → sleeps until next tick; if ticks missed, catches up and returns count
- **Test:** `WaitTimer OK / WaitTimer catchup OK / FreeTimer OK / DONE` ✓

### Milestone 23: System / Process Functions ✓ COMPLETE
*Touch: `bb_system.h`, `bb_runtime.h`, `emitter.h`*
- [x] `bb_AppTitle(title)` — stores in `bb_app_title_`; no-op until SDL3
- [x] `bb_CommandLine()` → returns `argv[1..]` joined by spaces
- [x] `bb_ExecFile(path)` → `std::system(path.c_str())`, returns exit code
- [x] `bb_RuntimeError(msg)` → stderr + `std::exit(1)`
- [x] `bb_SetEnv`/`bb_GetEnv` → `_putenv_s`/`setenv` + `getenv`; `""` for missing key
- [x] `bb_SystemProperty(prop)` → stub returning `""`
- [x] `bb_ShowPointer()` / `bb_HidePointer()` — no-op stubs
- [x] `bb_CallDLL(dll, func, ...)` — logs to stderr, returns 0
- [x] `bbInit(argc, argv)` captures command-line; emitter passes `argc, argv`
- **Test:** `AppTitle OK / CommandLine OK / SetEnv-GetEnv OK / DONE` ✓

---

## PHASE F — Runtime: File I/O (bb_file.h)

### Milestone 24: File Open/Close/Seek ✓ COMPLETE
*Touch: `bb_file.h` (new file)*
- [x] `bb_OpenFile(path)` → `"r+b"`; `bb_ReadFile` → `"rb"`; `bb_WriteFile` → `"wb"`; handle (0 = error)
- [x] `bb_CloseFile(handle)` → `fclose` + erase from `bb_file_handles_` map
- [x] `bb_FilePos(handle)` → `ftell`
- [x] `bb_SeekFile(handle, pos)` → `fseek(f, pos, SEEK_SET)`
- [x] `bb_Eof(handle)` → position-based check (`ftell == file_size`); correct for empty files
- [x] `bb_ReadAvail(handle)` → `file_size - ftell`
- **Test:** `WriteFile OK / FilePos OK / Eof empty OK / ReadAvail empty OK / CloseFile OK / ReadFile OK / SeekFile OK / DONE` ✓

### Milestone 25: File Read Primitives ✓ COMPLETE
*Touch: `bb_file.h`*
- [x] `bb_ReadByte`, `bb_ReadShort`, `bb_ReadInt`, `bb_ReadFloat` — little-endian via `fread`
- [x] `bb_ReadString(handle)` → reads until `\0`; `bb_ReadLine(handle)` → reads until `\n`, strips `\r`
- [x] `bb_ReadBytes` — stub (logs warning; bank support pending M28)
- **Test:** combined with M26 ✓

### Milestone 26: File Write Primitives ✓ COMPLETE
*Touch: `bb_file.h`*
- [x] `bb_WriteByte`, `bb_WriteShort`, `bb_WriteInt`, `bb_WriteFloat` — little-endian via `fwrite`
- [x] `bb_WriteString(handle, s)` — null-terminated; `bb_WriteLine(handle, s)` — appends `\n`
- [x] `bb_WriteBytes` — stub (logs warning; bank support pending M28)
- **Test:** `Write OK / ReadByte OK / ReadShort OK / ReadInt OK / ReadFloat OK / ReadString OK / ReadLine OK / Eof after read OK / DONE` ✓

### Milestone 27: Directory & File Info ✓ COMPLETE
*Touch: `bb_file.h`*
- [x] `bb_ReadDir(path)` → handle; `bb_NextFile(handle)` → filename string; `bb_CloseDir(handle)` — `std::filesystem::directory_iterator`
- [x] `bb_CurrentDir()` → `fs::current_path().string()`; `bb_ChangeDir(path)` → `fs::current_path(path)`
- [x] `bb_CreateDir(path)` → `fs::create_directory`; `bb_DeleteDir(path)` → `fs::remove_all`
- [x] `bb_FileType(path)` → 0=none,1=file,2=dir; `bb_FileSize(path)` → `fs::file_size`
- [x] `bb_CopyFile(src, dst)` → `fs::copy_file overwrite_existing`; `bb_DeleteFile(path)` → `fs::remove`
- **Test:** `FileType dir OK / FileType file OK / FileType none OK / FileSize OK / CurrentDir OK / CreateDir OK / DeleteDir OK / CopyFile OK / DeleteFile OK / ReadDir-NextFile OK / DONE` ✓

---

## PHASE G — Runtime: Memory Banks (bb_bank.h)

### Milestone 28: Bank Allocation ✓ COMPLETE
*Touch: `bb_bank.h` (new file)*
- [x] `bb_CreateBank(size)` → handle (int, wraps `std::vector<uint8_t>`)
- [x] `bb_FreeBank(handle)`
- [x] `bb_BankSize(handle)` → size in bytes
- [x] `bb_ResizeBank(handle, newsize)` → `resize()`
- [x] `bb_CopyBank(src, srcOffset, dst, dstOffset, count)`
- [x] Removed `ReadBytes`/`WriteBytes` stubs from `bb_file.h`; real implementations live in `bb_bank.h`
- **Test:** `CreateBank OK / ResizeBank OK / CopyBank OK / FreeBank OK / DONE` ✓

### Milestone 29: Bank Peek/Poke ✓ COMPLETE
*Touch: `bb_bank.h`*
- [x] `bb_PokeByte`, `bb_PokeShort`, `bb_PokeInt`, `bb_PokeFloat`
- [x] `bb_PeekByte`, `bb_PeekShort`, `bb_PeekInt`, `bb_PeekFloat`
- [x] Bounds-check → `bb_RuntimeError` on out-of-range; uses `std::memcpy` for unaligned access safety
- [x] `bb_ReadBytes` / `bb_WriteBytes` fully implemented (File ↔ Bank transfer)
- **Test:** `PokeByte/PeekByte OK / PokeShort/PeekShort OK / PokeInt/PeekInt OK / PokeFloat/PeekFloat OK / DONE` ✓

---

## PHASE H — SDL3 Window & Event Infrastructure

### Milestone 30: SDL3 Init + Headless Event Loop ✓ COMPLETE
*Touch: `bb_sdl.h` (new file), `bb_runtime.h`*
- [x] `bb_sdl_ensure_()` lazy-initializes SDL3 (SDL_Init VIDEO+EVENTS); `bb_window_` starts as `nullptr`
- [x] Global event pump: `bb_PollEvents()` drains SDL event queue; SDL_EVENT_QUIT → `std::exit(0)`
- [x] `bb_WaitKey()` rewired to SDL event loop (SDL_EVENT_KEY_DOWN or SDL_EVENT_QUIT); falls back to `std::cin.get()` if SDL init fails; skips entirely when stdin is not a real console (non-interactive / test runner)
- [x] `bbEnd()` calls `bb_sdl_quit_()` → `SDL_DestroyRenderer`, `SDL_DestroyWindow`, `SDL_Quit`
- [x] SDL3 import library (`libSDL3.dll.a`) linked by compile step; `SDL3.dll` auto-copied next to executable
- **Test:** `Print "SDL OK"` → `SDL OK` ✓ (headless, SDL stays uninitialized; zero overhead)

---

## PHASE I — Input: Keyboard & Mouse (bb_input.h)

### Milestone 31: Keyboard Input ✓ COMPLETE
*Touch: `bb_input.h` (new file), `bb_sdl.h`*
- [x] `bb_KeyDown(code)` — pumps events if SDL running, reads `bb_sdl_key_down_[sdl_sc]`; returns 0 in headless mode
- [x] `bb_KeyHit(code)` — edge-triggered: `bb_sdl_key_hit_raw_[sdl_sc]` set on KEY_DOWN (non-repeat), cleared after read
- [x] `bb_GetKey()` — drains queue; blocks via `SDL_WaitEvent` + `bb_sdl_process_event_()` until key arrives; returns Blitz3D code (0 = unmapped/no SDL)
- [x] `bb_FlushKeys()` — zeros all key-down and key-hit arrays, drains SDL key event queue via `SDL_FlushEvents`
- [x] `bb_blitz_to_sdl_[256]` — Blitz3D DIK code → SDL_Scancode (full mapping: all printable keys, F1–F12, numpad, cursor, home/end/pgup/pgdn/ins/del, Win keys)
- [x] `bb_sdl_to_blitz_[512]` — reverse map; built at startup from `bb_blitz_to_sdl_`
- [x] `bb_sdl.h` extended: `bb_sdl_key_down_[512]`, `bb_sdl_key_hit_raw_[512]`, key FIFO (`BB_KEY_QUEUE_CAP=64`); `bb_sdl_process_event_()` called by `bb_PollEvents` and `bb_WaitKey`
- **Test:** `KeyDown(1) = 0 / KeyHit(1) = 0 / FlushKeys OK / DONE` ✓ (headless; SDL never initialized)

### Milestone 32: Mouse Input ✓ COMPLETE
*Touch: `bb_input.h`, `bb_sdl.h`*
- [x] `bb_MouseX()`, `bb_MouseY()` → SDL_GetMouseState via MOUSE_MOTION accumulator
- [x] `bb_MouseZ()` → scroll wheel accumulator (positive = up; FLIPPED direction handled)
- [x] `bb_MouseXSpeed()`, `bb_MouseYSpeed()`, `bb_MouseZSpeed()` → delta since last call, reset on read
- [x] `bb_MouseDown(button)`, `bb_MouseHit(button)` — edge-triggered hit flag, 1=left/2=right/3=middle
- [x] `bb_WaitMouse()` — blocks via `SDL_WaitEvent` + `bb_sdl_process_event_()` until button press; returns button number
- [x] `bb_GetMouse()` → alias for `bb_WaitMouse()`
- [x] `bb_FlushMouse()` — clears held/hit flags, speed accumulators, FIFO, drains SDL mouse events
- [x] `bb_MoveMouse(x, y)` → `SDL_WarpMouseInWindow` (falls back to `SDL_WarpMouseGlobal` if no window)
- [x] `bb_sdl.h` extended: mouse state arrays (`bb_mouse_down_[4]`, `bb_mouse_hit_[4]`), position/speed floats, FIFO (`BB_MOUSE_QUEUE_CAP=16`), `bb_sdl_btn_to_blitz_()` helper; `bb_sdl_process_event_()` handles MOUSE_MOTION, BUTTON_DOWN, BUTTON_UP, MOUSE_WHEEL
- **Test:** `MouseX() = 0 / MouseY() = 0 / MouseZ() = 0 / MouseDown(1) = 0 / MouseHit(1) = 0 / FlushMouse OK / DONE` ✓ (headless; SDL never initialized)

### Milestone 33: Joystick Input ✓ COMPLETE
*Touch: `bb_input.h`, `bb_sdl.h`*
- [x] `bb_JoyType(port)` → 0=none, 1=joystick, 2=gamepad; reads `bb_joy_[port].is_gamepad`
- [x] `bb_JoyX/Y/Z/U/V(port)` → -1.0..1.0; mapped from SDL axes 0–4 (Sint16 / 32767.0f, clamped)
- [x] `bb_JoyHat(port)` → 0=center, 1=up … 8=up-left; SDL hat bitmask → `bb_sdl_hat_to_blitz_()`
- [x] `bb_JoyDown(port, btn)` / `bb_JoyHit(port, btn)` — 1-based button; held + edge-triggered flags
- [x] `bb_WaitJoy(port)` — blocks via `SDL_WaitEvent` + `bb_sdl_process_event_()`; dequeues 1-based button
- [x] `bb_GetJoy(port)` → alias for `bb_WaitJoy(port)`
- [x] `bb_FlushJoy(port)` — clears held/hit arrays and button FIFO; preserves axis/hat state
- [x] `bb_sdl.h`: `BB_JOY_MAX_PORTS=4`, `BB_JOY_MAX_BUTTONS=32`, `bb_JoyPort_` struct; `bb_sdl_ensure_()` adds `SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD`; `bb_sdl_quit_()` closes handles; `bb_sdl_process_event_()` handles all joystick events
- [x] `blitzcc.cpp` `kCommands[]` updated with 12 joystick entries
- **Test:** `JoyType(0)=0 / JoyX(0)=0 / JoyHat(0)=0 / JoyDown(0,1)=0 / FlushJoy OK / DONE` ✓ (headless)

---

## PHASE J — Audio (bb_sound.h)

### Milestone 34: Sound Loading & Playback ✓ COMPLETE
*Touch: `bb_sound.h` (new file), `bb_sdl.h`, `bb_runtime.h`*
- [x] SDL3 audio subsystem lazily initialised by `bb_snd_ensure_()` (separate from SDL video/events); `bbEnd()` calls `bb_snd_quit_()` before `bb_sdl_quit_()`
- [x] `bb_LoadSound(file)` → WAV loaded with `SDL_LoadWAV`; stored in `bb_snd_sounds_[1..63]`; returns 1-based handle (0 on failure)
- [x] `bb_FreeSound(handle)` → stops all channels using the sound, calls `SDL_free`, zeros slot
- [x] `bb_PlaySound(handle)` → creates `SDL_AudioStream` (src→device spec), binds to audio device, puts PCM data, flushes; returns channel handle
- [x] `bb_LoopSound(handle)` → same as PlaySound but no flush; `bb_snd_update_()` refills when queued < sound length
- [x] `bb_StopChannel(channel)` → unbinds stream, destroys, zeros slot
- [x] `bb_snd_update_()` registered as `bb_audio_update_hook_` in `bb_sdl.h` at startup; called by `bb_PollEvents()` every frame; auto-cleans finished one-shot channels + refills looping ones
- [x] `bb_sdl.h` extended: `bb_audio_update_hook_` function pointer; `bb_PollEvents()` calls it after draining SDL events
- **Test:** `LoadSound=0 / PlaySound=0 / StopChannel OK / FreeSound OK / DONE` ✓ (headless; no audio device)

### Milestone 35: Channel Control ✓ COMPLETE
*Touch: `bb_sound.h`*
- [x] `bb_PauseChannel(ch)` — `SDL_UnbindAudioStream`; sets `paused=true`; skipped by `bb_snd_update_()` so looping channels don't refill while paused
- [x] `bb_ResumeChannel(ch)` — `SDL_BindAudioStream`; clears `paused`; playback resumes from current buffer position
- [x] `bb_ChannelPlaying(ch)` → 1 if stream non-null (active or paused), 0 when done/missing
- [x] `bb_ChannelVolume(ch, vol)` → `SDL_SetAudioStreamGain`; stored in `bb_Channel_.gain` for ResumeChannel restore
- [x] `bb_ChannelPan(ch, pan)` → stored in `bb_Channel_.pan`; full stereo pan deferred (SDL3 has no per-stream channel gain)
- [x] `bb_ChannelPitch(ch, hz)` → `SDL_SetAudioStreamFrequencyRatio(stream, hz / src_freq)`
- [x] `bb_SoundVolume/Pan/Pitch(snd, …)` → stored in `bb_Sound_.vol/pan/pitch`; applied to each new channel in `bb_play_sound_()`
- [x] `bb_Sound_` extended: `vol=1.0f`, `pan=0.0f`, `pitch=0.0f` default fields
- [x] `bb_Channel_` extended: `paused=false`, `gain=1.0f`, `pan=0.0f` fields
- [x] `blitzcc.cpp` `kCommands[]` updated with 9 channel-control entries
- **Test:** `ChannelVolume OK / ChannelPan OK / ChannelPitch OK / PauseChannel OK / ResumeChannel OK / ChannelPlaying=0 / SoundVolume OK / SoundPan OK / SoundPitch OK / DONE` ✓ (headless)

### Milestone 36: Music & CD ✓ COMPLETE
*Touch: `bb_sound.h`*
- [x] `bb_PlayMusic(file)` → SDL3 streaming audio (WAV via SDL_LoadWAV; OGG/MP3 fail gracefully — no SDL_mixer)
- [x] `bb_PlayCDTrack(track)` → stub (deprecated hardware; logs warning to stderr)
- [x] `bb_StopMusic()`, `bb_MusicPlaying()` — companion functions
- **Test:** `tests/test_m36_music.bb` → headless OK

### Milestone 37: 3D Sound ✓ COMPLETE
*Touch: `bb_sound3d.h` (new file)*
- [x] `bb_Load3DSound(file)` → delegates to `bb_LoadSound`; handle usable with `PlaySound`/`LoopSound`
- [x] `bb_SoundRange(snd, inner, outer)` — stores falloff radii for future distance attenuation
- [x] `bb_Channel3DPosition`, `bb_Channel3DVelocity` — per-channel world position + Doppler state (stored)
- [x] `bb_ListenerPosition`, `bb_ListenerOrientation`, `bb_ListenerVelocity` — global listener state (stored)
- [x] `bb_WaitSound(ch)` — blocks until channel finishes (10 ms poll loop via `bb_snd_update_`)
- [x] `bb_3DSoundVolume`, `bb_3DSoundPan`, `bb_3DChannelVolume`, `bb_3DChannelPan` — C++ aliases delegating to 2D counterparts (BB-level exposure deferred; identifiers starting with digit require lexer extension)
- **Test:** `tests/test_m37_sound3d.bb` → headless OK ✓

---

## PHASE K — 2D Graphics: Window & Buffer (bb_graphics2d.h)

### Milestone 38: Graphics Mode Init ✓ COMPLETE
*Touch: `bb_graphics2d.h` (new file), `bb_runtime.h`*
- [x] `bb_Graphics(w, h, depth, mode)` → `SDL_CreateWindow` + `SDL_CreateRenderer`; mode 0=windowed, 1=fullscreen, 2=fullscreen-desktop, 6=fullscreen+vsync
- [x] `bb_EndGraphics()` — destroys window and renderer; bbEnd() also does this
- [x] `bb_GraphicsWidth()`, `bb_GraphicsHeight()`, `bb_GraphicsDepth()`, `bb_GraphicsRate()` — return stored values (valid even on headless machines)
- [x] `bb_TotalVidMem()`, `bb_AvailVidMem()` → 512 MB stub (SDL3 has no VRAM query API)
- [x] `bb_GraphicsMode(w, h, depth, rate)` → delegates to `bb_Graphics(w, h, depth, 0)`
- **Test:** `tests/test_m38_graphics.bb` → headless OK ✓

### Milestone 39: Buffer & Flip ✓ COMPLETE
*Touch: `bb_graphics2d.h`*
- [x] `bb_BackBuffer()` → 1; `bb_FrontBuffer()` → 2 (integer handle tokens)
- [x] `bb_SetBuffer(buf)` — stores active render target; bookkeeping only (SDL3 always uses back buffer internally)
- [x] `bb_Cls()` → `SDL_RenderClear` using stored `bb_cls_r_/g_/b_` (default black); `ClsColor` (M40) sets these bytes
- [x] `bb_Flip(vblank=1)` → `SDL_RenderPresent` + `bb_PollEvents()`; vblank arg accepted for API parity
- [x] `bb_CopyRect(sx,sy,sw,sh, dx,dy, srcbuf, dstbuf)` — silent stub (render-to-texture deferred to M46)
- **Test:** `tests/test_m39_buffer.bb` → headless OK ✓

### Milestone 40: Color & Pixel Primitives ✓ COMPLETE
*Touch: `bb_graphics2d.h`*
- [x] `bb_Color(r, g, b)` → sets draw color (clamped 0–255); stored in `bb_draw_r_/g_/b_` (default white)
- [x] `bb_ClsColor(r, g, b)` → sets background clear color in `bb_cls_r_/g_/b_` (declared in M39)
- [x] `bb_GetColor(x, y)` → `SDL_RenderReadPixels` (1×1 rect) + `SDL_ReadSurfacePixel`; overwrites draw-colour state; returns 0 headless
- [x] `bb_ColorRed()`, `bb_ColorGreen()`, `bb_ColorBlue()` → return `bb_draw_r_/g_/b_` as int
- [x] `bb_Plot(x, y)` → `SDL_SetRenderDrawColor` + `SDL_RenderPoint`; safe no-op headless
- **Test:** `tests/test_m40_color.bb` → headless OK ✓

### Milestone 41: Line & Shape Primitives
*Touch: `bb_graphics2d.h`*
- [ ] `bb_Line(x1,y1, x2,y2)`
- [ ] `bb_Rect(x,y,w,h, solid)` → filled or outline
- [ ] `bb_Oval(x,y,w,h, solid)`
- [ ] `bb_Poly(x0,y0, x1,y1, x2,y2)` → triangle
- **Test:** `Line 0,0,100,100 : Rect 50,50,40,40,1`

### Milestone 42: Text & Console Output
*Touch: `bb_graphics2d.h`*
- [ ] `bb_Locate(x, y)` → cursor position for `Print`/`Write`
- [ ] `bb_Write(s)` → print without newline at cursor pos
- [ ] `bb_Text(x, y, s, centerX, centerY)` → SDL_RenderText or bitmap font
- [ ] Basic built-in bitmap font (8×8 baked into header as array)
- **Test:** `Text 100,100,"Hello World"`

### Milestone 43: Fonts
*Touch: `bb_graphics2d.h`*
- [ ] `bb_LoadFont(name, height, bold, italic, underline)` → font handle
- [ ] `bb_SetFont(handle)` → sets active font
- [ ] `bb_FreeFont(handle)`
- [ ] `bb_FontWidth()`, `bb_FontHeight()`
- [ ] `bb_StringWidth(s)`, `bb_StringHeight(s)`
- [ ] SDL3_ttf integration (or stb_truetype as fallback)
- **Test:** `SetFont LoadFont("arial", 16) : Text 10,10,"Hello"`

### Milestone 44: Image Loading & Drawing
*Touch: `bb_image.h` (new file)*
- [ ] `bb_LoadImage(file)` → SDL3 texture handle
- [ ] `bb_CreateImage(w, h)` → blank texture
- [ ] `bb_FreeImage(handle)`
- [ ] `bb_ImageWidth(handle)`, `bb_ImageHeight(handle)`
- [ ] `bb_DrawImage(img, x, y)`, `bb_DrawImageRect(img, x, y, sx, sy, sw, sh)`
- [ ] `bb_DrawBlock(img, x, y)`, `bb_DrawBlockRect(img, x, y, sx, sy, sw, sh)`
- **Test:** `Local img = LoadImage("player.png") : DrawImage img,100,100`

### Milestone 45: Image Manipulation
*Touch: `bb_image.h`*
- [ ] `bb_MaskImage(img, r, g, b)` → transparent color keying
- [ ] `bb_HandleImage(img, x, y)` / `bb_MidHandle(img)` / `bb_AutoMidHandle(on)`
- [ ] `bb_ScaleImage(img, sx, sy)`, `bb_RotateImage(img, deg)`
- [ ] `bb_TileImage(img, x, y)`, `bb_TileBlock(img, x, y)`
- [ ] `bb_DrawImageEllipse(img, x, y, rx, ry)`
- [ ] `bb_SaveImage(img, file)` → PNG via SDL3
- [ ] Overlap/collision: `bb_ImagesOverlap`, `bb_ImageRectOverlap`, `bb_ImagesColl`, `bb_ImageXColl`, `bb_ImageYColl`
- **Test:** `MidHandle img : DrawImage img, 400,300`

### Milestone 46: Pixel Buffer Access
*Touch: `bb_image.h`*
- [ ] `bb_ImageBuffer(img)` → buffer handle for direct pixel access
- [ ] `bb_LockBuffer(buf)`, `bb_UnlockBuffer(buf)`
- [ ] `bb_ReadPixel(x, y, buf)` → packed ARGB int
- [ ] `bb_WritePixel(x, y, color, buf)`
- [ ] `bb_ReadPixelFast(x, y, buf)` (no bounds check), `bb_WritePixelFast`
- [ ] `bb_CopyPixel(sx, sy, sbuf, dx, dy, dbuf)`, `bb_CopyPixelFast`
- [ ] `bb_LoadBuffer(buf, file)`, `bb_SaveBuffer(buf, file)`
- [ ] `bb_BufferWidth(buf)`, `bb_BufferHeight(buf)`
- **Test:** `LockBuffer BackBuffer() : WritePixelFast 10,10,Rgb(255,0,0),BackBuffer()`

---

## PHASE L — 3D Graphics: Foundation (bb_graphics3d.h)

### Milestone 47: Graphics3D Init & Scene Control
*Touch: `bb_graphics3d.h` (new file), `bb_sdl.h`*
- [ ] `bb_Graphics3D(w, h, depth, mode)` → SDL3 + OpenGL/Vulkan context
- [ ] `bb_UpdateWorld()`, `bb_RenderWorld()`, `bb_ClearWorld()`
- [ ] `bb_CaptureWorld()` → snapshot scene state
- [ ] `bb_TrisRendered()` → triangle count from last frame
- [ ] `bb_Dither(on)`, `bb_WBuffer(on)`, `bb_AntiAlias(on)`, `bb_Wireframe(on)`
- [ ] `bb_HWMultiTex(on)`, `bb_LoaderMatrix(file, xx,...)`
- **Test:** `Graphics3D 800,600,32,1 : UpdateWorld : RenderWorld : Flip : WaitKey`

### Milestone 48: Scene-Level Settings
*Touch: `bb_graphics3d.h`*
- [ ] `bb_AmbientLight(r, g, b)`
- [ ] `bb_ClearCollisions()`
- [ ] `bb_Collisions(typeA, typeB, method, response)`
- **Test:** `AmbientLight 128,128,128`

---

## PHASE M — 3D: Textures (bb_texture.h)

### Milestone 49: Texture Creation & Loading
*Touch: `bb_texture.h` (new file)*
- [ ] `bb_CreateTexture(w, h, flags)` → texture handle
- [ ] `bb_LoadTexture(file, flags)` → texture handle
- [ ] `bb_LoadAnimTexture(file, flags, fw, fh, first, count)`
- [ ] `bb_FreeTexture(handle)`
- [ ] `bb_TextureWidth(handle)`, `bb_TextureHeight(handle)`, `bb_TextureName(handle)`
- **Test:** `Local tex = LoadTexture("wall.png")`

### Milestone 50: Texture Transforms
*Touch: `bb_texture.h`*
- [ ] `bb_TextureBlend(handle, blend)`
- [ ] `bb_TextureCoords(handle, coords)`
- [ ] `bb_ScaleTexture(handle, u, v)`, `bb_PositionTexture(handle, u, v)`, `bb_RotateTexture(handle, angle)`
- [ ] `bb_TextureBuffer(handle)` → pixel buffer for direct access
- [ ] `bb_SetCubeFace(handle, face)`, `bb_SetCubeMode(handle, mode)`
- **Test:** `ScaleTexture tex, 2.0, 2.0`

---

## PHASE N — 3D: Brushes (bb_brush.h)

### Milestone 51: Brush Creation & Properties
*Touch: `bb_brush.h` (new file)*
- [ ] `bb_CreateBrush(r, g, b)` → brush handle
- [ ] `bb_LoadBrush(file, flags, su, sv)`
- [ ] `bb_FreeBrush(handle)`
- [ ] `bb_BrushColor(handle, r, g, b)`, `bb_BrushAlpha(handle, a)`
- [ ] `bb_BrushShininess(handle, s)`, `bb_BrushBlend(handle, blend)`, `bb_BrushFX(handle, fx)`
- [ ] `bb_BrushTexture(handle, tex, frame, index)`
- [ ] `bb_GetEntityBrush(entity)`, `bb_GetSurfaceBrush(surface)`
- **Test:** `Local b = CreateBrush(255,0,0) : BrushAlpha b, 0.5`

---

## PHASE O — 3D: Mesh & Surface (bb_mesh.h)

### Milestone 52: Primitive Mesh Creation
*Touch: `bb_mesh.h` (new file)*
- [ ] `bb_CreateMesh()`, `bb_CreateCube(parent)`, `bb_CreateSphere(segs, parent)`
- [ ] `bb_CreateCylinder(segs, open, parent)`, `bb_CreateCone(segs, open, parent)`
- [ ] `bb_MeshWidth`, `bb_MeshHeight`, `bb_MeshDepth`
- **Test:** `Local cube = CreateCube()`

### Milestone 53: Mesh Loading & Manipulation
*Touch: `bb_mesh.h`*
- [ ] `bb_LoadMesh(file, parent)`, `bb_LoadAnimMesh(file, parent)`
- [ ] `bb_CopyMesh(mesh, parent)`, `bb_AddMesh(src, dst)`
- [ ] `bb_FlipMesh(mesh)`, `bb_PaintMesh(mesh, brush)`
- [ ] `bb_LightMesh(mesh, r, g, b, range, x, y, z)`
- [ ] `bb_FitMesh(mesh, x, y, z, w, h, d, uniform)`, `bb_ScaleMesh`, `bb_RotateMesh`, `bb_PositionMesh`
- [ ] `bb_UpdateNormals(mesh)`, `bb_MeshesIntersect(m1, m2)`
- **Test:** `Local m = LoadMesh("ship.b3d")`

### Milestone 54: Surface Operations
*Touch: `bb_mesh.h`*
- [ ] `bb_CreateSurface(mesh, brush)`, `bb_FindSurface(mesh, brush)`, `bb_FreeSurface(surf)`
- [ ] `bb_PaintSurface(surf, brush)`, `bb_ClearSurface(surf, verts, tris)`
- [ ] `bb_SurfaceBrush(surf)`, `bb_SurfaceWidth(surf)`, `bb_SurfaceDepth(surf)`
- [ ] `bb_CountSurfaces(mesh)`, `bb_GetSurface(mesh, index)`, `bb_CountVertices(surf)`, `bb_CountTriangles(surf)`
- **Test:** `Local s = CreateSurface(mesh) : Print CountVertices(s)`

### Milestone 55: Vertex & Triangle Operations
*Touch: `bb_mesh.h`*
- [ ] `bb_AddVertex(surf, x, y, z, u, v, w)`
- [ ] `bb_VertexCoords(surf, idx, x, y, z)`, `bb_VertexNormal(surf, idx, nx, ny, nz)`
- [ ] `bb_VertexTexCoords(surf, idx, u, v, w, set)`
- [ ] `bb_VertexColor(surf, idx, r, g, b, a)`, `bb_VertexAlpha(surf, idx, a)`
- [ ] `bb_VertexX/Y/Z/NX/NY/NZ/U/V/W(surf, idx)` — query functions
- [ ] `bb_AddTriangle(surf, v0, v1, v2)`, `bb_TriangleVertex(surf, tri, corner)`
- **Test:** `AddVertex surf, 0,1,0`

---

## PHASE P — 3D: Terrain & Loaders (bb_terrain.h, bb_loader.h)

### Milestone 56: Terrain
*Touch: `bb_terrain.h` (new file)*
- [ ] `bb_CreateTerrain(size, parent)`, `bb_LoadTerrain(file, parent)`
- [ ] `bb_TerrainSize(terrain)`, `bb_TerrainDetail(terrain, detail, morph)`
- [ ] `bb_TerrainShading(terrain, on)`
- [ ] `bb_TerrainHeight(terrain, x, z)` → float height
- [ ] `bb_ModifyTerrain(terrain, x, z, height, realtime)`
- [ ] `bb_TerrainX/Y/Z(terrain, x, h, z)` → world coords
- **Test:** `Local t = CreateTerrain(64)`

### Milestone 57: MD2 & BSP Loaders
*Touch: `bb_loader.h` (new file)*
- [ ] `bb_LoadMD2(file, parent)` → entity handle
- [ ] `bb_AnimateMD2(entity, mode, speed, first, last, transition)`
- [ ] `bb_MD2AnimTime(entity)`, `bb_MD2AnimLength(entity)`, `bb_MD2Animating(entity)`
- [ ] `bb_LoadBSP(file, grav, light_gamma, ambient)`
- [ ] `bb_BSPAmbientLight(r, g, b)`, `bb_BSPLighting(entity, on)`
- **Test:** `Local md2 = LoadMD2("soldier.md2")`

---

## PHASE Q — 3D: Entity Core (bb_entity.h)

### Milestone 58: Entity Creation & Hierarchy
*Touch: `bb_entity.h` (new file)*
- [ ] `bb_CreatePivot(parent)` → pivot entity handle
- [ ] `bb_CopyEntity(entity, parent)`
- [ ] `bb_FreeEntity(entity)` → recursive free
- [ ] `bb_EntityParent(entity, parent, glob)` (re-parent)
- [ ] `bb_EntityOrder(entity, order)` (render order)
- [ ] `bb_GetParent(entity)`, `bb_FindChild(entity, name)`
- [ ] `bb_CountChildren(entity)`
- **Test:** `Local p = CreatePivot() : FreeEntity p`

### Milestone 59: Entity Transforms
*Touch: `bb_entity.h`*
- [ ] `bb_PositionEntity(entity, x, y, z, glob)`
- [ ] `bb_MoveEntity(entity, dx, dy, dz)`
- [ ] `bb_TranslateEntity(entity, dx, dy, dz, glob)`
- [ ] `bb_RotateEntity(entity, rx, ry, rz, glob)`
- [ ] `bb_TurnEntity(entity, rx, ry, rz, glob)`
- [ ] `bb_ScaleEntity(entity, sx, sy, sz, glob)`
- [ ] `bb_PointEntity(entity, target, roll)`
- [ ] `bb_AlignToVector(entity, nx, ny, nz, axis, rate)`
- **Test:** `PositionEntity cube, 0,0,5`

### Milestone 60: Entity State & Appearance
*Touch: `bb_entity.h`*
- [ ] `bb_HideEntity(entity)`, `bb_ShowEntity(entity)`
- [ ] `bb_EntityAlpha(entity, alpha)`, `bb_EntityColor(entity, r, g, b)`
- [ ] `bb_EntityShininess(entity, s)`, `bb_EntityBlend(entity, blend)`, `bb_EntityFX(entity, fx)`
- [ ] `bb_EntityAutoFade(entity, near, far)`
- [ ] `bb_EntityTexture(entity, tex, frame, index)`, `bb_PaintEntity(entity, brush)`
- **Test:** `EntityColor cube, 255,0,0 : EntityAlpha cube, 0.5`

### Milestone 61: Entity Info Queries
*Touch: `bb_entity.h`*
- [ ] `bb_EntityX/Y/Z(entity, glob)`, `bb_EntityRoll/Yaw/Pitch(entity, glob)`
- [ ] `bb_EntityName(entity)`, `bb_EntityClass(entity)`
- [ ] `bb_EntityDistance(e1, e2)`
- [ ] `bb_EntityVisible(entity, target)`
- [ ] `bb_ResetEntity(entity)` (clears velocity/collision state)
- **Test:** `Print EntityX(cube, True)`

---

## PHASE R — 3D: Camera (bb_camera.h)

### Milestone 62: Camera Creation & Projection
*Touch: `bb_camera.h` (new file)*
- [ ] `bb_CreateCamera(parent)` → camera entity
- [ ] `bb_CameraProjMode(cam, mode)` (perspective / ortho)
- [ ] `bb_CameraRange(cam, near, far)`
- [ ] `bb_CameraZoom(cam, zoom)`
- [ ] `bb_CameraViewport(cam, x, y, w, h)`
- [ ] `bb_CameraClsMode(cam, cls_color, cls_zbuf)`
- [ ] `bb_CameraClsColor(cam, r, g, b)`
- **Test:** `Local cam = CreateCamera() : CameraRange cam, 1, 1000`

### Milestone 63: Camera Fog & Picking
*Touch: `bb_camera.h`*
- [ ] `bb_CameraFogMode(cam, mode)`, `bb_CameraFogRange(cam, near, far)`, `bb_CameraFogColor(cam, r, g, b)`
- [ ] `bb_CameraPick(cam, x, y)` → entity at screen position
- [ ] `bb_PickedX/Y/Z()`, `bb_PickedNX/NY/NZ()`, `bb_PickedTime()`
- [ ] `bb_PickedEntity()`, `bb_PickedSurface()`, `bb_PickedTriangle()`
- [ ] `bb_CameraProject(cam, x, y, z)` → projects 3D to 2D
- [ ] `bb_ProjectedX/Y/Z()`, `bb_EntityInView(entity, cam)`
- **Test:** `CameraPick cam, MouseX(), MouseY()`

---

## PHASE S — 3D: Lights & Sprites (bb_light.h, bb_sprite.h)

### Milestone 64: Lights
*Touch: `bb_light.h` (new file)*
- [ ] `bb_CreateLight(type, parent)` → light entity (0=dir, 1=point, 2=spot)
- [ ] `bb_LightColor(light, r, g, b)`
- [ ] `bb_LightRange(light, range)`
- [ ] `bb_LightConeAngles(light, inner, outer)` (spot light)
- **Test:** `Local l = CreateLight(1) : LightRange l, 50`

### Milestone 65: Sprites & Planes
*Touch: `bb_sprite.h` (new file)*
- [ ] `bb_CreateSprite(parent)`, `bb_LoadSprite(file, flags, parent)`
- [ ] `bb_RotateSprite(sprite, angle)`, `bb_ScaleSprite(sprite, sx, sy)`
- [ ] `bb_HandleSprite(sprite, hx, hy)`, `bb_SpriteViewMode(sprite, mode)`
- [ ] `bb_CreatePlane(segs, parent)`, `bb_CreateMirror(parent)`
- **Test:** `Local s = CreateSprite() : ScaleSprite s, 2, 2`

---

## PHASE T — 3D: Collision & Animation (bb_collision.h)

### Milestone 66: Entity Collision Setup
*Touch: `bb_collision.h` (new file)*
- [ ] `bb_EntityRadius(entity, xr, yr)`, `bb_EntityBox(entity, x, y, z, w, h, d)`
- [ ] `bb_EntityType(entity, type, recurse)`, `bb_EntityPickMode(entity, mode, obscure)`
- [ ] `bb_GetEntityType(entity)`
- **Test:** `EntityRadius cube, 1.0`

### Milestone 67: Collision Query
*Touch: `bb_collision.h`*
- [ ] `bb_EntityCollided(entity, type)` → collided entity
- [ ] `bb_CountCollisions(entity)` → number of collisions
- [ ] `bb_CollisionX/Y/Z(entity, index)` → collision point
- [ ] `bb_CollisionNX/NY/NZ(entity, index)` → collision normal
- [ ] `bb_CollisionTime(entity, index)`, `bb_CollisionEntity(entity, index)`
- [ ] `bb_CollisionSurface(entity, index)`, `bb_CollisionTriangle(entity, index)`
- **Test:** `If EntityCollided(player, 1) Then Print "hit"`

### Milestone 68: Animation Keys
*Touch: `bb_animation.h` (new file)*
- [ ] `bb_LoadAnimSeq(entity, file)` → sequence index
- [ ] `bb_SetAnimKey(entity, frame, pos, rot, scale)`
- [ ] `bb_AddAnimSeq(entity, length)`, `bb_ExtractAnimSeq(entity, first, last, seq)`
- **Test:** `LoadAnimSeq soldier, "walk.b3d"`

### Milestone 69: Animation Playback
*Touch: `bb_animation.h`*
- [ ] `bb_Animate(entity, mode, speed, seq, transition)`
- [ ] `bb_SetAnimTime(entity, time, seq)`
- [ ] `bb_AnimSeq(entity)`, `bb_AnimLength(entity, seq)`, `bb_AnimTime(entity)`, `bb_Animating(entity)`
- **Test:** `Animate soldier, 1, 1.0 : If Animating(soldier) Then Print "running"`

---

## PHASE U — 3D: Math Utilities (bb_3dmath.h)

### Milestone 70: Vector & Transform Math
*Touch: `bb_3dmath.h` (new file)*
- [ ] `bb_VectorDistance(x1, y1, z1, x2, y2, z2)` (→ float)
- [ ] `bb_VectorYaw(dx, dy, dz)`, `bb_VectorPitch(dx, dy, dz)` → angles in degrees
- [ ] `bb_TFormPoint(x, y, z, src, dst)`, `bb_TFormVector(x, y, z, src, dst)`, `bb_TFormNormal(x, y, z, src, dst)`
- [ ] `bb_TFormedX()`, `bb_TFormedY()`, `bb_TFormedZ()` → last TForm result
- [ ] `bb_GetMatElement(entity, row, col)` → matrix element
- **Test:** `Print VectorDistance(0,0,0, 3,4,0)` → 5.0

---

## Command Parity Progress
Based on [kippykip.com Docs](https://kippykip.com/b3ddocs/commands/index.htm)

### 2D / Core Modules
- [x] **Basic (Language)**
    - [x] If, Then, Else, ElseIf, EndIf, Select, Case, Default, End Select
    - [x] And, Or, Not, Xor, Repeat, Until, Forever, While, Wend, For, To, Step, Next, Exit
    - [x] Global, Local, Dim (declaration only)
    - [x] Function, End Function, Return (value and bare)
    - [x] True, False, Null
    - [x] Include (`#Include` preprocessor)
    - [x] Goto, Gosub *(M11)*
    - [x] Const *(M9)*
    - [x] Dim indexing *(M10)*
    - [x] Type, Field, End Type, New, Delete, Each, First, Last, Before, After, Insert *(M13–M16)*
    - [x] Data, Read, Restore *(M12)*
- [ ] **Maths** *(M17–M18)*
    - [x] Pi, Sgn, ASin, ACos, ATan, ATan2, Log10, Int(float) *(M17)*
    - [ ] Rnd, Rand, SeedRnd, RndSeed *(M18)*
    - [x] Abs, Sqr, Sin, Cos, Tan, Exp, Log, Floor, Ceil, Int, Float
    - [x] Mod, Shl, Shr, Sar, Xor (operators)
- [ ] **String** *(M19–M20)*
    - [x] Str, Len
    - [ ] Left, Right, Mid, Replace, Instr, Upper, Lower, Trim, LSet, RSet, Chr, Asc, Hex, Bin, String
- [ ] **Text / Output**
    - [x] Print (console)
    - [ ] Write, Locate *(M42)* ; Text (positioned) *(M42)*
    - [ ] LoadFont, SetFont, FreeFont, FontWidth, FontHeight, StringWidth, StringHeight *(M43)*
- [ ] **Input** *(M31–M33)*
    - [x] Input (console readline), WaitKey (console)
    - [ ] KeyDown, KeyHit, GetKey, FlushKeys, MoveMouse, MouseDown, MouseHit, GetMouse, WaitMouse
    - [ ] MouseX, MouseY, MouseZ, MouseXSpeed, MouseYSpeed, MouseZSpeed, FlushMouse
    - [ ] JoyType, JoyDown, JoyHit, GetJoy, WaitJoy, JoyX, JoyY, JoyZ, JoyU, JoyV, JoyHat, FlushJoy
- [ ] **Bank** *(M28–M29)*
    - [ ] CreateBank, FreeBank, BankSize, ResizeBank, CopyBank, PeekByte, PeekShort, PeekInt, PeekFloat, PokeByte, PokeShort, PokeInt, PokeFloat
- [ ] **File / Stream** *(M24–M27)*
    - [x] OpenFile, ReadFile, WriteFile, CloseFile, FilePos, SeekFile, Eof, ReadAvail *(M24)*
    - [x] ReadDir, CloseDir, NextFile, CurrentDir, ChangeDir, CreateDir, DeleteDir, FileType, FileSize, CopyFile, DeleteFile *(M27)*
    - [x] ReadByte, ReadShort, ReadInt, ReadFloat, ReadString, ReadLine, ReadBytes(stub) *(M25)*
    - [x] WriteByte, WriteShort, WriteInt, WriteFloat, WriteString, WriteLine, WriteBytes(stub) *(M26)*
- [ ] **Time / System** *(M21–M23)*
    - [x] Delay, MilliSecs, CurrentDate, CurrentTime *(M21)*
    - [x] CreateTimer, FreeTimer, WaitTimer *(M22)*
    - [x] ShowPointer, HidePointer, AppTitle, CommandLine, SystemProperty, SetEnv, GetEnv, CallDLL, ExecFile, RuntimeError *(M23)*
    - [ ] End *(already emitted as bbEnd())*

### 3D Module
- [ ] **Global / Scene** *(M47–M48)*
    - [ ] Graphics3D, Dither, WBuffer, AntiAlias, Wireframe, HWMultiTex, AmbientLight, ClearCollisions, Collisions
    - [ ] UpdateWorld, CaptureWorld, RenderWorld, ClearWorld, LoaderMatrix, TrisRendered
- [ ] **Texture** *(M49–M50)*
    - [ ] CreateTexture, LoadTexture, LoadAnimTexture, FreeTexture, TextureBlend, TextureCoords, ScaleTexture, PositionTexture, RotateTexture, TextureWidth, TextureHeight, TextureBuffer, TextureName, SetCubeFace, SetCubeMode
- [ ] **Brush** *(M51)*
    - [ ] CreateBrush, LoadBrush, FreeBrush, BrushColor, BrushAlpha, BrushShininess, BrushTexture, BrushBlend, BrushFX, GetEntityBrush, GetSurfaceBrush
- [ ] **Geometry (Mesh/Surface/Terrain/MD2/BSP)** *(M52–M57)*
    - [ ] CreateMesh, LoadMesh, LoadAnimMesh, CreateCube, CreateSphere, CreateCylinder, CreateCone, CopyMesh, AddMesh, FlipMesh, PaintMesh, LightMesh, FitMesh, ScaleMesh, RotateMesh, PositionMesh, UpdateNormals, MeshesIntersect, MeshWidth, MeshHeight, MeshDepth, CountSurfaces, GetSurface
    - [ ] CreateSurface, FindSurface, FreeSurface, PaintSurface, ClearSurface, SurfaceBrush, SurfaceWidth, SurfaceDepth, CountVertices, CountTriangles, AddVertex, AddTriangle, TriangleVertex
    - [ ] VertexX/Y/Z/NX/NY/NZ/U/V/W, VertexCoords, VertexNormal, VertexTexCoords, VertexColor, VertexAlpha
    - [ ] CreateTerrain, LoadTerrain, TerrainSize, TerrainDetail, TerrainShading, TerrainHeight, ModifyTerrain, TerrainX, TerrainY, TerrainZ
    - [ ] LoadMD2, AnimateMD2, MD2AnimTime, MD2AnimLength, MD2Animating, LoadBSP, BSPAmbientLight, BSPLighting
- [ ] **Entities (Camera/Light/Pivot/Sprite/Plane/Mirror)** *(M58–M65)*
    - [ ] CreateCamera, CameraProjMode, CameraFogMode, CameraFogRange, CameraFogColor, CameraViewport, CameraClsMode, CameraClsColor, CameraRange, CameraZoom, CameraPick, PickedX/Y/Z, PickedNX/NY/NZ, PickedTime, PickedEntity, PickedSurface, PickedTriangle, CameraProject, ProjectedX/Y/Z, EntityInView
    - [ ] CreateLight, LightColor, LightRange
    - [ ] CreatePivot, CreateSprite, LoadSprite, RotateSprite, ScaleSprite, HandleSprite, SpriteViewMode
    - [ ] CreatePlane, CreateMirror
- [ ] **Logic (Movement/Collision/Animation/State/3D Maths)** *(M58–M70)*
    - [ ] ScaleEntity, PositionEntity, MoveEntity, TranslateEntity, RotateEntity, TurnEntity, PointEntity, AlignToVector
    - [ ] ResetEntity, EntityRadius, EntityBox, EntityType, EntityPickMode, EntityCollided, CountCollisions, CollisionX/Y/Z, CollisionNX/NY/NZ, CollisionTime, CollisionEntity, CollisionSurface, CollisionTriangle, GetEntityType
    - [ ] LoadAnimSeq, SetAnimKey, AddAnimSeq, ExtractAnimSeq, Animate, SetAnimTime, AnimSeq, AnimLength, AnimTime, Animating
    - [ ] EntityX/Y/Z, EntityRoll/Yaw/Pitch, EntityName, EntityClass, EntityDistance, EntityVisible, GetParent, FindChild
    - [ ] HideEntity, ShowEntity, EntityParent, EntityOrder, EntityAlpha, EntityColor, EntityShininess, EntityTexture, EntityBlend, EntityFX, EntityAutoFade, PaintEntity, FreeEntity
    - [ ] VectorDistance, VectorYaw, VectorPitch, TFormPoint, TFormVector, TFormNormal, TFormedX/Y/Z, GetMatElement
- [ ] **2D Graphics** *(M38–M46)*
    - [ ] Graphics, GraphicsWidth, GraphicsHeight, GraphicsDepth, GraphicsRate, AvailVidMem, TotalVidMem
    - [ ] SetBuffer, BackBuffer, FrontBuffer, Cls, Flip, CopyRect, GrabImage
    - [ ] Color, ClsColor, GetColor, ColorRed, ColorGreen, ColorBlue, Plot, Line, Rect, Oval, Poly
    - [ ] LoadImage, CreateImage, FreeImage, SaveImage, ImageWidth, ImageHeight, ImageBuffer
    - [ ] DrawImage, DrawImageRect, DrawBlock, DrawBlockRect, DrawImageEllipse, TileImage, TileBlock
    - [ ] MaskImage, HandleImage, MidHandle, AutoMidHandle, ScaleImage, RotateImage, ImagesOverlap, ImageRectOverlap, ImagesColl, ImageXColl, ImageYColl
    - [ ] LockBuffer, UnlockBuffer, ReadPixel, WritePixel, ReadPixelFast, WritePixelFast, CopyPixel, CopyPixelFast, LoadBuffer, SaveBuffer, BufferWidth, BufferHeight
- [ ] **Sound / Interaction** *(M34–M37)*
    - [ ] LoadSound, FreeSound, LoopSound, SoundPitch, SoundVolume, SoundPan, PlaySound, PlayMusic, PlayCDTrack, StopChannel, PauseChannel, ResumeChannel, ChannelPitch, ChannelVolume, ChannelPan, ChannelPlaying
    - [ ] Load3DSound, 3DWaitSound, 3DSoundVolume, 3DSoundPan, 3DChannelVolume, 3DChannelPan
