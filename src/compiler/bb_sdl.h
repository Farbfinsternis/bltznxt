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

// Das HWND des Programmfensters fuer SystemProperty("apphwnd") (bb_system.h,
// BUG-122); ohne Fenster nullptr.
inline void *bb_window_hwnd_() {
#ifdef _WIN32
  if (!bb_window_) return nullptr;
  return SDL_GetPointerProperty(SDL_GetWindowProperties(bb_window_),
                                SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#else
  return nullptr;
#endif
}

inline SDL_Renderer* bb_renderer_        = nullptr;
// 2D: der Bildschirm als Zieltextur, damit er nach Flip erhalten bleibt
// (bb_graphics2d.h, BUG-221); nullptr im 3D-Modus.
inline SDL_Texture*  bb_screen_tex_      = nullptr;
inline SDL_GLContext bb_gl_ctx_          = nullptr;
inline bool          bb_gl_active_       = false;
inline bool          bb_sdl_initialized_ = false;

// Unseren GL-Kontext aktuell machen. Im 3D-Modus arbeitet der 2D-Renderer in
// einem eigenen GL-Kontext (bb_Graphics3D in bb_gl_ctx.h) und laesst ihn nach
// jedem Zeichnen aktuell stehen; jeder GL-Aufruf der Runtime ausserhalb von
// RenderWorld muss deshalb vorher hierher (BUG-63). Billig, wenn der Kontext
// schon stimmt.
inline void bb_gl_use_() {
  if (bb_gl_ctx_ && bb_window_ && SDL_GL_GetCurrentContext() != bb_gl_ctx_)
    SDL_GL_MakeCurrent(bb_window_, bb_gl_ctx_);
}

// ---- Textfenster vor Graphics ----
//
// Blitz3D zeigt Print, Write und Input vor Graphics in einem eigenen
// Textfenster, das Graphics dann ersetzt. Bei uns ist das die Konsole. Ein
// Programm mit Graphics ist aber ein Fensterprogramm ohne eigene Konsole
// (BUG-186); aus einem Terminal haengt es sich an dessen Konsole (bbInit),
// aus der Blitz3D-IDE oder per Doppelklick gestartet hat es keine. Dann
// verschwand der Text, und Input$ lieferte sofort "" - eine Abfrage wie die
// in samples/AGore/start.bb ("Use windowed mode?") drehte sich unsichtbar
// mit voller Last im Kreis. Deshalb bekommt ein solches Programm beim
// ersten Text eine eigene Konsole, und Graphics/Graphics3D schliessen sie
// wieder. Umgeleitete Ein- und Ausgabe (Datei, Pipe, Testsuite) bleibt,
// wo sie ist.
inline bool bb_console_own_  = false;   // von uns geoeffnet, Graphics schliesst
inline bool bb_console_done_ = false;   // schon geprueft

// Print und Input$ arbeiten mit ANSI-Bytes wie die Literale im Quelltext;
// eine Konsole steht unter deutschem Windows aber auf OEM (850) - "\xe4" kam
// als anderes Zeichen an (BUG-203). Deshalb die Codepage der Konsole fuer die
// Laufzeit des Programms auf ANSI, und danach zurueck: die Konsole eines
// Terminals, an das sich das Programm haengt, gehoert dem Nutzer.
inline void bb_console_ansi_() {
#ifdef _WIN32
  static bool done = false;
  if (done || !GetConsoleWindow()) return;
  done = true;
  static UINT old_out = GetConsoleOutputCP(), old_in = GetConsoleCP();
  SetConsoleOutputCP(GetACP());
  SetConsoleCP(GetACP());
  std::atexit([] { SetConsoleOutputCP(old_out); SetConsoleCP(old_in); });
#endif
}

inline void bb_console_ensure_() {
#ifdef _WIN32
  if (bb_console_done_ || bb_window_) return;
  bb_console_done_ = true;
  auto missing = [](DWORD std) {
    const HANDLE h = GetStdHandle(std);
    return !h || h == INVALID_HANDLE_VALUE || GetFileType(h) == FILE_TYPE_UNKNOWN;
  };
  const bool out = missing(STD_OUTPUT_HANDLE), err = missing(STD_ERROR_HANDLE),
             in  = missing(STD_INPUT_HANDLE);
  if (!out && !in) return;
  if (!GetConsoleWindow() && !AttachConsole(ATTACH_PARENT_PROCESS)) {
    if (!AllocConsole()) return;
    bb_console_own_ = true;
    SetForegroundWindow(GetConsoleWindow());
  }
  if (out) freopen("CONOUT$", "w", stdout);
  if (err) freopen("CONOUT$", "w", stderr);
  if (in) {
    freopen("CONIN$", "r", stdin);
    std::cin.clear();
  }
  bb_console_ansi_();
#endif
}

inline void bb_console_release_() {
#ifdef _WIN32
  if (!bb_console_own_) return;
  std::cout.flush();
  fflush(stdout);
  FreeConsole();
  bb_console_own_  = false;
  bb_console_done_ = false;   // nach EndGraphics oeffnet Text sie neu
#endif
}

// ---- Keyboard state (raw SDL scancodes; read by bb_input.h) ----
//
// Indexed by SDL_Scancode (max value 511; we allocate 512 slots).
// bb_sdl_key_down_   : true while the key is held.
// bb_sdl_key_hit_raw_: Anzahl der Druecke seit dem letzten KeyHit (hit_count
//                      in gxdevice.cpp, BUG-206).
// bb_key_queue_buf_  : circular FIFO of scancodes for GetKey().

inline constexpr int BB_KEY_QUEUE_CAP                    = 64;
inline bool          bb_sdl_key_down_[512]               = {};
inline int           bb_sdl_key_hit_raw_[512]            = {};
inline SDL_Scancode  bb_key_queue_buf_[BB_KEY_QUEUE_CAP] = {};
inline int           bb_key_queue_head_                  = 0;
inline int           bb_key_queue_tail_                  = 0;

// ---- Mouse state (read by bb_input.h) ----
//
// bb_mouse_x_ / bb_mouse_y_   : last known cursor position (pixels).
// bb_mouse_z_                  : scroll-wheel accumulator (ticks, up = +).
// bb_mouse_zrel_               : wheel delta accumulator; reset on each read.
// bb_mouse_down_[1..3]         : 1 = held; indices: 1=left,2=right,3=middle.
// bb_mouse_hit_[1..3]          : Anzahl der Klicks seit dem letzten MouseHit.
// bb_mouse_queue_buf_          : FIFO of button numbers for WaitMouse().

inline float bb_mouse_x_    = 0.0f, bb_mouse_y_    = 0.0f;
inline float bb_mouse_z_    = 0.0f;
inline float bb_mouse_zrel_ = 0.0f;
inline int   bb_mouse_speed_x_ = 0, bb_mouse_speed_y_ = 0;   // MouseXSpeed/YSpeed
// Wie der DirectInput-Puffer des Originals: Bewegung, die das Programm noch
// nicht gelesen hat, geht bei MoveMouse nicht verloren (BUG-165). read_ ist
// die zuletzt gelesene bzw. per MoveMouse gesetzte Lage, off_ der Versatz
// zwischen Cursor und Spiel-Lage, der die ungelesene Bewegung traegt.
inline float bb_mouse_read_x_ = 0.0f, bb_mouse_read_y_ = 0.0f;
inline float bb_mouse_off_x_  = 0.0f, bb_mouse_off_y_  = 0.0f;

// ---- Abbildung Spielbild -> Fenster (BUG-159) ----
//
// Im Vollbild ist das Fenster so gross wie der Bildschirm, das Programm rechnet
// aber in der Aufloesung, die es bei Graphics/Graphics3D angegeben hat. Bild
// und Maus muessen deshalb umgerechnet werden. Im Fenstermodus steht der
// Massstab auf 1 und der Rand auf 0, dann faellt die Rechnung weg.
inline float bb_present_scale_   = 1.0f;   // x; im Vollbild und Fenster 5 auch y
inline float bb_present_scale_y_ = 1.0f;
inline float bb_present_ox_    = 0.0f;
inline float bb_present_oy_    = 0.0f;
inline int   bb_present_pw_    = 0;   // Fenstergroesse in Pixeln
inline int   bb_present_ph_    = 0;
// Fensterpixel je Fensterkoordinate. Mausereignisse und SDL_WarpMouseInWindow
// rechnen in Fensterkoordinaten, Massstab und Rand in Pixeln; bei einer
// Windows-Skalierung ueber 100 % ist das nicht dasselbe.
inline float bb_present_density_ = 1.0f;
// Spielaufloesung und Art der Abbildung; gestreckt (ohne Rand, x und y
// getrennt) im skalierten Fenster der Modi 3 und 7 wie im Original (BUG-208).
inline int   bb_present_gw_      = 0;
inline int   bb_present_gh_      = 0;
inline bool  bb_present_stretch_ = false;

inline float bb_present_to_game_x_(float x) { return (x * bb_present_density_ - bb_present_ox_) / bb_present_scale_; }
inline float bb_present_to_game_y_(float y) { return (y * bb_present_density_ - bb_present_oy_) / bb_present_scale_y_; }
inline float bb_present_to_window_x_(float x) { return (bb_present_ox_ + x * bb_present_scale_) / bb_present_density_; }
inline float bb_present_to_window_y_(float y) { return (bb_present_oy_ + y * bb_present_scale_y_) / bb_present_density_; }

// Massstab und Rand aus Fenstergroesse und Spielaufloesung bestimmen. Passt
// das Seitenverhaeltnis nicht, bleibt oben/unten bzw. links/rechts ein Rand -
// das Bild wird nie beschnitten. Gestreckt fuellt es das Fenster. Auch nach
// jeder Groessenaenderung des Fensters (bb_sdl_process_event_).
inline void bb_present_fit_() {
  const int gw = bb_present_gw_, gh = bb_present_gh_;
  bb_present_scale_ = bb_present_scale_y_ = 1.0f;
  bb_present_density_ = 1.0f;
  bb_present_ox_ = bb_present_oy_ = 0.0f;
  bb_present_pw_ = gw;
  bb_present_ph_ = gh;
  if (!bb_window_ || gw <= 0 || gh <= 0) return;
  int pw = 0, ph = 0;
  SDL_GetWindowSizeInPixels(bb_window_, &pw, &ph);
  if (pw <= 0 || ph <= 0) return;
  int ww = 0, wh = 0;
  SDL_GetWindowSize(bb_window_, &ww, &wh);
  if (ww > 0) bb_present_density_ = (float)pw / (float)ww;
  bb_present_pw_ = pw;
  bb_present_ph_ = ph;
  const float sx = (float)pw / (float)gw;
  const float sy = (float)ph / (float)gh;
  if (bb_present_stretch_) {
    bb_present_scale_ = sx;
    bb_present_scale_y_ = sy;
    return;
  }
  const float sc = sx < sy ? sx : sy;
  bb_present_scale_ = bb_present_scale_y_ = sc;
  bb_present_ox_ = (pw - gw * sc) * 0.5f;
  bb_present_oy_ = (ph - gh * sc) * 0.5f;
}

inline void bb_present_update_(int gw, int gh, bool stretch = false) {
  // Nach einem Moduswechsel steht die Maus im Original auf 0,0, bis sie sich
  // bewegt. Das Ereignis, mit dem SDL beim Anlegen des Fensters die Lage des
  // Cursors meldet, wird deshalb verworfen.
  bb_mouse_x_ = bb_mouse_y_ = 0.0f;
  bb_mouse_speed_x_ = bb_mouse_speed_y_ = 0;
  bb_mouse_read_x_ = bb_mouse_read_y_ = 0.0f;
  bb_mouse_off_x_ = bb_mouse_off_y_ = 0.0f;
  if (bb_window_) {
    SDL_PumpEvents();
    SDL_FlushEvent(SDL_EVENT_MOUSE_MOTION);
  }
  bb_present_gw_ = gw;
  bb_present_gh_ = gh;
  bb_present_stretch_ = stretch;
  bb_present_fit_();
}

inline bool  bb_mouse_down_[4] = {};  // [1]=left  [2]=right  [3]=middle
inline int   bb_mouse_hit_[4]  = {};
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
// Achsen und Hat werden wie im Original bei jeder Abfrage frisch gelesen
// (bb_joy_axis_ in bb_input.h); hier stehen nur die Knoepfe, weil Treffer
// und Warteschlange Ereignisse brauchen.
// Buttons are 1-based in the Blitz3D API; stored 0-based internally.

inline constexpr int BB_JOY_MAX_PORTS    = 4;
inline constexpr int BB_JOY_MAX_BUTTONS  = 32;
inline constexpr int BB_JOY_BTN_QUEUE_CAP = 32;   // QUE_SIZE in gxdevice.h

struct bb_JoyPort_ {
  SDL_Joystick*  handle     = nullptr;
  SDL_Gamepad*   pad        = nullptr;   // gesetzt, wenn SDL das Geraet als Gamepad kennt
  SDL_JoystickID id         = 0;
  bool           btn_down[BB_JOY_MAX_BUTTONS] = {};
  int            btn_hit[BB_JOY_MAX_BUTTONS]  = {};   // Anzahl wie hit_count
  int            btn_queue[BB_JOY_BTN_QUEUE_CAP] = {};
  int            btn_q_head = 0, btn_q_tail = 0;
};

inline bb_JoyPort_ bb_joy_[BB_JOY_MAX_PORTS] = {};

// Gamepad-Knoepfe in der Reihenfolge, in der DirectInput einen
// Xbox-Controller meldet (Knopf 1 = A ... 10 = rechter Stick). Das
// Steuerkreuz ist dort kein Knopf, sondern der Hat; Guide fehlt.
inline int bb_pad_button_to_blitz_(int b) {
  switch (b) {
    case SDL_GAMEPAD_BUTTON_SOUTH:          return 0;
    case SDL_GAMEPAD_BUTTON_EAST:           return 1;
    case SDL_GAMEPAD_BUTTON_WEST:           return 2;
    case SDL_GAMEPAD_BUTTON_NORTH:          return 3;
    case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:  return 4;
    case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return 5;
    case SDL_GAMEPAD_BUTTON_BACK:           return 6;
    case SDL_GAMEPAD_BUTTON_START:          return 7;
    case SDL_GAMEPAD_BUTTON_LEFT_STICK:     return 8;
    case SDL_GAMEPAD_BUTTON_RIGHT_STICK:    return 9;
    default:                                return -1;
  }
}

// gxDevice::downEvent: Zustand, Trefferzahl, Warteschlange.
inline void bb_joy_button_(int p, int b, bool down) {
  if (p < 0 || b < 0 || b >= BB_JOY_MAX_BUTTONS) return;
  bb_JoyPort_ &jp = bb_joy_[p];
  jp.btn_down[b] = down;
  if (!down) return;
  ++jp.btn_hit[b];
  int next = (jp.btn_q_tail + 1) % BB_JOY_BTN_QUEUE_CAP;
  if (next != jp.btn_q_head) {
    jp.btn_queue[jp.btn_q_tail] = b + 1; // 1-based
    jp.btn_q_tail = next;
  }
}

// Find the port index for a given SDL joystick instance ID (-1 = not found).
inline int bb_joy_find_port_(SDL_JoystickID id) {
  for (int i = 0; i < BB_JOY_MAX_PORTS; ++i)
    if (bb_joy_[i].handle && bb_joy_[i].id == id) return i;
  return -1;
}

// SDL-Hat-Bitmaske → JoyHat des Originals: der DirectInput-POV in Grad
// (0 = oben, 45, 90 = rechts ... 315), Mittelstellung -1 (gxinput.cpp).
inline int bb_sdl_hat_to_blitz_(Uint8 v) {
  switch (v) {
    case SDL_HAT_UP:        return 0;
    case SDL_HAT_RIGHTUP:   return 45;
    case SDL_HAT_RIGHT:     return 90;
    case SDL_HAT_RIGHTDOWN: return 135;
    case SDL_HAT_DOWN:      return 180;
    case SDL_HAT_LEFTDOWN:  return 225;
    case SDL_HAT_LEFT:      return 270;
    case SDL_HAT_LEFTUP:    return 315;
    default:                return -1;
  }
}

// ---- Quit hooks (forward declarations) ----
//
// Declared before bb_sdl_quit_() so the function body can reference them.
// Set by bb_graphics2d.h (TTF), bb_image.h (image textures), and
// bb_gl_ctx.h (OpenGL context) at startup.

inline void (*bb_gl_quit_hook_)()     = nullptr;
inline void (*bb_ttf_quit_hook_)()   = nullptr;
inline void (*bb_image_quit_hook_)() = nullptr;
inline void (*bb_entity_quit_hook_)() = nullptr;
inline void (*bb_shader_quit_hook_)() = nullptr;
inline void (*bb_texture_quit_hook_)() = nullptr;

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
  if (bb_ttf_quit_hook_)    { bb_ttf_quit_hook_();    bb_ttf_quit_hook_    = nullptr; }
  if (bb_image_quit_hook_)  { bb_image_quit_hook_();  bb_image_quit_hook_  = nullptr; }
  if (bb_entity_quit_hook_) { bb_entity_quit_hook_(); bb_entity_quit_hook_ = nullptr; }
  if (bb_texture_quit_hook_){ bb_texture_quit_hook_(); bb_texture_quit_hook_= nullptr; }
  if (bb_shader_quit_hook_) { bb_shader_quit_hook_(); bb_shader_quit_hook_ = nullptr; }
  for (int i = 0; i < BB_JOY_MAX_PORTS; ++i) {
    if (bb_joy_[i].pad) SDL_CloseGamepad(bb_joy_[i].pad);
    if (bb_joy_[i].handle) SDL_CloseJoystick(bb_joy_[i].handle);
    bb_joy_[i] = bb_JoyPort_();
  }
  if (bb_renderer_) { SDL_DestroyRenderer(bb_renderer_); bb_renderer_ = nullptr; bb_screen_tex_ = nullptr; }
  if (bb_gl_quit_hook_)    { bb_gl_quit_hook_();    bb_gl_quit_hook_    = nullptr; }
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

// ---- Anhalten ohne Fokus (BUG-208) ----
//
// Wie auto_suspend in gxruntime.cpp: im Vollbild (Modus 0 und 1) und in den
// Fenstermodi 6 und 7 haelt das Programm an, sobald sein Fenster den Fokus
// verliert, und laeuft weiter, wenn es ihn zurueckbekommt. Ausloeser ist der
// Wechsel (WM_ACTIVATEAPP im Original), nicht ein Fenster, das nie den Fokus
// hatte. Beim Anhalten und Weiterlaufen werden Tasten und Maus zurueckgesetzt
// (input->reset) und der Ton angehalten; bb_PollEvents wartet so lange.
inline bool bb_auto_suspend_ = false;
inline bool bb_suspended_    = false;
inline void (*bb_audio_pause_hook_)(bool pause) = nullptr;   // bb_sound.h

inline void bb_input_reset_() {
  for (int i = 0; i < 512; ++i) { bb_sdl_key_down_[i] = false; bb_sdl_key_hit_raw_[i] = 0; }
  bb_key_queue_head_ = bb_key_queue_tail_ = 0;
  for (int i = 0; i < 4; ++i) { bb_mouse_down_[i] = false; bb_mouse_hit_[i] = 0; }
  bb_mouse_queue_head_ = bb_mouse_queue_tail_ = 0;
}

inline void bb_suspend_(bool on) {
  if (on == bb_suspended_) return;
  bb_suspended_ = on;
  bb_input_reset_();
  if (bb_audio_pause_hook_) bb_audio_pause_hook_(on);
}

inline void bb_sdl_process_event_(const SDL_Event &ev) {
  if (ev.type == SDL_EVENT_QUIT) {
#ifdef _WIN32
    // Die zweite Angabe von AppTitle ist die Rueckfrage beim Schliessen des
    // Fensters (gxRuntime, WM_CLOSE): nur bei OK endet das Programm
    // (BUG-210). Bis dahin wurde sie gespeichert und nie gezeigt.
    if (!bb_app_close_prompt_.empty() && bb_window_) {
      const std::wstring msg = bb_wide_from_(bb_app_close_prompt_, CP_ACP);
      const std::wstring ttl = bb_wide_from_(bb_app_title_, CP_ACP);
      if (MessageBoxW(static_cast<HWND>(bb_window_hwnd_()), msg.c_str(), ttl.c_str(),
                      MB_OKCANCEL | MB_ICONWARNING | MB_SETFOREGROUND | MB_TOPMOST) != IDOK)
        return;
    }
#endif
    bb_sdl_quit_();
    std::exit(0);
  }
  if (ev.type == SDL_EVENT_WINDOW_FOCUS_LOST) {
    if (bb_auto_suspend_) bb_suspend_(true);
  }
  if (ev.type == SDL_EVENT_WINDOW_FOCUS_GAINED) bb_suspend_(false);
  if (ev.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) bb_present_fit_();
  if (ev.type == SDL_EVENT_KEY_DOWN) {
    int sc = (int)ev.key.scancode;
    if (sc > 0 && sc < 512) {
      if (!ev.key.repeat) {
        ++bb_sdl_key_hit_raw_[sc];
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
    // Die Maus meldet Fensterkoordinaten; das Programm erwartet die seiner
    // eigenen Aufloesung (BUG-159).
    bb_mouse_x_    = bb_present_to_game_x_(ev.motion.x) + bb_mouse_off_x_;
    bb_mouse_y_    = bb_present_to_game_y_(ev.motion.y) + bb_mouse_off_y_;
  }
  if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
    int btn = bb_sdl_btn_to_blitz_(ev.button.button);
    if (btn >= 1 && btn <= 3) {
      bb_mouse_down_[btn] = true;
      ++bb_mouse_hit_[btn];
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
  // Achsen und Hat liest bb_joy_axis_ (bb_input.h) bei jeder Abfrage; hier
  // nur Anmelden, Abmelden und Knoepfe. Ein Gamepad liefert seine Knoepfe
  // ueber die Gamepad-Ereignisse in DirectInput-Reihenfolge, seine rohen
  // Joystick-Knoepfe werden dann uebergangen.
  if (ev.type == SDL_EVENT_JOYSTICK_ADDED) {
    SDL_JoystickID jid = ev.jdevice.which;
    if (bb_joy_find_port_(jid) < 0) {          // not already open
      for (int i = 0; i < BB_JOY_MAX_PORTS; ++i) {
        if (!bb_joy_[i].handle) {
          bb_joy_[i].handle = SDL_OpenJoystick(jid);
          if (bb_joy_[i].handle) {
            bb_joy_[i].id  = jid;
            bb_joy_[i].pad = SDL_IsGamepad(jid) ? SDL_OpenGamepad(jid) : nullptr;
          }
          break;
        }
      }
    }
  }
  if (ev.type == SDL_EVENT_JOYSTICK_REMOVED) {
    int p = bb_joy_find_port_(ev.jdevice.which);
    if (p >= 0) {
      if (bb_joy_[p].pad) SDL_CloseGamepad(bb_joy_[p].pad);
      SDL_CloseJoystick(bb_joy_[p].handle);
      bb_joy_[p] = bb_JoyPort_();
    }
  }
  if (ev.type == SDL_EVENT_JOYSTICK_BUTTON_DOWN || ev.type == SDL_EVENT_JOYSTICK_BUTTON_UP) {
    int p = bb_joy_find_port_(ev.jbutton.which);
    if (p >= 0 && !bb_joy_[p].pad)
      bb_joy_button_(p, ev.jbutton.button, ev.type == SDL_EVENT_JOYSTICK_BUTTON_DOWN);
  }
  if (ev.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN || ev.type == SDL_EVENT_GAMEPAD_BUTTON_UP) {
    int p = bb_joy_find_port_(ev.gbutton.which);
    if (p >= 0 && bb_joy_[p].pad)
      bb_joy_button_(p, bb_pad_button_to_blitz_(ev.gbutton.button),
                     ev.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);
  }
}

// ---- Audio update hook ----
//
// Set by bb_sound.h at startup to refill looping audio streams.
// Null until bb_sound.h is included; called after every SDL event drain.

inline void (*bb_audio_update_hook_)() = nullptr;

// Zeigt, was in den Frontpuffer gezeichnet wurde (bb_graphics2d.h, BUG-219);
// wie der Audio-Haken nach jedem Abholen der Ereignisse.
inline void (*bb_front_show_hook_)() = nullptr;

// ---- TTF quit hook ----
//
// Set by bb_graphics2d.h (M43) when SDL3_ttf is compiled in.
// Called from bb_sdl_quit_() before SDL itself shuts down so that all
// open TTF_Font* handles are closed and TTF_Quit() is called in the
// correct order (TTF → Sound → SDL).
// (Variable declared earlier, before bb_sdl_quit_().)

// ---- Event pump ----
//
// Drains all pending SDL events, updating keyboard/mouse/joystick state.
// Then calls the audio update hook (if registered) to refill looping streams.
// Returns false (legacy compatibility; Quit is handled by std::exit).

inline bool bb_PollEvents() {
  SDL_Event ev;
  while (SDL_PollEvent(&ev))
    bb_sdl_process_event_(ev);
  // Angehalten: warten, bis das Fenster den Fokus zurueckbekommt (BUG-208).
  while (bb_suspended_ && bb_window_)
    if (SDL_WaitEvent(&ev)) bb_sdl_process_event_(ev);
  if (bb_audio_update_hook_) bb_audio_update_hook_();
  if (bb_front_show_hook_) bb_front_show_hook_();
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

// Rueckgabewert: das Zeichen der Taste wie bei GetKey, 0 wenn keine kam
// (Fenster geschlossen, kein Terminal, SDL nicht verfuegbar). Im Original
// meldet +k `WaitKey ( )` mit int (BUG-44).
//
// Wie bbWaitKey (bbinput.cpp) nimmt WaitKey aus derselben Warteschlange wie
// GetKey und kehrt erst bei einer Taste mit Zeichen zurueck - Shift allein
// zaehlt nicht, eine schon vorher gedrueckte Taste dagegen sofort (gemessen,
// BUG-205). Bis dahin lieferte es den rohen SDL-Scancode der naechsten neuen
// Taste: "If WaitKey()=27" griff nie.
inline int bb_key_queue_pop_ascii_(bool *empty);   // bb_input.h
inline void bb_console_keys_();                    // bb_input.h

inline int bb_WaitKey() {
  // Case 1: graphical window is open — use SDL unconditionally.
  if (bb_window_) {
    for (;;) {
      bb_PollEvents();
      bool empty = false;
      while (!empty)
        if (int a = bb_key_queue_pop_ascii_(&empty)) return a;
      if (!bb_window_) return 0;
      SDL_Event ev;
      if (SDL_WaitEventTimeout(&ev, 20)) bb_sdl_process_event_(ev);
    }
  }

  // Case 2: console-only — skip if stdin is not a real terminal (test runner).
  if (!bb_stdin_is_console_()) return 0;

#ifdef _WIN32
  // Ohne Fenster kommen die Tasten aus der Konsole (BUG-214); SDL bekaeme
  // keine und wartete ewig.
  for (;;) {
    bb_console_keys_();
    bool empty = false;
    while (!empty)
      if (int a = bb_key_queue_pop_ascii_(&empty)) return a;
    Sleep(20);
  }
#endif

  // Interactive console path: ensure SDL is up, then wait for a key-down event.
  bb_sdl_ensure_();
  if (!bb_sdl_initialized_) {
    std::cin.get();
    return 0;
  }

  SDL_Event ev;
  while (SDL_WaitEvent(&ev)) {
    bb_sdl_process_event_(ev);
    if (ev.type == SDL_EVENT_QUIT) return 0;
    bool empty = false;
    while (!empty)
      if (int a = bb_key_queue_pop_ascii_(&empty)) return a;
  }
  return 0;
}

#endif // BLITZNEXT_BB_SDL_H
