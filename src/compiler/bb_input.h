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
  return (int)bb_mouse_x_;
}
inline int bb_MouseY() {
  if (bb_sdl_initialized_) bb_PollEvents();
  return (int)bb_mouse_y_;
}

// Scroll-wheel accumulator (ticks; positive = scroll up).
inline int bb_MouseZ() {
  if (bb_sdl_initialized_) bb_PollEvents();
  return (int)bb_mouse_z_;
}

// Delta since last call (accumulator is reset to zero on each read).
inline int bb_MouseXSpeed() {
  if (bb_sdl_initialized_) bb_PollEvents();
  int v = (int)bb_mouse_xrel_;
  bb_mouse_xrel_ = 0.0f;
  return v;
}
inline int bb_MouseYSpeed() {
  if (bb_sdl_initialized_) bb_PollEvents();
  int v = (int)bb_mouse_yrel_;
  bb_mouse_yrel_ = 0.0f;
  return v;
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
  bb_mouse_xrel_ = bb_mouse_yrel_ = bb_mouse_zrel_ = 0.0f;
  bb_mouse_queue_head_ = bb_mouse_queue_tail_ = 0;
  if (bb_sdl_initialized_)
    SDL_FlushEvents(SDL_EVENT_MOUSE_MOTION, SDL_EVENT_MOUSE_WHEEL);
}

// Warps the cursor to (x, y) within the window; global warp if no window yet.
inline void bb_MoveMouse(int x, int y) {
  bb_sdl_ensure_();
  if (!bb_sdl_initialized_) return;
  if (bb_window_)
    SDL_WarpMouseInWindow(bb_window_, (float)x, (float)y);
  else
    SDL_WarpMouseGlobal((float)x, (float)y);
}

// ---- Joystick API ----
//
// port: 0-based port index (0 = first joystick connected).
// btn:  1-based button number.

// Returns 0 = no device, 1 = joystick, 2 = gamepad.
inline int bb_JoyType(int port) {
  if (bb_sdl_initialized_) bb_PollEvents();
  if (port < 0 || port >= BB_JOY_MAX_PORTS) return 0;
  if (!bb_joy_[port].handle) return 0;
  return bb_joy_[port].is_gamepad ? 2 : 1;
}

// Axis values: -1.0 to 1.0.
inline float bb_JoyX(int port) {
  if (bb_sdl_initialized_) bb_PollEvents();
  if (port < 0 || port >= BB_JOY_MAX_PORTS) return 0.0f;
  return bb_joy_[port].x;
}
inline float bb_JoyY(int port) {
  if (bb_sdl_initialized_) bb_PollEvents();
  if (port < 0 || port >= BB_JOY_MAX_PORTS) return 0.0f;
  return bb_joy_[port].y;
}
inline float bb_JoyZ(int port) {
  if (bb_sdl_initialized_) bb_PollEvents();
  if (port < 0 || port >= BB_JOY_MAX_PORTS) return 0.0f;
  return bb_joy_[port].z;
}
inline float bb_JoyU(int port) {
  if (bb_sdl_initialized_) bb_PollEvents();
  if (port < 0 || port >= BB_JOY_MAX_PORTS) return 0.0f;
  return bb_joy_[port].u;
}
inline float bb_JoyV(int port) {
  if (bb_sdl_initialized_) bb_PollEvents();
  if (port < 0 || port >= BB_JOY_MAX_PORTS) return 0.0f;
  return bb_joy_[port].v;
}

// Hat direction: 0=center, 1=up, 2=up-right, 3=right, 4=down-right,
//                5=down, 6=down-left, 7=left, 8=up-left.
inline int bb_JoyHat(int port) {
  if (bb_sdl_initialized_) bb_PollEvents();
  if (port < 0 || port >= BB_JOY_MAX_PORTS) return 0;
  return bb_joy_[port].hat;
}

// Returns non-zero while button (1-based) is held.
inline int bb_JoyDown(int port, int btn) {
  if (bb_sdl_initialized_) bb_PollEvents();
  if (port < 0 || port >= BB_JOY_MAX_PORTS) return 0;
  if (btn < 1 || btn > BB_JOY_MAX_BUTTONS) return 0;
  return bb_joy_[port].btn_down[btn - 1] ? 1 : 0;
}

// Returns non-zero if button was pressed since the last JoyHit call.
// Edge-triggered: flag is cleared after reading.
inline int bb_JoyHit(int port, int btn) {
  if (bb_sdl_initialized_) bb_PollEvents();
  if (port < 0 || port >= BB_JOY_MAX_PORTS) return 0;
  if (btn < 1 || btn > BB_JOY_MAX_BUTTONS) return 0;
  bool hit = bb_joy_[port].btn_hit[btn - 1];
  bb_joy_[port].btn_hit[btn - 1] = false;
  return hit ? 1 : 0;
}

// Blocks until any button on the port is pressed; returns button number (1-based).
inline int bb_WaitJoy(int port) {
  if (port < 0 || port >= BB_JOY_MAX_PORTS) port = 0;
  bb_sdl_ensure_();
  if (!bb_sdl_initialized_) return 1;
  bb_PollEvents();
  bb_JoyPort_ &jp = bb_joy_[port];
  while (jp.btn_q_head == jp.btn_q_tail) {
    SDL_Event ev;
    if (!SDL_WaitEvent(&ev)) break;
    bb_sdl_process_event_(ev);
  }
  if (jp.btn_q_head == jp.btn_q_tail) return 1;
  int btn = jp.btn_queue[jp.btn_q_head];
  jp.btn_q_head = (jp.btn_q_head + 1) % BB_JOY_BTN_QUEUE_CAP;
  return btn;
}

// Alias (Blitz3D compat).
inline int bb_GetJoy(int port) { return bb_WaitJoy(port); }

// Clears all joystick state for the port (keeps handle/id/type intact).
inline void bb_FlushJoy(int port) {
  if (port < 0 || port >= BB_JOY_MAX_PORTS) return;
  bb_JoyPort_ &jp = bb_joy_[port];
  for (int i = 0; i < BB_JOY_MAX_BUTTONS; ++i) {
    jp.btn_down[i] = false;
    jp.btn_hit[i]  = false;
  }
  jp.btn_q_head = jp.btn_q_tail = 0;
  // Note: x/y/z/u/v/hat are not reset — they reflect physical state.
}

#endif // BLITZNEXT_BB_INPUT_H
