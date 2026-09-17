#ifndef BLITZNEXT_BB_CANVAS_H
#define BLITZNEXT_BB_CANVAS_H

// ============================================================
// Zeichnen in Image- und Texturpuffer (BUG-141, BUG-127, BUG-118)
//
// Im Original ist jeder Puffer ein gxCanvas mit eigenem Origin, Viewport,
// Handle und eigener Maskenfarbe; die 2D-Befehle zeichnen in den Canvas, den
// SetBuffer gesetzt hat. Der Bildschirm laeuft hier weiter ueber den
// SDL-Renderer. Fuer Images und Texturen zeichnen die Befehle in deren
// RGBA-Kopie im Speicher; die Algorithmen folgen gxruntime/gxcanvas.cpp
// Zeile fuer Zeile, damit die Pixel dieselben sind (gemessen 2026-09-17).
// Hochgeladen wird erst, wenn die Kopie gebraucht wird - bei Images vor dem
// naechsten Zeichnen auf den Bildschirm, bei Texturen vor dem naechsten
// RenderWorld. Wo der Puffer spaeter liegt (GPU-Ziele), entscheidet Schritt 7
// des Engine-Entwurfs (O8).
//
// Pixel in Images tragen ihre Maske im Alpha: 0 genau dann, wenn die Farbe
// der Maskenfarbe des Image entspricht. So bleibt DrawImage auf den
// Bildschirm unveraendert; gelesen wird immer mit Alpha 255 wie im Original.
// ============================================================

#include "bb_image.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

#ifdef BB_HAS_SDL3_TTF
#include <SDL3_ttf/SDL_ttf.h>
#endif

struct bb_CRect_ { int l = 0, t = 0, r = 0, b = 0; };

inline bb_CRect_ bb_crect_(int x, int y, int w, int h) { return { x, y, x + w, y + h }; }

// ::clip( viewport,RECT* ) aus gxcanvas.cpp
inline bool bb_cclip_(const bb_CRect_& vp, bb_CRect_* d) {
  if (d->r <= d->l || d->b <= d->t || d->l >= vp.r || d->r <= vp.l ||
      d->t >= vp.b || d->b <= vp.t) return false;
  if (d->l < vp.l) d->l = vp.l;
  if (d->r > vp.r) d->r = vp.r;
  if (d->t < vp.t) d->t = vp.t;
  if (d->b > vp.b) d->b = vp.b;
  return true;
}

// ::clip( viewport,RECT *d,RECT *s ) - s wird um dasselbe verschoben
inline bool bb_cclip2_(const bb_CRect_& vp, bb_CRect_* d, bb_CRect_* s) {
  if (d->r <= d->l || d->b <= d->t || d->l >= vp.r || d->r <= vp.l ||
      d->t >= vp.b || d->b <= vp.t) return false;
  int dx, dy;
  if ((dx = vp.l - d->l) > 0) { d->l += dx; s->l += dx; }
  if ((dx = vp.r - d->r) < 0) { d->r += dx; s->r += dx; }
  if ((dy = vp.t - d->t) > 0) { d->t += dy; s->t += dy; }
  if ((dy = vp.b - d->b) < 0) { d->b += dy; s->b += dy; }
  return true;
}

// Origin und Viewport je Puffer. Ein Puffer, den noch niemand gesetzt hat,
// hat Origin 0 und den ganzen Puffer als Viewport - wie ein frischer
// gxCanvas; SetBuffer stellt genau das wieder her.
struct bb_CanvasSt_ {
  bool      init = false;
  int       ox = 0, oy = 0;
  bb_CRect_ vp;
};
inline std::unordered_map<int, bb_CanvasSt_> bb_canvas_st_;

struct bb_Canvas_ {
  uint8_t*       px = nullptr;
  int            w = 0, h = 0;
  bb_CanvasSt_*  st = nullptr;
  int            mask = 0;          // Maskenfarbe RGB
  bool           mask_alpha = false; // Image: Alpha folgt der Maske
  int            hx = 0, hy = 0;    // Handle, wenn der Canvas Quelle ist
  bb_FrameData_* fd = nullptr;      // Image-Frame, wird als veraltet markiert
  bool*          dirty = nullptr;   // Textur: neu hochladen
  bool           keep_alpha = false; // Textur mit Alphakanal: WritePixel behaelt Alpha
  bb_CRect_ clip() const { return { 0, 0, w, h }; }
};

// Texturen melden sich hier an (bb_texture.h); die 2D-Schicht kennt sie nicht.
inline bool (*bb_canvas_tex_hook_)(int buf, bb_Canvas_& c) = nullptr;

// Die Pixelkopie eines Frame sicherstellen. Ohne Kopie ist das Frame ein
// frisches CreateImage: schwarz (ReadPixel liefert $FF000000) und damit ganz
// maskiert. Aus seiner SDL-Zieltextur gibt es nichts zurueckzulesen.
inline void bb_img_ensure_pixels_(int handle, bb_FrameData_& fd) {
  const auto& img = bb_images_[handle];
  const size_t n = static_cast<size_t>(img.width) * img.height * 4;
  if (fd.pixels.size() == n) return;
  fd.pixels.assign(n, 0);
}

inline bool bb_canvas_from_img_(int handle, int frame, bb_Canvas_& c) {
  if (!bb_img_ok_(handle)) return false;
  auto& img = bb_images_[handle];
  if (frame < 0 || frame >= static_cast<int>(img.frames.size())) return false;
  auto& fd = img.frames[frame];
  bb_img_ensure_pixels_(handle, fd);
  if (fd.pixels.empty()) return false;
  c.px = fd.pixels.data();
  c.w = img.width;
  c.h = img.height;
  c.mask = img.mask;
  c.mask_alpha = true;
  c.hx = fd.handle_x;
  c.hy = fd.handle_y;
  c.fd = &fd;
  return true;
}

inline bb_CanvasSt_& bb_canvas_state_(int buf, int w, int h) {
  bb_CanvasSt_& s = bb_canvas_st_[buf];
  if (!s.init) { s.init = true; s.ox = s.oy = 0; s.vp = { 0, 0, w, h }; }
  return s;
}

inline bool bb_canvas_open_(int buf, bb_Canvas_& c) {
  if (buf >= BB_TEX_BUF_BASE_) {
    if (!bb_canvas_tex_hook_ || !bb_canvas_tex_hook_(buf, c)) return false;
  } else {
    int ih = 0, fr = 0;
    if (!bb_decode_img_buf_(buf, ih, fr)) return false;
    if (!bb_canvas_from_img_(ih, fr, c)) return false;
  }
  c.st = &bb_canvas_state_(buf, c.w, c.h);
  return true;
}

inline void bb_canvas_done_(bb_Canvas_& c) {
  if (c.fd) c.fd->stale = true;
  if (c.dirty) *c.dirty = true;
}

inline void bb_canvas_put_(bb_Canvas_& c, int x, int y, int rgb) {
  uint8_t* p = c.px + (static_cast<size_t>(y) * c.w + x) * 4;
  p[0] = static_cast<uint8_t>((rgb >> 16) & 0xFF);
  p[1] = static_cast<uint8_t>((rgb >> 8) & 0xFF);
  p[2] = static_cast<uint8_t>(rgb & 0xFF);
  p[3] = (c.mask_alpha && (rgb & 0xFFFFFF) == c.mask) ? 0 : 255;
}

inline int bb_canvas_rgb_(const bb_Canvas_& c, int x, int y) {
  const uint8_t* p = c.px + (static_cast<size_t>(y) * c.w + x) * 4;
  return (p[0] << 16) | (p[1] << 8) | p[2];
}

inline int bb_canvas_color_()     { return (bb_draw_r_ << 16) | (bb_draw_g_ << 8) | bb_draw_b_; }
inline int bb_canvas_cls_color_() { return (bb_cls_r_ << 16) | (bb_cls_g_ << 8) | bb_cls_b_; }

inline void bb_canvas_fill_(bb_Canvas_& c, const bb_CRect_& r, int rgb) {
  for (int y = r.t; y < r.b; ++y)
    for (int x = r.l; x < r.r; ++x) bb_canvas_put_(c, x, y, rgb);
}

// gxCanvas::setPixel - mit Origin und Viewport
inline void bb_canvas_set_pixel_(bb_Canvas_& c, int x, int y, int rgb) {
  x += c.st->ox; if (x < c.st->vp.l || x >= c.st->vp.r) return;
  y += c.st->oy; if (y < c.st->vp.t || y >= c.st->vp.b) return;
  bb_canvas_put_(c, x, y, rgb);
}

// ---- Befehle auf dem aktiven Puffer ------------------------------------

inline void bb_canvas_reset_(int buf) { bb_canvas_st_.erase(buf); }

inline void bb_canvas_origin_(int x, int y) {
  bb_Canvas_ c;
  if (!bb_canvas_open_(bb_active_buffer_, c)) return;
  c.st->ox = x; c.st->oy = y;
}

inline void bb_canvas_viewport_(int x, int y, int w, int h) {
  bb_Canvas_ c;
  if (!bb_canvas_open_(bb_active_buffer_, c)) return;
  bb_CRect_ r = bb_crect_(x, y, w, h);
  if (!bb_cclip_(c.clip(), &r)) r = {};
  c.st->vp = r;
}

inline void bb_canvas_cls_() {
  bb_Canvas_ c;
  if (!bb_canvas_open_(bb_active_buffer_, c)) return;
  bb_canvas_fill_(c, c.st->vp, bb_canvas_cls_color_());
  bb_canvas_done_(c);
}

inline void bb_canvas_plot_(int x, int y) {
  bb_Canvas_ c;
  if (!bb_canvas_open_(bb_active_buffer_, c)) return;
  bb_canvas_set_pixel_(c, x, y, bb_canvas_color_());
  bb_canvas_done_(c);
}

inline void bb_canvas_line_(int x0, int y0, int x1, int y1) {
  bb_Canvas_ c;
  if (!bb_canvas_open_(bb_active_buffer_, c)) return;
  const int color = bb_canvas_color_();
  x0 += c.st->ox; y0 += c.st->oy;
  x1 += c.st->ox; y1 += c.st->oy;
  const int cx0 = c.st->vp.l, cx1 = c.st->vp.r - 1;
  const int cy0 = c.st->vp.t, cy1 = c.st->vp.b - 1;
  while (true) {
    int clip0 = 0, clip1 = 0;
    if (y0 > cy1) clip0 |= 1; else if (y0 < cy0) clip0 |= 2;
    if (x0 > cx1) clip0 |= 4; else if (x0 < cx0) clip0 |= 8;
    if (y1 > cy1) clip1 |= 1; else if (y1 < cy0) clip1 |= 2;
    if (x1 > cx1) clip1 |= 4; else if (x1 < cx0) clip1 |= 8;
    if ((clip0 | clip1) == 0) break;
    if ((clip0 & clip1) != 0) return;
    if ((clip0 & 1) == 1) { x0 = x0 + ((x1 - x0) * (cy1 - y0)) / (y1 - y0); y0 = cy1; continue; }
    if ((clip0 & 2) == 2) { x0 = x0 + ((x1 - x0) * (cy0 - y0)) / (y1 - y0); y0 = cy0; continue; }
    if ((clip0 & 4) == 4) { y0 = y0 + ((y1 - y0) * (cx1 - x0)) / (x1 - x0); x0 = cx1; continue; }
    if ((clip0 & 8) == 8) { y0 = y0 + ((y1 - y0) * (cx0 - x0)) / (x1 - x0); x0 = cx0; continue; }
    if ((clip1 & 1) == 1) { x1 = x0 + ((x1 - x0) * (cy1 - y0)) / (y1 - y0); y1 = cy1; continue; }
    if ((clip1 & 2) == 2) { x1 = x0 + ((x1 - x0) * (cy0 - y0)) / (y1 - y0); y1 = cy0; continue; }
    if ((clip1 & 4) == 4) { y1 = y0 + ((y1 - y0) * (cx1 - x0)) / (x1 - x0); x1 = cx1; continue; }
    if ((clip1 & 8) == 8) { y1 = y0 + ((y1 - y0) * (cx0 - x0)) / (x1 - x0); x1 = cx0; continue; }
  }
  int dx = x1 - x0, dy = y1 - y0;
  if ((dx | dy) == 0) {
    // Das Original ruft hier setPixel, das den Origin noch einmal addiert.
    bb_canvas_set_pixel_(c, x0, y0, color);
    bb_canvas_done_(c);
    return;
  }
  int sx, sy, ax, ay;
  if (dx >= 0) { sx = 1; ax = dx; } else { sx = -1; ax = -dx; }
  if (dy >= 0) { sy = 1; ay = dy; } else { sy = -1; ay = -dy; }
  if (ax > ay) {
    int ddf = -ax, sadj = ax + ax, padj = ay + ay;
    while (ax-- >= 0) {
      bb_canvas_put_(c, x0, y0, color);
      x0 += sx; ddf += padj; if (ddf >= 0) { y0 += sy; ddf -= sadj; }
    }
  } else {
    int ddf = -ay, sadj = ay + ay, padj = ax + ax;
    while (ay-- >= 0) {
      bb_canvas_put_(c, x0, y0, color);
      y0 += sy; ddf += padj; if (ddf >= 0) { x0 += sx; ddf -= sadj; }
    }
  }
  bb_canvas_done_(c);
}

inline void bb_canvas_rect_(int x, int y, int w, int h, bool solid) {
  bb_Canvas_ c;
  if (!bb_canvas_open_(bb_active_buffer_, c)) return;
  const int color = bb_canvas_color_();
  x += c.st->ox; y += c.st->oy;
  bb_CRect_ dest = bb_crect_(x, y, w, h);
  if (!bb_cclip_(c.st->vp, &dest)) return;
  if (solid) {
    bb_canvas_fill_(c, dest, color);
  } else {
    bb_CRect_ r1 = bb_crect_(x, y, w, 1);         if (bb_cclip_(c.st->vp, &r1)) bb_canvas_fill_(c, r1, color);
    bb_CRect_ r2 = bb_crect_(x, y, 1, h);         if (bb_cclip_(c.st->vp, &r2)) bb_canvas_fill_(c, r2, color);
    bb_CRect_ r3 = bb_crect_(x + w - 1, y, 1, h); if (bb_cclip_(c.st->vp, &r3)) bb_canvas_fill_(c, r3, color);
    bb_CRect_ r4 = bb_crect_(x, y + h - 1, w, 1); if (bb_cclip_(c.st->vp, &r4)) bb_canvas_fill_(c, r4, color);
  }
  bb_canvas_done_(c);
}

inline void bb_canvas_oval_(int x1, int y1, int w, int h, bool solid) {
  bb_Canvas_ c;
  if (!bb_canvas_open_(bb_active_buffer_, c)) return;
  const int color = bb_canvas_color_();
  const bb_CRect_& vp = c.st->vp;
  x1 += c.st->ox; y1 += c.st->oy;
  bb_CRect_ dest = bb_crect_(x1, y1, w, h);
  if (!bb_cclip_(vp, &dest)) return;

  const float xr = w * .5f, yr = h * .5f, ar = static_cast<float>(w) / static_cast<float>(h);
  const float cx = x1 + xr + .5f, cy = y1 + yr - .5f, rsq = yr * yr;
  float y;

  if (solid) {
    y = dest.t - cy;
    for (int t = dest.t; t < dest.b; ++y, ++t) {
      float x = static_cast<float>(std::sqrt(static_cast<double>(rsq - y * y)) * ar);
      int xa = static_cast<int>(std::floor(static_cast<double>(cx - x)));
      int xb = static_cast<int>(std::floor(static_cast<double>(cx + x)));
      if (xb <= xa || xa >= vp.r || xb <= vp.l) continue;
      bb_CRect_ dr;
      dr.t = t; dr.b = t + 1;
      dr.l = xa < vp.l ? vp.l : xa;
      dr.r = xb > vp.r ? vp.r : xb;
      bb_canvas_fill_(c, dr, color);
    }
    bb_canvas_done_(c);
    return;
  }

  int p_xa, p_xb, t;
  const int hh = static_cast<int>(std::floor(static_cast<double>(cy)));
  p_xa = p_xb = static_cast<int>(cx);
  t = dest.t; y = t - cy;
  if (dest.t > y1) { --t; --y; }
  for (; t <= hh; ++y, ++t) {
    float x = static_cast<float>(std::sqrt(static_cast<double>(rsq - y * y)) * ar);
    int xa = static_cast<int>(std::floor(static_cast<double>(cx - x)));
    int xb = static_cast<int>(std::floor(static_cast<double>(cx + x)));
    bb_CRect_ r1 = bb_crect_(xa, t, p_xa - xa, 1); if (r1.r <= r1.l) r1.r = r1.l + 1;
    if (bb_cclip_(vp, &r1)) bb_canvas_fill_(c, r1, color);
    bb_CRect_ r2 = bb_crect_(p_xb, t, xb - p_xb, 1); if (r2.l >= r2.r) r2.l = r2.r - 1;
    if (bb_cclip_(vp, &r2)) bb_canvas_fill_(c, r2, color);
    p_xa = xa; p_xb = xb;
  }
  p_xa = p_xb = static_cast<int>(cx);
  t = dest.b - 1; y = t - cy;
  if (dest.b < y1 + h) { ++t; ++y; }
  for (; t > hh; --y, --t) {
    float x = static_cast<float>(std::sqrt(static_cast<double>(rsq - y * y)) * ar);
    int xa = static_cast<int>(std::floor(static_cast<double>(cx - x)));
    int xb = static_cast<int>(std::floor(static_cast<double>(cx + x)));
    bb_CRect_ r1 = bb_crect_(xa, t, p_xa - xa, 1); if (r1.r <= r1.l) r1.r = r1.l + 1;
    if (bb_cclip_(vp, &r1)) bb_canvas_fill_(c, r1, color);
    bb_CRect_ r2 = bb_crect_(p_xb, t, xb - p_xb, 1); if (r2.l >= r2.r) r2.l = r2.r - 1;
    if (bb_cclip_(vp, &r2)) bb_canvas_fill_(c, r2, color);
    p_xa = xa; p_xb = xb;
  }
  bb_canvas_done_(c);
}

// gxCanvas::blit - der Handle der Quelle verschiebt, Origin und Viewport des
// Ziels gelten, "solid" kopiert auch die Maskenfarbe.
inline void bb_canvas_blit_(bb_Canvas_& d, int x, int y, const bb_Canvas_& s,
                            int sx, int sy, int sw, int sh, bool solid,
                            bb_CRect_* written = nullptr) {
  x += d.st->ox - s.hx;
  y += d.st->oy - s.hy;
  bb_CRect_ dr = bb_crect_(x, y, sw, sh), sr = bb_crect_(sx, sy, sw, sh);
  if (!bb_cclip2_(d.st->vp, &dr, &sr)) return;
  if (!bb_cclip2_(s.clip(), &sr, &dr)) return;
  const int w = dr.r - dr.l, h = dr.b - dr.t;
  if (w <= 0 || h <= 0) return;
  if (written) *written = dr;
  // Quelle und Ziel duerfen derselbe Puffer sein
  std::vector<int> row(static_cast<size_t>(w));
  const bool down = (s.px == d.px) && (sr.t < dr.t);
  for (int k = 0; k < h; ++k) {
    const int j = down ? h - 1 - k : k;
    for (int i = 0; i < w; ++i) row[i] = bb_canvas_rgb_(s, sr.l + i, sr.t + j);
    for (int i = 0; i < w; ++i) {
      if (!solid && row[i] == s.mask) continue;
      bb_canvas_put_(d, dr.l + i, dr.t + j, row[i]);
    }
  }
}

inline bool bb_canvas_draw_image_(int handle, int x, int y, int frame,
                                  int sx, int sy, int sw, int sh, bool whole, bool solid) {
  if (bb_buf_is_screen_(bb_active_buffer_)) return false;
  bb_Canvas_ d, s;
  if (!bb_canvas_open_(bb_active_buffer_, d)) return true;
  if (!bb_canvas_from_img_(handle, frame, s)) return true;
  if (whole) { sx = 0; sy = 0; sw = s.w; sh = s.h; }
  bb_canvas_blit_(d, x, y, s, sx, sy, sw, sh, solid);
  bb_canvas_done_(d);
  return true;
}

// tile() aus bbruntime/bbgraphics.cpp
inline bool bb_canvas_tile_(int handle, int x, int y, int frame, bool solid) {
  if (bb_buf_is_screen_(bb_active_buffer_)) return false;
  bb_Canvas_ d, s;
  if (!bb_canvas_open_(bb_active_buffer_, d)) return true;
  if (!bb_canvas_from_img_(handle, frame, s)) return true;
  const int w = s.w, h = s.h;
  if (w <= 0 || h <= 0) return true;
  const int vp_x = d.st->vp.l, vp_y = d.st->vp.t;
  const int vp_w = d.st->vp.r - d.st->vp.l, vp_h = d.st->vp.b - d.st->vp.t;
  int dx = vp_x - d.st->ox + s.hx;
  int dy = vp_y - d.st->oy + s.hy;
  x -= dx; y -= dy;
  dx += (x >= 0 ? x % w : w - (-x % w));
  dy += (y >= 0 ? y % h : h - (-y % h));
  for (int ty = -h; ty < vp_h; ty += h)
    for (int tx = -w; tx < vp_w; tx += w)
      bb_canvas_blit_(d, tx + dx, ty + dy, s, 0, 0, w, h, solid);
  bb_canvas_done_(d);
  return true;
}

// gxCanvas::text ueber gxFont::render: die Zeichen werden in der Farbe
// gesetzt, Schwarz wird dabei zu $000010, damit es nicht wegmaskiert wird.
inline void bb_canvas_text_(int x, int y, const bbString& s, int centerX, int centerY) {
  bb_Canvas_ c;
  if (!bb_canvas_open_(bb_active_buffer_, c)) return;
  int color = bb_canvas_color_();
  if ((color & 0xFFFFFF) == 0) color = 0x10;

  int gw = 0, gh = 0;
  std::vector<uint8_t> cov;   // 1 = Zeichenpixel
#ifdef BB_HAS_SDL3_TTF
  if (bb_active_font_ > 0 && bb_active_font_ < static_cast<int>(bb_fonts_.size()) &&
      bb_fonts_[bb_active_font_].ttf) {
    SDL_Color fg = { 255, 255, 255, 255 };
    SDL_Surface* surf = TTF_RenderText_Solid(bb_fonts_[bb_active_font_].ttf, s.c_str(), 0, fg);
    if (!surf) return;
    SDL_Surface* rgba = SDL_ConvertSurface(surf, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(surf);
    if (!rgba) return;
    gw = rgba->w; gh = rgba->h;
    cov.assign(static_cast<size_t>(gw) * gh, 0);
    const uint8_t* p = static_cast<const uint8_t*>(rgba->pixels);
    for (int j = 0; j < gh; ++j)
      for (int i = 0; i < gw; ++i)
        cov[static_cast<size_t>(j) * gw + i] = p[(j * rgba->pitch) + i * 4 + 3] >= 128 ? 1 : 0;
    SDL_DestroySurface(rgba);
  } else
#endif
  {
    gw = static_cast<int>(s.size()) * 8; gh = 8;
    cov.assign(static_cast<size_t>(gw) * gh, 0);
    for (int ci = 0; ci < static_cast<int>(s.size()); ++ci) {
      unsigned char ch = static_cast<unsigned char>(s[ci]);
      if (ch > 127) ch = static_cast<unsigned char>('?');
      const uint8_t* glyph = bb_font8x8_[ch];
      for (int row = 0; row < 8; ++row)
        for (int col = 0; col < 8; ++col)
          if (glyph[row] & (1u << col)) cov[static_cast<size_t>(row) * gw + ci * 8 + col] = 1;
    }
  }
  if (centerX) x -= gw / 2;
  if (centerY) y -= gh / 2;
  for (int j = 0; j < gh; ++j)
    for (int i = 0; i < gw; ++i)
      if (cov[static_cast<size_t>(j) * gw + i]) bb_canvas_set_pixel_(c, x + i, y + j, color);
  bb_canvas_done_(c);
}

// ReadPixel/WritePixel ohne LockBuffer: gxCanvas::getPixel/setPixel mit
// Origin und Viewport; ausserhalb liefert ReadPixel die Maskenfarbe.
inline int bb_canvas_read_pixel_(int buf, int x, int y) {
  bb_Canvas_ c;
  if (!bb_canvas_open_(buf, c)) return 0;
  x += c.st->ox; y += c.st->oy;
  if (x < c.st->vp.l || x >= c.st->vp.r || y < c.st->vp.t || y >= c.st->vp.b)
    return static_cast<int>(0xFF000000u | static_cast<unsigned>(c.mask));
  // Ohne Sperre immer mit Alpha FF, auch bei Alpha-Texturen (gemessen).
  return static_cast<int>(0xFF000000u | static_cast<unsigned>(bb_canvas_rgb_(c, x, y)));
}

inline void bb_canvas_write_pixel_(int buf, int x, int y, int argb) {
  bb_Canvas_ c;
  if (!bb_canvas_open_(buf, c)) return;
  x += c.st->ox; y += c.st->oy;
  if (x < c.st->vp.l || x >= c.st->vp.r || y < c.st->vp.t || y >= c.st->vp.b) return;
  bb_canvas_put_(c, x, y, argb);
  // Texturen mit Alphakanal behalten das geschriebene Alpha - es wirkt im
  // Bild, auch 0 (gemessen 2026-09-17, BUG-127).
  if (c.keep_alpha)
    c.px[(static_cast<size_t>(y) * c.w + x) * 4 + 3] = static_cast<uint8_t>((argb >> 24) & 0xFF);
  bb_canvas_done_(c);
}

// ---- CopyRect (BUG-118) -------------------------------------------------

// Der Bildschirm als Canvas fuer CopyRect: eine Kopie des Backbuffer, dazu
// Origin und Viewport des Bildschirms nach den Regeln des Originals (der
// Viewport liegt absolut, der Origin verschiebt ihn nicht).
inline bool bb_canvas_screen_(bb_CanvasSt_& st, std::vector<uint8_t>& px,
                              bb_Canvas_& c, bool read) {
  const int w = bb_gfx_width_, h = bb_gfx_height_;
  if (!bb_renderer_ || w <= 0 || h <= 0) return false;
  px.assign(static_cast<size_t>(w) * h * 4, 0);
  if (read) {
    SDL_SetRenderViewport(bb_renderer_, nullptr);
    SDL_Surface* s = SDL_RenderReadPixels(bb_renderer_, nullptr);
    bb_apply_viewport_();
    if (s) {
      SDL_Surface* rgba = SDL_ConvertSurface(s, SDL_PIXELFORMAT_RGBA32);
      SDL_DestroySurface(s);
      if (rgba) {
        const int cw = std::min(w, rgba->w), ch = std::min(h, rgba->h);
        const uint8_t* src = static_cast<const uint8_t*>(rgba->pixels);
        for (int j = 0; j < ch; ++j)
          std::memcpy(px.data() + static_cast<size_t>(j) * w * 4,
                      src + static_cast<size_t>(j) * rgba->pitch, static_cast<size_t>(cw) * 4);
        SDL_DestroySurface(rgba);
      }
    }
  }
  st.init = true;
  st.ox = bb_origin_x_; st.oy = bb_origin_y_;
  st.vp = { 0, 0, w, h };
  if (bb_viewport_active_) {
    bb_CRect_ r = bb_crect_(bb_viewport_rect_.x, bb_viewport_rect_.y,
                            bb_viewport_rect_.w, bb_viewport_rect_.h);
    if (!bb_cclip_(st.vp, &r)) r = {};
    st.vp = r;
  }
  c.px = px.data(); c.w = w; c.h = h; c.st = &st;
  return true;
}

inline void bb_canvas_copyrect_(int sx, int sy, int sw, int sh, int dx, int dy,
                                int srcbuf, int dstbuf) {
  const bool src_screen = bb_buf_is_screen_(srcbuf);
  const bool dst_screen = bb_buf_is_screen_(dstbuf);
  bb_CanvasSt_ screen_st;
  std::vector<uint8_t> screen_px;
  bb_Canvas_ s, d;

  if (src_screen || dst_screen) {
    bb_Canvas_ sc;
    if (!bb_canvas_screen_(screen_st, screen_px, sc, src_screen)) return;
    if (src_screen) s = sc;
    if (dst_screen) d = sc;
  }
  if (!src_screen && !bb_canvas_open_(srcbuf, s)) return;
  if (!dst_screen && !bb_canvas_open_(dstbuf, d)) return;

  bb_CRect_ written;
  bool any = false;
  {
    bb_CRect_ probe{ 0, 0, 0, 0 };
    bb_canvas_blit_(d, dx, dy, s, sx, sy, sw, sh, true, &probe);
    written = probe;
    any = probe.r > probe.l && probe.b > probe.t;
  }
  if (!any) return;
  if (!dst_screen) { bb_canvas_done_(d); return; }

  // Das geaenderte Rechteck zurueck auf den Bildschirm, Farbe ersetzen.
  const int w = written.r - written.l, h = written.b - written.t;
  SDL_Surface* surf = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32);
  if (!surf) return;
  for (int j = 0; j < h; ++j)
    std::memcpy(static_cast<uint8_t*>(surf->pixels) + static_cast<size_t>(j) * surf->pitch,
                d.px + (static_cast<size_t>(written.t + j) * d.w + written.l) * 4,
                static_cast<size_t>(w) * 4);
  SDL_Texture* tex = SDL_CreateTextureFromSurface(bb_renderer_, surf);
  SDL_DestroySurface(surf);
  if (!tex) return;
  SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_NONE);
  SDL_SetRenderViewport(bb_renderer_, nullptr);
  SDL_FRect dst = { static_cast<float>(written.l), static_cast<float>(written.t),
                    static_cast<float>(w), static_cast<float>(h) };
  SDL_RenderTexture(bb_renderer_, tex, nullptr, &dst);
  bb_apply_viewport_();
  SDL_DestroyTexture(tex);
}

// LockBuffer auf einem Texturpuffer: die Sperre arbeitet auf einer Kopie,
// UnlockBuffer schreibt sie zurueck; die Textur wird vor dem naechsten
// RenderWorld hochgeladen.
inline bool bb_canvas_lock_copy_(int buf, std::vector<uint8_t>& px, int& w, int& h) {
  bb_Canvas_ c;
  if (!bb_canvas_open_(buf, c)) return false;
  w = c.w; h = c.h;
  px.assign(c.px, c.px + static_cast<size_t>(c.w) * c.h * 4);
  return true;
}

inline void bb_canvas_unlock_copy_(int buf, const std::vector<uint8_t>& px) {
  bb_Canvas_ c;
  if (!bb_canvas_open_(buf, c)) return;
  const size_t n = static_cast<size_t>(c.w) * c.h * 4;
  if (px.size() != n) return;
  std::copy(px.begin(), px.end(), c.px);
  bb_canvas_done_(c);
}

inline bool bb_canvas_keeps_alpha_(int buf) {
  bb_Canvas_ c;
  return bb_canvas_open_(buf, c) && c.keep_alpha;
}

inline int bb_canvas_size_(int buf, bool width) {
  bb_Canvas_ c;
  if (!bb_canvas_open_(buf, c)) return 0;
  return width ? c.w : c.h;
}

#endif // BLITZNEXT_BB_CANVAS_H
