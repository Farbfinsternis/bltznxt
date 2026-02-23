#ifndef BLITZNEXT_BB_SYSTEM_H
#define BLITZNEXT_BB_SYSTEM_H

#include <chrono>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <unordered_map>
#include "bb_string.h"

// ---- Command-line storage (filled by bbInit) ----

inline int    bb_argc_ = 0;
inline char** bb_argv_ = nullptr;

// ---- Program Start Time (used by MilliSecs) ----

inline const auto bb_start_time_ = std::chrono::steady_clock::now();

// ---- Time Functions ----

inline int bb_MilliSecs() {
  auto now = std::chrono::steady_clock::now();
  return static_cast<int>(
    std::chrono::duration_cast<std::chrono::milliseconds>(now - bb_start_time_).count()
  );
}

// Returns current date as "DD Mon YYYY" (e.g. "23 Feb 2026")
inline bbString bb_CurrentDate() {
  std::time_t t = std::time(nullptr);
  char buf[32];
  std::strftime(buf, sizeof(buf), "%d %b %Y", std::localtime(&t));
  return buf;
}

// Returns current time as "HH:MM:SS" (e.g. "14:30:45")
inline bbString bb_CurrentTime() {
  std::time_t t = std::time(nullptr);
  char buf[16];
  std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&t));
  return buf;
}

// ---- Timer Objects ----
//
// bb_CreateTimer(hz)    → int handle; ticks at hz times per second
// bb_WaitTimer(handle)  → blocks until next tick; returns ticks elapsed
// bb_FreeTimer(handle)  → releases the timer

struct bb_Timer_ {
  std::chrono::milliseconds                period;
  std::chrono::steady_clock::time_point    next_tick;
};

inline std::unordered_map<int, bb_Timer_> bb_timers_;
inline int                                bb_timer_next_id_ = 1;

inline int bb_CreateTimer(int hz) {
  if (hz <= 0) hz = 1;
  int id = bb_timer_next_id_++;
  bb_Timer_ t;
  t.period    = std::chrono::milliseconds(1000 / hz);
  t.next_tick = std::chrono::steady_clock::now() + t.period;
  bb_timers_[id] = t;
  return id;
}

// Returns number of ticks elapsed since previous call (≥1).
// If already past one or more tick deadlines, catches up and returns that count.
inline int bb_WaitTimer(int handle) {
  auto it = bb_timers_.find(handle);
  if (it == bb_timers_.end()) return 0;
  auto &t = it->second;
  auto now = std::chrono::steady_clock::now();
  if (now < t.next_tick) {
    std::this_thread::sleep_until(t.next_tick);
    t.next_tick += t.period;
    return 1;
  }
  // Already past one or more ticks — count and advance without sleeping.
  int ticks = 0;
  while (t.next_tick <= now) {
    t.next_tick += t.period;
    ++ticks;
  }
  return ticks;
}

inline void bb_FreeTimer(int handle) {
  bb_timers_.erase(handle);
}

// ---- Delay (Sleep) ----

#ifdef _WIN32
#include <windows.h>
inline void bb_Delay(int ms) { Sleep(static_cast<DWORD>(ms)); }
#else
#include <unistd.h>
inline void bb_Delay(int ms) { usleep(static_cast<useconds_t>(ms) * 1000u); }
#endif

// ---- System / Process Functions ----

// Stores window title for later use by SDL3; no-op in console mode.
// bb_title_update_hook_ is set by bb_graphics2d.h once a window exists,
// so calling AppTitle() after Graphics() also updates the live window title.
inline bbString bb_app_title_;
inline void (*bb_title_update_hook_)(const char*) = nullptr;
inline void bb_AppTitle(const bbString &title) {
  bb_app_title_ = title;
  if (bb_title_update_hook_) bb_title_update_hook_(title.c_str());
}

// Returns argv[1..] joined by spaces, as Blitz3D CommandLine() does.
inline bbString bb_CommandLine() {
  bbString result;
  for (int i = 1; i < bb_argc_; ++i) {
    if (i > 1) result += " ";
    result += bb_argv_[i];
  }
  return result;
}

// Runs an external program via the OS shell; returns the exit code.
inline int bb_ExecFile(const bbString &path) {
  return std::system(path.c_str());
}

// Prints an error message to stderr and terminates the program.
inline void bb_RuntimeError(const bbString &msg) {
  std::cerr << "Runtime Error: " << msg << "\n";
  std::exit(1);
}

// Environment variable access.
inline bbString bb_GetEnv(const bbString &key) {
  const char *val = std::getenv(key.c_str());
  return val ? bbString(val) : bbString("");
}

inline void bb_SetEnv(const bbString &key, const bbString &val) {
#ifdef _WIN32
  _putenv_s(key.c_str(), val.c_str());
#else
  setenv(key.c_str(), val.c_str(), 1);
#endif
}

// Returns a system property string; stub for unknown properties.
inline bbString bb_SystemProperty(const bbString &/*prop*/) { return ""; }

// Pointer visibility — no-op stubs until SDL3 window exists.
inline void bb_ShowPointer() {}
inline void bb_HidePointer() {}

// CallDLL — advanced stub: logs the call and returns 0.
inline int bb_CallDLL(const bbString &dll, const bbString &func,
                      int /*in_bank*/ = 0, int /*out_bank*/ = 0) {
  std::cerr << "[stub] CallDLL \"" << dll << "\", \"" << func << "\"\n";
  return 0;
}

#endif // BLITZNEXT_BB_SYSTEM_H
