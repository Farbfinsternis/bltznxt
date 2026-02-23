#ifndef BLITZNEXT_BB_FILE_H
#define BLITZNEXT_BB_FILE_H

#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <unordered_map>
#include <iostream>
#include "bb_string.h"

// ---- File Handle Management ----
//
// Blitz3D file handles are opaque ints (0 = invalid/error).
// Internally we map int → FILE*.

inline std::unordered_map<int, FILE*> bb_file_handles_;
inline int                            bb_file_next_id_ = 1;

// Returns the FILE* for a handle, or nullptr if invalid.
inline FILE* bb_file_get_(int handle) {
  auto it = bb_file_handles_.find(handle);
  return (it != bb_file_handles_.end()) ? it->second : nullptr;
}

// Internal helper — opens a file, stores it, returns handle (0 on failure).
inline int bb_file_open_(const bbString &path, const char *mode) {
  FILE *f = std::fopen(path.c_str(), mode);
  if (!f) {
    std::cerr << "[runtime] Cannot open file: " << path << "\n";
    return 0;
  }
  int id = bb_file_next_id_++;
  bb_file_handles_[id] = f;
  return id;
}

// ---- Open / Close ----

// OpenFile: read+write access to an existing file.
inline int bb_OpenFile(const bbString &path) {
  return bb_file_open_(path, "r+b");
}

// ReadFile: open for reading only.
inline int bb_ReadFile(const bbString &path) {
  return bb_file_open_(path, "rb");
}

// WriteFile: create or truncate for writing.
inline int bb_WriteFile(const bbString &path) {
  return bb_file_open_(path, "wb");
}

inline void bb_CloseFile(int handle) {
  auto it = bb_file_handles_.find(handle);
  if (it != bb_file_handles_.end()) {
    if (it->second) std::fclose(it->second);
    bb_file_handles_.erase(it);
  }
}

// ---- Position / Seek ----

inline int bb_FilePos(int handle) {
  FILE *f = bb_file_get_(handle);
  return f ? static_cast<int>(std::ftell(f)) : 0;
}

// Seeks to an absolute byte offset from the start of the file.
inline void bb_SeekFile(int handle, int pos) {
  FILE *f = bb_file_get_(handle);
  if (f) std::fseek(f, static_cast<long>(pos), SEEK_SET);
}

// ---- Status ----

// Returns true if the current position is at or past end-of-file.
inline bool bb_Eof(int handle) {
  FILE *f = bb_file_get_(handle);
  if (!f) return true;
  long cur = std::ftell(f);
  std::fseek(f, 0, SEEK_END);
  long end = std::ftell(f);
  std::fseek(f, cur, SEEK_SET);
  return cur >= end;
}

// Returns the number of bytes remaining from current position to end.
inline int bb_ReadAvail(int handle) {
  FILE *f = bb_file_get_(handle);
  if (!f) return 0;
  long cur = std::ftell(f);
  std::fseek(f, 0, SEEK_END);
  long end = std::ftell(f);
  std::fseek(f, cur, SEEK_SET);
  return static_cast<int>(end - cur);
}

// ---- Write Primitives (M26) ----

inline void bb_WriteByte(int handle, int val) {
  FILE *f = bb_file_get_(handle);
  if (!f) return;
  uint8_t b = static_cast<uint8_t>(val & 0xFF);
  std::fwrite(&b, 1, 1, f);
}

inline void bb_WriteShort(int handle, int val) {
  FILE *f = bb_file_get_(handle);
  if (!f) return;
  uint16_t s = static_cast<uint16_t>(val & 0xFFFF);
  std::fwrite(&s, 2, 1, f);
}

inline void bb_WriteInt(int handle, int val) {
  FILE *f = bb_file_get_(handle);
  if (!f) return;
  std::fwrite(&val, 4, 1, f);
}

inline void bb_WriteFloat(int handle, float val) {
  FILE *f = bb_file_get_(handle);
  if (!f) return;
  std::fwrite(&val, 4, 1, f);
}

// Writes a null-terminated string (no trailing newline).
inline void bb_WriteString(int handle, const bbString &s) {
  FILE *f = bb_file_get_(handle);
  if (!f) return;
  std::fwrite(s.c_str(), 1, s.size() + 1, f); // +1 for null terminator
}

// Writes a string followed by a newline character.
inline void bb_WriteLine(int handle, const bbString &s) {
  FILE *f = bb_file_get_(handle);
  if (!f) return;
  std::fwrite(s.c_str(), 1, s.size(), f);
  std::fputc('\n', f);
}

// WriteBytes / ReadBytes are implemented in bb_bank.h (requires bank handles).

// ---- Read Primitives (M25) ----

inline int bb_ReadByte(int handle) {
  FILE *f = bb_file_get_(handle);
  if (!f) return 0;
  uint8_t b = 0;
  std::fread(&b, 1, 1, f);
  return static_cast<int>(b);
}

inline int bb_ReadShort(int handle) {
  FILE *f = bb_file_get_(handle);
  if (!f) return 0;
  uint16_t s = 0;
  std::fread(&s, 2, 1, f);
  return static_cast<int>(s);
}

inline int bb_ReadInt(int handle) {
  FILE *f = bb_file_get_(handle);
  if (!f) return 0;
  int val = 0;
  std::fread(&val, 4, 1, f);
  return val;
}

inline float bb_ReadFloat(int handle) {
  FILE *f = bb_file_get_(handle);
  if (!f) return 0.0f;
  float val = 0.0f;
  std::fread(&val, 4, 1, f);
  return val;
}

// Reads a null-terminated string.
inline bbString bb_ReadString(int handle) {
  FILE *f = bb_file_get_(handle);
  if (!f) return "";
  bbString result;
  int c;
  while ((c = std::fgetc(f)) != EOF && c != '\0')
    result += static_cast<char>(c);
  return result;
}

// Reads a newline-terminated string, stripping \r\n.
inline bbString bb_ReadLine(int handle) {
  FILE *f = bb_file_get_(handle);
  if (!f) return "";
  bbString result;
  int c;
  while ((c = std::fgetc(f)) != EOF && c != '\n')
    if (c != '\r') result += static_cast<char>(c);
  return result;
}


// ---- Directory Iteration (M27) ----

inline std::unordered_map<int, std::filesystem::directory_iterator> bb_dir_handles_;
inline int                                                           bb_dir_next_id_ = 1;

// Opens a directory for iteration; returns handle (0 on failure).
inline int bb_ReadDir(const bbString &path) {
  try {
    int id = bb_dir_next_id_++;
    bb_dir_handles_[id] = std::filesystem::directory_iterator(path);
    return id;
  } catch (...) {
    std::cerr << "[runtime] ReadDir: cannot open directory: " << path << "\n";
    return 0;
  }
}

// Returns the next filename in the directory, or "" when exhausted.
inline bbString bb_NextFile(int handle) {
  auto it = bb_dir_handles_.find(handle);
  if (it == bb_dir_handles_.end()) return "";
  auto &di = it->second;
  if (di == std::filesystem::directory_iterator{}) return "";
  bbString name = di->path().filename().string();
  ++di;
  return name;
}

inline void bb_CloseDir(int handle) {
  bb_dir_handles_.erase(handle);
}

// ---- Directory & File Operations (M27) ----

inline bbString bb_CurrentDir() {
  std::error_code ec;
  auto p = std::filesystem::current_path(ec);
  return ec ? bbString("") : p.string();
}

inline void bb_ChangeDir(const bbString &path) {
  std::error_code ec;
  std::filesystem::current_path(path, ec);
  if (ec) std::cerr << "[runtime] ChangeDir failed: " << ec.message() << "\n";
}

inline void bb_CreateDir(const bbString &path) {
  std::error_code ec;
  std::filesystem::create_directory(path, ec);
  if (ec) std::cerr << "[runtime] CreateDir failed: " << ec.message() << "\n";
}

// Removes directory and all its contents (like Blitz3D DeleteDir).
inline void bb_DeleteDir(const bbString &path) {
  std::error_code ec;
  std::filesystem::remove_all(path, ec);
  if (ec) std::cerr << "[runtime] DeleteDir failed: " << ec.message() << "\n";
}

// Returns 0=none, 1=file, 2=directory.
inline int bb_FileType(const bbString &path) {
  std::error_code ec;
  auto st = std::filesystem::status(path, ec);
  if (ec || !std::filesystem::exists(st)) return 0;
  if (std::filesystem::is_directory(st))   return 2;
  if (std::filesystem::is_regular_file(st)) return 1;
  return 0;
}

// Returns file size in bytes, or 0 on error.
inline int bb_FileSize(const bbString &path) {
  std::error_code ec;
  auto sz = std::filesystem::file_size(path, ec);
  return ec ? 0 : static_cast<int>(sz);
}

// Copies src to dst, overwriting if dst exists.
inline void bb_CopyFile(const bbString &src, const bbString &dst) {
  std::error_code ec;
  std::filesystem::copy_file(src, dst,
    std::filesystem::copy_options::overwrite_existing, ec);
  if (ec) std::cerr << "[runtime] CopyFile failed: " << ec.message() << "\n";
}

inline void bb_DeleteFile(const bbString &path) {
  std::error_code ec;
  std::filesystem::remove(path, ec);
  if (ec) std::cerr << "[runtime] DeleteFile failed: " << ec.message() << "\n";
}

#endif // BLITZNEXT_BB_FILE_H
