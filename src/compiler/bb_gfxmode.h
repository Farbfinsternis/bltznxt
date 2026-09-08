#ifndef BLITZNEXT_BB_GFXMODE_H
#define BLITZNEXT_BB_GFXMODE_H

#include "bb_sdl.h"     // bb_sdl_ensure_()
#include "bb_string.h"  // bbString
#include <vector>

// ============================================================
//  Grafikmodus- und Treiberaufzaehlung  —  bb_gfxmode.h
//
//  Blitz3D-Programme fragen vor "Graphics3D" ab, welche Modi es ueberhaupt
//  gibt. Das ist keine Rendering-Frage, sondern eine reine Abfrage-API, und
//  sie steht deshalb nicht auf dem 3D-Meilensteinplan - blockiert aber 30 der
//  Beispielprogramme, weil die gemeinsame "start.bb" der mak-Beispiele damit
//  beginnt.
//
//  **Alle Indizes sind 1-basiert.** Das ist nicht geraten, sondern aus dem
//  tatsaechlichen Gebrauch im Beispielbestand abgelesen:
//
//      For k=1 To CountGfxModes3D()
//        Print k+":"+GfxModeWidth(k)+","+GfxModeHeight(k)
//      Next
//      driver = Input$( "Display driver (1-"+CountGfxDrivers()+"):" )
//
//  Ein Index ausserhalb des Bereichs liefert 0 bzw. den leeren String, statt
//  zu stuerzen. Blitz3D meldet dort einen Laufzeitfehler; ein stiller
//  Nullwert ist die vorsichtigere Wahl, solange wir dessen genaue Form nicht
//  gemessen haben.
//
//  Die Signaturen entsprechen denen des Originals, abgelesen mit
//  "blitzcc +k":
//      Windowed3D ( )            CountGfxModes3D ( )
//      CountGfxModes ( )         GfxModeWidth ( mode )
//      GfxModeHeight ( mode )    GfxModeDepth ( mode )
//      GfxModeExists ( width,height,depth )
//      CountGfxDrivers ( )       GfxDriverName$ ( driver )
//      SetGfxDriver driver
// ============================================================

struct bb_GfxMode_ {
  int w = 0, h = 0, depth = 0;
};

inline std::vector<bb_GfxMode_> bb_gfx_modes_;
inline bool                     bb_gfx_modes_ready_ = false;
inline int                      bb_gfx_driver_      = 1; // 1-basiert

// Die Modi einmal einsammeln. SDL3 liefert die Vollbildmodi des primaeren
// Displays; Duplikate nach (Breite, Hoehe, Tiefe) fallen weg, weil Blitz3D
// keine Bildwiederholrate in dieser Liste fuehrt und ein Programm sonst
// dieselbe Aufloesung mehrfach angeboten bekaeme.
inline void bb_gfx_modes_ensure_() {
  if (bb_gfx_modes_ready_) return;
  bb_gfx_modes_ready_ = true;
  bb_sdl_ensure_();

  SDL_DisplayID disp = SDL_GetPrimaryDisplay();
  if (!disp) return;
  int count = 0;
  SDL_DisplayMode **modes = SDL_GetFullscreenDisplayModes(disp, &count);
  if (!modes) return;

  for (int i = 0; i < count; ++i) {
    if (!modes[i]) continue;
    int bpp = 32;
    if (const SDL_PixelFormatDetails *d =
            SDL_GetPixelFormatDetails(modes[i]->format))
      bpp = d->bits_per_pixel;
    bool dup = false;
    for (const auto &m : bb_gfx_modes_)
      if (m.w == modes[i]->w && m.h == modes[i]->h && m.depth == bpp) {
        dup = true;
        break;
      }
    if (!dup) bb_gfx_modes_.push_back({modes[i]->w, modes[i]->h, bpp});
  }
  SDL_free(modes);
}

// ---- Modusabfragen ----

inline int bb_CountGfxModes3D() {
  bb_gfx_modes_ensure_();
  return (int)bb_gfx_modes_.size();
}

// Blitz3D unterscheidet 2D- und 3D-faehige Modi. Auf heutiger Hardware ist
// jeder Modus 3D-faehig, deshalb dieselbe Liste - eine Abweichung, die im
// Zweifel mehr zulaesst statt weniger.
inline int bb_CountGfxModes() { return bb_CountGfxModes3D(); }

inline int bb_GfxModeWidth(int mode) {
  bb_gfx_modes_ensure_();
  if (mode < 1 || mode > (int)bb_gfx_modes_.size()) return 0;
  return bb_gfx_modes_[mode - 1].w;
}

inline int bb_GfxModeHeight(int mode) {
  bb_gfx_modes_ensure_();
  if (mode < 1 || mode > (int)bb_gfx_modes_.size()) return 0;
  return bb_gfx_modes_[mode - 1].h;
}

inline int bb_GfxModeDepth(int mode) {
  bb_gfx_modes_ensure_();
  if (mode < 1 || mode > (int)bb_gfx_modes_.size()) return 0;
  return bb_gfx_modes_[mode - 1].depth;
}

inline int bb_GfxModeExists(int width, int height, int depth) {
  bb_gfx_modes_ensure_();
  for (const auto &m : bb_gfx_modes_)
    if (m.w == width && m.h == height && m.depth == depth) return 1;
  return 0;
}

// Fenster-3D koennen wir immer: der GL-Kontext haengt nicht am Vollbild.
// Blitz3D musste das noch verneinen, wenn der Desktop nicht in 16 oder 32 Bit
// lief. Programme lesen das als "darf ich im Fenster starten" und waehlen
// sonst Vollbild - die Antwort 1 ist also die vertraeglichere.
inline int bb_Windowed3D() { return 1; }

// ---- Treiberabfragen ----

inline int bb_CountGfxDrivers() {
  bb_sdl_ensure_();
  int n = SDL_GetNumVideoDrivers();
  return n > 0 ? n : 1;
}

inline bbString bb_GfxDriverName(int driver) {
  bb_sdl_ensure_();
  int n = SDL_GetNumVideoDrivers();
  if (driver < 1 || driver > n) return bbString();
  const char *name = SDL_GetVideoDriver(driver - 1);
  return name ? bbString(name) : bbString();
}

// SDL3 waehlt den Videotreiber beim Initialisieren; danach laesst er sich
// nicht mehr wechseln. Der Wert wird deshalb nur gemerkt, damit ein Programm,
// das ihn setzt und wieder liest, sich nicht widerspricht. Ein echter Wechsel
// findet nicht statt - das ist eine bewusste Abweichung und keine Auslassung.
inline void bb_SetGfxDriver(int driver) {
  if (driver >= 1) bb_gfx_driver_ = driver;
}

#endif // BLITZNEXT_BB_GFXMODE_H
