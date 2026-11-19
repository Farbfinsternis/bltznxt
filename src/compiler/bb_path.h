#ifndef BLITZNEXT_BB_PATH_H
#define BLITZNEXT_BB_PATH_H

#include <filesystem>
#include <string>
#include <type_traits>
#include "bb_string.h"

#ifdef _WIN32
  #include <windows.h>  // MultiByteToWideChar, WideCharToMultiByte
#endif

// ---- Dateinamen eines Blitz-Programms (BUG-202) ----
//
// Ein Blitz3D-String ist eine Bytefolge in der ANSI-Codepage (unter
// deutschem Windows Windows-1252): so steht ein Dateiname im Quelltext aus
// der Blitz-IDE, so liefert ihn das Original aus NextFile und CurrentDir.
// std::filesystem::path liest einen std::string unter MinGW dagegen als
// UTF-8 und wirft beim ersten Umlaut ("Illegal byte sequence") - FileType,
// ReadDir, CopyFile, die Loader ... brachen das Programm ab. SDL erwartet
// UTF-8. fopen und stb_image dagegen nehmen ANSI und brauchen nichts.
//
// Deshalb an jeder Grenze zu std::filesystem und zu SDL ueber diese Helfer:
//   bb_path_  ANSI-String -> Pfad
//   bb_ansi_  Pfad -> ANSI-String (fuer NextFile, CurrentDir, fopen, Schluessel)
//   bb_utf8_  ANSI-String -> UTF-8 (fuer Text an SDL)
//   bb_os_path_   ANSI-String -> Pfad fuer fopen, stb_image und die Lader
//   bb_sdl_path_  ANSI-String -> Dateiname fuer SDL (SDL_LoadWAV, TTF_OpenFont)
// Unter Linux wandeln sie zusaetzlich Zeichensatz, Backslashes und die
// Schreibung (siehe unten).

#ifdef _WIN32
inline std::wstring bb_wide_from_(const std::string &s, UINT cp) {
  if (s.empty()) return std::wstring();
  const int n = MultiByteToWideChar(cp, 0, s.data(), (int)s.size(), nullptr, 0);
  std::wstring w(n > 0 ? n : 0, L'\0');
  if (n > 0) MultiByteToWideChar(cp, 0, s.data(), (int)s.size(), w.data(), n);
  return w;
}

inline std::string bb_narrow_from_(const std::wstring &w, UINT cp) {
  if (w.empty()) return std::string();
  const int n = WideCharToMultiByte(cp, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
  std::string s(n > 0 ? n : 0, '\0');
  if (n > 0) WideCharToMultiByte(cp, 0, w.data(), (int)w.size(), s.data(), n, nullptr, nullptr);
  return s;
}

inline std::filesystem::path bb_path_(const bbString &ansi) {
  return std::filesystem::path(bb_wide_from_(ansi, CP_ACP));
}
inline bbString bb_ansi_(const std::filesystem::path &p) {
  return bb_narrow_from_(p.wstring(), CP_ACP);
}
inline std::string bb_utf8_(const bbString &ansi) {
  return bb_narrow_from_(bb_wide_from_(ansi, CP_ACP), CP_UTF8);
}
// Der Pfad, wie ihn das Betriebssystem fuer fopen, stb_image und die Lader
// erwartet. Unter Windows sind das die ANSI-Bytes selbst.
inline std::string bb_os_path_(const bbString &ansi) { return ansi; }
// Ein Dateiname fuer SDL (SDL_LoadWAV, TTF_OpenFont): UTF-8
inline std::string bb_sdl_path_(const bbString &ansi) { return bb_utf8_(ansi); }
// Text fuer die Konsole: dort gilt nach bb_console_ansi_ die ANSI-Codepage
inline const bbString &bb_console_text_(const bbString &s) { return s; }
inline bbString bb_console_in_(const std::string &s) { return s; }
inline bbString bb_arg_text_(const char *s) { return s; }
#else
// ---- Linux (LINUX_PORT.md, L4) ----
//
// Die Strings eines Blitz-Programms bleiben Windows-1252-Bytes wie unter
// Windows - Len, Mid$, Asc und gespeicherte Dateien verhalten sich gleich.
// Linux erwartet dagegen UTF-8: in Dateinamen, im Terminal, bei SDL. An diesen
// Grenzen wird gewandelt. Ist ein String schon gueltiges UTF-8 (Quelltext mit
// einem Linux-Editor gespeichert, Name aus fontconfig), bleibt er, wie er ist.
//
// Und ein altes Programm schreibt Pfade wie unter Windows: "Media\Bild.PNG"
// fuer media/bild.png. bb_os_path_ dreht die Backslashes um und sucht, wenn
// es den Pfad so nicht gibt, jeden Teil ohne Ruecksicht auf Gross- und
// Kleinschreibung - wie NTFS.

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

// Windows-1252 0x80..0x9F; die fuenf freien Stellen bleiben die
// Steuerzeichen gleicher Nummer, damit nichts verloren geht.
inline constexpr unsigned short bb_cp1252_hi_[32] = {
  0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
  0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008D, 0x017D, 0x008F,
  0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
  0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x009D, 0x017E, 0x0178 };

// Naechstes Zeichen aus UTF-8; -1 bei einer ungueltigen Folge
inline long bb_utf8_next_(const std::string &s, size_t &i) {
  const unsigned char c = static_cast<unsigned char>(s[i]);
  const int n = c < 0x80 ? 0 : (c >> 5) == 6 ? 1 : (c >> 4) == 14 ? 2 : (c >> 3) == 30 ? 3 : -1;
  if (n < 0) return -1;
  long cp = n == 0 ? c : n == 1 ? (c & 0x1F) : n == 2 ? (c & 0x0F) : (c & 0x07);
  for (int k = 1; k <= n; ++k) {
    if (i + k >= s.size()) return -1;
    const unsigned char d = static_cast<unsigned char>(s[i + k]);
    if ((d >> 6) != 2) return -1;
    cp = (cp << 6) | (d & 0x3F);
  }
  static const long minv[4] = { 0, 0x80, 0x800, 0x10000 };
  if (cp < minv[n] || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) return -1;
  i += n + 1;
  return cp;
}

inline bool bb_is_utf8_(const std::string &s) {
  for (size_t i = 0; i < s.size();)
    if (bb_utf8_next_(s, i) < 0) return false;
  return true;
}

inline void bb_utf8_put_(std::string &out, long cp) {
  if (cp < 0x80) out += static_cast<char>(cp);
  else if (cp < 0x800) {
    out += static_cast<char>(0xC0 | (cp >> 6));
    out += static_cast<char>(0x80 | (cp & 0x3F));
  } else if (cp < 0x10000) {
    out += static_cast<char>(0xE0 | (cp >> 12));
    out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
    out += static_cast<char>(0x80 | (cp & 0x3F));
  } else {
    out += static_cast<char>(0xF0 | (cp >> 18));
    out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
    out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
    out += static_cast<char>(0x80 | (cp & 0x3F));
  }
}

// Windows-1252 -> UTF-8; schon gueltiges UTF-8 (auch reines ASCII) bleibt.
inline std::string bb_utf8_(const bbString &s) {
  if (bb_is_utf8_(s)) return s;
  std::string out;
  out.reserve(s.size() + s.size() / 2);
  for (unsigned char c : s) {
    if (c < 0x80)      out += static_cast<char>(c);
    else if (c < 0xA0) bb_utf8_put_(out, bb_cp1252_hi_[c - 0x80]);
    else               bb_utf8_put_(out, c);
  }
  return out;
}

// UTF-8 -> Windows-1252, wenn jedes Zeichen dort vorkommt; sonst unveraendert
// (dann findet bb_utf8_ das gueltige UTF-8 wieder, ein Name geht nie verloren).
inline bbString bb_ansi_from_utf8_(const std::string &s) {
  bbString out;
  out.reserve(s.size());
  for (size_t i = 0; i < s.size();) {
    const long cp = bb_utf8_next_(s, i);
    if (cp < 0) return s;
    if (cp < 0x80 || (cp >= 0xA0 && cp <= 0xFF)) { out += static_cast<char>(cp); continue; }
    int k = 0;
    while (k < 32 && bb_cp1252_hi_[k] != cp) ++k;
    if (k == 32) return s;
    out += static_cast<char>(0x80 + k);
  }
  return out;
}

// Gross-/Kleinschreibung wie NTFS fuer ASCII und die Buchstaben von Latin-1
inline long bb_fold_cp_(long cp) {
  if (cp >= 'A' && cp <= 'Z') return cp + 32;
  if (cp >= 0xC0 && cp <= 0xDE && cp != 0xD7) return cp + 32;
  return cp;
}
inline bool bb_name_ieq_(const std::string &a, const std::string &b) {
  size_t i = 0, j = 0;
  while (i < a.size() && j < b.size()) {
    const long x = bb_utf8_next_(a, i), y = bb_utf8_next_(b, j);
    if (x < 0 || y < 0) return a == b;
    if (bb_fold_cp_(x) != bb_fold_cp_(y)) return false;
  }
  return i == a.size() && j == b.size();
}

inline bool bb_os_exists_(const std::string &p) {
  struct stat st;
  return ::stat(p.c_str(), &st) == 0;
}

inline std::string bb_os_path_(const bbString &ansi) {
  std::string p = bb_utf8_(ansi);
  for (char &c : p) if (c == '\\') c = '/';
  if (p.empty() || bb_os_exists_(p)) return p;
  // Teil fuer Teil; was es auch ohne Ruecksicht auf die Schreibung nicht
  // gibt, bleibt wie geschrieben (eine neue Datei fuer WriteFile).
  std::string cur = p[0] == '/' ? "/" : "";
  size_t pos = p[0] == '/' ? 1 : 0;
  while (pos <= p.size()) {
    size_t slash = p.find('/', pos);
    if (slash == std::string::npos) slash = p.size();
    const std::string part = p.substr(pos, slash - pos);
    const bool last = slash >= p.size();
    pos = slash + 1;
    if (part.empty()) { if (last) break; continue; }   // "a//b", Schluss-'/'
    std::string next = cur + part;
    if (!bb_os_exists_(next) && part != "." && part != "..") {
      bool found = false;
      if (DIR *d = opendir(cur.empty() ? "." : cur.c_str())) {
        while (dirent *e = readdir(d)) {
          if (bb_name_ieq_(e->d_name, part)) { next = cur + e->d_name; found = true; break; }
        }
        closedir(d);
      }
      if (!found) return cur + p.substr(pos - part.size() - 1);
    }
    cur = next;
    if (!last) cur += '/';
    else break;
  }
  return cur;
}

inline std::filesystem::path bb_path_(const bbString &s) {
  return std::filesystem::path(bb_os_path_(s));
}
// Name aus dem Dateisystem (NextFile, CurrentDir) als Blitz-String
inline bbString bb_ansi_(const std::filesystem::path &p) { return bb_ansi_from_utf8_(p.string()); }
inline std::string bb_sdl_path_(const bbString &s) { return bb_os_path_(s); }

// Print und Write: ein Terminal bekommt UTF-8. Umgeleitete Ausgabe (Datei,
// Pipe) bleibt Byte fuer Byte wie unter Windows.
inline bool bb_stdout_tty_() {
  static const bool tty = isatty(STDOUT_FILENO) != 0;
  return tty;
}
inline bbString bb_console_text_(const bbString &s) { return bb_stdout_tty_() ? bb_utf8_(s) : s; }
// Input$ aus einem Terminal kommt als UTF-8
inline bbString bb_console_in_(const std::string &s) {
  return isatty(STDIN_FILENO) ? bb_ansi_from_utf8_(s) : s;
}
// CommandLine$: Argumente kommen unter Linux als UTF-8
inline bbString bb_arg_text_(const char *s) { return bb_ansi_from_utf8_(s); }
#endif

// Print/Write: nur echte Strings gehen durch bb_console_text_. Typen mit
// eigenem operator<< oder einer Umwandlung nach bbString bleiben, wie sie sind.
template <typename T>
inline constexpr bool bb_is_text_v_ =
    std::is_same_v<std::decay_t<T>, bbString> || std::is_same_v<std::decay_t<T>, const char *> ||
    std::is_same_v<std::decay_t<T>, char *>;

#endif // BLITZNEXT_BB_PATH_H
