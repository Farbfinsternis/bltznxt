#ifndef BLITZNEXT_BB_GRAPHICS2D_H
#define BLITZNEXT_BB_GRAPHICS2D_H

#include <iostream>
#include <cmath>
#include <vector>
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

  // Blitz3D Graphics mode parameter:
  //   0 = auto (windowed in debug, fullscreen in release — we always use windowed)
  //   1 = fullscreen
  //   2 = windowed
  //   3 = scaled window (windowed, SDL handles scaling)
  //   6 = fullscreen with vsync (BlitzNext extension)
  if (mode == 1) {
    flags |= SDL_WINDOW_FULLSCREEN;
  } else if (mode == 6) {
    flags |= SDL_WINDOW_FULLSCREEN;
  }
  // mode 0, 2, 3: windowed — no extra flags

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
inline int bb_vsync_mode_    = -1;                // -1 = not set yet

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
//  vblank : 1 = sync to vertical blank (default, smooth); 0 = no wait.
// SDL_SetRenderVSync is called only when the mode changes, so repeated
// Flip() calls with the same argument have no per-frame overhead.
// Events are pumped after every present so the game loop stays responsive
// even when the program does not call PollEvent() explicitly.

inline void bb_Flip(int vblank = 1) {
  if (!bb_renderer_) return;
  int want = vblank ? 1 : 0;
  if (want != bb_vsync_mode_) {
    SDL_SetRenderVSync(bb_renderer_, want);
    bb_vsync_mode_ = want;
  }
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

// ---- Rgb(r, g, b) → packed colour integer ----
//
// Returns the colour packed as (r << 16) | (g << 8) | b.
// Used with WritePixel / WritePixelFast and anywhere a packed colour is needed.
// Alpha bits are 0; WritePixel treats alpha=0 as fully opaque.

inline int bb_Rgb(int r, int g, int b) {
  return ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);
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

// ==========================================================================
// MILESTONE 41 — Line & Shape Primitives
// ==========================================================================

// ---- Line(x1, y1, x2, y2) ----
//
// Draws a straight line from (x1,y1) to (x2,y2) in the current draw colour.

inline void bb_Line(int x1, int y1, int x2, int y2) {
  if (!bb_renderer_) return;
  SDL_SetRenderDrawColor(bb_renderer_, bb_draw_r_, bb_draw_g_, bb_draw_b_, 255);
  SDL_RenderLine(bb_renderer_, (float)x1, (float)y1, (float)x2, (float)y2);
}

// ---- Rect(x, y, w, h [, solid]) ----
//
// Draws a rectangle with top-left at (x,y), dimensions w×h.
//   solid = 1 (default) : filled rectangle
//   solid = 0           : outline only

inline void bb_Rect(int x, int y, int w, int h, int solid = 1) {
  if (!bb_renderer_) return;
  SDL_SetRenderDrawColor(bb_renderer_, bb_draw_r_, bb_draw_g_, bb_draw_b_, 255);
  SDL_FRect r = { (float)x, (float)y, (float)w, (float)h };
  if (solid)
    SDL_RenderFillRect(bb_renderer_, &r);
  else
    SDL_RenderRect(bb_renderer_, &r);
}

// ---- Oval(x, y, w, h [, solid]) ----
//
// Draws an ellipse whose bounding box has top-left (x,y) and size w×h.
//   solid = 1 (default) : filled with horizontal scanlines
//   solid = 0           : outline via parametric point loop
//
// SDL3 has no native ellipse primitive; both paths are software-computed.

inline void bb_Oval(int x, int y, int w, int h, int solid = 1) {
  if (!bb_renderer_) return;
  if (w <= 0 || h <= 0) return;
  SDL_SetRenderDrawColor(bb_renderer_, bb_draw_r_, bb_draw_g_, bb_draw_b_, 255);

  const float cx = x + w * 0.5f;
  const float cy = y + h * 0.5f;
  const float rx = w * 0.5f;
  const float ry = h * 0.5f;

  if (solid) {
    // Fill: one horizontal scanline per pixel row inside the bounding box.
    for (int sy = y; sy <= y + h; ++sy) {
      const float dy = sy - cy;
      if (std::fabs(dy) > ry) continue;
      const float t  = dy / ry;
      const float dx = rx * std::sqrt(1.0f - t * t);
      SDL_RenderLine(bb_renderer_, cx - dx, (float)sy, cx + dx, (float)sy);
    }
  } else {
    // Outline: parametric ellipse, enough steps for a smooth curve.
    const int steps = static_cast<int>(2.0f * 3.14159265f * std::fmax(rx, ry)) + 4;
    const int N     = steps < 16 ? 16 : steps;
    std::vector<SDL_FPoint> pts(N + 1);
    for (int i = 0; i <= N; ++i) {
      const float angle = 2.0f * 3.14159265f * i / N;
      pts[i] = { cx + rx * std::cos(angle), cy + ry * std::sin(angle) };
    }
    SDL_RenderLines(bb_renderer_, pts.data(), (int)pts.size());
  }
}

// ---- Poly(x0, y0, x1, y1, x2, y2) ----
//
// Draws a filled triangle.  Blitz3D Poly has no solid parameter — it always
// fills.  Uses SDL_RenderGeometry so the triangle is hardware-accelerated.

inline void bb_Poly(int x0, int y0, int x1, int y1, int x2, int y2) {
  if (!bb_renderer_) return;
  const float r = bb_draw_r_ / 255.0f;
  const float g = bb_draw_g_ / 255.0f;
  const float b = bb_draw_b_ / 255.0f;
  SDL_Vertex verts[3] = {
    { {(float)x0, (float)y0}, {r, g, b, 1.0f}, {0.0f, 0.0f} },
    { {(float)x1, (float)y1}, {r, g, b, 1.0f}, {0.0f, 0.0f} },
    { {(float)x2, (float)y2}, {r, g, b, 1.0f}, {0.0f, 0.0f} },
  };
  SDL_RenderGeometry(bb_renderer_, nullptr, verts, 3, nullptr, 0);
}

// ==========================================================================
// MILESTONE 42 — Text & Console Output
// ==========================================================================

// ---- Built-in 8×8 bitmap font (ASCII 0–127) ----
//
// Each entry is 8 bytes: one byte per pixel row, MSB = leftmost pixel.
// Characters 0x00–0x1F are all-zero (control / unused).
// Data is the classic VGA/CP437 8×8 bitmap font (public domain).

#include <cstdint>

static constexpr uint8_t bb_font8x8_[128][8] = {
    // 0x00–0x1F: control characters (blank glyphs)
    {0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},
    // 0x20 SPACE
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    // 0x21 !
    {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00},
    // 0x22 "
    {0x36,0x36,0x00,0x00,0x00,0x00,0x00,0x00},
    // 0x23 #
    {0x36,0x36,0x7F,0x36,0x7F,0x36,0x36,0x00},
    // 0x24 $
    {0x0C,0x3E,0x03,0x1E,0x30,0x1F,0x0C,0x00},
    // 0x25 %
    {0x00,0x63,0x33,0x18,0x0C,0x66,0x63,0x00},
    // 0x26 &
    {0x1C,0x36,0x1C,0x6E,0x3B,0x33,0x6E,0x00},
    // 0x27 '
    {0x06,0x06,0x03,0x00,0x00,0x00,0x00,0x00},
    // 0x28 (
    {0x18,0x0C,0x06,0x06,0x06,0x0C,0x18,0x00},
    // 0x29 )
    {0x06,0x0C,0x18,0x18,0x18,0x0C,0x06,0x00},
    // 0x2A *
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00},
    // 0x2B +
    {0x00,0x0C,0x0C,0x3F,0x0C,0x0C,0x00,0x00},
    // 0x2C ,
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x06},
    // 0x2D -
    {0x00,0x00,0x00,0x3F,0x00,0x00,0x00,0x00},
    // 0x2E .
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x00},
    // 0x2F /
    {0x60,0x30,0x18,0x0C,0x06,0x03,0x01,0x00},
    // 0x30 0
    {0x3E,0x63,0x73,0x7B,0x6F,0x67,0x3E,0x00},
    // 0x31 1
    {0x0C,0x0E,0x0C,0x0C,0x0C,0x0C,0x3F,0x00},
    // 0x32 2
    {0x1E,0x33,0x30,0x1C,0x06,0x33,0x3F,0x00},
    // 0x33 3
    {0x1E,0x33,0x30,0x1C,0x30,0x33,0x1E,0x00},
    // 0x34 4
    {0x38,0x3C,0x36,0x33,0x7F,0x30,0x78,0x00},
    // 0x35 5
    {0x3F,0x03,0x1F,0x30,0x30,0x33,0x1E,0x00},
    // 0x36 6
    {0x1C,0x06,0x03,0x1F,0x33,0x33,0x1E,0x00},
    // 0x37 7
    {0x3F,0x33,0x30,0x18,0x0C,0x0C,0x0C,0x00},
    // 0x38 8
    {0x1E,0x33,0x33,0x1E,0x33,0x33,0x1E,0x00},
    // 0x39 9
    {0x1E,0x33,0x33,0x3E,0x30,0x18,0x0E,0x00},
    // 0x3A :
    {0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x00},
    // 0x3B ;
    {0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x06},
    // 0x3C <
    {0x18,0x0C,0x06,0x03,0x06,0x0C,0x18,0x00},
    // 0x3D =
    {0x00,0x00,0x3F,0x00,0x00,0x3F,0x00,0x00},
    // 0x3E >
    {0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00},
    // 0x3F ?
    {0x1E,0x33,0x30,0x18,0x0C,0x00,0x0C,0x00},
    // 0x40 @
    {0x3E,0x63,0x7B,0x7B,0x7B,0x03,0x1E,0x00},
    // 0x41 A
    {0x0C,0x1E,0x33,0x33,0x3F,0x33,0x33,0x00},
    // 0x42 B
    {0x3F,0x66,0x66,0x3E,0x66,0x66,0x3F,0x00},
    // 0x43 C
    {0x3C,0x66,0x03,0x03,0x03,0x66,0x3C,0x00},
    // 0x44 D
    {0x1F,0x36,0x66,0x66,0x66,0x36,0x1F,0x00},
    // 0x45 E
    {0x7F,0x46,0x16,0x1E,0x16,0x46,0x7F,0x00},
    // 0x46 F
    {0x7F,0x46,0x16,0x1E,0x16,0x06,0x0F,0x00},
    // 0x47 G
    {0x3C,0x66,0x03,0x03,0x73,0x66,0x7C,0x00},
    // 0x48 H
    {0x33,0x33,0x33,0x3F,0x33,0x33,0x33,0x00},
    // 0x49 I
    {0x1E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00},
    // 0x4A J
    {0x78,0x30,0x30,0x30,0x33,0x33,0x1E,0x00},
    // 0x4B K
    {0x67,0x66,0x36,0x1E,0x36,0x66,0x67,0x00},
    // 0x4C L
    {0x0F,0x06,0x06,0x06,0x46,0x66,0x7F,0x00},
    // 0x4D M
    {0x63,0x77,0x7F,0x7F,0x6B,0x63,0x63,0x00},
    // 0x4E N
    {0x63,0x67,0x6F,0x7B,0x73,0x63,0x63,0x00},
    // 0x4F O
    {0x1C,0x36,0x63,0x63,0x63,0x36,0x1C,0x00},
    // 0x50 P
    {0x3F,0x66,0x66,0x3E,0x06,0x06,0x0F,0x00},
    // 0x51 Q
    {0x1E,0x33,0x33,0x33,0x3B,0x1E,0x38,0x00},
    // 0x52 R
    {0x3F,0x66,0x66,0x3E,0x36,0x66,0x67,0x00},
    // 0x53 S
    {0x1E,0x33,0x07,0x0E,0x38,0x33,0x1E,0x00},
    // 0x54 T
    {0x3F,0x2D,0x0C,0x0C,0x0C,0x0C,0x1E,0x00},
    // 0x55 U
    {0x33,0x33,0x33,0x33,0x33,0x33,0x3F,0x00},
    // 0x56 V
    {0x33,0x33,0x33,0x33,0x33,0x1E,0x0C,0x00},
    // 0x57 W
    {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00},
    // 0x58 X
    {0x63,0x63,0x36,0x1C,0x1C,0x36,0x63,0x00},
    // 0x59 Y
    {0x33,0x33,0x33,0x1E,0x0C,0x0C,0x1E,0x00},
    // 0x5A Z
    {0x7F,0x63,0x31,0x18,0x4C,0x66,0x7F,0x00},
    // 0x5B [
    {0x1E,0x06,0x06,0x06,0x06,0x06,0x1E,0x00},
    // 0x5C backslash
    {0x03,0x06,0x0C,0x18,0x30,0x60,0x40,0x00},
    // 0x5D ]
    {0x1E,0x18,0x18,0x18,0x18,0x18,0x1E,0x00},
    // 0x5E ^
    {0x08,0x1C,0x36,0x63,0x00,0x00,0x00,0x00},
    // 0x5F _
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF},
    // 0x60 `
    {0x0C,0x0C,0x18,0x00,0x00,0x00,0x00,0x00},
    // 0x61 a
    {0x00,0x00,0x1E,0x30,0x3E,0x33,0x6E,0x00},
    // 0x62 b
    {0x07,0x06,0x06,0x3E,0x66,0x66,0x3B,0x00},
    // 0x63 c
    {0x00,0x00,0x1E,0x33,0x03,0x33,0x1E,0x00},
    // 0x64 d
    {0x38,0x30,0x30,0x3E,0x33,0x33,0x6E,0x00},
    // 0x65 e
    {0x00,0x00,0x1E,0x33,0x3F,0x03,0x1E,0x00},
    // 0x66 f
    {0x1C,0x36,0x06,0x0F,0x06,0x06,0x0F,0x00},
    // 0x67 g
    {0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x1F},
    // 0x68 h
    {0x07,0x06,0x36,0x6E,0x66,0x66,0x67,0x00},
    // 0x69 i
    {0x0C,0x00,0x0E,0x0C,0x0C,0x0C,0x1E,0x00},
    // 0x6A j
    {0x30,0x00,0x30,0x30,0x30,0x33,0x33,0x1E},
    // 0x6B k
    {0x07,0x06,0x66,0x36,0x1E,0x36,0x67,0x00},
    // 0x6C l
    {0x0E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00},
    // 0x6D m
    {0x00,0x00,0x33,0x7F,0x7F,0x6B,0x63,0x00},
    // 0x6E n
    {0x00,0x00,0x1F,0x33,0x33,0x33,0x33,0x00},
    // 0x6F o
    {0x00,0x00,0x1E,0x33,0x33,0x33,0x1E,0x00},
    // 0x70 p
    {0x00,0x00,0x3B,0x66,0x66,0x3E,0x06,0x0F},
    // 0x71 q
    {0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x78},
    // 0x72 r
    {0x00,0x00,0x3B,0x6E,0x66,0x06,0x0F,0x00},
    // 0x73 s
    {0x00,0x00,0x1E,0x03,0x1E,0x30,0x1F,0x00},
    // 0x74 t
    {0x08,0x0C,0x3E,0x0C,0x0C,0x2C,0x18,0x00},
    // 0x75 u
    {0x00,0x00,0x33,0x33,0x33,0x33,0x6E,0x00},
    // 0x76 v
    {0x00,0x00,0x33,0x33,0x33,0x1E,0x0C,0x00},
    // 0x77 w
    {0x00,0x00,0x63,0x6B,0x7F,0x7F,0x36,0x00},
    // 0x78 x
    {0x00,0x00,0x63,0x36,0x1C,0x36,0x63,0x00},
    // 0x79 y
    {0x00,0x00,0x33,0x33,0x33,0x3E,0x30,0x1F},
    // 0x7A z
    {0x00,0x00,0x3F,0x19,0x0C,0x26,0x3F,0x00},
    // 0x7B {
    {0x38,0x0C,0x0C,0x07,0x0C,0x0C,0x38,0x00},
    // 0x7C |
    {0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00},
    // 0x7D }
    {0x07,0x0C,0x0C,0x38,0x0C,0x0C,0x07,0x00},
    // 0x7E ~
    {0x6E,0x3B,0x00,0x00,0x00,0x00,0x00,0x00},
    // 0x7F DEL
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

// ---- Write(val) ----
//
// Prints val to stdout without a trailing newline (unlike Print).
// Accepts any printable type — same template strategy as bb_Print.

template <typename T>
inline void bb_Write(const T& val) {
    std::cout << val << std::flush;
}

// ---- Locate(x, y) ----
//
// Positions the console text cursor at column x, row y (0-based).
// Uses ANSI escape codes which work on modern terminals (including
// Windows 10+ with Virtual Terminal Processing).
// Emits nothing when stdout is not a terminal (e.g. headless test runs).

#ifdef _WIN32
#  include <io.h>
#  define bb_isatty_stdout_ (_isatty(_fileno(stdout)))
#else
#  include <unistd.h>
#  define bb_isatty_stdout_ (isatty(fileno(stdout)))
#endif

inline void bb_Locate(int x, int y) {
    if (!bb_isatty_stdout_) return;
    std::cout << "\x1b[" << (y + 1) << ";" << (x + 1) << "H" << std::flush;
}

// ---- Text() ----
// Full implementation (bitmap + TTF dispatch) is in M43 below.

// ==========================================================================
// MILESTONE 43 — Font System (SDL3_ttf)
// ==========================================================================
//
// Design:
//   handle 0  = built-in 8×8 bitmap font (always available; active by default
//               and after SetFont(0) or after the active font is freed)
//   handle 1+ = user-loaded TTF fonts (bb_fonts_ vector, 1-based)
//
// When compiled with -DBB_HAS_SDL3_TTF:
//   - SDL3_ttf is used for real font loading, metrics and rendering
//   - bb_Text() switches between bitmap glyph (handle 0) and TTF (handle 1+)
// Without the define:
//   - All functions fall back to the built-in 8×8 bitmap metrics/glyphs

#ifdef BB_HAS_SDL3_TTF
#  include <SDL3_ttf/SDL_ttf.h>
#  include <filesystem>
#endif

// ---- bb_Font_ struct ----

struct bb_Font_ {
    int  height = 8;
    int  width  = 8;
    bool valid  = false;
#ifdef BB_HAS_SDL3_TTF
    TTF_Font* ttf = nullptr;
#endif
};

inline std::vector<bb_Font_> bb_fonts_(1); // slot 0 = built-in bitmap
inline int bb_active_font_ = 0;

// ---- SDL3_ttf lifecycle ----

#ifdef BB_HAS_SDL3_TTF
inline bool bb_ttf_initialized_ = false;

inline void bb_ttf_ensure_() {
    if (bb_ttf_initialized_) return;
    if (TTF_Init()) bb_ttf_initialized_ = true;
}

// Called via bb_ttf_quit_hook_ from bb_sdl_quit_() at program end.
inline void bb_ttf_quit_impl_() {
    for (auto& f : bb_fonts_) {
        if (f.valid && f.ttf) { TTF_CloseFont(f.ttf); f.ttf = nullptr; }
    }
    if (bb_ttf_initialized_) { TTF_Quit(); bb_ttf_initialized_ = false; }
}

// Register the quit hook at startup using the inline-bool trick.
inline const bool bb_ttf_hook_reg_ = (bb_ttf_quit_hook_ = bb_ttf_quit_impl_, true);

// Resolve "Arial" → absolute path: try as-is, then C:\Windows\Fonts\, then
// common Linux font dirs.  Returns "" if not found.
inline bbString bb_find_font_file_(const bbString& name) {
    namespace fs = std::filesystem;

    // 1. Name as provided (absolute or relative path with extension)
    if (fs::exists(name)) return name;

    // 2. Append .ttf
    bbString withExt = name + ".ttf";
    if (fs::exists(withExt)) return withExt;

    // 3. Windows Fonts directory
    bbString lower = name;
    for (auto& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

#ifdef _WIN32
    const char* windir = std::getenv("WINDIR");
    bbString fontsDir  = (windir ? bbString(windir) : bbString("C:\\Windows")) + "\\Fonts\\";
    for (const bbString& n : {lower, name}) {
        bbString p = fontsDir + n + ".ttf";
        if (fs::exists(p)) return p;
    }
#else
    // Linux / macOS common font directories
    for (const char* dir : {
            "/usr/share/fonts/truetype/",
            "/usr/share/fonts/TTF/",
            "/usr/local/share/fonts/",
            "/System/Library/Fonts/",
            "/Library/Fonts/" }) {
        for (const bbString& n : {lower, name}) {
            bbString p = bbString(dir) + n + ".ttf";
            if (fs::exists(p)) return p;
        }
    }
#endif
    return "";
}
#endif // BB_HAS_SDL3_TTF

// ---- LoadFont(name, height [, bold [, italic [, underline]]]) → handle ----
//
// Opens a TTF font file and returns a handle ≥1.
// Falls back gracefully: if SDL3_ttf is not compiled in, or the font file
// cannot be found, returns a handle with 8×8 bitmap metrics (no crash).

inline int bb_LoadFont(const bbString& name, int height = 8,
                       int /*bold*/ = 0, int /*italic*/ = 0,
                       int /*underline*/ = 0) {
    if (height <= 0) return 0;

    bb_Font_ f;
    f.height = 8;
    f.width  = 8;
    f.valid  = true;

#ifdef BB_HAS_SDL3_TTF
    bb_ttf_ensure_();
    if (bb_ttf_initialized_) {
        bbString path = bb_find_font_file_(name);
        if (!path.empty()) {
            TTF_Font* ttf = TTF_OpenFont(path.c_str(), static_cast<float>(height));
            if (ttf) {
                f.ttf    = ttf;
                f.height = TTF_GetFontHeight(ttf);
                // Measure a representative character for the cell width
                int mw = 0, mh = 0;
                if (TTF_GetStringSize(ttf, "M", 1, &mw, &mh) && mw > 0)
                    f.width = mw;
                else
                    f.width = f.height / 2;
            }
        }
    }
#endif

    bb_fonts_.push_back(f);
    return static_cast<int>(bb_fonts_.size()) - 1;
}

// ---- SetFont(handle) ----

inline void bb_SetFont(int handle) {
    if (handle == 0) { bb_active_font_ = 0; return; }
    if (handle > 0 && handle < static_cast<int>(bb_fonts_.size())
        && bb_fonts_[handle].valid)
        bb_active_font_ = handle;
}

// ---- FreeFont(handle) ----

inline void bb_FreeFont(int handle) {
    if (handle > 0 && handle < static_cast<int>(bb_fonts_.size())) {
#ifdef BB_HAS_SDL3_TTF
        if (bb_fonts_[handle].ttf) {
            TTF_CloseFont(bb_fonts_[handle].ttf);
            bb_fonts_[handle].ttf = nullptr;
        }
#endif
        bb_fonts_[handle] = bb_Font_{};
        if (bb_active_font_ == handle) bb_active_font_ = 0;
    }
}

// ---- FontWidth() / FontHeight() ----

inline int bb_FontWidth() {
    if (bb_active_font_ <= 0
        || bb_active_font_ >= static_cast<int>(bb_fonts_.size()))
        return 8;
    return bb_fonts_[bb_active_font_].width;
}

inline int bb_FontHeight() {
    if (bb_active_font_ <= 0
        || bb_active_font_ >= static_cast<int>(bb_fonts_.size()))
        return 8;
    return bb_fonts_[bb_active_font_].height;
}

// ---- StringWidth(s) / StringHeight(s) ----
//
// Pixel extents of s with the active font.
// With SDL3_ttf: uses TTF_GetStringSize(font, text, 0, &w, &h) for accurate metrics.
// Bitmap fallback: len × FontWidth() / FontHeight().

inline int bb_StringWidth(const bbString& s) {
#ifdef BB_HAS_SDL3_TTF
    if (bb_active_font_ > 0
        && bb_active_font_ < static_cast<int>(bb_fonts_.size())
        && bb_fonts_[bb_active_font_].ttf) {
        int w = 0, h = 0;
        TTF_GetStringSize(bb_fonts_[bb_active_font_].ttf, s.c_str(), 0, &w, &h);
        return w;
    }
#endif
    return static_cast<int>(s.size()) * bb_FontWidth();
}

inline int bb_StringHeight(const bbString& s) {
#ifdef BB_HAS_SDL3_TTF
    if (bb_active_font_ > 0
        && bb_active_font_ < static_cast<int>(bb_fonts_.size())
        && bb_fonts_[bb_active_font_].ttf) {
        int w = 0, h = 0;
        TTF_GetStringSize(bb_fonts_[bb_active_font_].ttf, s.c_str(), 0, &w, &h);
        return h;
    }
#endif
    (void)s;
    return bb_FontHeight();
}

// ---- TTF render helper ----

inline void bb_Text_ttf_(int x, int y, const bbString& s,
                          int centerX, int centerY) {
#ifdef BB_HAS_SDL3_TTF
    TTF_Font* font = bb_fonts_[bb_active_font_].ttf;
    SDL_Color fg   = { bb_draw_r_, bb_draw_g_, bb_draw_b_, 255 };
    SDL_Surface* surf = TTF_RenderText_Blended(font, s.c_str(), 0, fg);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(bb_renderer_, surf);
    SDL_DestroySurface(surf);
    if (!tex) return;
    float texW = 0, texH = 0;
    SDL_GetTextureSize(tex, &texW, &texH);
    float drawX = static_cast<float>(x) - (centerX ? texW * 0.5f : 0.0f);
    float drawY = static_cast<float>(y) - (centerY ? texH * 0.5f : 0.0f);
    SDL_FRect dst = { drawX, drawY, texW, texH };
    SDL_RenderTexture(bb_renderer_, tex, nullptr, &dst);
    SDL_DestroyTexture(tex);
#else
    (void)x; (void)y; (void)s; (void)centerX; (void)centerY;
#endif
}

// ---- Text(x, y, s [, centerX [, centerY]]) ----
//
// Renders text using TTF (if active font is a loaded TTF) or the built-in
// 8×8 bitmap glyph array.  Safe no-op when no renderer is available.
//   centerX=1 : x is the horizontal centre of the rendered string
//   centerY=1 : y is the vertical centre of the rendered string

inline void bb_Text(int x, int y, const bbString& s,
                    int centerX = 0, int centerY = 0) {
    if (!bb_renderer_ || s.empty()) return;
#ifdef BB_HAS_SDL3_TTF
    if (bb_active_font_ > 0
        && bb_active_font_ < static_cast<int>(bb_fonts_.size())
        && bb_fonts_[bb_active_font_].ttf) {
        bb_Text_ttf_(x, y, s, centerX, centerY);
        return;
    }
#endif
    // Built-in 8×8 bitmap glyph path
    const int charW  = 8;
    const int charH  = 8;
    const int totalW = static_cast<int>(s.size()) * charW;
    int drawX = x - (centerX ? totalW / 2 : 0);
    int drawY = y - (centerY ? charH  / 2 : 0);
    SDL_SetRenderDrawColor(bb_renderer_, bb_draw_r_, bb_draw_g_, bb_draw_b_, 255);
    for (int ci = 0; ci < static_cast<int>(s.size()); ++ci) {
        unsigned char ch = static_cast<unsigned char>(s[ci]);
        if (ch > 127) ch = static_cast<unsigned char>('?');
        const uint8_t* glyph = bb_font8x8_[ch];
        for (int row = 0; row < charH; ++row) {
            const uint8_t bits = glyph[row];
            for (int col = 0; col < charW; ++col) {
                if (bits & (0x80u >> col)) {
                    SDL_RenderPoint(bb_renderer_,
                                   static_cast<float>(drawX + ci * charW + col),
                                   static_cast<float>(drawY + row));
                }
            }
        }
    }
}

#endif // BLITZNEXT_BB_GRAPHICS2D_H
