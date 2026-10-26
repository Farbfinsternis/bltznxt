#ifndef BLITZNEXT_BB_SYSTEM_H
#define BLITZNEXT_BB_SYSTEM_H

#include <chrono>
#include <ctime>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <unordered_map>
#include "bb_string.h"
#include "bb_path.h"     // bb_wide_from_

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
#include <shellapi.h>   // ShellExecuteW (ExecFile)
inline void bb_sleep_ms_(int ms) { Sleep(static_cast<DWORD>(ms)); }
#else
#include <unistd.h>
inline void bb_sleep_ms_(int ms) { usleep(static_cast<useconds_t>(ms) * 1000u); }
#endif

// Ereignisse abholen, solange ein Fenster offen ist; gesetzt von
// bb_graphics2d.h.
inline void (*bb_idle_hook_)() = nullptr;

// Wie gxRuntime::delay: in Schritten von hoechstens 100 ms schlafen und
// davor jeweils Nachrichten abholen. Erst dabei erscheint, was in den
// Frontpuffer gezeichnet wurde (BUG-219).
inline void bb_Delay(int ms) {
  if (!bb_idle_hook_) { if (ms > 0) bb_sleep_ms_(ms); return; }
  const auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
  for (;;) {
    bb_idle_hook_();
    const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - std::chrono::steady_clock::now()).count();
    if (left <= 0) return;
    bb_sleep_ms_(static_cast<int>(left > 100 ? 100 : left));
  }
}

// ---- System / Process Functions ----

// Stores window title for later use by SDL3; no-op in console mode.
// bb_title_update_hook_ is set by bb_graphics2d.h once a window exists,
// so calling AppTitle() after Graphics() also updates the live window title.
inline bbString bb_app_title_;
inline void (*bb_title_update_hook_)(const char*) = nullptr;
// Der zweite Parameter ist die Rueckfrage beim Schliessen des Fensters - im
// Original `AppTitle title$[,close_prompt$]`. Wir merken sie uns; ausgewertet
// wird sie noch nicht, weil unsere SDL-Schicht das Fenster ohne Rueckfrage
// schliesst (BUG-44).
inline bbString bb_app_close_prompt_;
inline void bb_AppTitle(const bbString &title, const bbString &close_prompt = "") {
  bb_app_title_ = title;
  bb_app_close_prompt_ = close_prompt;
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

// Runs an external program via the OS shell.
// Ohne Rueckgabewert: im Original ist `ExecFile command$` eine Anweisung
// (BUG-44).
// Wie gxRuntime::execute (BUG-210): Befehl und Parameter trennen - ein Befehl in
// Anfuehrungszeichen oder bis zum ersten Leerzeichen - und ueber ShellExecute
// starten. Das kehrt sofort zurueck und oeffnet ueber die Dateizuordnung auch
// Dokumente und Webadressen ("ExecFile \"http://...\""). Bis dahin hielt
// std::system das Spiel an, bis das Programm endete, und oeffnete ein
// Konsolenfenster.
inline void bb_ExecFile(const bbString &command) {
  bbString cmd = command, params;
  while (!cmd.empty() && cmd[0] == ' ') cmd.erase(0, 1);
  if (cmd.empty()) return;
  if (cmd[0] == '"') {
    const size_t n = cmd.find('"', 1);
    if (n != bbString::npos) { params = cmd.substr(n + 1); cmd = cmd.substr(1, n - 1); }
  } else {
    const size_t n = cmd.find(' ');
    if (n != bbString::npos) { params = cmd.substr(n + 1); cmd = cmd.substr(0, n); }
  }
  while (!params.empty() && params.front() == ' ') params.erase(0, 1);
  while (!params.empty() && params.back() == ' ') params.pop_back();
#ifdef _WIN32
  const std::wstring wc = bb_wide_from_(cmd, CP_ACP), wp = bb_wide_from_(params, CP_ACP);
  ShellExecuteW(GetDesktopWindow(), nullptr, wc.c_str(), params.empty() ? nullptr : wp.c_str(),
                nullptr, SW_SHOW);
#else
  std::system((cmd + " " + params + " &").c_str());
#endif
}

// DebugLog schreibt im Original ins Protokoll des IDE-Debuggers
// (gx_runtime->debugLog); ohne angehaengten Debugger geschieht nichts
// Sichtbares und das Programm laeuft weiter. Einen Debugger gibt es hier
// nicht, die Zeile geht deshalb auf stderr - wie RuntimeError - und die
// Programmausgabe auf stdout bleibt unberuehrt.
inline void bb_DebugLog(const bbString &text) {
  std::cerr << text << "\n";
}

// Stop haelt im Original den Debugger an (gx_runtime->debugStop). Ohne
// Debugger setzt das Original das Programm aus, bis das Fenster neu
// aktiviert wird (forceSuspend, gemessen 2026-10-04); das nuetzt ohne
// Debugger niemandem, darum laeuft es hier weiter (Nutzerentscheidung
// 2026-10-04, spaeter Haltepunkt fuer die IDE). RuntimeStats oeffnet
// im Debug-Build die Laufzeitstatistik des Debuggers. Einen Debugger gibt es
// hier nicht, beide sind darum leer (BUG-195).
inline void bb_Stop() {}
inline void bb_RuntimeStats() {}

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

// SystemProperty wie gxRuntime::systemProperty (BUG-122): bis dahin immer
// leer, und Programme, die mit "appdir" oder "tempdir" Pfade bauen, fanden
// ihre Dateien nicht. Gemessen an Blitz3D 11.8 unter Windows 11: cpu "Intel",
// os leer (GetVersionEx meldet 6.2, das kennt die Tabelle nicht), Ordner mit
// abschliessendem Backslash, apphwnd/apphinstance nie leer, die D3D7-Zeiger
// ohne Grafik "0". Die gibt es hier nicht, sie bleiben "0".
inline void *bb_window_hwnd_();   // bb_sdl.h - das HWND des Programmfensters

inline bbString bb_SystemProperty(const bbString &prop) {
  bbString t = prop;
  for (auto &c : t) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  auto toDir = [](bbString d) {
    if (!d.empty() && d.back() != '\\') d += '\\';
    return d;
  };
  if (t == "cpu") return "Intel";
  if (t == "direct3d7" || t == "direct3ddevice7" || t == "directdraw7" ||
      t == "directinput7")
    return "0";
#ifdef _WIN32
  char buff[MAX_PATH + 1];
  if (t == "os") {
    OSVERSIONINFOA vi = {};
    vi.dwOSVersionInfoSize = sizeof(vi);
    if (!GetVersionExA(&vi)) return "";
    switch (vi.dwMajorVersion) {
      case 3: if (vi.dwMinorVersion == 51) return "Windows NT 3.1"; break;
      case 4:
        if (vi.dwMinorVersion == 0)  return "Windows 95";
        if (vi.dwMinorVersion == 10) return "Windows 98";
        if (vi.dwMinorVersion == 90) return "Windows ME";
        break;
      case 5:
        if (vi.dwMinorVersion == 0) return "Windows 2000";
        if (vi.dwMinorVersion == 1) return "Windows XP";
        if (vi.dwMinorVersion == 2) return "Windows Server 2003";
        break;
      case 6:
        if (vi.dwMinorVersion == 0) return "Windows Vista";
        if (vi.dwMinorVersion == 1) return "Windows 7";
        break;
    }
    return "";
  }
  if (t == "appdir") {
    const DWORD n = GetModuleFileNameA(nullptr, buff, MAX_PATH);
    if (!n) return "";
    bbString d(buff, n);
    const size_t k = d.find_last_of('\\');
    if (k != bbString::npos) d = d.substr(0, k);
    return toDir(d);
  }
  if (t == "apphwnd")
    return std::to_string(static_cast<long long>(reinterpret_cast<intptr_t>(bb_window_hwnd_())));
  if (t == "apphinstance")
    return std::to_string(static_cast<long long>(reinterpret_cast<intptr_t>(GetModuleHandleA(nullptr))));
  if (t == "windowsdir" && GetWindowsDirectoryA(buff, MAX_PATH)) return toDir(buff);
  if (t == "systemdir" && GetSystemDirectoryA(buff, MAX_PATH)) return toDir(buff);
  if (t == "tempdir" && GetTempPathA(MAX_PATH, buff)) return toDir(buff);
#endif
  return "";
}

// ShowPointer / HidePointer stehen in bb_input.h (BUG-124).

// CallDLL — advanced stub: logs the call and returns 0.
inline int bb_CallDLL(const bbString &dll, const bbString &func,
                      int /*in_bank*/ = 0, int /*out_bank*/ = 0) {
  std::cerr << "[stub] CallDLL \"" << dll << "\", \"" << func << "\"\n";
  return 0;
}

// ---- Message Dialogs (Blitz2D) ----
// Notify: info box (OK only).
// Confirm: Yes/No — returns true if Yes.
// Proceed: OK/Cancel — returns true if OK.

#ifdef _WIN32
inline void bb_Notify(const bbString &msg) {
  MessageBoxA(nullptr, msg.c_str(), bb_app_title_.c_str(),
              MB_OK | MB_ICONINFORMATION);
}
inline bool bb_Confirm(const bbString &msg) {
  return MessageBoxA(nullptr, msg.c_str(), bb_app_title_.c_str(),
                     MB_YESNO | MB_ICONQUESTION) == IDYES;
}
inline bool bb_Proceed(const bbString &msg) {
  return MessageBoxA(nullptr, msg.c_str(), bb_app_title_.c_str(),
                     MB_OKCANCEL | MB_ICONQUESTION) == IDOK;
}
#else
inline void bb_Notify (const bbString &msg) { std::cerr << "[Notify] "  << msg << "\n"; }
inline bool bb_Confirm(const bbString &msg) { std::cerr << "[Confirm] " << msg << "\n"; return true; }
inline bool bb_Proceed(const bbString &msg) { std::cerr << "[Proceed] " << msg << "\n"; return true; }
#endif

#endif // BLITZNEXT_BB_SYSTEM_H
