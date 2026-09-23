#ifndef BLITZNEXT_BB_IMAGE_H
#define BLITZNEXT_BB_IMAGE_H

#include <vector>
#include <unordered_map>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "bb_sdl.h"    // bb_renderer_, bb_gfx_width_, bb_gfx_height_
#include "bb_graphics2d.h" // bb_active_buffer_ - Vorgabe der Pixel-Befehle
#include "bb_string.h" // bbString

// ---- stb_image (single-header, public domain) ----
//
// Provides PNG, JPG, BMP, TGA, GIF, PSD, HDR, PIC loading.
// STB_IMAGE_IMPLEMENTATION is defined here because bb_image.h is included
// exactly once per generated translation unit (via bb_runtime.h).

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG       // human-readable error messages
#include "../thirdparty/stb/stb_image.h"

// ==========================================================================
// BUG-67 — BMP mit Lauflaengenkodierung (BI_RLE8 / BI_RLE4)
// ==========================================================================
//
// stb_image liest BMP mit 1, 4, 8, 24 und 32 Bit je Bildpunkt, aber keine der
// beiden komprimierten Varianten.  Das Original laedt seine Bilder ueber
// FreeImage 2.4.1 (gxruntime/ddutil.cpp), dessen BMP-Modul sie kennt.  Ohne
// sie liefert LoadTexture Handle 0 — und beim Laden eines Modells fallen alle
// Materialien mit fehlgeschlagener Textur zu einem Brush zusammen, der Fehler
// wird also an der Flaechenzahl sichtbar und nicht am Bild.
//
// Nicht ueberdeckte Bildpunkte — was Sprung- und Zeilenendekommandos
// ueberspringen — bleiben auf Palettenindex 0: FreeImage legt seinen Puffer
// genullt an und fuellt nur, was die Lauflaengen beschreiben.
//
// Rueckgabe wie stbi_load: RGBA, oberste Zeile zuerst, mit malloc angelegt und
// daher mit stbi_image_free freizugeben.  nullptr, wenn die Datei keine
// lauflaengenkodierte BMP ist — dann hat schon stb_image das letzte Wort.

inline unsigned char* bb_load_bmp_rle_(const char* path,
                                       int* out_w, int* out_h, int* out_ch) {
    std::FILE* f = std::fopen(path, "rb");
    if (!f) return nullptr;
    std::fseek(f, 0, SEEK_END);
    long fsize = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (fsize < 54) { std::fclose(f); return nullptr; }
    std::vector<unsigned char> buf((size_t)fsize);
    size_t got = std::fread(buf.data(), 1, (size_t)fsize, f);
    std::fclose(f);
    if (got != (size_t)fsize) return nullptr;

    auto u16 = [&](size_t o) -> unsigned {
        return (unsigned)buf[o] | ((unsigned)buf[o + 1] << 8);
    };
    auto u32 = [&](size_t o) -> unsigned {
        return (unsigned)buf[o]            | ((unsigned)buf[o + 1] << 8)
             | ((unsigned)buf[o + 2] << 16) | ((unsigned)buf[o + 3] << 24);
    };

    if (buf[0] != 'B' || buf[1] != 'M') return nullptr;
    unsigned data_off = u32(10);
    unsigned hdr_size = u32(14);
    if (hdr_size < 40) return nullptr;   // BITMAPCOREHEADER kennt keine Kompression
    int      w        = (int)u32(18);
    int      h        = (int)u32(22);
    unsigned bpp      = u16(28);
    unsigned comp     = u32(30);
    unsigned clr_used = u32(46);

    if (!((comp == 1 && bpp == 8) || (comp == 2 && bpp == 4))) return nullptr;
    if (w <= 0 || h == 0) return nullptr;
    bool top_down = (h < 0);   // bei RLE nicht vorgesehen, kostet aber nichts
    if (top_down) h = -h;
    if ((long long)w * h > 64LL * 1024 * 1024) return nullptr;

    // ---- Palette (BGRA je Eintrag) ----
    unsigned ncolors = clr_used ? clr_used : (1u << bpp);
    if (ncolors > 256) ncolors = 256;
    size_t pal_off = 14 + (size_t)hdr_size;
    if (pal_off + (size_t)ncolors * 4 > buf.size()) return nullptr;
    unsigned char pal[256][4];
    std::memset(pal, 0, sizeof(pal));
    for (int i = 0; i < 256; ++i) pal[i][3] = 255;
    for (unsigned i = 0; i < ncolors; ++i) {
        pal[i][0] = buf[pal_off + i * 4 + 2];   // R
        pal[i][1] = buf[pal_off + i * 4 + 1];   // G
        pal[i][2] = buf[pal_off + i * 4 + 0];   // B
    }

    // ---- Lauflaengen ausrollen; y zaehlt die Bildzeilen in Dateireihenfolge ----
    std::vector<unsigned char> idx((size_t)w * (size_t)h, 0);
    size_t p    = data_off;
    int    x    = 0, y = 0;
    bool   done = false;
    while (!done && p + 1 < buf.size()) {
        if (y < 0 || y >= h) break;
        unsigned cnt = buf[p], val = buf[p + 1];
        p += 2;
        if (cnt > 0) {                       // Lauf gleicher Bildpunkte
            for (unsigned i = 0; i < cnt && x < w; ++i, ++x) {
                unsigned v = (bpp == 8) ? val
                                        : ((i & 1) ? (val & 0x0fu) : (val >> 4));
                idx[(size_t)y * w + x] = (unsigned char)v;
            }
        } else if (val == 0) {               // Zeilenende
            x = 0;
            if (++y >= h) done = true;
        } else if (val == 1) {               // Bildende
            done = true;
        } else if (val == 2) {               // Sprung um dx, dy
            if (p + 1 >= buf.size()) break;
            x += buf[p];
            y += buf[p + 1];
            p += 2;
            if (y >= h) done = true;
        } else {                             // Rohdaten, auf gerade Byte-Zahl gefuellt
            unsigned n     = val;
            size_t   bytes = (bpp == 8) ? n : ((n + 1) / 2);
            if (p + bytes > buf.size()) break;
            for (unsigned i = 0; i < n && x < w; ++i, ++x) {
                unsigned v = (bpp == 8)
                    ? buf[p + i]
                    : ((i & 1) ? (buf[p + i / 2] & 0x0fu) : (buf[p + i / 2] >> 4));
                idx[(size_t)y * w + x] = (unsigned char)v;
            }
            p += bytes + (bytes & 1);
        }
    }

    unsigned char* out = (unsigned char*)std::malloc((size_t)w * (size_t)h * 4);
    if (!out) return nullptr;
    for (int row = 0; row < h; ++row) {
        // Zeile 0 der Datei ist die unterste, ausser bei negativer Hoehe
        int                  src = top_down ? row : (h - 1 - row);
        const unsigned char* s   = &idx[(size_t)src * w];
        unsigned char*       d   = out + (size_t)row * w * 4;
        for (int i = 0; i < w; ++i) {
            const unsigned char* c = pal[s[i]];
            d[i * 4 + 0] = c[0];
            d[i * 4 + 1] = c[1];
            d[i * 4 + 2] = c[2];
            d[i * 4 + 3] = c[3];
        }
    }
    *out_w = w;
    *out_h = h;
    if (out_ch) *out_ch = 3;   // Palettenbild ohne Alphakanal, wie stb_image es meldet
    return out;
}

// stbi_load mit dem RLE-Nachweg.  Jeder Ladebefehl geht hier durch, damit
// LoadImage, LoadAnimImage, LoadBuffer, LoadTexture und LoadAnimTexture
// dieselben Dateien annehmen.
inline unsigned char* bb_load_rgba_(const char* path, int* w, int* h, int* ch) {
    if (unsigned char* d = stbi_load(path, w, h, ch, 4)) return d;
    return bb_load_bmp_rle_(path, w, h, ch);
}

// Ein geladenes Image ist im Original sofort mit Schwarz maskiert, auch ohne
// MaskImage: gemessen 2026-09-19 an nextstage.bmp aus blox-n-balls (24 Bit,
// 9040 Pixel 0,0,0) - DrawImage laesst sie aus, bei 16 und 32 Bit Farbtiefe;
// fast schwarze Pixel bleiben. Wie bb_canvas_put_ steht das im Alphakanal
// (BUG-172).
inline void bb_img_mask_black_(unsigned char* p, int w, int h) {
    for (size_t i = 0, n = static_cast<size_t>(w) * h; i < n; ++i, p += 4)
        if (p[0] == 0 && p[1] == 0 && p[2] == 0) p[3] = 0;
}


// ==========================================================================
// MILESTONE 44/45/46/46b — Image System
// ==========================================================================
//
// Image handles are 1-based integers into bb_images_.
// Slot 0 is reserved as the null/invalid handle.
//
// M46b: Multi-frame support.
//   bb_Image_ holds a vector of bb_FrameData_ (one per animation frame).
//   Single-frame images (LoadImage, CreateImage with default) have exactly
//   one frame at index 0.  LoadAnimImage produces N frames in one handle.
//   All drawing/manipulation functions accept an optional frame%=0 parameter.
//
// Buffer handle encoding (M46):
//   BackBuffer()    → 1 (BB_BACK_BUFFER_H)
//   FrontBuffer()   → 2 (BB_FRONT_BUFFER_H)
//   ImageBuffer(img [,frame]) → (img-1) + frame * BB_IMG_BUF_STRIDE_ + 3
//   For frame=0 this equals the old (img + 2) encoding — fully backward compat.

// ==========================================================================
// Data structures
// ==========================================================================

struct bb_FrameData_ {
    SDL_Texture*         tex      = nullptr;
    std::vector<uint8_t> pixels;   // raw RGBA copy (used by Save/Mask/Flip/Lock)
    int                  handle_x = 0;
    int                  handle_y = 0;
    float                scale_x  = 1.0f;
    float                scale_y  = 1.0f;
    float                rotation = 0.0f;
    // Die Pixelkopie wurde ueber bb_canvas.h geaendert; tex vor dem
    // naechsten Zeichnen auf den Bildschirm neu erzeugen (BUG-141).
    bool                 stale    = false;
};

struct bb_Image_ {
    int  width  = 0;   // per-frame cell width  (same for all frames)
    int  height = 0;   // per-frame cell height (same for all frames)
    bool valid  = false;
    int  mask   = 0;   // Maskenfarbe RGB, Vorgabe Schwarz (MaskImage)
    std::vector<bb_FrameData_> frames;
};

inline std::vector<bb_Image_> bb_images_(1);  // slot 0 = null/invalid

// Global AutoMidHandle flag (default off)
inline bool bb_auto_mid_handle_ = false;

// ---- Buffer encoding constants ----
//
// BB_IMG_BUF_OFFSET_ = 3  (first image buffer handle for img=1, frame=0 → 3
//   which equals the old (1 + 2) = 3, so existing code is unaffected)
// BB_IMG_BUF_STRIDE_ = 65536  (max image handles before frame bits overflow)

inline constexpr int BB_IMG_BUF_OFFSET_ = 3;
inline constexpr int BB_IMG_BUF_STRIDE_ = 65536;

// ---- Image cleanup hook ----

inline void bb_image_quit_impl_() {
    for (auto& img : bb_images_) {
        for (auto& fd : img.frames) {
            if (fd.tex) { SDL_DestroyTexture(fd.tex); fd.tex = nullptr; }
        }
    }
    bb_images_.assign(1, bb_Image_{});
    bb_auto_mid_handle_ = false;
}

inline const bool bb_image_hook_reg_ =
    (bb_image_quit_hook_ = bb_image_quit_impl_, true);

// ---- Internal helpers ----

inline bool bb_img_ok_(int h) {
    return h > 0 && h < static_cast<int>(bb_images_.size())
           && bb_images_[h].valid;
}

inline void bb_img_reupload_frame_(int handle, bb_FrameData_* fd);

// DrawBlock & Co.: Farbe ersetzen, Alpha des Ziels stehen lassen. Mit
// SDL_BLENDMODE_NONE kaeme das Masken-Alpha 0 in den Bildschirm, und
// ReadPixel lieferte dort $00000000 statt $FF000000 (BUG-141).
inline SDL_BlendMode bb_blockblend_() {
    static const SDL_BlendMode m = SDL_ComposeCustomBlendMode(
        SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ZERO, SDL_BLENDOPERATION_ADD,
        SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
    return m;
}

// Returns a pointer to the requested frame (clamped to [0, frames.size()-1]).
// Returns nullptr if the handle is invalid or has no frames. Eine ueber
// bb_canvas.h geaenderte Pixelkopie wird dabei hochgeladen.
inline bb_FrameData_* bb_img_frame_(int handle, int frame) {
    if (!bb_img_ok_(handle)) return nullptr;
    auto& img = bb_images_[handle];
    if (img.frames.empty()) return nullptr;
    if (frame < 0 || frame >= static_cast<int>(img.frames.size())) frame = 0;
    bb_FrameData_* fd = &img.frames[frame];
    if (fd->stale) { fd->stale = false; bb_img_reupload_frame_(handle, fd); }
    return fd;
}

// Re-uploads a frame's pixel buffer to its SDL_Texture.
// Creates a new texture if none exists.  No-op in headless mode.
inline void bb_img_reupload_frame_(int handle, bb_FrameData_* fd) {
    if (!bb_renderer_ || !fd || fd->pixels.empty()) return;
    const auto& img = bb_images_[handle];
    if (fd->tex) { SDL_DestroyTexture(fd->tex); fd->tex = nullptr; }
    SDL_Surface* surf = SDL_CreateSurfaceFrom(
        img.width, img.height, SDL_PIXELFORMAT_RGBA32,
        fd->pixels.data(), img.width * 4);
    if (surf) {
        SDL_SetSurfaceBlendMode(surf, SDL_BLENDMODE_NONE);
        fd->tex = SDL_CreateTextureFromSurface(bb_renderer_, surf);
        if (fd->tex) SDL_SetTextureBlendMode(fd->tex, SDL_BLENDMODE_BLEND);
        SDL_DestroySurface(surf);
    }
}

// ==========================================================================
// M44: Core Image API
// ==========================================================================

// ---- LoadImage(file) → handle ----

inline int bb_LoadImage(const bbString& file) {
    int w = 0, h = 0, ch = 0;
    unsigned char* data = bb_load_rgba_(file.c_str(), &w, &h, &ch);
    if (!data) {
        std::cerr << "[runtime] LoadImage: cannot load '" << file << "'\n";
        return 0;
    }
    bb_img_mask_black_(data, w, h);

    bb_Image_ img;
    img.width  = w;
    img.height = h;
    img.valid  = true;

    bb_FrameData_ fd;
    if (bb_renderer_) {
        SDL_Surface* surf = SDL_CreateSurfaceFrom(
            w, h, SDL_PIXELFORMAT_RGBA32, data, w * 4);
        if (surf) {
            SDL_SetSurfaceBlendMode(surf, SDL_BLENDMODE_NONE);
            fd.tex = SDL_CreateTextureFromSurface(bb_renderer_, surf);
            if (fd.tex)
                SDL_SetTextureBlendMode(fd.tex, SDL_BLENDMODE_BLEND);
            SDL_DestroySurface(surf);
        }
    }
    fd.pixels.assign(data, data + w * h * 4);
    stbi_image_free(data);

    if (bb_auto_mid_handle_) {
        fd.handle_x = w / 2;
        fd.handle_y = h / 2;
    }

    img.frames.push_back(std::move(fd));
    bb_images_.push_back(std::move(img));
    return static_cast<int>(bb_images_.size()) - 1;
}

// ---- CreateImage(w, h [,frames=1]) → handle ----

inline int bb_CreateImage(int w, int h, int nframes = 1) {
    if (w <= 0 || h <= 0 || nframes < 1) return 0;

    bb_Image_ img;
    img.width  = w;
    img.height = h;
    img.valid  = true;

    for (int f = 0; f < nframes; ++f) {
        bb_FrameData_ fd;
        if (bb_renderer_) {
            fd.tex = SDL_CreateTexture(bb_renderer_,
                                       SDL_PIXELFORMAT_RGBA32,
                                       SDL_TEXTUREACCESS_TARGET,
                                       w, h);
            if (fd.tex)
                SDL_SetTextureBlendMode(fd.tex, SDL_BLENDMODE_BLEND);
        }
        if (bb_auto_mid_handle_) {
            fd.handle_x = w / 2;
            fd.handle_y = h / 2;
        }
        img.frames.push_back(std::move(fd));
    }

    bb_images_.push_back(std::move(img));
    return static_cast<int>(bb_images_.size()) - 1;
}

// ---- FreeImage(handle) ----

inline void bb_FreeImage(int handle) {
    if (!bb_img_ok_(handle)) return;
    for (auto& fd : bb_images_[handle].frames) {
        if (fd.tex) { SDL_DestroyTexture(fd.tex); fd.tex = nullptr; }
    }
    bb_images_[handle] = bb_Image_{};
}

// ---- ImageWidth / ImageHeight  (frame param accepted for API compat) ----

// Ohne frame-Parameter: im Original `ImageWidth ( image )`. Alle Frames eines
// Bildes haben ohnehin dieselbe Groesse - der Parameter wurde hier auch schon
// vorher ignoriert (BUG-44).
inline int bb_ImageWidth(int handle) {
    return bb_img_ok_(handle) ? bb_images_[handle].width : 0;
}
inline int bb_ImageHeight(int handle) {
    return bb_img_ok_(handle) ? bb_images_[handle].height : 0;
}

// ---- DrawImage(handle, x, y [,frame=0]) ----

inline bool bb_canvas_draw_image_(int handle, int x, int y, int frame,
                                  int sx, int sy, int sw, int sh, bool whole, bool solid);
inline bool bb_canvas_tile_(int handle, int x, int y, int frame, bool solid);

inline void bb_DrawImage(int handle, int x, int y, int frame = 0) {
    if (bb_canvas_draw_image_(handle, x, y, frame, 0, 0, 0, 0, true, false)) return;
    if (!bb_renderer_ || !bb_img_ok_(handle)) return;
    const bb_FrameData_* fd = bb_img_frame_(handle, frame);
    if (!fd || !fd->tex) return;
    const auto& img = bb_images_[handle];

    float dw = img.width  * fd->scale_x;
    float dh = img.height * fd->scale_y;
    float dx = static_cast<float>(x) - fd->handle_x * fd->scale_x;
    float dy = static_cast<float>(y) - fd->handle_y * fd->scale_y;
    SDL_FRect dst = { dx, dy, dw, dh };

    if (fd->rotation != 0.0f) {
        SDL_FPoint center = { fd->handle_x * fd->scale_x,
                              fd->handle_y * fd->scale_y };
        SDL_RenderTextureRotated(bb_renderer_, fd->tex, nullptr, &dst,
                                 static_cast<double>(fd->rotation),
                                 &center, SDL_FLIP_NONE);
    } else {
        SDL_RenderTexture(bb_renderer_, fd->tex, nullptr, &dst);
    }
}

// ---- DrawImageRect(handle, x, y, sx, sy, sw, sh [,frame=0]) ----

inline void bb_DrawImageRect(int handle, int x, int y,
                              int sx, int sy, int sw, int sh,
                              int frame = 0) {
    if (bb_canvas_draw_image_(handle, x, y, frame, sx, sy, sw, sh, false, false)) return;
    if (!bb_renderer_ || !bb_img_ok_(handle)) return;
    const bb_FrameData_* fd = bb_img_frame_(handle, frame);
    if (!fd || !fd->tex) return;
    SDL_FRect src = { static_cast<float>(sx), static_cast<float>(sy),
                      static_cast<float>(sw), static_cast<float>(sh) };
    SDL_FRect dst = { static_cast<float>(x),  static_cast<float>(y),
                      static_cast<float>(sw),  static_cast<float>(sh) };
    SDL_RenderTexture(bb_renderer_, fd->tex, &src, &dst);
}

// ---- DrawBlock(handle, x, y [,frame=0]) ----
//
// Like DrawImage but ignores handle offset.  Scale and rotation still apply.

inline void bb_DrawBlock(int handle, int x, int y, int frame = 0) {
    if (bb_canvas_draw_image_(handle, x, y, frame, 0, 0, 0, 0, true, true)) return;
    if (!bb_renderer_ || !bb_img_ok_(handle)) return;
    const bb_FrameData_* fd = bb_img_frame_(handle, frame);
    if (!fd || !fd->tex) return;
    const auto& img = bb_images_[handle];

    float dw = img.width  * fd->scale_x;
    float dh = img.height * fd->scale_y;
    SDL_FRect dst = { static_cast<float>(x), static_cast<float>(y), dw, dh };

    // DrawBlock zeichnet auch die maskierten Pixel (gemessen 2026-09-17,
    // BUG-141).
    SDL_SetTextureBlendMode(fd->tex, bb_blockblend_());
    if (fd->rotation != 0.0f) {
        SDL_FPoint center = { dw * 0.5f, dh * 0.5f };
        SDL_RenderTextureRotated(bb_renderer_, fd->tex, nullptr, &dst,
                                 static_cast<double>(fd->rotation),
                                 &center, SDL_FLIP_NONE);
    } else {
        SDL_RenderTexture(bb_renderer_, fd->tex, nullptr, &dst);
    }
    SDL_SetTextureBlendMode(fd->tex, SDL_BLENDMODE_BLEND);
}

// ---- DrawBlockRect(handle, x, y, sx, sy, sw, sh [,frame=0]) ----

inline void bb_DrawBlockRect(int handle, int x, int y,
                              int sx, int sy, int sw, int sh,
                              int frame = 0) {
    if (bb_canvas_draw_image_(handle, x, y, frame, sx, sy, sw, sh, false, true)) return;
    bb_FrameData_* fd = bb_img_frame_(handle, frame);
    if (!fd || !fd->tex) return;
    SDL_SetTextureBlendMode(fd->tex, bb_blockblend_());
    bb_DrawImageRect(handle, x, y, sx, sy, sw, sh, frame);
    SDL_SetTextureBlendMode(fd->tex, SDL_BLENDMODE_BLEND);
}

// ==========================================================================
// M45: Image Manipulation
// ==========================================================================

// ---- HandleImage / MidHandle / AutoMidHandle ----

// Ohne frame-Parameter: im Original `HandleImage image,x,y`. Der Griffpunkt
// gilt dort dem ganzen Bild, also allen Frames (BUG-44).
inline void bb_HandleImage(int handle, int hx, int hy) {
    if (!bb_img_ok_(handle)) return;
    for (auto& fd : bb_images_[handle].frames) {
        fd.handle_x = hx;
        fd.handle_y = hy;
    }
}

// Ohne frame-Parameter: im Original `MidHandle image` (BUG-44).
inline void bb_MidHandle(int handle) {
    if (!bb_img_ok_(handle)) return;
    auto& img = bb_images_[handle];
    for (auto& fd : img.frames) {
        fd.handle_x = img.width  / 2;
        fd.handle_y = img.height / 2;
    }
}

inline void bb_AutoMidHandle(int on) {
    bb_auto_mid_handle_ = (on != 0);
}

// ---- ImageXHandle / ImageYHandle ----

// Ohne frame-Parameter: im Original `ImageXHandle ( image )`. Der Griffpunkt
// ist fuer alle Frames derselbe, seit HandleImage ihn ueberall setzt (BUG-44).
inline int bb_ImageXHandle(int handle) {
    const bb_FrameData_* fd = bb_img_frame_(handle, 0);
    return fd ? fd->handle_x : 0;
}
inline int bb_ImageYHandle(int handle) {
    const bb_FrameData_* fd = bb_img_frame_(handle, 0);
    return fd ? fd->handle_y : 0;
}

// ---- ScaleImage / RotateImage ----

// Ohne frame-Parameter: im Original `ScaleImage image,xscale#,yscale#` und
// `RotateImage image,angle#` - beide wirken auf das ganze Bild (BUG-44).
inline void bb_ScaleImage(int handle, float sx, float sy) {
    if (!bb_img_ok_(handle)) return;
    for (auto& fd : bb_images_[handle].frames) {
        fd.scale_x = (sx > 0.0f) ? sx : 0.0f;
        fd.scale_y = (sy > 0.0f) ? sy : 0.0f;
    }
}

inline void bb_RotateImage(int handle, float deg) {
    if (!bb_img_ok_(handle)) return;
    for (auto& fd : bb_images_[handle].frames) fd.rotation = deg;
}

// ---- MaskImage(handle, r, g, b) ----
//
// Ohne frame-Parameter: im Original `MaskImage image,red,green,blue`. Die
// Maskenfarbe gilt dem ganzen Bild, also jedem Frame (BUG-44).

inline void bb_MaskImage(int handle, int r, int g, int b) {
    if (!bb_img_ok_(handle)) return;
    auto& img = bb_images_[handle];
    img.mask = ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);
    const int n = img.width * img.height;
    for (auto& fd : img.frames) {
        if (fd.pixels.empty()) continue;
        uint8_t* p = fd.pixels.data();
        // Es gilt nur die neue Maskenfarbe: Pixel der alten (seit BUG-172
        // auch des vorgegebenen Schwarz) werden wieder sichtbar.
        for (int i = 0; i < n; ++i, p += 4) {
            p[3] = (p[0] == static_cast<uint8_t>(r) &&
                    p[1] == static_cast<uint8_t>(g) &&
                    p[2] == static_cast<uint8_t>(b)) ? 0 : 255;
        }
        bb_img_reupload_frame_(handle, &fd);
    }
}

// ---- TileImage / TileBlock ----

// x und y sind optional: im Original `TileImage image[,x][,y][,frame]` (BUG-44).
inline void bb_TileImage(int handle, int x = 0, int y = 0, int frame = 0) {
    if (bb_canvas_tile_(handle, x, y, frame, false)) return;
    if (!bb_renderer_ || !bb_img_ok_(handle)) return;
    const bb_FrameData_* fd = bb_img_frame_(handle, frame);
    const auto& img = bb_images_[handle];
    if (!fd || !fd->tex || img.width <= 0 || img.height <= 0) return;

    int gw = bb_gfx_width_, gh = bb_gfx_height_;
    int ox = ((x - fd->handle_x) % img.width  + img.width)  % img.width;
    int oy = ((y - fd->handle_y) % img.height + img.height) % img.height;

    for (int ty = ox - img.height; ty < gh; ty += img.height) {
        for (int tx = oy - img.width; tx < gw; tx += img.width) {
            SDL_FRect dst = { static_cast<float>(tx), static_cast<float>(ty),
                              static_cast<float>(img.width),
                              static_cast<float>(img.height) };
            SDL_RenderTexture(bb_renderer_, fd->tex, nullptr, &dst);
        }
    }
}

inline void bb_TileBlock(int handle, int x = 0, int y = 0, int frame = 0) {
    if (bb_canvas_tile_(handle, x, y, frame, true)) return;
    if (!bb_renderer_ || !bb_img_ok_(handle)) return;
    const bb_FrameData_* fd = bb_img_frame_(handle, frame);
    const auto& img = bb_images_[handle];
    if (!fd || !fd->tex || img.width <= 0 || img.height <= 0) return;

    int gw = bb_gfx_width_, gh = bb_gfx_height_;
    int ox = (x % img.width  + img.width)  % img.width;
    int oy = (y % img.height + img.height) % img.height;

    SDL_SetTextureBlendMode(fd->tex, bb_blockblend_());   // wie DrawBlock
    for (int ty = ox - img.height; ty < gh; ty += img.height) {
        for (int tx = oy - img.width; tx < gw; tx += img.width) {
            SDL_FRect dst = { static_cast<float>(tx), static_cast<float>(ty),
                              static_cast<float>(img.width),
                              static_cast<float>(img.height) };
            SDL_RenderTexture(bb_renderer_, fd->tex, nullptr, &dst);
        }
    }
    SDL_SetTextureBlendMode(fd->tex, SDL_BLENDMODE_BLEND);
}

// ---- DrawImageEllipse(handle, x, y, rx, ry [,frame=0]) ----

inline void bb_DrawImageEllipse(int handle, int x, int y, int rx, int ry,
                                 int frame = 0) {
    if (!bb_renderer_ || !bb_img_ok_(handle)) return;
    const bb_FrameData_* fd = bb_img_frame_(handle, frame);
    const auto& img = bb_images_[handle];
    if (!fd || !fd->tex) return;

    SDL_Rect clip = { x - rx, y - ry, 2 * rx, 2 * ry };
    SDL_SetRenderClipRect(bb_renderer_, &clip);

    float dw = img.width  * fd->scale_x;
    float dh = img.height * fd->scale_y;
    float dx = static_cast<float>(x) - fd->handle_x * fd->scale_x;
    float dy = static_cast<float>(y) - fd->handle_y * fd->scale_y;
    SDL_FRect dst = { dx, dy, dw, dh };

    if (fd->rotation != 0.0f) {
        SDL_FPoint center = { fd->handle_x * fd->scale_x,
                              fd->handle_y * fd->scale_y };
        SDL_RenderTextureRotated(bb_renderer_, fd->tex, nullptr, &dst,
                                 static_cast<double>(fd->rotation),
                                 &center, SDL_FLIP_NONE);
    } else {
        SDL_RenderTexture(bb_renderer_, fd->tex, nullptr, &dst);
    }

    SDL_SetRenderClipRect(bb_renderer_, nullptr);
}

// SaveImage steht bei SaveBuffer (M46), weil es dessen BMP schreibt.

// ==========================================================================
// M45: Collision / Overlap  (bounding-box)
// ==========================================================================

struct bb_AABB_ { int x1, y1, x2, y2; };

inline bb_AABB_ bb_img_aabb_(int handle, int x, int y) {
    const auto& img = bb_images_[handle];
    // Use frame 0 handle offset for bounding-box (consistent with DrawImage)
    int hx = img.frames.empty() ? 0 : img.frames[0].handle_x;
    int hy = img.frames.empty() ? 0 : img.frames[0].handle_y;
    int lx = x - hx;
    int ly = y - hy;
    return { lx, ly, lx + img.width, ly + img.height };
}

inline bool bb_aabb_overlap_(const bb_AABB_& a, const bb_AABB_& b) {
    return a.x1 < b.x2 && a.x2 > b.x1
        && a.y1 < b.y2 && a.y2 > b.y1;
}

inline int bb_ImagesOverlap(int h1, int x1, int y1,
                             int h2, int x2, int y2) {
    if (!bb_img_ok_(h1) || !bb_img_ok_(h2)) return 0;
    return bb_aabb_overlap_(bb_img_aabb_(h1, x1, y1),
                            bb_img_aabb_(h2, x2, y2)) ? 1 : 0;
}

inline int bb_ImageRectOverlap(int handle, int x, int y,
                                int rx, int ry, int rw, int rh) {
    if (!bb_img_ok_(handle)) return 0;
    bb_AABB_ img  = bb_img_aabb_(handle, x, y);
    bb_AABB_ rect = { rx, ry, rx + rw, ry + rh };
    return bb_aabb_overlap_(img, rect) ? 1 : 0;
}

inline int bb_ImagesColl(int h1, int x1, int y1,
                          int h2, int x2, int y2) {
    return bb_ImagesOverlap(h1, x1, y1, h2, x2, y2);
}

inline int bb_ImageXColl(int h1, int x1, int y1,
                          int h2, int x2, int y2) {
    if (!bb_img_ok_(h1) || !bb_img_ok_(h2)) return 0;
    bb_AABB_ a = bb_img_aabb_(h1, x1, y1);
    bb_AABB_ b = bb_img_aabb_(h2, x2, y2);
    if (!bb_aabb_overlap_(a, b)) return 0;
    int ox1 = (a.x1 > b.x1) ? a.x1 : b.x1;
    int ox2 = (a.x2 < b.x2) ? a.x2 : b.x2;
    return (ox1 + ox2) / 2;
}

inline int bb_ImageYColl(int h1, int x1, int y1,
                          int h2, int x2, int y2) {
    if (!bb_img_ok_(h1) || !bb_img_ok_(h2)) return 0;
    bb_AABB_ a = bb_img_aabb_(h1, x1, y1);
    bb_AABB_ b = bb_img_aabb_(h2, x2, y2);
    if (!bb_aabb_overlap_(a, b)) return 0;
    int oy1 = (a.y1 > b.y1) ? a.y1 : b.y1;
    int oy2 = (a.y2 < b.y2) ? a.y2 : b.y2;
    return (oy1 + oy2) / 2;
}

// ---- ImagesCollide / ImageRectCollide (M46b stubs — bounding-box for now) ----

inline int bb_ImagesCollide(int h1, int x1, int y1, int /*f1*/,
                             int h2, int x2, int y2, int /*f2*/) {
    return bb_ImagesOverlap(h1, x1, y1, h2, x2, y2);
}

inline int bb_ImageRectCollide(int handle, int x, int y, int /*frame*/,
                                int rx, int ry, int rw, int rh) {
    return bb_ImageRectOverlap(handle, x, y, rx, ry, rw, rh);
}

// ==========================================================================
// M46: Pixel Buffer Access
// ==========================================================================
//
// Buffer handles:
//   BB_BACK_BUFFER_H  (1) — back buffer
//   BB_FRONT_BUFFER_H (2) — front buffer
//   ImageBuffer(img, frame) → (img-1) + frame * BB_IMG_BUF_STRIDE_ + 3
//     For frame=0: equals the old (img + 2) — fully backward compatible.
//
// bb_BufLock_ stores both img_h and img_frame for correct UnlockBuffer flush.

inline int bb_ImageBuffer(int img, int frame = 0) {
    if (!bb_img_ok_(img)) return 0;
    const auto& image = bb_images_[img];
    if (frame < 0 || frame >= static_cast<int>(image.frames.size())) frame = 0;
    return (img - 1) + frame * BB_IMG_BUF_STRIDE_ + BB_IMG_BUF_OFFSET_;
}

struct bb_BufLock_ {
    std::vector<uint8_t> pixels;
    int  width     = 0;
    int  height    = 0;
    bool dirty     = false;
    bool locked    = false;
    int  img_h     = 0;   // 0 = screen buffer
    int  img_frame = 0;
};

inline std::unordered_map<int, bb_BufLock_> bb_buf_locks_;

// ---- Decode an image buffer handle → img_h, frame ----

inline bool bb_decode_img_buf_(int buf, int& img_h, int& frame) {
    if (buf <= 2 || buf >= BB_TEX_BUF_BASE_) return false;
    int raw = buf - BB_IMG_BUF_OFFSET_;           // = (img-1) + frame * STRIDE
    img_h = raw % BB_IMG_BUF_STRIDE_ + 1;         // 1-based
    frame = raw / BB_IMG_BUF_STRIDE_;
    return true;
}

inline int  bb_canvas_read_pixel_(int buf, int x, int y);
inline void bb_canvas_write_pixel_(int buf, int x, int y, int argb);
// Texturpuffer: Kopie der Pixel fuer LockBuffer holen und zurueckschreiben
// (bb_canvas.h, BUG-127).
inline bool bb_canvas_lock_copy_(int buf, std::vector<uint8_t>& px, int& w, int& h);
inline void bb_canvas_unlock_copy_(int buf, const std::vector<uint8_t>& px);

// ---- bb_LockBuffer(buf) ----

// Der Buffer ist optional; ohne Angabe gilt der zuletzt mit SetBuffer
// gesetzte. Im Original sind das `LockBuffer [buffer]` (BUG-44).
inline void bb_LockBuffer(int buf = bb_active_buffer_) {
    bb_BufLock_& lock = bb_buf_locks_[buf];
    lock = bb_BufLock_{};

    if (buf == BB_BACK_BUFFER_H || buf == BB_FRONT_BUFFER_H) {
        lock.width  = bb_gfx_width_;
        lock.height = bb_gfx_height_;
        lock.img_h  = 0;
        const int n = lock.width * lock.height * 4;
        if (n <= 0) { lock.locked = false; return; }

        if (bb_renderer_) {
            SDL_Surface* surf = SDL_RenderReadPixels(bb_renderer_, nullptr);
            if (surf) {
                SDL_Surface* rgba = SDL_ConvertSurface(surf, SDL_PIXELFORMAT_RGBA32);
                SDL_DestroySurface(surf);
                if (rgba) {
                    const int sz = rgba->w * rgba->h * 4;
                    lock.pixels.resize(sz);
                    std::memcpy(lock.pixels.data(), rgba->pixels, sz);
                    lock.width  = rgba->w;
                    lock.height = rgba->h;
                    SDL_DestroySurface(rgba);
                    lock.locked = true;
                    return;
                }
            }
        }
        lock.pixels.assign(n, 0);
        lock.locked = true;

    } else if (buf >= BB_TEX_BUF_BASE_) {
        lock.locked = bb_canvas_lock_copy_(buf, lock.pixels, lock.width, lock.height);

    } else {
        int img_h = 0, img_frame = 0;
        if (!bb_decode_img_buf_(buf, img_h, img_frame)) return;
        if (!bb_img_ok_(img_h)) return;

        auto& img = bb_images_[img_h];
        if (img_frame < 0 || img_frame >= static_cast<int>(img.frames.size()))
            img_frame = 0;
        auto& fd        = img.frames[img_frame];
        lock.width      = img.width;
        lock.height     = img.height;
        lock.img_h      = img_h;
        lock.img_frame  = img_frame;
        const int n     = img.width * img.height * 4;

        if (!fd.pixels.empty()) {
            // Ein Image hat im Original keinen Alphakanal, die Maske ist ein
            // Farbschluessel: ReadPixelFast liefert auch fuer maskierte Pixel
            // Alpha FF (gemessen, BUG-172). Beim Entsperren entsteht das
            // Alpha wieder aus der Maskenfarbe.
            lock.pixels = fd.pixels;
            for (size_t i = 3; i < lock.pixels.size(); i += 4) lock.pixels[i] = 255;
            lock.locked = true;
        } else if (bb_renderer_ && fd.tex) {
            if (SDL_SetRenderTarget(bb_renderer_, fd.tex)) {
                SDL_Surface* surf = SDL_RenderReadPixels(bb_renderer_, nullptr);
                SDL_SetRenderTarget(bb_renderer_, nullptr);
                if (surf) {
                    SDL_Surface* rgba = SDL_ConvertSurface(surf, SDL_PIXELFORMAT_RGBA32);
                    SDL_DestroySurface(surf);
                    if (rgba) {
                        lock.pixels.resize(rgba->w * rgba->h * 4);
                        std::memcpy(lock.pixels.data(), rgba->pixels, lock.pixels.size());
                        SDL_DestroySurface(rgba);
                        lock.locked = true;
                        return;
                    }
                }
                SDL_SetRenderTarget(bb_renderer_, nullptr);
            } else {
                SDL_SetRenderTarget(bb_renderer_, nullptr);
            }
            lock.pixels.assign(n, 0);
            lock.locked = true;
        } else {
            if (n > 0) lock.pixels.assign(n, 0);
            lock.locked = (n > 0);
        }
    }
}

// ---- bb_UnlockBuffer(buf) ----

inline void bb_UnlockBuffer(int buf = bb_active_buffer_) {
    auto it = bb_buf_locks_.find(buf);
    if (it == bb_buf_locks_.end() || !it->second.locked) return;
    bb_BufLock_& lock = it->second;

    if (lock.dirty) {
        if (buf == BB_BACK_BUFFER_H || buf == BB_FRONT_BUFFER_H) {
            if (bb_renderer_ && !lock.pixels.empty()) {
                SDL_Surface* surf = SDL_CreateSurfaceFrom(
                    lock.width, lock.height, SDL_PIXELFORMAT_RGBA32,
                    lock.pixels.data(), lock.width * 4);
                if (surf) {
                    SDL_Texture* tex = SDL_CreateTextureFromSurface(bb_renderer_, surf);
                    SDL_DestroySurface(surf);
                    if (tex) {
                        SDL_SetRenderTarget(bb_renderer_, nullptr);
                        SDL_RenderTexture(bb_renderer_, tex, nullptr, nullptr);
                        SDL_DestroyTexture(tex);
                    }
                }
            }
        } else if (buf >= BB_TEX_BUF_BASE_) {
            bb_canvas_unlock_copy_(buf, lock.pixels);
        } else if (lock.img_h > 0 && bb_img_ok_(lock.img_h)) {
            auto& img = bb_images_[lock.img_h];
            int   f   = lock.img_frame;
            if (f < 0 || f >= static_cast<int>(img.frames.size())) f = 0;
            auto& fd  = img.frames[f];
            fd.pixels = lock.pixels;
            for (size_t i = 0; i + 3 < fd.pixels.size(); i += 4) {
                const int rgb = (fd.pixels[i] << 16) | (fd.pixels[i + 1] << 8) | fd.pixels[i + 2];
                fd.pixels[i + 3] = (rgb == img.mask) ? 0 : 255;
            }
            if (bb_renderer_) {
                if (fd.tex) { SDL_DestroyTexture(fd.tex); fd.tex = nullptr; }
                SDL_Surface* surf = SDL_CreateSurfaceFrom(
                    lock.width, lock.height, SDL_PIXELFORMAT_RGBA32,
                    fd.pixels.data(), lock.width * 4);
                if (surf) {
                    SDL_SetSurfaceBlendMode(surf, SDL_BLENDMODE_NONE);
                    fd.tex = SDL_CreateTextureFromSurface(bb_renderer_, surf);
                    if (fd.tex)
                        SDL_SetTextureBlendMode(fd.tex, SDL_BLENDMODE_BLEND);
                    SDL_DestroySurface(surf);
                }
            }
        }
    }

    lock.locked = false;
    lock.dirty  = false;
}

// ---- Internal: pixel pointer helpers ----

inline uint8_t* bb_buf_pixel_(bb_BufLock_& lock, int x, int y) {
    if (x < 0 || y < 0 || x >= lock.width || y >= lock.height) return nullptr;
    return lock.pixels.data() + (y * lock.width + x) * 4;
}

inline uint8_t* bb_buf_pixel_fast_(bb_BufLock_& lock, int x, int y) {
    return lock.pixels.data() + (y * lock.width + x) * 4;
}

inline void bb_pixel_write_(uint8_t* p, int color) {
    p[0] = static_cast<uint8_t>((color >> 16) & 0xFF);
    p[1] = static_cast<uint8_t>((color >>  8) & 0xFF);
    p[2] = static_cast<uint8_t>( color        & 0xFF);
    const uint8_t a = static_cast<uint8_t>((color >> 24) & 0xFF);
    p[3] = (a == 0) ? 255 : a;
}

// Texturpuffer mit Alphakanal behalten das Alpha so, wie es geschrieben
// wird; ohne Alphakanal ist es 255 (BUG-127).
inline bool bb_canvas_keeps_alpha_(int buf);
inline void bb_pixel_write_buf_(uint8_t* p, int color, int buf) {
    bb_pixel_write_(p, color);
    if (buf >= BB_TEX_BUF_BASE_)
        p[3] = bb_canvas_keeps_alpha_(buf) ? static_cast<uint8_t>((color >> 24) & 0xFF) : 255;
}

inline int bb_pixel_read_(const uint8_t* p) {
    return (static_cast<int>(p[3]) << 24) |
           (static_cast<int>(p[0]) << 16) |
           (static_cast<int>(p[1]) <<  8) |
            static_cast<int>(p[2]);
}

// Der Buffer ist optional; ohne Angabe gilt der zuletzt mit SetBuffer
// gesetzte. Im Original sind das `ReadPixel ( x,y[,buffer] )` (BUG-44).
inline int bb_ReadPixel(int x, int y, int buf = bb_active_buffer_) {
    auto it = bb_buf_locks_.find(buf);
    if (it == bb_buf_locks_.end() || !it->second.locked) {
        // Ohne LockBuffer sperrt ReadPixel im Original selbst (lock, getPixel,
        // unlock). Bis BUG-63 lieferte es hier ungesperrt immer 0. Fuer den
        // Bildschirm genuegt ein einzelner Bildpunkt statt des ganzen Puffers.
        if (buf == BB_BACK_BUFFER_H || buf == BB_FRONT_BUFFER_H) {
            if (!bb_renderer_ || x < 0 || y < 0 ||
                x >= bb_gfx_width_ || y >= bb_gfx_height_) return 0;
            SDL_Rect rect = { x, y, 1, 1 };
            SDL_Surface* surf = SDL_RenderReadPixels(bb_renderer_, &rect);
            if (!surf) return 0;
            Uint8 px[4] = { 0, 0, 0, 255 };
            SDL_ReadSurfacePixel(surf, 0, 0, &px[0], &px[1], &px[2], &px[3]);
            SDL_DestroySurface(surf);
            return bb_pixel_read_(px);
        }
        return bb_canvas_read_pixel_(buf, x, y);
    }
    const uint8_t* p = bb_buf_pixel_(it->second, x, y);
    return p ? bb_pixel_read_(p) : 0;
}

inline void bb_WritePixel(int x, int y, int color, int buf = bb_active_buffer_) {
    auto it = bb_buf_locks_.find(buf);
    if (it == bb_buf_locks_.end() || !it->second.locked) {
        // Wie ReadPixel: ungesperrt sperrt WritePixel selbst (BUG-63). Auf dem
        // Bildschirm ist das ein einzelner Punkt in genau dieser Farbe.
        if (buf == BB_BACK_BUFFER_H || buf == BB_FRONT_BUFFER_H) {
            if (!bb_renderer_) return;
            SDL_SetRenderDrawColor(bb_renderer_, (color >> 16) & 0xFF,
                                   (color >> 8) & 0xFF, color & 0xFF, 255);
            SDL_RenderPoint(bb_renderer_, (float)x, (float)y);
            return;
        }
        bb_canvas_write_pixel_(buf, x, y, color);
        return;
    }
    uint8_t* p = bb_buf_pixel_(it->second, x, y);
    if (!p) return;
    bb_pixel_write_buf_(p, color, buf);
    it->second.dirty = true;
}

inline int bb_ReadPixelFast(int x, int y, int buf = bb_active_buffer_) {
    auto it = bb_buf_locks_.find(buf);
    if (it == bb_buf_locks_.end() || !it->second.locked) return 0;
    return bb_pixel_read_(bb_buf_pixel_fast_(it->second, x, y));
}

inline void bb_WritePixelFast(int x, int y, int color, int buf = bb_active_buffer_) {
    auto it = bb_buf_locks_.find(buf);
    if (it == bb_buf_locks_.end() || !it->second.locked) return;
    bb_pixel_write_buf_(bb_buf_pixel_fast_(it->second, x, y), color, buf);
    it->second.dirty = true;
}

// Der Buffer ist optional; ohne Angabe gilt der zuletzt mit SetBuffer
// gesetzte. Im Original sind das `CopyPixel src_x,src_y,src_buffer,dest_x,dest_y[,dest_buffer]` (BUG-44).
inline void bb_CopyPixel(int sx, int sy, int sbuf,
                          int dx, int dy, int dbuf = bb_active_buffer_) {
    auto sit = bb_buf_locks_.find(sbuf);
    auto dit = bb_buf_locks_.find(dbuf);
    if (sit == bb_buf_locks_.end() || !sit->second.locked) return;
    if (dit == bb_buf_locks_.end() || !dit->second.locked) return;
    const uint8_t* sp = bb_buf_pixel_(sit->second, sx, sy);
    uint8_t*       dp = bb_buf_pixel_(dit->second, dx, dy);
    if (!sp || !dp) return;
    dp[0] = sp[0]; dp[1] = sp[1]; dp[2] = sp[2]; dp[3] = sp[3];
    dit->second.dirty = true;
}

inline void bb_CopyPixelFast(int sx, int sy, int sbuf,
                               int dx, int dy, int dbuf = bb_active_buffer_) {
    auto sit = bb_buf_locks_.find(sbuf);
    auto dit = bb_buf_locks_.find(dbuf);
    if (sit == bb_buf_locks_.end() || !sit->second.locked) return;
    if (dit == bb_buf_locks_.end() || !dit->second.locked) return;
    const uint8_t* sp = bb_buf_pixel_fast_(sit->second, sx, sy);
    uint8_t*       dp = bb_buf_pixel_fast_(dit->second, dx, dy);
    dp[0] = sp[0]; dp[1] = sp[1]; dp[2] = sp[2]; dp[3] = sp[3];
    dit->second.dirty = true;
}

inline int bb_LoadBuffer(int buf, const bbString& file) {
    auto it = bb_buf_locks_.find(buf);
    if (it == bb_buf_locks_.end() || !it->second.locked) return 0;
    bb_BufLock_& lock = it->second;

    int w = 0, h = 0, ch = 0;
    unsigned char* data = bb_load_rgba_(file.c_str(), &w, &h, &ch);
    if (!data) return 0;

    lock.width  = w;
    lock.height = h;
    lock.pixels.assign(data, data + w * h * 4);
    lock.dirty  = true;
    stbi_image_free(data);

    if (lock.img_h > 0 && bb_img_ok_(lock.img_h)) {
        bb_images_[lock.img_h].width  = w;
        bb_images_[lock.img_h].height = h;
    }
    return 1;
}

// ---- SaveBuffer / SaveImage (BUG-167) ----
//
// Beide schreiben im Original ueber saveCanvas (bbgraphics.cpp): immer eine
// unkomprimierte 24-Bit-BMP, egal welche Endung der Name hat, Zeilen von
// unten nach oben, je Zeile auf 4 Byte aufgefuellt; im Kopf sind nur Typ,
// Groessen, Offset, Breite, Hoehe, Ebenen und Bittiefe gesetzt, der Rest 0.
// Der Puffer muss nicht gesperrt sein - saveCanvas sperrt selbst. Am
// Original gemessen (2026-09-23, build/save20260923): Back-, Front-, Bild-
// und Texturpuffer, gesperrt und ungesperrt, ".png" - alle Dateien
// byteweise wie hier.

// Den Inhalt eines Puffers als RGBA holen, ohne eine bestehende Sperre zu
// beruehren: gesperrt aus der Sperre (mit allem, was WritePixelFast seither
// geschrieben hat), sonst ueber eine kurze eigene Sperre.
inline bool bb_buf_snapshot_(int buf, std::vector<uint8_t>& px, int& w, int& h) {
    auto it = bb_buf_locks_.find(buf);
    if (it != bb_buf_locks_.end() && it->second.locked) {
        px = it->second.pixels; w = it->second.width; h = it->second.height;
    } else {
        const bool had = (it != bb_buf_locks_.end());
        const bb_BufLock_ saved = had ? it->second : bb_BufLock_{};
        bb_LockBuffer(buf);
        bb_BufLock_& lock = bb_buf_locks_[buf];
        const bool ok = lock.locked;
        px = std::move(lock.pixels); w = lock.width; h = lock.height;
        if (had) bb_buf_locks_[buf] = saved; else bb_buf_locks_.erase(buf);
        if (!ok) return false;
    }
    return w > 0 && h > 0 && px.size() >= static_cast<size_t>(w) * h * 4;
}

inline bool bb_save_bmp_(const bbString& file, const std::vector<uint8_t>& px,
                         int w, int h) {
    FILE* f = std::fopen(file.c_str(), "wb");
    if (!f) return false;
    const uint32_t stride = (static_cast<uint32_t>(w) * 3 + 3) & ~3u;
    const uint32_t off    = 14 + 40;
    const uint32_t size   = off + stride * static_cast<uint32_t>(h);
    uint8_t hd[54] = { 0 };
    auto put16 = [&](int o, uint32_t v) { hd[o] = v & 0xFF; hd[o + 1] = (v >> 8) & 0xFF; };
    auto put32 = [&](int o, uint32_t v) { put16(o, v & 0xFFFF); put16(o + 2, v >> 16); };
    hd[0] = 'B'; hd[1] = 'M';
    put32(2, size);
    put32(10, off);
    put32(14, 40);
    put32(18, static_cast<uint32_t>(w));
    put32(22, static_cast<uint32_t>(h));
    put16(26, 1);
    put16(28, 24);
    bool ok = std::fwrite(hd, 1, sizeof(hd), f) == sizeof(hd);
    std::vector<uint8_t> row(stride, 0);
    for (int y = h - 1; ok && y >= 0; --y) {
        const uint8_t* s = px.data() + static_cast<size_t>(y) * w * 4;
        for (int x = 0; x < w; ++x) {
            row[x * 3 + 0] = s[x * 4 + 2];
            row[x * 3 + 1] = s[x * 4 + 1];
            row[x * 3 + 2] = s[x * 4 + 0];
        }
        ok = std::fwrite(row.data(), 1, stride, f) == stride;
    }
    return (std::fclose(f) == 0) && ok;
}

inline int bb_SaveBuffer(int buf, const bbString& file) {
    std::vector<uint8_t> px;
    int w = 0, h = 0;
    if (!bb_buf_snapshot_(buf, px, w, h)) return 0;
    return bb_save_bmp_(file, px, w, h) ? 1 : 0;
}

// ---- SaveImage(handle, file [,frame=0]) → 1 / 0 ----
// Im Original saveCanvas auf das Frame - dieselbe BMP wie SaveBuffer.
// Bis 2026-09-23 schrieb es hier PNG.
inline int bb_SaveImage(int handle, const bbString& file, int frame = 0) {
    if (!bb_img_ok_(handle)) return 0;
    if (!bb_img_frame_(handle, frame)) return 0;
    return bb_SaveBuffer(bb_ImageBuffer(handle, frame), file);
}

inline int bb_canvas_size_(int buf, bool width);

inline int bb_BufferWidth(int buf) {
    auto it = bb_buf_locks_.find(buf);
    if (it != bb_buf_locks_.end() && (it->second.locked || it->second.width > 0))
        return it->second.width;
    if (buf == BB_BACK_BUFFER_H || buf == BB_FRONT_BUFFER_H)
        return bb_gfx_width_;
    if (buf >= BB_TEX_BUF_BASE_) return bb_canvas_size_(buf, true);
    int img_h = 0, frame = 0;
    if (!bb_decode_img_buf_(buf, img_h, frame)) return 0;
    return bb_img_ok_(img_h) ? bb_images_[img_h].width : 0;
}

inline int bb_BufferHeight(int buf) {
    auto it = bb_buf_locks_.find(buf);
    if (it != bb_buf_locks_.end() && (it->second.locked || it->second.height > 0))
        return it->second.height;
    if (buf == BB_BACK_BUFFER_H || buf == BB_FRONT_BUFFER_H)
        return bb_gfx_height_;
    if (buf >= BB_TEX_BUF_BASE_) return bb_canvas_size_(buf, false);
    int img_h = 0, frame = 0;
    if (!bb_decode_img_buf_(buf, img_h, frame)) return 0;
    return bb_img_ok_(img_h) ? bb_images_[img_h].height : 0;
}

// ==========================================================================
// M46b: Animated Images & Image API Completion
// ==========================================================================

// ---- LoadAnimImage(file, fw, fh, first, count) → handle ----
//
// Loads a sprite strip and slices it into `count` frames of size fw×fh.
// Cells are arranged left-to-right, top-to-bottom in the source image.
// `first` is the 0-based index of the first cell to include.
// Returns 0 on failure (file not found, invalid dimensions).

inline int bb_LoadAnimImage(const bbString& file,
                             int fw, int fh, int first, int count) {
    if (fw <= 0 || fh <= 0 || count <= 0) return 0;

    int sw = 0, sh = 0, ch = 0;
    unsigned char* src = bb_load_rgba_(file.c_str(), &sw, &sh, &ch);
    if (!src) return 0;
    bb_img_mask_black_(src, sw, sh);

    const int cols = sw / fw;
    if (cols < 1) { stbi_image_free(src); return 0; }

    bb_Image_ img;
    img.width  = fw;
    img.height = fh;
    img.valid  = true;

    for (int i = 0; i < count; ++i) {
        int cell = first + i;
        int cx   = (cell % cols) * fw;
        int cy   = (cell / cols) * fh;

        bb_FrameData_ fd;
        fd.pixels.resize(fw * fh * 4, 0);

        for (int row = 0; row < fh; ++row) {
            int sr = cy + row;
            if (sr >= sh) continue;
            int src_off  = (sr * sw + cx) * 4;
            int dst_off  = row * fw * 4;
            int copy_w   = std::min(fw, sw - cx);
            if (copy_w > 0)
                std::memcpy(fd.pixels.data() + dst_off,
                            src + src_off, copy_w * 4);
        }

        if (bb_renderer_) {
            SDL_Surface* surf = SDL_CreateSurfaceFrom(
                fw, fh, SDL_PIXELFORMAT_RGBA32, fd.pixels.data(), fw * 4);
            if (surf) {
                SDL_SetSurfaceBlendMode(surf, SDL_BLENDMODE_NONE);
                fd.tex = SDL_CreateTextureFromSurface(bb_renderer_, surf);
                if (fd.tex)
                    SDL_SetTextureBlendMode(fd.tex, SDL_BLENDMODE_BLEND);
                SDL_DestroySurface(surf);
            }
        }

        if (bb_auto_mid_handle_) {
            fd.handle_x = fw / 2;
            fd.handle_y = fh / 2;
        }

        img.frames.push_back(std::move(fd));
    }

    stbi_image_free(src);
    bb_images_.push_back(std::move(img));
    return static_cast<int>(bb_images_.size()) - 1;
}

// ---- GrabImage(handle, x, y [,frame=0]) ----
//
// Copies ImageWidth×ImageHeight pixels from the current back buffer at (x,y)
// into the specified frame's pixel store and texture.

inline void bb_GrabImage(int handle, int x, int y, int frame = 0) {
    if (!bb_img_ok_(handle)) return;
    bb_FrameData_* fd = bb_img_frame_(handle, frame);
    if (!fd) return;
    const auto& img = bb_images_[handle];
    int w = img.width, h = img.height;
    if (w <= 0 || h <= 0) return;

    fd->pixels.assign(w * h * 4, 0);

    if (bb_renderer_) {
        SDL_Rect rect = { x, y, w, h };
        SDL_Surface* surf = SDL_RenderReadPixels(bb_renderer_, &rect);
        if (surf) {
            SDL_Surface* rgba = SDL_ConvertSurface(surf, SDL_PIXELFORMAT_RGBA32);
            SDL_DestroySurface(surf);
            if (rgba) {
                int sz = std::min(static_cast<int>(fd->pixels.size()),
                                  rgba->w * rgba->h * 4);
                std::memcpy(fd->pixels.data(), rgba->pixels, sz);
                SDL_DestroySurface(rgba);
            }
        }
    }

    bb_img_reupload_frame_(handle, fd);
}

// ---- CopyImage(handle) → new handle ----
//
// Deep-copies the entire image (all frames) into a new image handle.

inline int bb_CopyImage(int handle) {
    if (!bb_img_ok_(handle)) return 0;
    const auto& src = bb_images_[handle];

    bb_Image_ img;
    img.width  = src.width;
    img.height = src.height;
    img.valid  = true;

    for (const auto& sfd : src.frames) {
        bb_FrameData_ fd;
        fd.handle_x = sfd.handle_x;
        fd.handle_y = sfd.handle_y;
        fd.scale_x  = sfd.scale_x;
        fd.scale_y  = sfd.scale_y;
        fd.rotation = sfd.rotation;
        fd.pixels   = sfd.pixels;

        if (bb_renderer_ && !fd.pixels.empty()) {
            SDL_Surface* surf = SDL_CreateSurfaceFrom(
                src.width, src.height, SDL_PIXELFORMAT_RGBA32,
                fd.pixels.data(), src.width * 4);
            if (surf) {
                SDL_SetSurfaceBlendMode(surf, SDL_BLENDMODE_NONE);
                fd.tex = SDL_CreateTextureFromSurface(bb_renderer_, surf);
                if (fd.tex)
                    SDL_SetTextureBlendMode(fd.tex, SDL_BLENDMODE_BLEND);
                SDL_DestroySurface(surf);
            }
        } else if (bb_renderer_ && sfd.tex) {
            // Render-target frame: read back pixels first
            if (SDL_SetRenderTarget(bb_renderer_,
                    const_cast<SDL_Texture*>(sfd.tex))) {
                SDL_Surface* s = SDL_RenderReadPixels(bb_renderer_, nullptr);
                SDL_SetRenderTarget(bb_renderer_, nullptr);
                if (s) {
                    SDL_Surface* r = SDL_ConvertSurface(s, SDL_PIXELFORMAT_RGBA32);
                    SDL_DestroySurface(s);
                    if (r) {
                        fd.pixels.resize(r->w * r->h * 4);
                        std::memcpy(fd.pixels.data(), r->pixels, fd.pixels.size());
                        SDL_DestroySurface(r);
                        SDL_Surface* s2 = SDL_CreateSurfaceFrom(
                            src.width, src.height, SDL_PIXELFORMAT_RGBA32,
                            fd.pixels.data(), src.width * 4);
                        if (s2) {
                            SDL_SetSurfaceBlendMode(s2, SDL_BLENDMODE_NONE);
                            fd.tex = SDL_CreateTextureFromSurface(bb_renderer_, s2);
                            if (fd.tex)
                                SDL_SetTextureBlendMode(fd.tex, SDL_BLENDMODE_BLEND);
                            SDL_DestroySurface(s2);
                        }
                    }
                }
            } else {
                SDL_SetRenderTarget(bb_renderer_, nullptr);
            }
        }

        img.frames.push_back(std::move(fd));
    }

    bb_images_.push_back(std::move(img));
    return static_cast<int>(bb_images_.size()) - 1;
}

// ---- FlipImage(handle [,frame=0]) ----
//
// Vertical flip (top↔bottom) in-place on the specified frame.

inline void bb_FlipImage(int handle, int frame = 0) {
    if (!bb_img_ok_(handle)) return;
    bb_FrameData_* fd = bb_img_frame_(handle, frame);
    if (!fd || fd->pixels.empty()) return;
    const auto& img = bb_images_[handle];
    int w = img.width, h = img.height;

    std::vector<uint8_t> row(w * 4);
    for (int y = 0; y < h / 2; ++y) {
        uint8_t* top = fd->pixels.data() + y           * w * 4;
        uint8_t* bot = fd->pixels.data() + (h - 1 - y) * w * 4;
        std::memcpy(row.data(), top,      w * 4);
        std::memcpy(top,        bot,      w * 4);
        std::memcpy(bot,        row.data(), w * 4);
    }

    bb_img_reupload_frame_(handle, fd);
}

// ---- MirrorImage(handle [,frame=0]) ----
//
// Horizontal mirror (left↔right) in-place on the specified frame.

inline void bb_MirrorImage(int handle, int frame = 0) {
    if (!bb_img_ok_(handle)) return;
    bb_FrameData_* fd = bb_img_frame_(handle, frame);
    if (!fd || fd->pixels.empty()) return;
    const auto& img = bb_images_[handle];
    int w = img.width, h = img.height;

    for (int y = 0; y < h; ++y) {
        uint8_t* row = fd->pixels.data() + y * w * 4;
        for (int x = 0; x < w / 2; ++x) {
            uint8_t* l = row + x           * 4;
            uint8_t* r = row + (w - 1 - x) * 4;
            std::swap(l[0], r[0]);
            std::swap(l[1], r[1]);
            std::swap(l[2], r[2]);
            std::swap(l[3], r[3]);
        }
    }

    bb_img_reupload_frame_(handle, fd);
}

#endif // BLITZNEXT_BB_IMAGE_H

// Zeichnen in Image- und Texturpuffer; braucht alles oben (BUG-141).
#include "bb_canvas.h"
