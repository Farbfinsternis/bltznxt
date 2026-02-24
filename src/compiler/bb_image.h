#ifndef BLITZNEXT_BB_IMAGE_H
#define BLITZNEXT_BB_IMAGE_H

#include <vector>
#include <unordered_map>
#include <cmath>
#include <cstring>
#include "bb_sdl.h"    // bb_renderer_, bb_gfx_width_, bb_gfx_height_
#include "bb_string.h" // bbString

// ---- stb_image (single-header, public domain) ----
//
// Provides PNG, JPG, BMP, TGA, GIF, PSD, HDR, PIC loading.
// STB_IMAGE_IMPLEMENTATION is defined here because bb_image.h is included
// exactly once per generated translation unit (via bb_runtime.h).

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG       // human-readable error messages
#include "../thirdparty/stb/stb_image.h"

// ---- stb_image_write (single-header, public domain) ----
//
// Used by bb_SaveImage to write PNG files.

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../thirdparty/stb/stb_image_write.h"

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
};

struct bb_Image_ {
    int  width  = 0;   // per-frame cell width  (same for all frames)
    int  height = 0;   // per-frame cell height (same for all frames)
    bool valid  = false;
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

// Returns a pointer to the requested frame (clamped to [0, frames.size()-1]).
// Returns nullptr if the handle is invalid or has no frames.
inline bb_FrameData_* bb_img_frame_(int handle, int frame) {
    if (!bb_img_ok_(handle)) return nullptr;
    auto& img = bb_images_[handle];
    if (img.frames.empty()) return nullptr;
    if (frame < 0 || frame >= static_cast<int>(img.frames.size())) frame = 0;
    return &img.frames[frame];
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
    unsigned char* data = stbi_load(file.c_str(), &w, &h, &ch, 4);
    if (!data) return 0;

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

inline int bb_ImageWidth(int handle, int frame = 0) {
    (void)frame;
    return bb_img_ok_(handle) ? bb_images_[handle].width : 0;
}
inline int bb_ImageHeight(int handle, int frame = 0) {
    (void)frame;
    return bb_img_ok_(handle) ? bb_images_[handle].height : 0;
}

// ---- DrawImage(handle, x, y [,frame=0]) ----

inline void bb_DrawImage(int handle, int x, int y, int frame = 0) {
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
    if (!bb_renderer_ || !bb_img_ok_(handle)) return;
    const bb_FrameData_* fd = bb_img_frame_(handle, frame);
    if (!fd || !fd->tex) return;
    const auto& img = bb_images_[handle];

    float dw = img.width  * fd->scale_x;
    float dh = img.height * fd->scale_y;
    SDL_FRect dst = { static_cast<float>(x), static_cast<float>(y), dw, dh };

    if (fd->rotation != 0.0f) {
        SDL_FPoint center = { dw * 0.5f, dh * 0.5f };
        SDL_RenderTextureRotated(bb_renderer_, fd->tex, nullptr, &dst,
                                 static_cast<double>(fd->rotation),
                                 &center, SDL_FLIP_NONE);
    } else {
        SDL_RenderTexture(bb_renderer_, fd->tex, nullptr, &dst);
    }
}

// ---- DrawBlockRect(handle, x, y, sx, sy, sw, sh [,frame=0]) ----

inline void bb_DrawBlockRect(int handle, int x, int y,
                              int sx, int sy, int sw, int sh,
                              int frame = 0) {
    bb_DrawImageRect(handle, x, y, sx, sy, sw, sh, frame);
}

// ==========================================================================
// M45: Image Manipulation
// ==========================================================================

// ---- HandleImage / MidHandle / AutoMidHandle ----

inline void bb_HandleImage(int handle, int hx, int hy, int frame = 0) {
    bb_FrameData_* fd = bb_img_frame_(handle, frame);
    if (!fd) return;
    fd->handle_x = hx;
    fd->handle_y = hy;
}

inline void bb_MidHandle(int handle, int frame = 0) {
    if (!bb_img_ok_(handle)) return;
    const auto& img = bb_images_[handle];
    bb_FrameData_* fd = bb_img_frame_(handle, frame);
    if (!fd) return;
    fd->handle_x = img.width  / 2;
    fd->handle_y = img.height / 2;
}

inline void bb_AutoMidHandle(int on) {
    bb_auto_mid_handle_ = (on != 0);
}

// ---- ImageXHandle / ImageYHandle ----

inline int bb_ImageXHandle(int handle, int frame = 0) {
    const bb_FrameData_* fd = bb_img_frame_(handle, frame);
    return fd ? fd->handle_x : 0;
}
inline int bb_ImageYHandle(int handle, int frame = 0) {
    const bb_FrameData_* fd = bb_img_frame_(handle, frame);
    return fd ? fd->handle_y : 0;
}

// ---- ScaleImage / RotateImage ----

inline void bb_ScaleImage(int handle, float sx, float sy, int frame = 0) {
    bb_FrameData_* fd = bb_img_frame_(handle, frame);
    if (!fd) return;
    fd->scale_x = (sx > 0.0f) ? sx : 0.0f;
    fd->scale_y = (sy > 0.0f) ? sy : 0.0f;
}

inline void bb_RotateImage(int handle, float deg, int frame = 0) {
    bb_FrameData_* fd = bb_img_frame_(handle, frame);
    if (!fd) return;
    fd->rotation = deg;
}

// ---- MaskImage(handle, r, g, b [,frame=0]) ----

inline void bb_MaskImage(int handle, int r, int g, int b, int frame = 0) {
    bb_FrameData_* fd = bb_img_frame_(handle, frame);
    if (!fd || fd->pixels.empty()) return;
    const auto& img = bb_images_[handle];
    const int n = img.width * img.height;
    uint8_t* p  = fd->pixels.data();
    for (int i = 0; i < n; ++i, p += 4) {
        if (p[0] == static_cast<uint8_t>(r) &&
            p[1] == static_cast<uint8_t>(g) &&
            p[2] == static_cast<uint8_t>(b)) {
            p[3] = 0;
        }
    }
    bb_img_reupload_frame_(handle, fd);
}

// ---- TileImage / TileBlock ----

inline void bb_TileImage(int handle, int x, int y, int frame = 0) {
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

inline void bb_TileBlock(int handle, int x, int y, int frame = 0) {
    if (!bb_renderer_ || !bb_img_ok_(handle)) return;
    const bb_FrameData_* fd = bb_img_frame_(handle, frame);
    const auto& img = bb_images_[handle];
    if (!fd || !fd->tex || img.width <= 0 || img.height <= 0) return;

    int gw = bb_gfx_width_, gh = bb_gfx_height_;
    int ox = (x % img.width  + img.width)  % img.width;
    int oy = (y % img.height + img.height) % img.height;

    for (int ty = ox - img.height; ty < gh; ty += img.height) {
        for (int tx = oy - img.width; tx < gw; tx += img.width) {
            SDL_FRect dst = { static_cast<float>(tx), static_cast<float>(ty),
                              static_cast<float>(img.width),
                              static_cast<float>(img.height) };
            SDL_RenderTexture(bb_renderer_, fd->tex, nullptr, &dst);
        }
    }
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

// ---- SaveImage(handle, file [,frame=0]) → 1 / 0 ----

inline int bb_SaveImage(int handle, const bbString& file, int frame = 0) {
    if (!bb_img_ok_(handle)) return 0;
    const bb_FrameData_* fd = bb_img_frame_(handle, frame);
    if (!fd) return 0;
    const auto& img = bb_images_[handle];

    if (!fd->pixels.empty()) {
        return stbi_write_png(file.c_str(),
                              img.width, img.height, 4,
                              fd->pixels.data(), img.width * 4) ? 1 : 0;
    }

    if (bb_renderer_ && fd->tex) {
        if (!SDL_SetRenderTarget(bb_renderer_,
                const_cast<SDL_Texture*>(fd->tex))) return 0;
        SDL_Surface* surf = SDL_RenderReadPixels(bb_renderer_, nullptr);
        SDL_SetRenderTarget(bb_renderer_, nullptr);
        if (!surf) return 0;
        SDL_Surface* rgba = SDL_ConvertSurface(surf, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(surf);
        if (!rgba) return 0;
        int ok = stbi_write_png(file.c_str(),
                                rgba->w, rgba->h, 4,
                                rgba->pixels, rgba->pitch);
        SDL_DestroySurface(rgba);
        return ok ? 1 : 0;
    }

    return 0;
}

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
    if (buf <= 2) return false;
    int raw = buf - BB_IMG_BUF_OFFSET_;           // = (img-1) + frame * STRIDE
    img_h = raw % BB_IMG_BUF_STRIDE_ + 1;         // 1-based
    frame = raw / BB_IMG_BUF_STRIDE_;
    return true;
}

// ---- bb_LockBuffer(buf) ----

inline void bb_LockBuffer(int buf) {
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
            lock.pixels = fd.pixels;
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

inline void bb_UnlockBuffer(int buf) {
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
        } else if (lock.img_h > 0 && bb_img_ok_(lock.img_h)) {
            auto& img = bb_images_[lock.img_h];
            int   f   = lock.img_frame;
            if (f < 0 || f >= static_cast<int>(img.frames.size())) f = 0;
            auto& fd  = img.frames[f];
            fd.pixels = lock.pixels;
            if (bb_renderer_) {
                if (fd.tex) { SDL_DestroyTexture(fd.tex); fd.tex = nullptr; }
                SDL_Surface* surf = SDL_CreateSurfaceFrom(
                    lock.width, lock.height, SDL_PIXELFORMAT_RGBA32,
                    lock.pixels.data(), lock.width * 4);
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

inline int bb_pixel_read_(const uint8_t* p) {
    return (static_cast<int>(p[3]) << 24) |
           (static_cast<int>(p[0]) << 16) |
           (static_cast<int>(p[1]) <<  8) |
            static_cast<int>(p[2]);
}

inline int bb_ReadPixel(int x, int y, int buf) {
    auto it = bb_buf_locks_.find(buf);
    if (it == bb_buf_locks_.end() || !it->second.locked) return 0;
    const uint8_t* p = bb_buf_pixel_(it->second, x, y);
    return p ? bb_pixel_read_(p) : 0;
}

inline void bb_WritePixel(int x, int y, int color, int buf) {
    auto it = bb_buf_locks_.find(buf);
    if (it == bb_buf_locks_.end() || !it->second.locked) return;
    uint8_t* p = bb_buf_pixel_(it->second, x, y);
    if (!p) return;
    bb_pixel_write_(p, color);
    it->second.dirty = true;
}

inline int bb_ReadPixelFast(int x, int y, int buf) {
    auto it = bb_buf_locks_.find(buf);
    if (it == bb_buf_locks_.end() || !it->second.locked) return 0;
    return bb_pixel_read_(bb_buf_pixel_fast_(it->second, x, y));
}

inline void bb_WritePixelFast(int x, int y, int color, int buf) {
    auto it = bb_buf_locks_.find(buf);
    if (it == bb_buf_locks_.end() || !it->second.locked) return;
    bb_pixel_write_(bb_buf_pixel_fast_(it->second, x, y), color);
    it->second.dirty = true;
}

inline void bb_CopyPixel(int sx, int sy, int sbuf,
                          int dx, int dy, int dbuf) {
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
                               int dx, int dy, int dbuf) {
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
    unsigned char* data = stbi_load(file.c_str(), &w, &h, &ch, 4);
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

inline int bb_SaveBuffer(int buf, const bbString& file) {
    auto it = bb_buf_locks_.find(buf);
    if (it == bb_buf_locks_.end() || !it->second.locked) return 0;
    const bb_BufLock_& lock = it->second;
    if (lock.pixels.empty() || lock.width <= 0 || lock.height <= 0) return 0;
    return stbi_write_png(file.c_str(),
                          lock.width, lock.height, 4,
                          lock.pixels.data(), lock.width * 4) ? 1 : 0;
}

inline int bb_BufferWidth(int buf) {
    auto it = bb_buf_locks_.find(buf);
    if (it != bb_buf_locks_.end() && (it->second.locked || it->second.width > 0))
        return it->second.width;
    if (buf == BB_BACK_BUFFER_H || buf == BB_FRONT_BUFFER_H)
        return bb_gfx_width_;
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
    unsigned char* src = stbi_load(file.c_str(), &sw, &sh, &ch, 4);
    if (!src) return 0;

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
