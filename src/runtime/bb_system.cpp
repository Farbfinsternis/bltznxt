// =============================================================================
// bb_system.cpp — System commands (DebugLog, End, Delay).
// =============================================================================

#include "bb_system.h"
#include "bb_globals.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <KnownFolders.h>
#include <shlobj.h>
#include <windows.h>

#endif
#include <SDL3/SDL.h>
#include <cstdlib>
#include <iostream>
#include <string>

#ifdef _WIN32
static bb_string getKnownFolder(REFKNOWNFOLDERID rfid) {
  PWSTR path = NULL;
  HRESULT hr = SHGetKnownFolderPath(rfid, 0, NULL, &path);
  if (SUCCEEDED(hr)) {
    // Convert PWSTR to bb_string (UTF-8)
    int size = WideCharToMultiByte(CP_UTF8, 0, path, -1, NULL, 0, NULL, NULL);
    std::string result(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, path, -1, &result[0], size, NULL, NULL);
    CoTaskMemFree(path);
    if (result.back() != '\\' && result.back() != '/') {
      result += "\\";
    }
    return result;
  }
  return "";
}
#endif

void print(bb_string val) { std::cout << val << std::endl; }

void write(bb_string val) { std::cout << val; }

bb_string input(bb_string prompt) {
  if (!prompt.empty())
    std::cout << prompt;
  bb_string result;
  std::getline(std::cin, result);
  return result;
}

void bbStop() {
  std::cout << "Program Stopped" << std::endl;
  exit(0);
}

void bbEnd() { exit(0); }

void runtimeerror(bb_string msg) {
  std::cerr << "Runtime Error: " << msg << std::endl;
  exit(1);
}

void debuglog(bb_string message) {
  std::cout << "[Debug] " << message << std::endl;
}

void bbBanksCleanup();
void bbFilesCleanup();
void bbTimersCleanup();
void bbFontsCleanup();

void end() {
  bbBanksCleanup();
  bbFilesCleanup();
  bbTimersCleanup();
  bbFontsCleanup();
  std::exit(0);
}

void delay(bb_int ms) { SDL_Delay((uint32_t)ms); }

#include <filesystem>

namespace fs = std::filesystem;

// File system commands moved to bb_file.cpp

bb_string getenv(bb_string var) {
  const char *val = std::getenv(var.c_str());
  return val ? bb_string(val) : "";
}

void setenv(bb_string var, bb_string val) {
#ifdef _WIN32
  _putenv_s(var.c_str(), val.c_str());
#else
  setenv(var.c_str(), val.c_str(), 1);
#endif
}

bb_string systemproperty(bb_string prop) {
#ifdef _WIN32
  char buf[MAX_PATH];
  if (prop == "windowsdir") {
    if (GetWindowsDirectoryA(buf, MAX_PATH)) {
      bb_string s = buf;
      if (s.back() != '\\')
        s += '\\';
      return s;
    }
  } else if (prop == "systemdir") {
    if (GetSystemDirectoryA(buf, MAX_PATH)) {
      bb_string s = buf;
      if (s.back() != '\\')
        s += '\\';
      return s;
    }
  } else if (prop == "tempdir") {
    if (GetTempPathA(MAX_PATH, buf)) {
      return bb_string(buf);
    }
  } else if (prop == "appdata") {
    return getKnownFolder(FOLDERID_RoamingAppData);
  } else if (prop == "localappdata") {
    return getKnownFolder(FOLDERID_LocalAppData);
  } else if (prop == "documents") {
    return getKnownFolder(FOLDERID_Documents);
  } else if (prop == "desktop") {
    return getKnownFolder(FOLDERID_Desktop);
  }
#endif
  if (prop == "appdir")
    return fs::current_path().string();
  return "";
}

bb_string commandline() { return ""; }

bb_int millisecs() { return (bb_int)SDL_GetTicks(); }
