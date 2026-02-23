#ifndef BLITZNEXT_BB_GRAPHICS2D_H
#define BLITZNEXT_BB_GRAPHICS2D_H

#include <iostream>
#include "bb_sdl.h"     // bb_window_, bb_renderer_, bb_sdl_ensure_(), bb_sdl_initialized_
#include "bb_system.h"  // bb_app_title_
#include "bb_string.h"  // bbString

// ---- AppTitle integration ----
//
// Called by bb_AppTitle() (bb_system.h) whenever the program sets a new title.
// Updates the live window title when a window is already open.

inline void bb_update_window_title_(const char* title) {
  if (bb_window_) SDL_SetWindowTitle(bb_window_, title);
}

// Register at startup — inline bool trick (same pattern as bb_audio_update_hook_).
inline const bool bb_title_hook_reg_ =
    (bb_title_update_hook_ = bb_update_window_title_, true);

// ---- Stored display parameters ----
//
// Populated by bb_Graphics().  Returned by the query functions below even when
// the window could not be created (e.g. headless machine / no display), so
// programs that call GraphicsWidth() after Graphics() always get a sane value.

inline int bb_gfx_width_  = 0;
inline int bb_gfx_height_ = 0;
inline int bb_gfx_depth_  = 32;
inline int bb_gfx_rate_   = 0;

// ---- Graphics() ----
//
// Opens an SDL3 window and hardware renderer.
//
//  width, height : desired framebuffer size in pixels
//  depth         : colour depth in bits (stored for API parity; SDL3 is always 32-bit)
//  mode          : 0 = windowed
//                  1 = fullscreen (display-mode change)
//                  2 = fullscreen desktop (current desktop resolution)
//                  6 = fullscreen + vsync hint
//                  other values treated as windowed
//
// If SDL cannot be initialized (headless), the parameters are stored and the
// function returns quietly — all query functions still return the stored values.

inline void bb_Graphics(int width, int height, int depth = 32, int mode = 0) {
  // Store requested parameters unconditionally so query functions always work.
  bb_gfx_width_  = width;
  bb_gfx_height_ = height;
  bb_gfx_depth_  = depth;
  bb_gfx_rate_   = 0;

  bb_sdl_ensure_();
  if (!bb_sdl_initialized_) return;

  // Tear down any existing window/renderer before creating a new one.
  if (bb_renderer_) { SDL_DestroyRenderer(bb_renderer_); bb_renderer_ = nullptr; }
  if (bb_window_)   { SDL_DestroyWindow(bb_window_);     bb_window_   = nullptr; }

  SDL_WindowFlags flags = 0;

  if (mode == 1 || mode == 6) {
    flags |= SDL_WINDOW_FULLSCREEN;
  } else if (mode == 2) {
    // Fullscreen desktop: match the current desktop resolution.
    SDL_DisplayID prim = SDL_GetPrimaryDisplay();
    const SDL_DisplayMode* dm = SDL_GetCurrentDisplayMode(prim);
    if (dm) {
      width          = dm->w;
      height         = dm->h;
      bb_gfx_width_  = width;
      bb_gfx_height_ = height;
    }
    flags |= SDL_WINDOW_FULLSCREEN;
  }

  const char* title = bb_app_title_.empty() ? "BlitzNext" : bb_app_title_.c_str();

  bb_window_ = SDL_CreateWindow(title, width, height, flags);
  if (!bb_window_) {
    std::cerr << "[runtime] Graphics: SDL_CreateWindow failed: " << SDL_GetError() << "\n";
    return;
  }

  bb_renderer_ = SDL_CreateRenderer(bb_window_, nullptr);
  if (!bb_renderer_) {
    std::cerr << "[runtime] Graphics: SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
    SDL_DestroyWindow(bb_window_);
    bb_window_ = nullptr;
    return;
  }

  // Query the refresh rate from the display the window landed on.
  SDL_DisplayID disp = SDL_GetDisplayForWindow(bb_window_);
  const SDL_DisplayMode* dm = SDL_GetCurrentDisplayMode(disp);
  if (dm && dm->refresh_rate > 0.0f)
    bb_gfx_rate_ = static_cast<int>(dm->refresh_rate);

  // Enable vsync when mode == 6 (or as a default — smooth rendering).
  if (mode == 6)
    SDL_SetRenderVSync(bb_renderer_, 1);
}

// ---- EndGraphics() ----
//
// Destroys the window and renderer.  bbEnd() also tears down SDL, so an
// explicit EndGraphics call is only needed when the program switches back to
// console mode during a session.

inline void bb_EndGraphics() {
  if (bb_renderer_) { SDL_DestroyRenderer(bb_renderer_); bb_renderer_ = nullptr; }
  if (bb_window_)   { SDL_DestroyWindow(bb_window_);     bb_window_   = nullptr; }
  bb_gfx_width_ = bb_gfx_height_ = bb_gfx_depth_ = bb_gfx_rate_ = 0;
}

// ---- Query functions ----

inline int bb_GraphicsWidth()  { return bb_gfx_width_;  }
inline int bb_GraphicsHeight() { return bb_gfx_height_; }
inline int bb_GraphicsDepth()  { return bb_gfx_depth_;  }
inline int bb_GraphicsRate()   { return bb_gfx_rate_;   }

// ---- Video memory stubs ----
//
// SDL3 has no VRAM query API.  Return 512 MB — large enough that real Blitz3D
// programs that check AvailVidMem() before loading assets will always proceed.

inline int bb_TotalVidMem() { return 512 * 1024 * 1024; }
inline int bb_AvailVidMem() { return 512 * 1024 * 1024; }

// ---- GraphicsMode() ----
//
// Re-creates the window at a different resolution.  The refresh-rate parameter
// is accepted for API parity but ignored (SDL3 does not expose mode selection).

inline void bb_GraphicsMode(int w, int h, int depth, int rate) {
  (void)rate;
  bb_Graphics(w, h, depth, 0);
}

// ==========================================================================
// MILESTONE 39 — Buffer & Flip
// ==========================================================================

// ---- Buffer handle tokens ----
//
// BackBuffer() and FrontBuffer() return integer identifiers that programs
// store and pass to SetBuffer() / CopyRect().  SDL3 always renders into
// the same back-buffer surface internally; FrontBuffer is an alias here.

inline constexpr int BB_BACK_BUFFER_H  = 1;
inline constexpr int BB_FRONT_BUFFER_H = 2;

inline int bb_active_buffer_ = BB_BACK_BUFFER_H;  // default render target

// Clear colour used by Cls() — defaults to black (0,0,0).
// bb_ClsColor() (M40) updates these three bytes.

inline Uint8 bb_cls_r_ = 0, bb_cls_g_ = 0, bb_cls_b_ = 0;

// ---- BackBuffer / FrontBuffer / SetBuffer ----

inline int bb_BackBuffer()  { return BB_BACK_BUFFER_H;  }
inline int bb_FrontBuffer() { return BB_FRONT_BUFFER_H; }

// SetBuffer(buf) — switch the active render target.
// SDL3 has no separate front-buffer drawing surface; this is bookkeeping only.
// Render-to-texture targets (ImageBuffer) are handled in M46.
inline void bb_SetBuffer(int buf) {
  bb_active_buffer_ = buf;
}

// ---- Cls() ----
//
// Clears the current render target to bb_cls_r_/g_/b_ (default black).
// Safe no-op when no renderer is available (headless / EndGraphics called).

inline void bb_Cls() {
  if (!bb_renderer_) return;
  SDL_SetRenderDrawColor(bb_renderer_, bb_cls_r_, bb_cls_g_, bb_cls_b_, 255);
  SDL_RenderClear(bb_renderer_);
}

// ---- Flip() ----
//
// Presents the back buffer to the screen.
//  vblank : 1 = wait for vertical blank (default); 0 = no wait.
// SDL3 vsync is configured at renderer creation (mode 6); the vblank
// parameter is accepted for API parity but does not change SDL3 behaviour.
// Events are pumped after every present so the game loop stays responsive
// even when the program does not call PollEvent() explicitly.

inline void bb_Flip(int vblank = 1) {
  (void)vblank;
  if (!bb_renderer_) return;
  SDL_RenderPresent(bb_renderer_);
  bb_PollEvents();
}

// ---- CopyRect() ----
//
// Copies a pixel rectangle from srcbuf to dstbuf.
// Full implementation requires render-to-texture (M46).
// For now: silent stub so existing programs compile without errors.
//
//  sx, sy       : source top-left
//  sw, sh       : source width and height (0 = whole buffer)
//  dx, dy       : destination top-left
//  srcbuf       : source buffer handle  (default: BackBuffer)
//  dstbuf       : destination buffer    (default: BackBuffer)

inline void bb_CopyRect(int sx, int sy, int sw, int sh,
                        int dx, int dy,
                        int srcbuf = BB_BACK_BUFFER_H,
                        int dstbuf = BB_BACK_BUFFER_H) {
  (void)sx; (void)sy; (void)sw; (void)sh;
  (void)dx; (void)dy; (void)srcbuf; (void)dstbuf;
  // Stub — requires render-to-texture (M46).
}

// ==========================================================================
// MILESTONE 40 — Color & Pixel Primitives
// ==========================================================================

// ---- Current draw colour ----
//
// Set by Color(r,g,b); read back by ColorRed/Green/Blue().
// GetColor(x,y) samples a pixel and overwrites these bytes.
// All drawing functions (Plot, Line, Rect, …) use this colour.
//
// Default: white (255,255,255) — matches Blitz3D's startup state.

inline Uint8 bb_draw_r_ = 255, bb_draw_g_ = 255, bb_draw_b_ = 255;

// ---- Color(r, g, b) ----
//
// Sets the current draw colour.  Values are clamped to 0–255.
// Does NOT repaint anything already on screen.

inline void bb_Color(int r, int g, int b) {
  bb_draw_r_ = (Uint8)(r < 0 ? 0 : r > 255 ? 255 : r);
  bb_draw_g_ = (Uint8)(g < 0 ? 0 : g > 255 ? 255 : g);
  bb_draw_b_ = (Uint8)(b < 0 ? 0 : b > 255 ? 255 : b);
}

// ---- ClsColor(r, g, b) ----
//
// Sets the background clear colour used by Cls().
// Stored in bb_cls_r_/g_/b_ (declared in M39 section above).

inline void bb_ClsColor(int r, int g, int b) {
  bb_cls_r_ = (Uint8)(r < 0 ? 0 : r > 255 ? 255 : r);
  bb_cls_g_ = (Uint8)(g < 0 ? 0 : g > 255 ? 255 : g);
  bb_cls_b_ = (Uint8)(b < 0 ? 0 : b > 255 ? 255 : b);
}

// ---- ColorRed() / ColorGreen() / ColorBlue() ----
//
// Return the components of the current draw colour.

inline int bb_ColorRed()   { return (int)bb_draw_r_; }
inline int bb_ColorGreen() { return (int)bb_draw_g_; }
inline int bb_ColorBlue()  { return (int)bb_draw_b_; }

// ---- GetColor(x, y) ----
//
// Reads the pixel at (x, y) from the current render target and stores the
// result into the draw-colour state so ColorRed/Green/Blue() can read it.
//
// Implementation: SDL_RenderReadPixels() returns a 1×1 SDL_Surface* from
// which SDL_ReadSurfacePixel() extracts the RGBA bytes.  Both functions are
// SDL3 additions — no SDL2 equivalent.
//
// Returns 1 on success, 0 on failure (headless / out-of-bounds).
// Blitz3D treats GetColor as a statement; the return value is ignored.

inline int bb_GetColor(int x, int y) {
  if (!bb_renderer_) return 0;
  SDL_Rect rect = {x, y, 1, 1};
  SDL_Surface* surf = SDL_RenderReadPixels(bb_renderer_, &rect);
  if (!surf) return 0;
  Uint8 r = 0, g = 0, b = 0, a = 255;
  SDL_ReadSurfacePixel(surf, 0, 0, &r, &g, &b, &a);
  SDL_DestroySurface(surf);
  bb_draw_r_ = r;
  bb_draw_g_ = g;
  bb_draw_b_ = b;
  return 1;
}

// ---- Plot(x, y) ----
//
// Draws a single pixel at (x, y) in the current draw colour.
// Safe no-op when no renderer is available.

inline void bb_Plot(int x, int y) {
  if (!bb_renderer_) return;
  SDL_SetRenderDrawColor(bb_renderer_, bb_draw_r_, bb_draw_g_, bb_draw_b_, 255);
  SDL_RenderPoint(bb_renderer_, (float)x, (float)y);
}

#endif // BLITZNEXT_BB_GRAPHICS2D_H
