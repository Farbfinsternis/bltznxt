#ifndef BB_GRAPHICS3D_H
#define BB_GRAPHICS3D_H

// 3D scene control: UpdateWorld, RenderWorld, ClearWorld, scene globals.
// Includes bb_gl_ctx.h for GL access and bb_camera.h for Camera entities.

#include "bb_gl_ctx.h"
#include "bb_entity_core.h"

// ============================================================
// Global camera-clear fallback state
// Used by RenderWorld when no Camera entity exists (backward compat).
// Per-camera overrides live in bb_CameraEntity_ (bb_camera.h).
// ============================================================

inline bool bb_cam_cls_color_ = true;
inline bool bb_cam_cls_zbuf_  = true;
inline int  bb_cam_cls_r_     = 0;
inline int  bb_cam_cls_g_     = 0;
inline int  bb_cam_cls_b_     = 0;

// Global ambient light (uploaded to lit-shader uniform in 3D-12).
inline int bb_ambient_r_ = 0;
inline int bb_ambient_g_ = 0;
inline int bb_ambient_b_ = 0;

// Triangle counter — reset at start of RenderWorld, incremented by mesh draws.
inline int bb_tris_rendered_ = 0;

// bb_camera.h needs the fallback globals above to be declared first.
#include "bb_camera.h"
// bb_shader.h needs bb_gl_ctx.h symbols; include after camera.
#include "bb_shader.h"
#include "bb_mesh_core.h"
#include "bb_mesh.h"

// ============================================================
// UpdateWorld — propagate world transforms + future systems
// ============================================================

// Der Zeitschritt ist optional und wird noch nicht ausgewertet - im Original
// `UpdateWorld [elapsed_time#]` (BUG-44).
inline void bb_UpdateWorld(float elapsed_time = 1.0f) {
  (void)elapsed_time;
  bb_entity_update_all_();
  // 3D-18: collision detection (stub)
  // 3D-19: advance animation timers (stub)
}

// ============================================================
// RenderWorld
// ============================================================

// Der Tween-Faktor ist optional und wird noch nicht ausgewertet - im Original
// `RenderWorld [tween#]` (BUG-44).
inline void bb_RenderWorld(float tween = 1.0f) {
  (void)tween;
  if (!bb_gl_active_) return;

  // Lazy-compile shaders on first call (GL context must be active).
  if (!bb_shaders_ready_) bb_shaders_init_();

  bb_tris_rendered_ = 0;

  // Flush pending 2D draws (background sprites, etc.) before going GL.
  if (bb_renderer_) SDL_FlushRenderer(bb_renderer_);
  SDL_GL_MakeCurrent(bb_window_, bb_gl_ctx_);

  auto cams = bb_collect_cameras_();

  if (cams.empty()) {
    // No camera entity — use global fallback state (e.g. test_3d02).
    glViewport(0, 0, bb_gfx_width_, bb_gfx_height_);
    GLbitfield bits = 0;
    if (bb_cam_cls_color_) {
      glClearColor(bb_cam_cls_r_ / 255.0f,
                   bb_cam_cls_g_ / 255.0f,
                   bb_cam_cls_b_ / 255.0f, 1.0f);
      bits |= GL_COLOR_BUFFER_BIT;
    }
    if (bb_cam_cls_zbuf_) { glClearDepth(1.0); bits |= GL_DEPTH_BUFFER_BIT; }
    if (bits) glClear(bits);
    return;
  }

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  for (auto* cam : cams) {
    // Viewport — Blitz3D y=0 is top-left; GL y=0 is bottom-left, so flip.
    int vw = (cam->vpW > 0) ? cam->vpW : bb_gfx_width_;
    int vh = (cam->vpH > 0) ? cam->vpH : bb_gfx_height_;
    int gl_y = bb_gfx_height_ - cam->vpY - vh;
    glViewport(cam->vpX, gl_y, vw, vh);

    // Clear according to per-camera settings.
    GLbitfield bits = 0;
    if (cam->clsColor) {
      glClearColor(cam->clsR / 255.0f, cam->clsG / 255.0f,
                   cam->clsB / 255.0f, 1.0f);
      bits |= GL_COLOR_BUFFER_BIT;
    }
    if (cam->clsZbuf) { glClearDepth(1.0); bits |= GL_DEPTH_BUFFER_BIT; }
    if (bits) glClear(bits);

    // Build view and projection matrices.
    bb_cam_view_(cam);
    float aspect = (vw > 0 && vh > 0) ? (float)vw / (float)vh : 1.0f;
    if (cam->projMode == 2)
      bb_cam_proj_ortho_(cam, aspect);
    else
      bb_cam_proj_persp_(cam, aspect);

    // Draw all visible MeshEntities with the UNLIT shader.
    if (bb_shader_unlit_)
      bb_render_meshes_(bb_shader_unlit_, cam->view, cam->proj);
  }
}

// ============================================================
// ClearWorld / CaptureWorld
// ============================================================

// Die drei Schalter sind optional - im Original
// `ClearWorld [entities][,brushes][,textures]` (BUG-44).
inline void bb_ClearWorld(int entities = 1, int brushes = 1, int textures = 1) {
  (void)entities; (void)brushes; (void)textures;
  bb_entity_quit_();
}

inline void bb_CaptureWorld() { /* stub — rarely used */ }

// ============================================================
// Queries & scene settings
// ============================================================

inline int bb_TrisRendered() { return bb_tris_rendered_; }

inline void bb_AmbientLight(int r, int g, int b) {
  bb_ambient_r_ = r;
  bb_ambient_g_ = g;
  bb_ambient_b_ = b;
}

// ---- Render-state toggles ----

inline void bb_Dither(int)    { /* driver-controlled in modern GL */ }
inline void bb_WBuffer(int)   { /* not applicable in GL 3.3 Core   */ }
inline void bb_AntiAlias(int) { /* use multisampling via SDL attrs  */ }

inline void bb_Wireframe(int on) {
  if (glPolygonMode)
    glPolygonMode(GL_FRONT_AND_BACK, on ? GL_LINE : GL_FILL);
}

inline void bb_HWMultiTex(int) { /* always available in GL 3.3 */ }

#endif // BB_GRAPHICS3D_H
