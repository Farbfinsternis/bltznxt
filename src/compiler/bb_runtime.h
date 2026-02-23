#ifndef BLITZNEXT_BB_RUNTIME_H
#define BLITZNEXT_BB_RUNTIME_H

#include <cstdio>
#include <iostream>
#include <vector>
#include "bb_string.h"  // bbString typedef + string functions
#include "bb_math.h"    // math functions + Pi constant
#include "bb_system.h"  // MilliSecs, CurrentDate, CurrentTime, Delay
#include "bb_file.h"    // OpenFile, ReadFile, WriteFile, CloseFile, Seek, Eof
#include "bb_bank.h"    // CreateBank, FreeBank, BankSize, Peek/Poke, ReadBytes/WriteBytes
#include "bb_sdl.h"     // SDL3 init/quit, PollEvents, WaitKey, key state arrays
#include "bb_input.h"   // Keyboard + Mouse + Joystick input API
#include "bb_sound.h"   // Sound loading, playback, looping, channel control
#include "bb_sound3d.h"    // 3D/positional sound stubs, WaitSound, listener state
#include "bb_graphics2d.h" // Graphics(), GraphicsWidth/Height/Depth/Rate, VidMem stubs

// ---- Lifecycle ----

inline void bbInit(int argc = 0, char** argv = nullptr) {
  bb_argc_ = argc;
  bb_argv_ = argv;
  // SDL is initialised lazily by bb_sdl_ensure_() when a window is first needed.
}
inline void bbEnd() {
  bb_snd_quit_();   // close audio device + free sounds/channels
  bb_sdl_quit_();   // close joysticks, window, renderer, SDL
}

// ---- Output ----

// Accepts any printable type (int, float, bbString, …)
template <typename T>
inline void bb_Print(const T &val) {
  std::cout << val << "\n";
}

// ---- Input ----
// bb_WaitKey() is defined in bb_sdl.h

inline bbString bb_Input(const bbString &prompt = "") {
  if (!prompt.empty()) std::cout << prompt;
  bbString line;
  std::getline(std::cin, line);
  return line;
}

// ---- Data / Read / Restore ----
//
// bb_DataVal is a tagged value that auto-converts to int, float, or bbString.
// The emitter fills bb_data_pool_ at the top of main() then uses
// bb_DataRead() for each Read statement.

struct bb_DataVal {
  enum Kind { KIND_INT, KIND_FLOAT, KIND_STR } kind;
  int     ival = 0;
  float   fval = 0.0f;
  bbString sval;

  explicit bb_DataVal(int v)
      : kind(KIND_INT), ival(v), fval(static_cast<float>(v)) {}
  explicit bb_DataVal(float v)
      : kind(KIND_FLOAT), ival(static_cast<int>(v)), fval(v) {}
  explicit bb_DataVal(const bbString &v)
      : kind(KIND_STR), sval(v) {}

  operator int()     const {
    return kind == KIND_STR ? std::stoi(sval) : ival;
  }
  operator float()   const {
    return kind == KIND_STR ? std::stof(sval) : fval;
  }
  operator double()  const { return static_cast<double>(operator float()); }
  operator bbString() const {
    switch (kind) {
      case KIND_INT:   return std::to_string(ival);
      case KIND_FLOAT: {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%g", fval);
        return buf;
      }
      default:         return sval;
    }
  }
};

inline std::vector<bb_DataVal> bb_data_pool_;
inline size_t                  bb_data_idx_ = 0;

inline bb_DataVal bb_DataRead() {
  if (bb_data_idx_ >= bb_data_pool_.size()) {
    std::cerr << "[runtime] Read: past end of Data\n";
    return bb_DataVal(0);
  }
  return bb_data_pool_[bb_data_idx_++];
}

inline void bb_DataRestore(size_t idx = 0) {
  bb_data_idx_ = idx;
}

#endif // BLITZNEXT_BB_RUNTIME_H
