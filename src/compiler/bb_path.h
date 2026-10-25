#ifndef BLITZNEXT_BB_PATH_H
#define BLITZNEXT_BB_PATH_H

#include <filesystem>
#include <string>
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
//   bb_utf8_  ANSI-String -> UTF-8 (fuer Dateinamen an SDL)

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
#else
inline std::filesystem::path bb_path_(const bbString &s) { return std::filesystem::path(s); }
inline bbString bb_ansi_(const std::filesystem::path &p) { return p.string(); }
inline std::string bb_utf8_(const bbString &s) { return s; }
#endif

#endif // BLITZNEXT_BB_PATH_H
