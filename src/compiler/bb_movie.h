#ifndef BLITZNEXT_BB_MOVIE_H
#define BLITZNEXT_BB_MOVIE_H

// Filme: OpenMovie, DrawMovie, CloseMovie, MovieWidth, MovieHeight,
// MoviePlaying (BUG-195).
//
// Nachbau von gxruntime/gxmovie.cpp: DirectShow Multimedia Streaming
// (amstream) mit eigenem Graph-Takt (AMMSF_NOGRAPHTHREAD) dekodiert in eine
// DirectDraw-Flaeche, der Ton laeuft ueber den Standard-Renderer. Damit
// spielt BLTZNXT dieselben Formate wie das Original - was DirectShow auf dem
// Rechner kann. Jedes DrawMovie holt das naechste Bild: das Update wartet bis
// zu dessen Zeit, ein spaetes Bild kommt sofort, keines wird uebersprungen;
// nach dem letzten liefert es 0 und zeichnet nichts mehr (gemessen
// 2026-10-26 an build/movie_20261026/film.avi, 10 Bilder zu 100 ms).
// Gezeichnet wird skaliert auf x,y,w,h des aktuellen Puffers wie Blt.
// Abweichung: Origin und Viewport gelten wie bei DrawBlock (Blt im Original
// kennt beide nicht). ole32 und ddraw werden erst beim ersten OpenMovie
// geladen, ein Programm ohne Film bindet nichts davon.

#ifdef _WIN32
#include <ddraw.h>
#include <mmstream.h>
#include <amstream.h>
#include <ddstream.h>
#endif

#include <cstdint>
#include <map>
#include <vector>

#ifdef _WIN32

struct bb_Movie_ {
  IMultiMediaStream*       mm = nullptr;
  IMediaStream*            vid = nullptr;
  IDirectDrawMediaStream*  dds = nullptr;
  IDirectDrawSurface*      surf = nullptr;
  IDirectDrawStreamSample* sample = nullptr;
  int  w = 0, h = 0;
  bool playing = true;
};

inline std::map<int, bb_Movie_> bb_movies_;
inline int bb_movie_next_ = 1;

// Ohne Import-Bibliotheken: CLSID und Strom-IDs aus amstream.h/mmstream.h.
inline const GUID bb_mv_clsid_ = { 0x49c47ce5, 0x9ba4, 0x11d0, { 0x82, 0x12, 0x00, 0xc0, 0x4f, 0xc3, 0x2c, 0x45 } };
inline const GUID bb_mv_video_ = { 0xa35ff56a, 0x9fda, 0x11d0, { 0x8f, 0xdf, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d } };
inline const GUID bb_mv_audio_ = { 0xa35ff56b, 0x9fda, 0x11d0, { 0x8f, 0xdf, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0x9d } };

struct bb_MovieApi_ {
  bool tried = false, ok = false;
  HRESULT (WINAPI* coinit)(void*, DWORD) = nullptr;
  HRESULT (WINAPI* cocreate)(REFCLSID, IUnknown*, DWORD, REFIID, void**) = nullptr;
  HRESULT (WINAPI* ddcreate)(GUID*, IDirectDraw**, IUnknown*) = nullptr;
  IDirectDraw* dd = nullptr;
};
inline bb_MovieApi_ bb_movie_api_;

inline bool bb_movie_init_() {
  bb_MovieApi_& a = bb_movie_api_;
  if (a.tried) return a.ok;
  a.tried = true;
  HMODULE ole = LoadLibraryA("ole32.dll"), ddr = LoadLibraryA("ddraw.dll");
  if (!ole || !ddr) return false;
  a.coinit   = reinterpret_cast<decltype(a.coinit)>(reinterpret_cast<void*>(GetProcAddress(ole, "CoInitializeEx")));
  a.cocreate = reinterpret_cast<decltype(a.cocreate)>(reinterpret_cast<void*>(GetProcAddress(ole, "CoCreateInstance")));
  a.ddcreate = reinterpret_cast<decltype(a.ddcreate)>(reinterpret_cast<void*>(GetProcAddress(ddr, "DirectDrawCreate")));
  if (!a.coinit || !a.cocreate || !a.ddcreate) return false;
  a.coinit(nullptr, COINIT_APARTMENTTHREADED);   // S_FALSE/RPC_E_CHANGED_MODE: COM laeuft schon
  if (a.ddcreate(nullptr, &a.dd, nullptr) != DD_OK) return false;
  if (a.dd->SetCooperativeLevel(nullptr, DDSCL_NORMAL) != DD_OK) return false;
  a.ok = true;
  return true;
}

inline void bb_movie_free_(bb_Movie_& m) {
  if (m.mm) m.mm->SetState(STREAMSTATE_STOP);
  if (m.sample) m.sample->Release();
  if (m.surf) m.surf->Release();
  if (m.dds) m.dds->Release();
  if (m.vid) m.vid->Release();
  if (m.mm) m.mm->Release();
  m = bb_Movie_{};
}

inline bb_Movie_& bb_movie_chk_(int h) {
  auto it = bb_movies_.find(h);
  if (it == bb_movies_.end()) bb_RuntimeError("Movie does not exist");
  return it->second;
}

// Flaeche und Probe wie der gxMovie-Konstruktor: Groesse aus GetFormat, das
// Ziel in 32 Bit (das Original nahm einen Canvas im Bildschirmformat);
// DirectShow wandelt beim Update um.
inline bool bb_movie_setup_(bb_Movie_& m) {
  if (m.mm->GetMediaStream(bb_mv_video_, &m.vid) != S_OK) return false;
  if (m.vid->QueryInterface(__uuidof(IDirectDrawMediaStream),
                            reinterpret_cast<void**>(&m.dds)) != S_OK) return false;
  DDSURFACEDESC fmt = {};
  fmt.dwSize = sizeof(fmt);
  if (m.dds->GetFormat(&fmt, nullptr, nullptr, nullptr) != S_OK) return false;
  m.w = static_cast<int>(fmt.dwWidth);
  m.h = static_cast<int>(fmt.dwHeight);
  if (m.w <= 0 || m.h <= 0) return false;
  DDSURFACEDESC sd = {};
  sd.dwSize = sizeof(sd);
  sd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
  sd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
  sd.dwWidth = fmt.dwWidth;
  sd.dwHeight = fmt.dwHeight;
  sd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
  sd.ddpfPixelFormat.dwFlags = DDPF_RGB;
  sd.ddpfPixelFormat.dwRGBBitCount = 32;
  sd.ddpfPixelFormat.dwRBitMask = 0xFF0000;
  sd.ddpfPixelFormat.dwGBitMask = 0x00FF00;
  sd.ddpfPixelFormat.dwBBitMask = 0x0000FF;
  if (bb_movie_api_.dd->CreateSurface(&sd, &m.surf, nullptr) != DD_OK) return false;
  RECT r = { 0, 0, m.w, m.h };
  if (m.dds->CreateSample(m.surf, &r, 0, &m.sample) != S_OK) return false;
  return m.mm->SetState(STREAMSTATE_RUN) == S_OK;
}

inline int bb_OpenMovie(const bbString& file) {
  if (!bb_movie_init_()) return 0;
  IAMMultiMediaStream* am = nullptr;
  if (bb_movie_api_.cocreate(bb_mv_clsid_, nullptr, CLSCTX_INPROC_SERVER,
                             __uuidof(IAMMultiMediaStream),
                             reinterpret_cast<void**>(&am)) != S_OK) return 0;
  bb_Movie_ m;
  m.mm = am;
  bool ok = am->Initialize(STREAMTYPE_READ, AMMSF_NOGRAPHTHREAD, nullptr) == S_OK &&
            am->AddMediaStream(bb_movie_api_.dd, &bb_mv_video_, 0, nullptr) == S_OK;
  if (ok) {
    am->AddMediaStream(nullptr, &bb_mv_audio_, AMMSF_ADDDEFAULTRENDERER, nullptr);
    std::wstring wpath(file.size() + 1, L'\0');
    const int n = MultiByteToWideChar(CP_ACP, 0, file.c_str(), -1, &wpath[0],
                                      static_cast<int>(wpath.size()));
    wpath.resize(n > 0 ? n - 1 : 0);
    // Der relative Pfad gilt wie bei den Ladern vom Arbeitsverzeichnis aus.
    wchar_t full[MAX_PATH * 4];
    const DWORD fl = GetFullPathNameW(wpath.c_str(), MAX_PATH * 4, full, nullptr);
    ok = am->OpenFile(fl > 0 && fl < MAX_PATH * 4 ? full : wpath.c_str(), 0) == S_OK &&
         bb_movie_setup_(m);
  }
  if (!ok) { bb_movie_free_(m); return 0; }
  const int h = bb_movie_next_++;
  bb_movies_[h] = m;
  return h;
}

// Das Bild aus der DirectDraw-Flaeche als RGBA (Bytefolge wie bb_Canvas_).
inline bool bb_movie_pixels_(bb_Movie_& m, std::vector<uint8_t>& px) {
  DDSURFACEDESC d = {};
  d.dwSize = sizeof(d);
  if (m.surf->Lock(nullptr, &d, DDLOCK_WAIT | DDLOCK_READONLY, nullptr) != DD_OK) return false;
  px.resize(static_cast<size_t>(m.w) * m.h * 4);
  for (int y = 0; y < m.h; ++y) {
    const uint8_t* s = static_cast<const uint8_t*>(d.lpSurface) + static_cast<size_t>(y) * d.lPitch;
    uint8_t* o = px.data() + static_cast<size_t>(y) * m.w * 4;
    for (int x = 0; x < m.w; ++x, s += 4, o += 4) {
      o[0] = s[2]; o[1] = s[1]; o[2] = s[0]; o[3] = 255;
    }
  }
  m.surf->Unlock(nullptr);
  return true;
}

// Skaliert wie Blt (naechster Nachbar) auf den aktuellen Puffer.
inline void bb_movie_blit_(bb_Movie_& m, const std::vector<uint8_t>& px,
                           int x, int y, int w, int h) {
  if (w <= 0 || h <= 0) return;
  if (bb_buf_is_screen_(bb_active_buffer_)) {
    bb_front_mark_(bb_active_buffer_);
    if (!bb_renderer_) return;
    SDL_Texture* t = SDL_CreateTexture(bb_renderer_, SDL_PIXELFORMAT_RGBA32,
                                       SDL_TEXTUREACCESS_STATIC, m.w, m.h);
    if (!t) return;
    SDL_UpdateTexture(t, nullptr, px.data(), m.w * 4);
    SDL_SetTextureScaleMode(t, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(t, bb_blockblend_());
    const SDL_FRect dst = { static_cast<float>(x), static_cast<float>(y),
                            static_cast<float>(w), static_cast<float>(h) };
    SDL_RenderTexture(bb_renderer_, t, nullptr, &dst);
    SDL_DestroyTexture(t);
    return;
  }
  bb_Canvas_ d;
  if (!bb_canvas_open_(bb_active_buffer_, d)) return;
  bb_CRect_ r = bb_crect_(x + d.st->ox, y + d.st->oy, w, h);
  if (bb_cclip_(d.st->vp, &r)) {
    const int ox = x + d.st->ox, oy = y + d.st->oy;
    for (int j = r.t; j < r.b; ++j) {
      const int sy = static_cast<int>(static_cast<int64_t>(j - oy) * m.h / h);
      for (int i = r.l; i < r.r; ++i) {
        const int sx = static_cast<int>(static_cast<int64_t>(i - ox) * m.w / w);
        const uint8_t* s = px.data() + (static_cast<size_t>(sy) * m.w + sx) * 4;
        bb_canvas_put_(d, i, j, (s[0] << 16) | (s[1] << 8) | s[2]);
      }
    }
  }
  bb_canvas_done_(d);
}

inline int bb_DrawMovie(int movie, int x = 0, int y = 0, int w = -1, int h = -1) {
  bb_Movie_& m = bb_movie_chk_(movie);
  if (w < 0) w = m.w;
  if (h < 0) h = m.h;
  if (m.playing) {
    if (m.sample->Update(0, nullptr, nullptr, 0) == S_OK) {
      std::vector<uint8_t> px;
      if (bb_movie_pixels_(m, px)) bb_movie_blit_(m, px, x, y, w, h);
    } else {
      m.playing = false;
    }
  }
  if (bb_idle_hook_) bb_idle_hook_();
  return m.playing ? 1 : 0;
}

inline int bb_MovieWidth(int movie)   { return bb_movie_chk_(movie).w; }
inline int bb_MovieHeight(int movie)  { return bb_movie_chk_(movie).h; }
inline int bb_MoviePlaying(int movie) { return bb_movie_chk_(movie).playing ? 1 : 0; }

inline void bb_CloseMovie(int movie) {
  auto it = bb_movies_.find(movie);
  if (it == bb_movies_.end()) return;   // closeMovie: unbekannt -> nichts
  bb_movie_free_(it->second);
  bb_movies_.erase(it);
}

#else  // ohne DirectShow: kein Film laesst sich oeffnen

inline int  bb_OpenMovie(const bbString&) { return 0; }
inline int  bb_DrawMovie(int, int = 0, int = 0, int = -1, int = -1) { bb_RuntimeError("Movie does not exist"); return 0; }
inline int  bb_MovieWidth(int)   { bb_RuntimeError("Movie does not exist"); return 0; }
inline int  bb_MovieHeight(int)  { bb_RuntimeError("Movie does not exist"); return 0; }
inline int  bb_MoviePlaying(int) { bb_RuntimeError("Movie does not exist"); return 0; }
inline void bb_CloseMovie(int) {}

#endif

#endif // BLITZNEXT_BB_MOVIE_H
