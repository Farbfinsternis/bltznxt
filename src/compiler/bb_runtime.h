#ifndef BLITZNEXT_BB_RUNTIME_H
#define BLITZNEXT_BB_RUNTIME_H

#include <cstdio>
#include <iostream>
#include <vector>
#include <array>     // feste Arrays: "Local a[3]" wird std::array (BUG-59)
#include <type_traits>  // bb_DataVal: ein Konversionsoperator statt vier
#ifdef _WIN32
#include <winsock2.h>  // vor jedem windows.h, sonst bricht bb_socket.h (TCP)
#endif
#include "bb_string.h"  // bbString typedef + string functions
#include "bb_math.h"    // math functions + Pi constant
#include "bb_system.h"  // MilliSecs, CurrentDate, CurrentTime, Delay
#include "bb_file.h"    // OpenFile, ReadFile, WriteFile, CloseFile, Seek, Eof
#include "bb_bank.h"    // CreateBank, FreeBank, BankSize, Peek/Poke, ReadBytes/WriteBytes
#include "bb_socket.h"  // TCP-Streams, DottedIP, HostIP
#include "bb_sdl.h"     // SDL3 init/quit, PollEvents, WaitKey, key state arrays
#include "bb_input.h"   // Keyboard + Mouse + Joystick input API
#include "bb_sound.h"   // Sound loading, playback, looping, channel control
#include "bb_sound3d.h"    // 3D/positional sound stubs, WaitSound, listener state
#include "bb_graphics2d.h" // Graphics(), GraphicsWidth/Height/Depth/Rate, VidMem stubs
#include "bb_gfxmode.h"   // CountGfxModes3D/GfxModeWidth/... - Modus- und Treiberabfrage
#include "bb_image.h"      // LoadImage, CreateImage, DrawImage, ImageWidth/Height
#include "bb_graphics3d.h" // Graphics3D(), RenderWorld(), UpdateWorld(), GL loader

// ---- Lifecycle ----

#ifdef _WIN32
// Hardware-Ausnahmen enden wie im Original mit einer Laufzeitmeldung statt
// mit einem stummen Absturz: seTranslator in bbruntime_dll/bbruntime_dll.cpp
// ordnet dieselben vier Codes zu, alles andere wird "Unknown runtime
// exception". Vorher verlor "Print 7 / z" mit z = 0 sogar die schon
// geschriebene, noch gepufferte Ausgabe (BUG-164); bb_RuntimeError endet
// ueber exit(), das die Puffer leert.
inline const char *bb_seh_message_(DWORD code) {
  switch (code) {
    case EXCEPTION_INT_DIVIDE_BY_ZERO:  return "Integer divide by zero";
    case EXCEPTION_ACCESS_VIOLATION:    return "Memory access violation";
    case EXCEPTION_ILLEGAL_INSTRUCTION: return "Illegal instruction";
    case EXCEPTION_STACK_OVERFLOW:      return "Stack overflow!";
  }
  return "Unknown runtime exception";
}
inline LONG WINAPI bb_seh_filter_(EXCEPTION_POINTERS *p) {
  bb_RuntimeError(bb_seh_message_(p->ExceptionRecord->ExceptionCode));
  return EXCEPTION_EXECUTE_HANDLER;
}
// Die beiden Ganzzahl-Ausnahmen muessen vorne abgefangen werden: die
// MinGW-Laufzeit behandelt sie sonst selbst - Division durch 0 als SIGFPE mit
// stummem Abbruch, "-2147483648 / -1" (EXCEPTION_INT_OVERFLOW) sogar mit
// endloser Wiederholung der Anweisung. Beide loest kein Code absichtlich
// aus; Zugriffsfehler dagegen fangen manche Grafiktreiber intern selbst ab,
// die bleiben beim Filter fuer unbehandelte Ausnahmen.
inline LONG WINAPI bb_seh_vectored_(EXCEPTION_POINTERS *p) {
  const DWORD code = p->ExceptionRecord->ExceptionCode;
  if (code == EXCEPTION_INT_DIVIDE_BY_ZERO || code == EXCEPTION_INT_OVERFLOW)
    bb_RuntimeError(bb_seh_message_(code));
  return EXCEPTION_CONTINUE_SEARCH;
}
#endif

inline void bbInit(int argc = 0, char** argv = nullptr) {
  bb_argc_ = argc;
  bb_argv_ = argv;
#ifdef _WIN32
  AddVectoredExceptionHandler(1, bb_seh_vectored_);
  SetUnhandledExceptionFilter(bb_seh_filter_);
#endif
  // SDL is initialised lazily by bb_sdl_ensure_() when a window is first needed.
#ifdef _WIN32
  // Set Windows timer resolution to 1ms so WaitTimer / sleep_until are accurate.
  // Default Windows resolution is ~15.6ms (64Hz), causing WaitTimer jitter.
  timeBeginPeriod(1);
#endif
}
inline void bbEnd() {
#ifdef _WIN32
  timeEndPeriod(1);
#endif
  bb_socket_quit_(); // close open TCP streams and servers
  bb_file_quit_();  // close open file + dir handles
  bb_bank_quit_();  // free remaining bank handles
  bb_snd_quit_();   // close audio device + free sounds/channels
  bb_sdl_quit_();   // close joysticks, window, renderer, SDL
}

// ---- Output ----

// Accepts any printable type (int, float, bbString, …)
template <typename T>
inline void bb_Print(const T &val) {
  std::cout << val << "\n";
}
// Print nimmt im Original einen String; eine Kommazahl geht also durch
// ftoa wie bei Str() - "Print 2.0" schreibt "2.0" (BUG-68).
inline void bb_Print(float val)  { std::cout << bb_Str((double)val) << "\n"; }
inline void bb_Print(double val) { std::cout << bb_Str(val) << "\n"; }

// `Print` ohne Argument gibt eine Leerzeile aus - im Original `Print
// [string$]`, eine der haeufigsten Formen ueberhaupt. Wir haben sie bis
// 2026-09-07 abgelehnt: 'Not enough parameters for Print' (BUG-44).
inline void bb_Print() { std::cout << "\n"; }

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
  explicit bb_DataVal(long v)    // prevents long-literal ambiguity (long→int vs long→float)
      : kind(KIND_INT), ival(static_cast<int>(v)), fval(static_cast<float>(v)) {}
  explicit bb_DataVal(float v)
      : kind(KIND_FLOAT), ival(static_cast<int>(v)), fval(v) {}
  explicit bb_DataVal(const bbString &v)
      : kind(KIND_STR), sval(v) {}

  bbString alsKette() const {
    switch (kind) {
      case KIND_INT:   return std::to_string(ival);
      case KIND_FLOAT: return bb_FloatToStr_(fval); // ftoa wie Str (BUG-68)
      default:         return sval;
    }
  }

  // **Ein** Konversionsoperator statt vier, und zwar nur fuer die drei
  // Zieltypen, die die Sprache kennt.
  //
  // Mit vier einzelnen Operatoren war `feld$ = wert` mehrdeutig: bbString ist
  // ein std::string, und dessen Zuweisung nimmt auch ein einzelnes `char` -
  // dorthin fuehrt der Weg ueber `operator int`. Solange jedes Read seinen
  // Wert selbst castete, fiel das nicht auf; seit ein Feld das Ziel sein darf
  // (BUG-85), steht die Zuweisung ohne Cast da und der Uebersetzer bricht ab.
  //
  // Die Umwandlung Zeichenkette -> Zahl geht ueber bb_ToInt/bb_ToFloat, also
  // ueber atoi/atof wie in der Referenz. Vorher stand hier std::stoi, das bei
  // "abc" eine **Ausnahme wirft**, wo das Original stillschweigend 0 liefert -
  // derselbe Unterschied, der bei Int() BUG-82 war, nur eine Ebene tiefer.
  template <class T, class = std::enable_if_t<
      std::is_same_v<T, int>   || std::is_same_v<T, float> ||
      std::is_same_v<T, double> || std::is_same_v<T, bbString>>>
  operator T() const {
    if constexpr (std::is_same_v<T, bbString>) {
      return alsKette();
    } else if constexpr (std::is_same_v<T, int>) {
      return kind == KIND_STR ? bb_ToInt(sval) : ival;
    } else {
      return static_cast<T>(kind == KIND_STR ? bb_ToFloat(sval) : fval);
    }
  }
};

inline std::vector<bb_DataVal> bb_data_pool_;
inline size_t                  bb_data_idx_ = 0;

inline bb_DataVal bb_DataRead() {
  // Das Original bricht hier ab (RTEX("Out of data") in _bbReadInt/Float/Str,
  // bbruntime/basic.cpp) statt mit einem erfundenen Wert weiterzulaufen.
  if (bb_data_idx_ >= bb_data_pool_.size()) {
    bb_RuntimeError("Out of data");
    return bb_DataVal(0);
  }
  return bb_data_pool_[bb_data_idx_++];
}

inline void bb_DataRestore(size_t idx = 0) {
  bb_data_idx_ = idx;
}

#endif // BLITZNEXT_BB_RUNTIME_H
