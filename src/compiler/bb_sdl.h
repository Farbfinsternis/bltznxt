#ifndef BLITZNEXT_BB_SDL_H
#define BLITZNEXT_BB_SDL_H

// Prevent SDL3 from redefining main — blitzcc emits its own int main().
#define SDL_MAIN_HANDLED

#include <SDL3/SDL.h>
#include <cstdio>
#include <iostream>

#ifdef _WIN32
  #include <windows.h>  // GetFileType, GetStdHandle
#else
  #include <unistd.h>   // isatty, fileno
#endif

// ---- Global SDL state ----
//
// bb_window_ and bb_renderer_ are null until a graphics milestone creates them.
// bb_sdl_initialized_ guards calls to SDL functions.

inline SDL_Window*   bb_window_          = nullptr;
inline SDL_Renderer* bb_renderer_        = nullptr;
inline bool          bb_sdl_initialized_ = false;

// ---- Keyboard state (raw SDL scancodes; read by bb_input.h) ----
//
// Indexed by SDL_Scancode (max value 511; we allocate 512 slots).
// bb_sdl_key_down_   : true while the key is held.
// bb_sdl_key_hit_raw_: edge-triggered — set on first KEY_DOWN, cleared by KeyHit().
// bb_key_queue_buf_  : circular FIFO of scancodes for GetKey().

inline constexpr int BB_KEY_QUEUE_CAP                    = 64;
inline bool          bb_sdl_key_down_[512]               = {};
inline bool          bb_sdl_key_hit_raw_[512]            = {};
inline SDL_Scancode  bb_key_queue_buf_[BB_KEY_QUEUE_CAP] = {};
inline int           bb_key_queue_head_                  = 0;
inline int           bb_key_queue_tail_                  = 0;

// ---- Mouse state (read by bb_input.h) ----
//
// bb_mouse_x_ / bb_mouse_y_   : last known cursor position (pixels).
// bb_mouse_z_                  : scroll-wheel accumulator (ticks, up = +).
// bb_mouse_xrel_ / _yrel_ / _zrel_ : delta accumulators; reset on each read.
// bb_mouse_down_[1..3]         : 1 = held; indices: 1=left,2=right,3=middle.
// bb_mouse_hit_[1..3]          : edge-triggered press flag, cleared on read.
// bb_mouse_queue_buf_          : FIFO of button numbers for WaitMouse().

inline float bb_mouse_x_    = 0.0f, bb_mouse_y_    = 0.0f;
inline float bb_mouse_z_    = 0.0f;
inline float bb_mouse_xrel_ = 0.0f, bb_mouse_yrel_ = 0.0f, bb_mouse_zrel_ = 0.0f;
inline bool  bb_mouse_down_[4] = {};  // [1]=left  [2]=right  [3]=middle
inline bool  bb_mouse_hit_[4]  = {};
inline constexpr int BB_MOUSE_QUEUE_CAP                  = 16;
inline int           bb_mouse_queue_buf_[BB_MOUSE_QUEUE_CAP] = {};
inline int           bb_mouse_queue_head_                = 0;
inline int           bb_mouse_queue_tail_                = 0;

// Convert SDL button constant → Blitz3D button (1=left,2=right,3=middle,0=other)
inline int bb_sdl_btn_to_blitz_(Uint8 b) {
  if (b == SDL_BUTTON_LEFT)   return 1;
  if (b == SDL_BUTTON_RIGHT)  return 2;
  if (b == SDL_BUTTON_MIDDLE) return 3;
  return 0;
}

// ---- Joystick state (read by bb_input.h) ----
//
// Up to BB_JOY_MAX_PORTS devices, assigned in connection order.
// Axes 0-4 → X,Y,Z,U,V (Sint16 normalized to -1.0..1.0).
// Hat 0 converted to Blitz3D direction (0=center, 1=up, 2=up-right, ...).
// Buttons are 1-based in the Blitz3D API; stored 0-based internally.

inline constexpr int BB_JOY_MAX_PORTS    = 4;
inline constexpr int BB_JOY_MAX_BUTTONS  = 32;
inline constexpr int BB_JOY_BTN_QUEUE_CAP = 8;

struct bb_JoyPort_ {
  SDL_Joystick*  handle     = nullptr;
  SDL_JoystickID id         = 0;
  bool           is_gamepad = false;
  float          x = 0, y = 0, z = 0, u = 0, v = 0;
  int            hat        = 0;
  bool           btn_down[BB_JOY_MAX_BUTTONS] = {};
  bool           btn_hit[BB_JOY_MAX_BUTTONS]  = {};
  int            btn_queue[BB_JOY_BTN_QUEUE_CAP] = {};
  int            btn_q_head = 0, btn_q_tail = 0;
};

inline bb_JoyPort_ bb_joy_[BB_JOY_MAX_PORTS] = {};

// Find the port index for a given SDL joystick instance ID (-1 = not found).
inline int bb_joy_find_port_(SDL_JoystickID id) {
  for (int i = 0; i < BB_JOY_MAX_PORTS; ++i)
    if (bb_joy_[i].handle && bb_joy_[i].id == id) return i;
  return -1;
}

// Convert SDL hat bitmask → Blitz3D direction (0=center, 1=up … 8=up-left).
inline int bb_sdl_hat_to_blitz_(Uint8 v) {
  switch (v) {
    case SDL_HAT_UP:        return 1;
    case SDL_HAT_RIGHTUP:   return 2;
    case SDL_HAT_RIGHT:     return 3;
    case SDL_HAT_RIGHTDOWN: return 4;
    case SDL_HAT_DOWN:      return 5;
    case SDL_HAT_LEFTDOWN:  return 6;
    case SDL_HAT_LEFT:      return 7;
    case SDL_HAT_LEFTUP:    return 8;
    default:                return 0;
  }
}

// ---- Lifecycle ----

// Called lazily the first time a graphics or input function needs SDL.
// Text-mode programs never trigger this, so they pay zero SDL overhead.
inline void bb_sdl_ensure_() {
  if (bb_sdl_initialized_) return;
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD)) {
    std::cerr << "[runtime] SDL_Init failed: " << SDL_GetError() << "\n";
    return;
  }
  bb_sdl_initialized_ = true;
}

// Direct init for callers that know SDL is needed (kept for symmetry).
inline void bb_sdl_init_() { bb_sdl_ensure_(); }

inline void bb_sdl_quit_() {
  for (int i = 0; i < BB_JOY_MAX_PORTS; ++i) {
    if (bb_joy_[i].handle) {
      SDL_CloseJoystick(bb_joy_[i].handle);
      bb_joy_[i] = bb_JoyPort_();
    }
  }
  if (bb_renderer_) { SDL_DestroyRenderer(bb_renderer_); bb_renderer_ = nullptr; }
  if (bb_window_)   { SDL_DestroyWindow(bb_window_);     bb_window_   = nullptr; }
  if (bb_sdl_initialized_) { SDL_Quit(); bb_sdl_initialized_ = false; }
}

// ---- Interactive-mode detection ----
//
// Returns true only when stdin is an actual interactive console.
//
// Windows caveat: GetFileType() returns FILE_TYPE_CHAR for BOTH a real
// console AND the NUL device (Windows' /dev/null equivalent), so it cannot
// distinguish the two.  GetConsoleMode() is the reliable test: it succeeds
// only for genuine console handles, failing for NUL, pipes, and files.

inline bool bb_stdin_is_console_() {
#ifdef _WIN32
  DWORD mode;
  return GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode) != 0;
#else
  return isatty(fileno(stdin)) != 0;
#endif
}

// ---- Event processing ----
//
// Handles a single SDL event: updates keyboard state, signals Quit.
// Called by bb_PollEvents (non-blocking pump), bb_WaitKey, and
// bb_GetKey (defined in bb_input.h).

inline void bb_sdl_process_event_(const SDL_Event &ev) {
  if (ev.type == SDL_EVENT_QUIT) {
    bb_sdl_quit_();
    std::exit(0);
  }
  if (ev.type == SDL_EVENT_KEY_DOWN) {
    int sc = (int)ev.key.scancode;
    if (sc > 0 && sc < 512) {
      if (!ev.key.repeat) {
        bb_sdl_key_hit_raw_[sc] = true;
        int next = (bb_key_queue_tail_ + 1) % BB_KEY_QUEUE_CAP;
        if (next != bb_key_queue_head_) {
          bb_key_queue_buf_[bb_key_queue_tail_] = ev.key.scancode;
          bb_key_queue_tail_ = next;
        }
      }
      bb_sdl_key_down_[sc] = true;
    }
  }
  if (ev.type == SDL_EVENT_KEY_UP) {
    int sc = (int)ev.key.scancode;
    if (sc > 0 && sc < 512)
      bb_sdl_key_down_[sc] = false;
  }
  if (ev.type == SDL_EVENT_MOUSE_MOTION) {
    bb_mouse_x_    = ev.motion.x;
    bb_mouse_y_    = ev.motion.y;
    bb_mouse_xrel_ += ev.motion.xrel;
    bb_mouse_yrel_ += ev.motion.yrel;
  }
  if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
    int btn = bb_sdl_btn_to_blitz_(ev.button.button);
    if (btn >= 1 && btn <= 3) {
      bb_mouse_down_[btn] = true;
      bb_mouse_hit_[btn]  = true;
      int next = (bb_mouse_queue_tail_ + 1) % BB_MOUSE_QUEUE_CAP;
      if (next != bb_mouse_queue_head_) {
        bb_mouse_queue_buf_[bb_mouse_queue_tail_] = btn;
        bb_mouse_queue_tail_ = next;
      }
    }
  }
  if (ev.type == SDL_EVENT_MOUSE_BUTTON_UP) {
    int btn = bb_sdl_btn_to_blitz_(ev.button.button);
    if (btn >= 1 && btn <= 3)
      bb_mouse_down_[btn] = false;
  }
  if (ev.type == SDL_EVENT_MOUSE_WHEEL) {
    // SDL_MOUSEWHEEL_FLIPPED means the OS has already flipped the sign.
    float dy = (ev.wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
                 ? -ev.wheel.y : ev.wheel.y;
    bb_mouse_z_    += dy;
    bb_mouse_zrel_ += dy;
  }
  // ---- Joystick events ----
  if (ev.type == SDL_EVENT_JOYSTICK_ADDED) {
    SDL_JoystickID jid = ev.jdevice.which;
    if (bb_joy_find_port_(jid) < 0) {          // not already open
      for (int i = 0; i < BB_JOY_MAX_PORTS; ++i) {
        if (!bb_joy_[i].handle) {
          bb_joy_[i].handle = SDL_OpenJoystick(jid);
          if (bb_joy_[i].handle) {
            bb_joy_[i].id         = jid;
            bb_joy_[i].is_gamepad = SDL_IsGamepad(jid);
          }
          break;
        }
      }
    }
  }
  if (ev.type == SDL_EVENT_JOYSTICK_REMOVED) {
    int p = bb_joy_find_port_(ev.jdevice.which);
    if (p >= 0) {
      SDL_CloseJoystick(bb_joy_[p].handle);
      bb_joy_[p] = bb_JoyPort_();
    }
  }
  if (ev.type == SDL_EVENT_JOYSTICK_AXIS_MOTION) {
    int p = bb_joy_find_port_(ev.jaxis.which);
    if (p >= 0) {
      float v = ev.jaxis.value / 32767.0f;
      if (v < -1.0f) v = -1.0f;
      switch (ev.jaxis.axis) {
        case 0: bb_joy_[p].x = v; break;
        case 1: bb_joy_[p].y = v; break;
        case 2: bb_joy_[p].z = v; break;
        case 3: bb_joy_[p].u = v; break;
        case 4: bb_joy_[p].v = v; break;
        default: break;
      }
    }
  }
  if (ev.type == SDL_EVENT_JOYSTICK_HAT_MOTION) {
    int p = bb_joy_find_port_(ev.jhat.which);
    if (p >= 0 && ev.jhat.hat == 0)
      bb_joy_[p].hat = bb_sdl_hat_to_blitz_(ev.jhat.value);
  }
  if (ev.type == SDL_EVENT_JOYSTICK_BUTTON_DOWN) {
    int p = bb_joy_find_port_(ev.jbutton.which);
    if (p >= 0 && ev.jbutton.button < BB_JOY_MAX_BUTTONS) {
      int b = ev.jbutton.button;
      bb_joy_[p].btn_down[b] = true;
      bb_joy_[p].btn_hit[b]  = true;
      int next = (bb_joy_[p].btn_q_tail + 1) % BB_JOY_BTN_QUEUE_CAP;
      if (next != bb_joy_[p].btn_q_head) {
        bb_joy_[p].btn_queue[bb_joy_[p].btn_q_tail] = b + 1; // 1-based
        bb_joy_[p].btn_q_tail = next;
      }
    }
  }
  if (ev.type == SDL_EVENT_JOYSTICK_BUTTON_UP) {
    int p = bb_joy_find_port_(ev.jbutton.which);
    if (p >= 0 && ev.jbutton.button < BB_JOY_MAX_BUTTONS)
      bb_joy_[p].btn_down[ev.jbutton.button] = false;
  }
}

// ---- Audio update hook ----
//
// Set by bb_sound.h at startup to refill looping audio streams.
// Null until bb_sound.h is included; called after every SDL event drain.

inline void (*bb_audio_update_hook_)() = nullptr;

// ---- Event pump ----
//
// Drains all pending SDL events, updating keyboard/mouse/joystick state.
// Then calls the audio update hook (if registered) to refill looping streams.
// Returns false (legacy compatibility; Quit is handled by std::exit).

inline bool bb_PollEvents() {
  SDL_Event ev;
  while (SDL_PollEvent(&ev))
    bb_sdl_process_event_(ev);
  if (bb_audio_update_hook_) bb_audio_update_hook_();
  return false;
}

// ---- WaitKey ----
//
// Blocks until any key is pressed (SDL_EVENT_KEY_DOWN) or the window is closed.
//
// Priority:
//  1. If a graphical window is open (bb_window_ != nullptr), always block on
//     SDL events — this covers programs launched without a console (double-click,
//     IDE Run button) where stdin is not a real terminal.
//  2. Console-only programs: only block when stdin is an interactive terminal
//     so that headless test runners (piped stdin) return immediately.
//  3. Last resort: if SDL init failed, fall back to a raw std::cin.get().

inline void bb_WaitKey() {
  // Case 1: graphical window is open — use SDL unconditionally.
  if (bb_window_) {
    SDL_Event ev;
    while (SDL_WaitEvent(&ev)) {
      bb_sdl_process_event_(ev);
      if (ev.type == SDL_EVENT_KEY_DOWN || ev.type == SDL_EVENT_QUIT) return;
    }
    return;
  }

  // Case 2: console-only — skip if stdin is not a real terminal (test runner).
  if (!bb_stdin_is_console_()) return;

  // Interactive console path: ensure SDL is up, then wait for a key-down event.
  bb_sdl_ensure_();
  if (!bb_sdl_initialized_) {
    std::cin.get();
    return;
  }

  SDL_Event ev;
  while (SDL_WaitEvent(&ev)) {
    bb_sdl_process_event_(ev);
    if (ev.type == SDL_EVENT_KEY_DOWN || ev.type == SDL_EVENT_QUIT) return;
  }
}

#endif // BLITZNEXT_BB_SDL_H
