#ifndef BLITZNEXT_BB_INPUT_H
#define BLITZNEXT_BB_INPUT_H

#include "bb_sdl.h"
#include <array>

// ---- Blitz3D key code ↔ SDL_Scancode mapping ----
//
// Blitz3D uses DIK (DirectInput Key) codes — essentially PC/XT keyboard
// scan set 1 values (1–255).  SDL3 uses USB HID scancodes.
//
// bb_blitz_to_sdl_: Blitz3D code (1–255) → SDL_Scancode
// bb_sdl_to_blitz_: SDL_Scancode (0–511) → Blitz3D code (0 = unmapped)

inline const std::array<SDL_Scancode, 256> bb_blitz_to_sdl_ = []() {
  std::array<SDL_Scancode, 256> m{};  // default: SDL_SCANCODE_UNKNOWN = 0

  // Main keyboard — row by row (DIK layout matches US QWERTY)
  m[1]  = SDL_SCANCODE_ESCAPE;
  m[2]  = SDL_SCANCODE_1;         m[3]  = SDL_SCANCODE_2;
  m[4]  = SDL_SCANCODE_3;         m[5]  = SDL_SCANCODE_4;
  m[6]  = SDL_SCANCODE_5;         m[7]  = SDL_SCANCODE_6;
  m[8]  = SDL_SCANCODE_7;         m[9]  = SDL_SCANCODE_8;
  m[10] = SDL_SCANCODE_9;         m[11] = SDL_SCANCODE_0;
  m[12] = SDL_SCANCODE_MINUS;     m[13] = SDL_SCANCODE_EQUALS;
  m[14] = SDL_SCANCODE_BACKSPACE; m[15] = SDL_SCANCODE_TAB;

  m[16] = SDL_SCANCODE_Q;  m[17] = SDL_SCANCODE_W;  m[18] = SDL_SCANCODE_E;
  m[19] = SDL_SCANCODE_R;  m[20] = SDL_SCANCODE_T;  m[21] = SDL_SCANCODE_Y;
  m[22] = SDL_SCANCODE_U;  m[23] = SDL_SCANCODE_I;  m[24] = SDL_SCANCODE_O;
  m[25] = SDL_SCANCODE_P;
  m[26] = SDL_SCANCODE_LEFTBRACKET;  m[27] = SDL_SCANCODE_RIGHTBRACKET;
  m[28] = SDL_SCANCODE_RETURN;       m[29] = SDL_SCANCODE_LCTRL;

  m[30] = SDL_SCANCODE_A;  m[31] = SDL_SCANCODE_S;  m[32] = SDL_SCANCODE_D;
  m[33] = SDL_SCANCODE_F;  m[34] = SDL_SCANCODE_G;  m[35] = SDL_SCANCODE_H;
  m[36] = SDL_SCANCODE_J;  m[37] = SDL_SCANCODE_K;  m[38] = SDL_SCANCODE_L;
  m[39] = SDL_SCANCODE_SEMICOLON;  m[40] = SDL_SCANCODE_APOSTROPHE;
  m[41] = SDL_SCANCODE_GRAVE;      m[42] = SDL_SCANCODE_LSHIFT;
  m[43] = SDL_SCANCODE_BACKSLASH;

  m[44] = SDL_SCANCODE_Z;  m[45] = SDL_SCANCODE_X;  m[46] = SDL_SCANCODE_C;
  m[47] = SDL_SCANCODE_V;  m[48] = SDL_SCANCODE_B;  m[49] = SDL_SCANCODE_N;
  m[50] = SDL_SCANCODE_M;
  m[51] = SDL_SCANCODE_COMMA;  m[52] = SDL_SCANCODE_PERIOD;
  m[53] = SDL_SCANCODE_SLASH;  m[54] = SDL_SCANCODE_RSHIFT;

  m[55] = SDL_SCANCODE_KP_MULTIPLY;
  m[56] = SDL_SCANCODE_LALT;
  m[57] = SDL_SCANCODE_SPACE;
  m[58] = SDL_SCANCODE_CAPSLOCK;

  // Function keys
  m[59] = SDL_SCANCODE_F1;   m[60] = SDL_SCANCODE_F2;
  m[61] = SDL_SCANCODE_F3;   m[62] = SDL_SCANCODE_F4;
  m[63] = SDL_SCANCODE_F5;   m[64] = SDL_SCANCODE_F6;
  m[65] = SDL_SCANCODE_F7;   m[66] = SDL_SCANCODE_F8;
  m[67] = SDL_SCANCODE_F9;   m[68] = SDL_SCANCODE_F10;
  m[87] = SDL_SCANCODE_F11;  m[88] = SDL_SCANCODE_F12;

  // Numpad
  m[69] = SDL_SCANCODE_NUMLOCKCLEAR;  m[70] = SDL_SCANCODE_SCROLLLOCK;
  m[71] = SDL_SCANCODE_KP_7;          m[72] = SDL_SCANCODE_KP_8;
  m[73] = SDL_SCANCODE_KP_9;          m[74] = SDL_SCANCODE_KP_MINUS;
  m[75] = SDL_SCANCODE_KP_4;          m[76] = SDL_SCANCODE_KP_5;
  m[77] = SDL_SCANCODE_KP_6;          m[78] = SDL_SCANCODE_KP_PLUS;
  m[79] = SDL_SCANCODE_KP_1;          m[80] = SDL_SCANCODE_KP_2;
  m[81] = SDL_SCANCODE_KP_3;          m[82] = SDL_SCANCODE_KP_0;
  m[83] = SDL_SCANCODE_KP_PERIOD;
  m[156] = SDL_SCANCODE_KP_ENTER;
  m[181] = SDL_SCANCODE_KP_DIVIDE;

  // Right-side modifier keys
  m[157] = SDL_SCANCODE_RCTRL;
  m[183] = SDL_SCANCODE_PRINTSCREEN;
  m[184] = SDL_SCANCODE_RALT;
  m[197] = SDL_SCANCODE_PAUSE;

  // Navigation cluster (cursor keys + home/end/pgup/pgdn/ins/del)
  m[199] = SDL_SCANCODE_HOME;      m[200] = SDL_SCANCODE_UP;
  m[201] = SDL_SCANCODE_PAGEUP;    m[203] = SDL_SCANCODE_LEFT;
  m[205] = SDL_SCANCODE_RIGHT;     m[207] = SDL_SCANCODE_END;
  m[208] = SDL_SCANCODE_DOWN;      m[209] = SDL_SCANCODE_PAGEDOWN;
  m[210] = SDL_SCANCODE_INSERT;    m[211] = SDL_SCANCODE_DELETE;

  // Windows / application keys
  m[219] = SDL_SCANCODE_LGUI;
  m[220] = SDL_SCANCODE_RGUI;
  m[221] = SDL_SCANCODE_APPLICATION;

  return m;
}();

// Reverse map: SDL_Scancode (int, 0–511) → Blitz3D key code (0 = unmapped).
// Built at startup from bb_blitz_to_sdl_.
inline const std::array<int, 512> bb_sdl_to_blitz_ = []() {
  std::array<int, 512> m{};
  for (int bb = 1; bb < 256; ++bb) {
    int sc = (int)bb_blitz_to_sdl_[(size_t)bb];
    if (sc > 0 && sc < 512) m[(size_t)sc] = bb;
  }
  return m;
}();

// ---- Keyboard API ----

// Returns non-zero if the key (Blitz3D code) is currently held down.
// Pumps SDL events first if SDL is running (keeps state fresh in game loops).
inline int bb_KeyDown(int code) {
  if (bb_sdl_initialized_) bb_PollEvents();
  if (code < 1 || code > 255) return 0;
  int sc = (int)bb_blitz_to_sdl_[(size_t)code];
  if (sc <= 0) return 0;
  return bb_sdl_key_down_[sc] ? 1 : 0;
}

// Returns non-zero if the key was pressed since the last call to KeyHit
// for that key.  Edge-triggered: the flag is cleared after reading.
inline int bb_KeyHit(int code) {
  if (bb_sdl_initialized_) bb_PollEvents();
  if (code < 1 || code > 255) return 0;
  int sc = (int)bb_blitz_to_sdl_[(size_t)code];
  if (sc <= 0) return 0;
  bool hit = bb_sdl_key_hit_raw_[sc];
  bb_sdl_key_hit_raw_[sc] = false;
  return hit ? 1 : 0;
}

// Blocks until any key is pressed; returns its Blitz3D key code.
// Returns 0 if SDL cannot be initialized.
inline int bb_GetKey() {
  bb_sdl_ensure_();
  if (!bb_sdl_initialized_) return 0;
  bb_PollEvents();
  while (bb_key_queue_head_ == bb_key_queue_tail_) {
    SDL_Event ev;
    if (!SDL_WaitEvent(&ev)) break;
    bb_sdl_process_event_(ev);
  }
  if (bb_key_queue_head_ == bb_key_queue_tail_) return 0;
  SDL_Scancode sc = bb_key_queue_buf_[bb_key_queue_head_];
  bb_key_queue_head_ = (bb_key_queue_head_ + 1) % BB_KEY_QUEUE_CAP;
  int sc_int = (int)sc;
  return (sc_int > 0 && sc_int < 512) ? bb_sdl_to_blitz_[(size_t)sc_int] : 0;
}

// Clears all keyboard state: held flags, edge-triggered flags, and the queue.
inline void bb_FlushKeys() {
  for (int i = 0; i < 512; ++i) {
    bb_sdl_key_down_[i]    = false;
    bb_sdl_key_hit_raw_[i] = false;
  }
  bb_key_queue_head_ = bb_key_queue_tail_ = 0;
  if (bb_sdl_initialized_)
    SDL_FlushEvents(SDL_EVENT_KEY_DOWN, SDL_EVENT_KEY_UP);
}

// ---- Mouse API ----

// Current cursor position (pixels, relative to window top-left).
inline int bb_MouseX() {
  if (bb_sdl_initialized_) bb_PollEvents();
  // Als gelesen gilt, was das Programm bekommt; der abgeschnittene Bruchteil
  // bleibt ungelesen und wandert bei MoveMouse mit (BUG-165) - im skalierten
  // Vollbild ist ein Bildschirmpixel nur ein Bruchteil eines Spielpixels.
  bb_mouse_read_x_ = std::floor(bb_mouse_x_);
  return (int)bb_mouse_read_x_;
}
inline int bb_MouseY() {
  if (bb_sdl_initialized_) bb_PollEvents();
  // Als gelesen gilt, was das Programm bekommt; der abgeschnittene Bruchteil
  // bleibt ungelesen und wandert bei MoveMouse mit (BUG-165) - im skalierten
  // Vollbild ist ein Bildschirmpixel nur ein Bruchteil eines Spielpixels.
  bb_mouse_read_y_ = std::floor(bb_mouse_y_);
  return (int)bb_mouse_read_y_;
}

// Scroll-wheel accumulator (ticks; positive = scroll up).
inline int bb_MouseZ() {
  if (bb_sdl_initialized_) bb_PollEvents();
  return (int)bb_mouse_z_;
}

// Wie bbinput.cpp: die Geschwindigkeit ist der Abstand der Position zum
// letzten Aufruf bzw. zum letzten MoveMouse, nicht die Summe der
// Bewegungsereignisse. Sonst zaehlte der Sprung von MoveMouse selbst als
// Bewegung (BUG-159). Ausgangspunkt: bb_mouse_speed_x_/y_ in bb_sdl.h.
inline int bb_MouseXSpeed() {
  const int x = bb_MouseX();
  const int dx = x - bb_mouse_speed_x_;
  bb_mouse_speed_x_ = x;
  return dx;
}
inline int bb_MouseYSpeed() {
  const int y = bb_MouseY();
  const int dy = y - bb_mouse_speed_y_;
  bb_mouse_speed_y_ = y;
  return dy;
}
inline int bb_MouseZSpeed() {
  if (bb_sdl_initialized_) bb_PollEvents();
  int v = (int)bb_mouse_zrel_;
  bb_mouse_zrel_ = 0.0f;
  return v;
}

// Returns non-zero while button is held (1=left, 2=right, 3=middle).
inline int bb_MouseDown(int btn) {
  if (bb_sdl_initialized_) bb_PollEvents();
  if (btn < 1 || btn > 3) return 0;
  return bb_mouse_down_[btn] ? 1 : 0;
}

// Returns non-zero if button was pressed since the last MouseHit call
// for that button.  Edge-triggered: flag is cleared after reading.
inline int bb_MouseHit(int btn) {
  if (bb_sdl_initialized_) bb_PollEvents();
  if (btn < 1 || btn > 3) return 0;
  bool hit = bb_mouse_hit_[btn];
  bb_mouse_hit_[btn] = false;
  return hit ? 1 : 0;
}

// Blocks until any mouse button is pressed; returns button number (1/2/3).
inline int bb_WaitMouse() {
  bb_sdl_ensure_();
  if (!bb_sdl_initialized_) return 1;
  bb_PollEvents();
  while (bb_mouse_queue_head_ == bb_mouse_queue_tail_) {
    SDL_Event ev;
    if (!SDL_WaitEvent(&ev)) break;
    bb_sdl_process_event_(ev);
  }
  if (bb_mouse_queue_head_ == bb_mouse_queue_tail_) return 1;
  int btn = bb_mouse_queue_buf_[bb_mouse_queue_head_];
  bb_mouse_queue_head_ = (bb_mouse_queue_head_ + 1) % BB_MOUSE_QUEUE_CAP;
  return btn;
}

// Alias (Blitz3D compat — GetMouse() = WaitMouse()).
inline int bb_GetMouse() { return bb_WaitMouse(); }

// Clears all mouse state: held/hit flags, speed accumulators, and the queue.
inline void bb_FlushMouse() {
  for (int i = 0; i < 4; ++i) {
    bb_mouse_down_[i] = false;
    bb_mouse_hit_[i]  = false;
  }
  bb_mouse_zrel_ = 0.0f;
  bb_mouse_queue_head_ = bb_mouse_queue_tail_ = 0;
  // Bewegungen bleiben erhalten: gxDevice::flush im Original setzt nur Tasten
  // und Warteschlange zurueck. Vorher warf das hier auch die noch nicht
  // abgeholten Bewegungsereignisse weg (BUG-165).
  if (bb_sdl_initialized_)
    SDL_FlushEvents(SDL_EVENT_MOUSE_BUTTON_DOWN, SDL_EVENT_MOUSE_WHEEL);
}

// HidePointer hat den Zeiger versteckt (ShowPointer zeigt ihn wieder).
inline bool bb_pointer_hidden_ = false;

// Setzt die Maus auf (x, y) in Spielkoordinaten. Wie im Original gilt die neue
// Position sofort und ist zugleich Ausgangspunkt fuer MouseXSpeed/YSpeed. Im
// skalierten Vollbild wird in Fensterkoordinaten umgerechnet (BUG-159).
inline void bb_MoveMouse(int x, int y) {
  bb_sdl_ensure_();
  if (!bb_sdl_initialized_) return;
  bb_PollEvents();   // aeltere Bewegungen duerfen die neue Lage nicht ueberschreiben
  // Was sich seit dem letzten Lesen bewegt hat, gilt ab der neuen Lage weiter
  // (BUG-165): "MouseXSpeed ... RenderWorld : MoveMouse Mitte : Flip" verlor
  // sonst jede Bewegung, die waehrend RenderWorld/Flip eintraf.
  const float ux = bb_mouse_x_ - bb_mouse_read_x_;
  const float uy = bb_mouse_y_ - bb_mouse_read_y_;
  bb_mouse_off_x_ = ux;
  bb_mouse_off_y_ = uy;
  bb_mouse_x_ = (float)x + ux;
  bb_mouse_y_ = (float)y + uy;
  bb_mouse_read_x_ = (float)x;
  bb_mouse_read_y_ = (float)y;
  bb_mouse_speed_x_ = x;
  bb_mouse_speed_y_ = y;
  // Ohne Eingabefokus bleibt der echte Zeiger, wo er ist - bewusst anders als
  // das Original, das im Fenstermodus 2 weiterlaeuft und SetCursorPos auch
  // dann ruft: ein Spiel, das die Maus je Bild zur Mitte holt, fing sonst den
  // Zeiger des ganzen Rechners, sobald man in ein anderes Fenster wechselt
  // (2026-09-27). Das Programm merkt davon nichts: MouseX/Y und der
  // Ausgangspunkt fuer MouseXSpeed sind oben schon gesetzt.
  if (bb_window_) {
    if (SDL_GetWindowFlags(bb_window_) & SDL_WINDOW_INPUT_FOCUS) {
      SDL_WarpMouseInWindow(bb_window_, bb_present_to_window_x_((float)x),
                                        bb_present_to_window_y_((float)y));
      // Versteckter Zeiger, der je Bild zurueckgeholt wird: Mausblick. Der
      // Zeiger bleibt dann im Fenster, bis ShowPointer oder ein Wechsel in
      // ein anderes Fenster ihn freigibt - sonst entwischt er bei einer
      // schnellen Bewegung, und ein Klick trifft das Fenster dahinter.
      // Blitz3D sperrt ihn im Fenstermodus nicht; auf einem heutigen
      // Desktop macht erst das ein Spiel mit Mausblick spielbar.
      if (bb_pointer_hidden_) SDL_SetWindowMouseGrab(bb_window_, true);
    }
  } else {
    SDL_WarpMouseGlobal((float)x, (float)y);
  }
  // SDL legt fuer den Warp ein eigenes Bewegungsereignis mit der Zielposition
  // in die Warteschlange. Wurde es erst beim naechsten MouseXSpeed verarbeitet,
  // setzte es die Lage nach den echten Bewegungen wieder auf das Ziel - jede
  // Bewegung zwischen MoveMouse und dem naechsten Abfragen ging verloren. Die
  // uebliche Schleife "MouseXSpeed ... MoveMouse Mitte : Flip" lieferte so
  // immer 0 (blox-n-balls: Paddle unbeweglich, BUG-165). Das Original kennt
  // dieses Echo nicht; also sofort wegwerfen. (Die Lage stimmt auch ohne
  // diese Zeilen, weil das Echo genau die Ziellage meldet - aber es kostete
  // die ungelesene Bewegung, die der Versatz oben traegt.)
  SDL_PumpEvents();
  SDL_FlushEvent(SDL_EVENT_MOUSE_MOTION);
}

// ---- ShowPointer / HidePointer (BUG-124) ----
//
// Blendet den Zeiger ueber dem Fenster aus bzw. ein. Das ist mehr als Kosmetik:
// ein Blitz-Spiel mit Mausblick versteckt den Zeiger und holt ihn je Bild mit
// MoveMouse zur Mitte. Bei hoher Bildrate verschluckt das Zuruecksetzen fast
// jede Bewegung - gemessen in Friendly Fire bei ~2000 Bildern/s: rund 2 % kamen
// an (2026-09-27). SDL3 erkennt genau dieses Muster (versteckter Zeiger, Warps
// zur Fenstermitte) und liest die Maus dann relativ und roh, ohne den Zeiger
// wirklich zu bewegen (SDL_HINT_MOUSE_EMULATE_WARP_WITH_RELATIVE, Vorgabe an).
// MouseX/MouseY und MouseXSpeed bleiben dabei, was sie waren; verliert das
// Fenster den Fokus, gibt SDL den Zeiger frei.
inline void bb_HidePointer() {
  bb_sdl_ensure_();
  if (!bb_sdl_initialized_) return;
  SDL_HideCursor();
  bb_pointer_hidden_ = true;
}
inline void bb_ShowPointer() {
  bb_sdl_ensure_();
  if (!bb_sdl_initialized_) return;
  SDL_ShowCursor();
  bb_pointer_hidden_ = false;
  if (bb_window_) SDL_SetWindowMouseGrab(bb_window_, false);
}

// ---- Joystick API ----
//
// Nach bbinput.cpp und gxinput.cpp des Originals (BUG-195). Ports 0-basiert,
// Knoepfe 1-basiert. Ein Port ohne Geraet liefert ueberall 0.
//
// Achsen liest das Original bei jeder Abfrage per DirectInput in
// axis_states[0..8]: X, Y, Z, Schieber 0 (U), Schieber 1 (V), Rx (Pitch),
// Ry (Yaw), Rz (Roll), POV (Hat in Grad, Mitte -1). Jeder Wert ist
// (roh - min) / Bereich * 2 - 1; eine Achse, die das Geraet nicht hat,
// steht auf 0 im Bereich 0..65535 und meldet damit -1 (am Original mit
// einem Xbox-360-Pad gemessen: U = V = -1, Roll = -180, 2026-10-04).
//
// Einen Xbox-Controller meldet DirectInput so: linker Stick X/Y, beide
// Trigger zusammen auf Z, rechter Stick Rx/Ry. Ein Geraet, das SDL als
// Gamepad kennt, wird ueber die Gamepad-Schnittstelle in genau diese
// Belegung uebersetzt; jedes andere liefert seine Achsen in SDL-Reihenfolge,
// die unter Windows der DirectInput-Reihenfolge X, Y, Z, Rx, Ry, Rz,
// Schieber folgt.

// DirectInput-Rohwert 0..65535 in -1..1 wie gxinput.cpp.
static inline float bb_joy_norm_(int raw) {
  if (raw < 0) raw = 0;
  if (raw > 65535) raw = 65535;
  return raw / 65535.0f * 2 - 1;
}

static inline bb_JoyPort_* bb_joy_dev_(int port) {
  if (bb_sdl_initialized_) bb_PollEvents();
  if (port < 0 || port >= BB_JOY_MAX_PORTS || !bb_joy_[port].handle) return nullptr;
  return &bb_joy_[port];
}

// axis_states[k] des Originals fuer ein angeschlossenes Geraet.
static inline float bb_joy_axis_(const bb_JoyPort_& j, int k) {
  if (j.pad) {
    SDL_Gamepad* g = j.pad;
    // Sticks: Ruhe 0 wird zur DirectInput-Mitte 32767 (-1.5e-5 wie gemessen).
    auto stick = [&](SDL_GamepadAxis a) {
      return bb_joy_norm_(SDL_GetGamepadAxis(g, a) + 32767);
    };
    switch (k) {
      case 0: return stick(SDL_GAMEPAD_AXIS_LEFTX);
      case 1: return stick(SDL_GAMEPAD_AXIS_LEFTY);
      case 2: return bb_joy_norm_(32767 + SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_LEFT_TRIGGER)
                                        - SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER));
      case 5: return stick(SDL_GAMEPAD_AXIS_RIGHTX);
      case 6: return stick(SDL_GAMEPAD_AXIS_RIGHTY);
      case 8: {
        const bool u = SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_DPAD_UP);
        const bool d = SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
        const bool l = SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_DPAD_LEFT);
        const bool r = SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
        return static_cast<float>(bb_sdl_hat_to_blitz_(static_cast<Uint8>(
            (u ? SDL_HAT_UP : 0) | (d ? SDL_HAT_DOWN : 0) |
            (l ? SDL_HAT_LEFT : 0) | (r ? SDL_HAT_RIGHT : 0))));
      }
      default: return -1.0f;   // Schieber und Rz hat ein Xbox-Pad nicht
    }
  }
  if (k == 8)
    return SDL_GetNumJoystickHats(j.handle) > 0
        ? static_cast<float>(bb_sdl_hat_to_blitz_(SDL_GetJoystickHat(j.handle, 0)))
        : -1.0f;
  // SDL-Achse i -> axis_states-Platz: X, Y, Z, Rx, Ry, Rz, Schieber 0, 1.
  static const int from_sdl[8] = { 0, 1, 2, 5, 6, 7, 3, 4 };
  const int n = SDL_GetNumJoystickAxes(j.handle);
  for (int i = 0; i < n && i < 8; ++i)
    if (from_sdl[i] == k)
      return bb_joy_norm_(SDL_GetJoystickAxis(j.handle, i) + 32768);
  return -1.0f;
}

static inline float bb_joy_state_(int port, int k) {
  const bb_JoyPort_* j = bb_joy_dev_(port);
  return j ? bb_joy_axis_(*j, k) : 0.0f;
}

// 1 = Gamepad, 2 = anderer Joystick, wie DIDEVTYPEJOYSTICK_GAMEPAD im
// Original (gemessen: Xbox-360-Pad = 1).
inline int bb_JoyType(int port = 0) {
  const bb_JoyPort_* j = bb_joy_dev_(port);
  return j ? (j->pad ? 1 : 2) : 0;
}

inline float bb_JoyX(int port = 0) { return bb_joy_state_(port, 0); }
inline float bb_JoyY(int port = 0) { return bb_joy_state_(port, 1); }
inline float bb_JoyZ(int port = 0) { return bb_joy_state_(port, 2); }
inline float bb_JoyU(int port = 0) { return bb_joy_state_(port, 3); }
inline float bb_JoyV(int port = 0) { return bb_joy_state_(port, 4); }
inline float bb_JoyPitch(int port = 0) { return bb_joy_state_(port, 5) * 180; }
inline float bb_JoyYaw(int port = 0)   { return bb_joy_state_(port, 6) * 180; }
inline float bb_JoyRoll(int port = 0)  { return bb_joy_state_(port, 7) * 180; }
inline int   bb_JoyHat(int port = 0)   { return static_cast<int>(bb_joy_state_(port, 8)); }

// JoyXDir bis JoyVDir: -1, 0 oder 1 mit der Schwelle 1/3 (JLT/JHT).
static inline int bb_joy_dir_(int port, int k) {
  const bb_JoyPort_* j = bb_joy_dev_(port);
  if (!j) return 0;
  const float t = bb_joy_axis_(*j, k);
  const float JLT = -1.0f / 3.0f, JHT = 1.0f / 3.0f;
  return t < JLT ? -1 : (t > JHT ? 1 : 0);
}
inline int bb_JoyXDir(int port = 0) { return bb_joy_dir_(port, 0); }
inline int bb_JoyYDir(int port = 0) { return bb_joy_dir_(port, 1); }
inline int bb_JoyZDir(int port = 0) { return bb_joy_dir_(port, 2); }
inline int bb_JoyUDir(int port = 0) { return bb_joy_dir_(port, 3); }
inline int bb_JoyVDir(int port = 0) { return bb_joy_dir_(port, 4); }

// Button zuerst, Port optional - im Original `JoyDown ( button[,port] )`.
// Bei uns standen beide vertauscht (BUG-44).
inline int bb_JoyDown(int btn, int port = 0) {
  const bb_JoyPort_* j = bb_joy_dev_(port);
  if (!j || btn < 1 || btn > BB_JOY_MAX_BUTTONS) return 0;
  return j->btn_down[btn - 1] ? 1 : 0;
}

// Anzahl der Druecke seit dem letzten Aufruf (gxDevice::keyHit).
inline int bb_JoyHit(int btn, int port = 0) {
  bb_JoyPort_* j = bb_joy_dev_(port);
  if (!j || btn < 1 || btn > BB_JOY_MAX_BUTTONS) return 0;
  const int n = j->btn_hit[btn - 1];
  j->btn_hit[btn - 1] = 0;
  return n;
}

// Naechster Knopf aus der Warteschlange, sonst 0 - wartet nicht.
inline int bb_GetJoy(int port = 0) {
  bb_JoyPort_* j = bb_joy_dev_(port);
  if (!j || j->btn_q_head == j->btn_q_tail) return 0;
  const int btn = j->btn_queue[j->btn_q_head];
  j->btn_q_head = (j->btn_q_head + 1) % BB_JOY_BTN_QUEUE_CAP;
  return btn;
}

// Wartet auf einen Knopf; ein Port ohne Geraet liefert sofort 0.
inline int bb_WaitJoy(int port = 0) {
  bb_sdl_ensure_();
  if (!bb_sdl_initialized_ || !bb_joy_dev_(port)) return 0;
  for (;;) {
    if (int btn = bb_GetJoy(port)) return btn;
    if (!bb_joy_[port].handle) return 0;   // abgezogen
    SDL_Delay(20);
  }
}

// MouseWait und JoyWait sind im Original dieselben Funktionen wie
// WaitMouse und WaitJoy.
inline int bb_MouseWait() { return bb_WaitMouse(); }
inline int bb_JoyWait(int port = 0) { return bb_WaitJoy(port); }

// DirectInput gibt es hier nicht; SDL liest Tastatur, Maus und Joystick
// immer. Der Schalter wird nur gemerkt, damit ein Programm, das ihn setzt
// und wieder abfragt, seinen Wert zurueckbekommt. Vorgabe aus wie
// use_di(false) in gxruntime.cpp, am Original gemessen (2026-10-04).
inline bool bb_direct_input_ = false;
inline void bb_EnableDirectInput(int enable) { bb_direct_input_ = (enable != 0); }
inline int  bb_DirectInputEnabled() { return bb_direct_input_ ? 1 : 0; }

// Ohne Port: im Original ist `FlushJoy` parameterlos und raeumt damit jeden
// Port ab (BUG-44). Wie gxDevice::flush leert es Treffer und Warteschlange;
// gehaltene Knoepfe bleiben gehalten.
inline void bb_FlushJoy() {
  if (bb_sdl_initialized_) bb_PollEvents();
  for (int p = 0; p < BB_JOY_MAX_PORTS; ++p) {
    bb_JoyPort_ &jp = bb_joy_[p];
    for (int i = 0; i < BB_JOY_MAX_BUTTONS; ++i) jp.btn_hit[i] = 0;
    jp.btn_q_head = jp.btn_q_tail = 0;
  }
}

#endif // BLITZNEXT_BB_INPUT_H
