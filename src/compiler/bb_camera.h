#ifndef BB_CAMERA_H
#define BB_CAMERA_H

// Camera Entity (3D-06).
//
// Included from bb_graphics3d.h.
// Provides: bb_CameraEntity_, CreateCamera, CameraRange, CameraZoom,
//           CameraProjMode, CameraViewport, CameraClsMode, CameraClsColor,
//           bb_collect_cameras_() used by RenderWorld.

#include "bb_entity_core.h"   // entity base, math helpers, registry
#include "bb_gl_ctx.h"        // bb_gfx_width_/height_ via bb_graphics2d.h

#include <algorithm>

// ============================================================
// Camera entity struct
// ============================================================

struct bb_CameraEntity_ : bb_Entity_ {
  int   projMode = 1;        // 0=aus, 1=perspective, 2=ortho
  float near_    = 1.0f;     // near clip plane
  float far_     = 1000.0f;  // far clip plane
  float zoom     = 1.0f;     // zoom factor: 1.0 = 90° horizontal FOV

  // Viewport in pixels (0,0,0,0 = full window)
  int vpX = 0, vpY = 0, vpW = 0, vpH = 0;

  // Per-camera clear settings
  bool clsColor = true;
  bool clsZbuf  = true;
  float clsR = 0, clsG = 0, clsB = 0; // 0..255 wie im Aufruf, Float wie im Original (BUG-62)

  // Matrices computed each frame in RenderWorld
  float view[16];
  float proj[16];

  bb_CameraEntity_() { mat4_identity_(view); mat4_identity_(proj); }
  bb_EntityKind_ kind() const override { return bb_EntityKind_::Camera; }
};

// ============================================================
// Helper: downcast or return nullptr
// ============================================================

static inline bb_CameraEntity_* bb_cam_(int h) {
  bb_Entity_* e = bb_entity_get_(h);
  if (!e || e->kind() != bb_EntityKind_::Camera) return nullptr;
  return static_cast<bb_CameraEntity_*>(e);
}

// ============================================================
// CreateCamera
// ============================================================

inline int bb_CreateCamera(int parent = 0) {
  return bb_entity_register_(std::make_unique<bb_CameraEntity_>(), parent);
}

// ============================================================
// Camera settings
// ============================================================

inline void bb_CameraRange(int h, float near_clip, float far_clip) {
  if (auto* c = bb_cam_(h)) { c->near_ = near_clip; c->far_ = far_clip; }
}

// zoom=1 → 90° HFOV.  zoom=2 → ~53° HFOV.  Standard Blitz3D convention.
inline void bb_CameraZoom(int h, float zoom) {
  if (auto* c = bb_cam_(h); c && zoom > 0.0f) c->zoom = zoom;
}

inline void bb_CameraProjMode(int h, int mode) {
  if (auto* c = bb_cam_(h)) c->projMode = mode;
}

// Viewport in Blitz3D pixel coords (y=0 is top-left).
inline void bb_CameraViewport(int h, int x, int y, int w, int hh) {
  if (auto* c = bb_cam_(h)) { c->vpX = x; c->vpY = y; c->vpW = w; c->vpH = hh; }
}

// Per-camera cls mode. Ohne gueltige Kamera wirkungslos (BUG-137: der
// fruehere Ersatzzustand fuer RenderWorld ohne Kamera ist entfallen).
inline void bb_CameraClsMode(int h, int cls_color, int cls_zbuf) {
  if (auto* c = bb_cam_(h)) {
    c->clsColor = (cls_color != 0);
    c->clsZbuf  = (cls_zbuf  != 0);
  }
}

// Die Farbe ist im Original Float: "CameraClsColor%camera#red#green#blue",
// bbCameraClsColor rechnet r*ctof (1/255). Bis BUG-62 stand hier int, und
// ein "127.5" verlor still seinen Nachkommateil.
inline void bb_CameraClsColor(int h, float r, float g, float b) {
  if (auto* c = bb_cam_(h)) {
    c->clsR = r; c->clsG = g; c->clsB = b;
  }
}

// ============================================================
// Matrix builders
// ============================================================

// View matrix = inverse of camera world matrix, with Z-axis negated.
// Blitz3D cameras look toward +Z (local forward); OpenGL right-handed
// projection (m[11]=-1) expects objects in front at -Z in view space.
// Negating the Z row flips the convention without changing the projection.
static inline void bb_cam_view_(bb_CameraEntity_* c) {
  if (!mat4_inverse_(c->view, c->world))
    mat4_identity_(c->view);
  // Negate Z row (col-major row 2: indices 2, 6, 10, 14)
  c->view[2]  = -c->view[2];
  c->view[6]  = -c->view[6];
  c->view[10] = -c->view[10];
  c->view[14] = -c->view[14];
}

// Perspective: zoom=1/tan(hfov/2).  m[0]=zoom (HFOV), m[5]=zoom*(w/h) (VFOV).
// aspect = width/height (conventional).  For 800×600: aspect≈1.333 → VFOV narrower than HFOV.
static inline void bb_cam_proj_persp_(bb_CameraEntity_* c, float aspect) {
  float z = c->zoom, n = c->near_, f = c->far_;
  float* m = c->proj;
  memset(m, 0, 64);
  m[0]  = z;
  m[5]  = z * aspect;               // aspect = width/height
  m[10] = (f + n) / (n - f);
  m[11] = -1.0f;
  m[14] = (2.0f * f * n) / (n - f);
}

// Orthographic: maps ±(1/zoom) world units to NDC ±1, scaled by aspect.
static inline void bb_cam_proj_ortho_(bb_CameraEntity_* c, float aspect) {
  float z = c->zoom, n = c->near_, f = c->far_;
  float* m = c->proj;
  memset(m, 0, 64);
  m[0]  = z;
  m[5]  = z * aspect;
  m[10] = -2.0f / (f - n);
  m[14] = -(f + n) / (f - n);
  m[15] = 1.0f;
}

// ============================================================
// Collect visible cameras (sorted by render order) for RenderWorld
// ============================================================

// Eine Kamera mit CameraProjMode 0 ist abgeschaltet: sie rendert nicht und
// loescht auch nicht (gemessen 2026-09-17, BUG-137).
static inline std::vector<bb_CameraEntity_*> bb_collect_cameras_() {
  std::vector<bb_CameraEntity_*> cams;
  for (auto& [h, e] : bb_entities_) {
    if (e->kind() != bb_EntityKind_::Camera || !bb_entity_shown_(e.get()))
      continue;
    auto* c = static_cast<bb_CameraEntity_*>(e.get());
    if (c->projMode != 0) cams.push_back(c);
  }
  std::sort(cams.begin(), cams.end(),
            [](bb_CameraEntity_* a, bb_CameraEntity_* b) {
              return a->order < b->order;
            });
  return cams;
}

#endif // BB_CAMERA_H
