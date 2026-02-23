#ifndef BLITZNEXT_BB_BANK_H
#define BLITZNEXT_BB_BANK_H

#include <cstdint>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <iostream>
#include "bb_system.h"  // bb_RuntimeError
#include "bb_file.h"    // bb_file_get_ — for ReadBytes/WriteBytes

// ---- Bank Handle Management ----
//
// A "bank" in Blitz3D is a raw byte buffer addressed by an int handle.
// Internally we map int → std::vector<uint8_t>.

inline std::unordered_map<int, std::vector<uint8_t>> bb_bank_handles_;
inline int                                            bb_bank_next_id_ = 1;

// Returns pointer to the bank vector, or nullptr if handle is invalid.
inline std::vector<uint8_t>* bb_bank_get_(int handle) {
  auto it = bb_bank_handles_.find(handle);
  return (it != bb_bank_handles_.end()) ? &it->second : nullptr;
}

// Bounds-checks [offset, offset+size) against the bank; calls RuntimeError
// and returns false on violation.  Returns true when access is valid.
inline bool bb_bank_check_(std::vector<uint8_t>* b, int offset, int size) {
  if (!b || offset < 0 || offset + size > static_cast<int>(b->size())) {
    bb_RuntimeError("bank access out of bounds");
    return false;
  }
  return true;
}

// ---- Allocation (M28) ----

// Allocates a zeroed bank of `size` bytes; returns handle (never 0).
inline int bb_CreateBank(int size) {
  int id = bb_bank_next_id_++;
  bb_bank_handles_[id] = std::vector<uint8_t>(static_cast<size_t>(size), 0);
  return id;
}

inline void bb_FreeBank(int handle) {
  bb_bank_handles_.erase(handle);
}

// Returns the size in bytes of the bank, or 0 for an invalid handle.
inline int bb_BankSize(int handle) {
  auto* b = bb_bank_get_(handle);
  return b ? static_cast<int>(b->size()) : 0;
}

// Resizes the bank; zero-fills any newly added bytes.
inline void bb_ResizeBank(int handle, int newsize) {
  auto* b = bb_bank_get_(handle);
  if (b) b->resize(static_cast<size_t>(newsize), 0);
}

// Copies `count` bytes from src[srcOffset] to dst[dstOffset].
// Uses memmove so overlapping regions within the same buffer are safe.
inline void bb_CopyBank(int src, int srcOffset, int dst, int dstOffset, int count) {
  if (count <= 0) return;
  auto* s = bb_bank_get_(src);
  auto* d = bb_bank_get_(dst);
  if (!s || !d) { bb_RuntimeError("CopyBank: invalid handle"); return; }
  if (srcOffset < 0 || srcOffset + count > static_cast<int>(s->size()) ||
      dstOffset < 0 || dstOffset + count > static_cast<int>(d->size())) {
    bb_RuntimeError("CopyBank: out of bounds");
    return;
  }
  std::memmove(d->data() + dstOffset, s->data() + srcOffset,
               static_cast<size_t>(count));
}

// ---- Peek / Poke (M29) ----

inline void bb_PokeByte(int handle, int offset, int val) {
  auto* b = bb_bank_get_(handle);
  if (!bb_bank_check_(b, offset, 1)) return;
  (*b)[static_cast<size_t>(offset)] = static_cast<uint8_t>(val & 0xFF);
}

inline void bb_PokeShort(int handle, int offset, int val) {
  auto* b = bb_bank_get_(handle);
  if (!bb_bank_check_(b, offset, 2)) return;
  uint16_t s = static_cast<uint16_t>(val & 0xFFFF);
  std::memcpy(b->data() + offset, &s, 2);
}

inline void bb_PokeInt(int handle, int offset, int val) {
  auto* b = bb_bank_get_(handle);
  if (!bb_bank_check_(b, offset, 4)) return;
  std::memcpy(b->data() + offset, &val, 4);
}

inline void bb_PokeFloat(int handle, int offset, float val) {
  auto* b = bb_bank_get_(handle);
  if (!bb_bank_check_(b, offset, 4)) return;
  std::memcpy(b->data() + offset, &val, 4);
}

inline int bb_PeekByte(int handle, int offset) {
  auto* b = bb_bank_get_(handle);
  if (!bb_bank_check_(b, offset, 1)) return 0;
  return static_cast<int>((*b)[static_cast<size_t>(offset)]);
}

inline int bb_PeekShort(int handle, int offset) {
  auto* b = bb_bank_get_(handle);
  if (!bb_bank_check_(b, offset, 2)) return 0;
  uint16_t s;
  std::memcpy(&s, b->data() + offset, 2);
  return static_cast<int>(s);
}

inline int bb_PeekInt(int handle, int offset) {
  auto* b = bb_bank_get_(handle);
  if (!bb_bank_check_(b, offset, 4)) return 0;
  int val;
  std::memcpy(&val, b->data() + offset, 4);
  return val;
}

inline float bb_PeekFloat(int handle, int offset) {
  auto* b = bb_bank_get_(handle);
  if (!bb_bank_check_(b, offset, 4)) return 0.0f;
  float val;
  std::memcpy(&val, b->data() + offset, 4);
  return val;
}

// ---- ReadBytes / WriteBytes (File ↔ Bank) ----
//
// Transfers `count` bytes between a file handle and a bank at `offset`.
// Replaces the stubs that were in bb_file.h.

inline void bb_WriteBytes(int fileHandle, int bankHandle, int offset, int count) {
  if (count <= 0) return;
  FILE* f = bb_file_get_(fileHandle);
  auto* b = bb_bank_get_(bankHandle);
  if (!f) { std::cerr << "[runtime] WriteBytes: invalid file handle\n"; return; }
  if (!bb_bank_check_(b, offset, count)) return;
  std::fwrite(b->data() + offset, 1, static_cast<size_t>(count), f);
}

inline void bb_ReadBytes(int fileHandle, int bankHandle, int offset, int count) {
  if (count <= 0) return;
  FILE* f = bb_file_get_(fileHandle);
  auto* b = bb_bank_get_(bankHandle);
  if (!f) { std::cerr << "[runtime] ReadBytes: invalid file handle\n"; return; }
  if (!bb_bank_check_(b, offset, count)) return;
  std::fread(b->data() + offset, 1, static_cast<size_t>(count), f);
}

#endif // BLITZNEXT_BB_BANK_H
