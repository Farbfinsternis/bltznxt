#ifndef BLITZNEXT_BB_GFXMODE_H
#define BLITZNEXT_BB_GFXMODE_H

#include "bb_sdl.h"     // bb_sdl_ensure_()
#include "bb_string.h"  // bbString
#include <algorithm>    // std::sort
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
// Displays; Duplikate nach (Breite, Hoehe) fallen weg, weil Blitz3D keine
// Bildwiederholrate in dieser Liste fuehrt.
//
// Aufbau wie im Original (gemessen an Blitz3D 11.8, build/bug213_20261025):
// jede Aufloesung zweimal, erst die ganze Liste mit 32 Bit, dann dieselbe mit
// 16 Bit; vorn der Desktopmodus, danach aufsteigend. D3D7 zaehlt 16 Bit auf,
// auch wenn heutige Treiber es nur noch nachbilden - und Programme von 2001
// pruefen mit GfxModeExists(640,480,16), bevor sie starten (BUG-213,
// BBLauncher, SkyPlateau). SDL meldet nur noch 32 Bit.
inline void bb_gfx_modes_ensure_() {
  if (bb_gfx_modes_ready_) return;
  bb_gfx_modes_ready_ = true;
  bb_sdl_ensure_();

  SDL_DisplayID disp = SDL_GetPrimaryDisplay();
  if (!disp) return;
  int count = 0;
  SDL_DisplayMode **modes = SDL_GetFullscreenDisplayModes(disp, &count);
  if (!modes) return;

  std::vector<bb_GfxMode_> res;
  for (int i = 0; i < count; ++i) {
    if (!modes[i]) continue;
    bool dup = false;
    for (const auto &m : res)
      if (m.w == modes[i]->w && m.h == modes[i]->h) { dup = true; break; }
    if (!dup) res.push_back({modes[i]->w, modes[i]->h, 32});
  }
  SDL_free(modes);

  const SDL_DisplayMode *desk = SDL_GetDesktopDisplayMode(disp);
  std::sort(res.begin(), res.end(), [desk](const bb_GfxMode_ &a, const bb_GfxMode_ &b) {
    const bool da = desk && a.w == desk->w && a.h == desk->h;
    const bool db = desk && b.w == desk->w && b.h == desk->h;
    if (da != db) return da;
    return a.w != b.w ? a.w < b.w : a.h < b.h;
  });
  for (int depth : {32, 16})
    for (const auto &m : res) bb_gfx_modes_.push_back({m.w, m.h, depth});
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

// 3D-Faehigkeit je Modus und Treiber (BUG-195): wie oben ist heute jeder
// Modus und jeder Treiber 3D-faehig. GfxMode3DExists ist darum dasselbe wie
// GfxModeExists, GfxMode3D und GfxDriver3D melden 1 fuer jeden gueltigen
// Index (das Original bricht bei einem ungueltigen im Debug-Modus ab, hier
// wie bei GfxModeWidth 0).
inline int bb_GfxMode3DExists(int width, int height, int depth) {
  return bb_GfxModeExists(width, height, depth);
}
inline int bb_GfxMode3D(int mode) {
  bb_gfx_modes_ensure_();
  return (mode >= 1 && mode <= (int)bb_gfx_modes_.size()) ? 1 : 0;
}

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

// GfxDriver3D: jeder Treiber kann 3D (siehe oben, BUG-195).
inline int bb_GfxDriver3D(int driver) {
  return (driver >= 1 && driver <= bb_CountGfxDrivers()) ? 1 : 0;
}

// GfxDriverCaps3D ist im Original die Stufe aus gxScene: 100, mit
// Wuerfeltexturen 110. Wuerfelkarten kennt jeder GL-3.3-Treiber.
inline int bb_GfxDriverCaps3D() { return 110; }

// SDL3 waehlt den Videotreiber beim Initialisieren; danach laesst er sich
// nicht mehr wechseln. Der Wert wird deshalb nur gemerkt, damit ein Programm,
// das ihn setzt und wieder liest, sich nicht widerspricht. Ein echter Wechsel
// findet nicht statt - das ist eine bewusste Abweichung und keine Auslassung.
inline void bb_SetGfxDriver(int driver) {
  if (driver >= 1) bb_gfx_driver_ = driver;
}

#endif // BLITZNEXT_BB_GFXMODE_H
