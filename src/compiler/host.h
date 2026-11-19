#ifndef BLITZNEXT_HOST_H
#define BLITZNEXT_HOST_H

// Was blitzcc vom Betriebssystem braucht: Pfade wandeln (ansipath.h), den
// eigenen Ordner finden, g++ und das fertige Programm starten. Alles andere
// im Compiler ist plattformneutral (LINUX_PORT.md, Paket L2).
//
// Nur in blitzcc.cpp einbinden, und dort als letztes: <windows.h> bringt
// Makros mit (BOOL, ERROR, min/max, ...), die nicht in die Projektheader
// geraten duerfen. Die Funktionen sind deshalb hier definiert, nicht inline.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include "ansipath.h"

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#else
#  include <cerrno>
#  include <dirent.h>
#  include <sys/types.h>
#  include <sys/wait.h>
#  include <unistd.h>
#endif

namespace host {

namespace fs = std::filesystem;

// Eine Befehlszeile als Liste von Argumenten in der Zeichenbreite des Systems
// (UTF-16 unter Windows, Bytes sonst). So kommt ein Umlaut in einem Pfad
// unveraendert bei g++ an (BUG-201), und es gibt keine Shell dazwischen.
using Arg  = fs::path::string_type;
using Args = std::vector<Arg>;

inline Arg arg(const char *ascii) { return fs::path(ascii).native(); }
inline Arg arg(const fs::path &p) { return fs::path(p).make_preferred().native(); }
inline Arg arg(const char *prefix, const fs::path &p) { return arg(prefix) + arg(p); }

#ifdef _WIN32
// Name des fertigen Programms: "spiel" -> "spiel.exe"
constexpr const char *kExeSuffix = ".exe";
#else
constexpr const char *kExeSuffix = "";
#endif

struct RunOptions {
  const fs::path *cwd = nullptr;   // Arbeitsordner; nullptr = der eigene
  bool noConsole  = false;         // Windows: kein eigenes Konsolenfenster
  bool foreground = false;         // Windows: Fenster des Kindes darf nach vorn
};

// Startet args[0] (unter Windows mit Pfad, sonst auch ueber PATH), wartet und
// gibt den Exitcode zurueck; -1, wenn das Programm nicht starten konnte (die
// Meldung steht dann schon auf stderr).
int run(const Args &args, const RunOptions &opt = {});

// Ordner, in dem blitzcc liegt
fs::path exeDir(const char *argv0);

unsigned long processId();

// Programm im PATH suchen (fuer den Compiler unter Linux); leer, wenn keins
fs::path findInPath(const std::string &name);

} // namespace host

// ============================================================================
#ifdef _WIN32

// ---- ANSI-Pfade (ansipath.h, BUG-201) --------------------------------------

std::wstring ansiWide(const std::string &ansi) {
  if (ansi.empty()) return std::wstring();
  const int n = MultiByteToWideChar(CP_ACP, 0, ansi.data(), (int)ansi.size(), nullptr, 0);
  std::wstring w(n > 0 ? n : 0, L'\0');
  if (n > 0) MultiByteToWideChar(CP_ACP, 0, ansi.data(), (int)ansi.size(), w.data(), n);
  return w;
}

static std::string ansiFromWide(const std::wstring &w) {
  if (w.empty()) return std::string();
  const int n = WideCharToMultiByte(CP_ACP, 0, w.data(), (int)w.size(), nullptr, 0,
                                    nullptr, nullptr);
  std::string s(n > 0 ? n : 0, '\0');
  if (n > 0)
    WideCharToMultiByte(CP_ACP, 0, w.data(), (int)w.size(), s.data(), n, nullptr, nullptr);
  return s;
}

std::filesystem::path ansiPath(const std::string &ansi) {
  return std::filesystem::path(ansiWide(ansi));
}
std::string ansiString(const std::filesystem::path &p) { return ansiFromWide(p.wstring()); }
std::string ansiGeneric(const std::filesystem::path &p) {
  return ansiFromWide(p.generic_wstring());
}

namespace host {

// Ein Argument so in die Befehlszeile setzen, dass das Kind es mit den Regeln
// von CommandLineToArgvW genau so zurueckliest (Anfuehrungszeichen und
// Backslashes davor werden verdoppelt bzw. geschuetzt).
static std::wstring quoteArg(const std::wstring &a) {
  if (!a.empty() && a.find_first_of(L" \t\n\v\"") == std::wstring::npos) return a;
  std::wstring r = L"\"";
  for (size_t i = 0;; ++i) {
    size_t bs = 0;
    while (i < a.size() && a[i] == L'\\') { ++i; ++bs; }
    if (i == a.size()) { r.append(bs * 2, L'\\'); break; }
    if (a[i] == L'"') { r.append(bs * 2 + 1, L'\\'); r.push_back(L'"'); }
    else              { r.append(bs, L'\\');         r.push_back(a[i]); }
  }
  r.push_back(L'"');
  return r;
}

int run(const Args &args, const RunOptions &opt) {
  if (args.empty()) return -1;
  std::wstring cmd;
  for (const auto &a : args) {
    if (!cmd.empty()) cmd += L' ';
    cmd += quoteArg(a);
  }
  // Nach vorn darf ein Fenster nur, wer von der Anwendung im Vordergrund
  // gestartet wurde - das ist blitzcc (von der IDE), nicht das Programm
  // (von blitzcc). Ohne diese Weitergabe gehen Grafik- und Textfenster
  // hinter der IDE auf.
  if (opt.foreground) AllowSetForegroundWindow(ASFW_ANY);
  STARTUPINFOW si = {};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi = {};
  // Das Programm selbst mit vollem Pfad als Anwendungsname: dann sucht
  // Windows nicht erst im PATH und im eigenen Ordner.
  if (!CreateProcessW(args[0].c_str(), cmd.data(), nullptr, nullptr, FALSE,
                      opt.noConsole ? CREATE_NO_WINDOW : 0, nullptr,
                      opt.cwd ? opt.cwd->c_str() : nullptr, &si, &pi)) {
    std::fprintf(stderr, "[ERROR] CreateProcessW failed (code %lu)\n",
                 (unsigned long)GetLastError());
    return -1;
  }
  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD exitCode = 1;
  GetExitCodeProcess(pi.hProcess, &exitCode);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return (int)exitCode;
}

// Ueber die Wide-API: argv[0] ist ANSI und waere mit einem Umlaut im
// Installationspfad schon hier gescheitert (BUG-201).
fs::path exeDir(const char *argv0) {
  wchar_t buf[MAX_PATH * 4];
  const DWORD n = GetModuleFileNameW(nullptr, buf, (DWORD)(sizeof(buf) / sizeof(buf[0])));
  if (n > 0 && n < sizeof(buf) / sizeof(buf[0]))
    return fs::path(std::wstring(buf, n)).parent_path();
  if (argv0) return fs::absolute(ansiPath(argv0)).parent_path();
  return fs::path();
}

unsigned long processId() { return (unsigned long)GetCurrentProcessId(); }

fs::path findInPath(const std::string &name) {
  wchar_t buf[MAX_PATH * 4];
  const DWORD n = SearchPathW(nullptr, ansiWide(name).c_str(), L".exe",
                              (DWORD)(sizeof(buf) / sizeof(buf[0])), buf, nullptr);
  if (n > 0 && n < sizeof(buf) / sizeof(buf[0])) return fs::path(std::wstring(buf, n));
  return fs::path();
}

} // namespace host

// ============================================================================
#else // POSIX

// Unter Linux sind Pfade Bytes, in der Regel UTF-8. Ein altes Programm
// schreibt sie aber wie unter Windows: in Windows-1252, mit Backslash und ohne
// Ruecksicht auf die Schreibung - Include "Inc\Funcs.BB" fuer inc/funcs.bb.
// ansiPath macht daraus den Pfad, den es gibt, wie bb_os_path_ in der
// Laufzeit (bb_path.h, LINUX_PORT.md L4). Zurueck bleibt es UTF-8: der
// Rueckweg durch ansiPath findet gueltiges UTF-8 unveraendert wieder.
namespace host_detail {
inline constexpr unsigned short kCp1252Hi[32] = {
  0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
  0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008D, 0x017D, 0x008F,
  0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
  0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x009D, 0x017E, 0x0178 };

// naechstes Zeichen aus UTF-8, -1 bei einer ungueltigen Folge
inline long next(const std::string &s, size_t &i) {
  const unsigned char c = static_cast<unsigned char>(s[i]);
  const int n = c < 0x80 ? 0 : (c >> 5) == 6 ? 1 : (c >> 4) == 14 ? 2 : (c >> 3) == 30 ? 3 : -1;
  if (n < 0) return -1;
  long cp = n == 0 ? c : n == 1 ? (c & 0x1F) : n == 2 ? (c & 0x0F) : (c & 0x07);
  for (int k = 1; k <= n; ++k) {
    if (i + k >= s.size() || (static_cast<unsigned char>(s[i + k]) >> 6) != 2) return -1;
    cp = (cp << 6) | (s[i + k] & 0x3F);
  }
  static const long minv[4] = { 0, 0x80, 0x800, 0x10000 };
  if (cp < minv[n] || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) return -1;
  i += n + 1;
  return cp;
}

inline void put(std::string &out, long cp) {
  if (cp < 0x80) { out += static_cast<char>(cp); return; }
  if (cp < 0x800) { out += static_cast<char>(0xC0 | (cp >> 6)); }
  else { out += static_cast<char>(0xE0 | (cp >> 12)); out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F)); }
  out += static_cast<char>(0x80 | (cp & 0x3F));
}

inline std::string toUtf8(const std::string &s) {
  for (size_t i = 0; i < s.size();)
    if (next(s, i) < 0) {   // kein gueltiges UTF-8: Windows-1252
      std::string out;
      for (unsigned char c : s)
        put(out, c < 0x80 ? c : c < 0xA0 ? kCp1252Hi[c - 0x80] : c);
      return out;
    }
  return s;
}

inline long fold(long cp) {
  if (cp >= 'A' && cp <= 'Z') return cp + 32;
  if (cp >= 0xC0 && cp <= 0xDE && cp != 0xD7) return cp + 32;
  return cp;
}
inline bool sameName(const std::string &a, const std::string &b) {
  size_t i = 0, j = 0;
  while (i < a.size() && j < b.size()) {
    const long x = next(a, i), y = next(b, j);
    if (x < 0 || y < 0) return a == b;
    if (fold(x) != fold(y)) return false;
  }
  return i == a.size() && j == b.size();
}

inline bool exists(const std::string &p) { return access(p.c_str(), F_OK) == 0; }

inline std::string resolve(const std::string &ansi) {
  std::string p = toUtf8(ansi);
  for (char &c : p) if (c == '\\') c = '/';
  if (p.empty() || exists(p)) return p;
  std::string cur = p[0] == '/' ? "/" : "";
  size_t pos = p[0] == '/' ? 1 : 0;
  while (pos <= p.size()) {
    size_t slash = p.find('/', pos);
    if (slash == std::string::npos) slash = p.size();
    const std::string part = p.substr(pos, slash - pos);
    const bool last = slash >= p.size();
    const size_t partStart = pos;
    pos = slash + 1;
    if (part.empty()) { if (last) break; continue; }
    std::string cand = cur + part;
    if (!exists(cand) && part != "." && part != "..") {
      bool found = false;
      if (DIR *d = opendir(cur.empty() ? "." : cur.c_str())) {
        while (dirent *e = readdir(d))
          if (sameName(e->d_name, part)) { cand = cur + e->d_name; found = true; break; }
        closedir(d);
      }
      if (!found) return cur + p.substr(partStart);   // bleibt, wie geschrieben
    }
    cur = cand;
    if (last) break;
    cur += '/';
  }
  return cur;
}
} // namespace host_detail

std::filesystem::path ansiPath(const std::string &ansi) {
  return std::filesystem::path(host_detail::resolve(ansi));
}
std::string ansiString(const std::filesystem::path &p) { return p.string(); }
std::string ansiGeneric(const std::filesystem::path &p) { return p.generic_string(); }

namespace host {

int run(const Args &args, const RunOptions &opt) {
  if (args.empty()) return -1;
  std::vector<char *> argv;
  for (const auto &a : args) argv.push_back(const_cast<char *>(a.c_str()));
  argv.push_back(nullptr);

  const pid_t pid = fork();
  if (pid < 0) {
    std::fprintf(stderr, "[ERROR] fork failed (%s)\n", std::strerror(errno));
    return -1;
  }
  if (pid == 0) {
    if (opt.cwd && chdir(opt.cwd->c_str()) != 0) _exit(127);
    execvp(argv[0], argv.data());
    std::fprintf(stderr, "[ERROR] could not start \"%s\" (%s)\n", argv[0], std::strerror(errno));
    _exit(127);
  }
  int status = 0;
  while (waitpid(pid, &status, 0) < 0) {
    if (errno != EINTR) return -1;
  }
  if (WIFEXITED(status)) {
    const int code = WEXITSTATUS(status);
    return code == 127 ? -1 : code;   // 127: exec gescheitert, Meldung steht schon da
  }
  if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
  return -1;
}

fs::path exeDir(const char *argv0) {
  std::error_code ec;
  const fs::path self = fs::read_symlink("/proc/self/exe", ec);
  if (!ec && !self.empty()) return self.parent_path();
  if (argv0) return fs::absolute(argv0).parent_path();
  return fs::path();
}

unsigned long processId() { return (unsigned long)getpid(); }

fs::path findInPath(const std::string &name) {
  const char *path = std::getenv("PATH");
  if (!path) return fs::path();
  std::string all(path);
  size_t pos = 0;
  while (pos <= all.size()) {
    const size_t colon = all.find(':', pos);
    std::string dir = all.substr(pos, colon == std::string::npos ? std::string::npos
                                                                 : colon - pos);
    if (dir.empty()) dir = ".";
    const fs::path cand = fs::path(dir) / name;
    if (access(cand.c_str(), X_OK) == 0) return cand;
    if (colon == std::string::npos) break;
    pos = colon + 1;
  }
  return fs::path();
}

} // namespace host

#endif // _WIN32

#endif // BLITZNEXT_HOST_H
