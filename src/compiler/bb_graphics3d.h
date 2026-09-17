#ifndef BB_GRAPHICS3D_H
#define BB_GRAPHICS3D_H

// 3D scene control: UpdateWorld, RenderWorld, ClearWorld, scene globals.
// Includes bb_gl_ctx.h for GL access and bb_camera.h for Camera entities.

#include "bb_gl_ctx.h"
#include "bb_entity_core.h"

// Globales Umgebungslicht. **Vorgabe 127,127,127** laut Blitz3D-Doku
// (help/commands/3d_commands/AmbientLight.htm) - eine Szene ohne AmbientLight
// ist dort also mittelgrau beleuchtet, nicht schwarz. Float, weil das Original
// "AmbientLight red#,green#,blue#" fuehrt (3D-12).
inline float bb_ambient_r_ = 127.0f;
inline float bb_ambient_g_ = 127.0f;
inline float bb_ambient_b_ = 127.0f;

// Triangle counter — reset at start of RenderWorld, incremented by mesh draws.
inline int bb_tris_rendered_ = 0;

// bb_camera.h needs the fallback globals above to be declared first.
#include "bb_camera.h"
// bb_shader.h needs bb_gl_ctx.h symbols; include after camera.
#include "bb_shader.h"
#include "bb_mesh_core.h"
#include "bb_light.h"
#include "bb_texture.h"
#include "bb_mesh.h"
#include "bb_surface.h"
#include "bb_loader.h"

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
// Lichter einsammeln und hochladen (3D-12)
// ============================================================

// Bis zu acht sichtbare Lichter. Die Grenze steht so in der Blitz3D-Doku
// ("assume 8") und ist zugleich die Groesse der Shader-Uniformfelder.
inline constexpr int BB_MAX_LIGHTS = 8;

inline bb_LightEntity_ *bb_active_lights_[BB_MAX_LIGHTS] = {};

inline int bb_collect_lights_() {
  int n = 0;
  for (auto &[h, ent] : bb_entities_) {
    if (n >= BB_MAX_LIGHTS) break;
    if (ent->kind() != bb_EntityKind_::Light) continue;
    if (!bb_entity_shown_(ent.get())) continue;
    bb_active_lights_[n++] = static_cast<bb_LightEntity_ *>(ent.get());
  }
  return n;
}

inline void bb_upload_lights_(bb_Shader_ *sh, int n, bb_CameraEntity_ *cam) {
  float pos[BB_MAX_LIGHTS * 3] = {};
  float col[BB_MAX_LIGHTS * 3] = {};
  float dir[BB_MAX_LIGHTS * 3] = {};
  float rng[BB_MAX_LIGHTS]     = {};
  float ci[BB_MAX_LIGHTS]      = {};
  float co[BB_MAX_LIGHTS]      = {};
  int   typ[BB_MAX_LIGHTS]     = {};

  for (int i = 0; i < n; ++i) {
    bb_LightEntity_ *l = bb_active_lights_[i];
    const float *w = l->world;
    // Entities blicken nach +Z; die dritte Spalte der Weltmatrix ist die
    // Vorwaertsachse (siehe bb_PointEntity).
    float fx = w[8], fy = w[9], fz = w[10];
    float len = std::sqrt(fx * fx + fy * fy + fz * fz);
    if (len > 1e-6f) { fx /= len; fy /= len; fz /= len; }

    // Der Shader zaehlt ab 0: 0=directional, 1=point, 2=spot.
    typ[i] = (l->type == 1) ? 0 : (l->type == 2 ? 1 : 2);

    if (typ[i] == 0) {
      // Richtungslicht: u_light_pos traegt die Richtung ZUM Licht, also die
      // Gegenrichtung der Blickachse. Position und Reichweite sind laut Doku
      // unendlich und spielen keine Rolle.
      pos[i * 3 + 0] = -fx; pos[i * 3 + 1] = -fy; pos[i * 3 + 2] = -fz;
      rng[i] = 0.0f; // 0 = keine Abschwaechung
    } else {
      pos[i * 3 + 0] = w[12]; pos[i * 3 + 1] = w[13]; pos[i * 3 + 2] = w[14];
      rng[i] = l->range;
    }
    dir[i * 3 + 0] = fx; dir[i * 3 + 1] = fy; dir[i * 3 + 2] = fz;

    col[i * 3 + 0] = l->colR / 255.0f;
    col[i * 3 + 1] = l->colG / 255.0f;
    col[i * 3 + 2] = l->colB / 255.0f;

    // Kegelwinkel als Kosinus des Halbwinkels - der volle Winkel ist die hier
    // gewaehlte Lesart, siehe bb_light.h.
    ci[i] = std::cos(l->inner * 0.5f * 3.14159265358979f / 180.0f);
    co[i] = std::cos(l->outer * 0.5f * 3.14159265358979f / 180.0f);
  }

  bb_shader_uniform_i(sh, "u_light_count", n);
  bb_shader_uniform_v3v(sh, "u_light_pos",   n, pos);
  bb_shader_uniform_v3v(sh, "u_light_color", n, col);
  bb_shader_uniform_v3v(sh, "u_light_dir",   n, dir);
  bb_shader_uniform_fv(sh, "u_light_range",     n, rng);
  bb_shader_uniform_fv(sh, "u_light_cos_inner", n, ci);
  bb_shader_uniform_fv(sh, "u_light_cos_outer", n, co);
  bb_shader_uniform_iv(sh, "u_light_type", n, typ);

  bb_shader_uniform_v3(sh, "u_ambient", bb_ambient_r_ / 255.0f,
                      bb_ambient_g_ / 255.0f, bb_ambient_b_ / 255.0f);
  bb_shader_uniform_v3(sh, "u_view_pos", cam->world[12], cam->world[13],
                      cam->world[14]);
}

// ============================================================

// Der Tween-Faktor ist optional und wird noch nicht ausgewertet - im Original
// `RenderWorld [tween#]` (BUG-44).
inline void bb_RenderWorld(float tween = 1.0f) {
  (void)tween;
  if (!bb_gl_active_) return;

  // Die Weltmatrizen der ganzen Szene auffrischen, bevor irgendetwas
  // gezeichnet wird (BUG-71). Am Original gemessen (2026-09-11): ein
  // `PositionEntity` gefolgt von `RenderWorld` **ohne** UpdateWorld setzt das
  // Entity dort sofort um - der Wuerfel verschwindet aus dem Bild. Bei uns
  // zeichnete der Renderpfad ihn bis zum naechsten UpdateWorld an der alten
  // Stelle weiter.
  //
  // Hier laeuft bewusst der Sammeldurchlauf ueber alle Wurzeln und nicht die
  // Kette je Entity: gezeichnet wird ohnehin die ganze Szene, und das ist ein
  // Durchlauf statt einer Kette je Objekt. Dass UpdateWorld dieselbe Arbeit
  // gleich noch einmal tut, faellt gegen das Zeichnen nicht ins Gewicht.
  bb_entity_update_all_();

  bb_tris_rendered_ = 0;

  // Flush pending 2D draws (background sprites, etc.) before going GL.
  if (bb_renderer_) SDL_FlushRenderer(bb_renderer_);
  SDL_GL_MakeCurrent(bb_window_, bb_gl_ctx_);

  // Die Shader erst jetzt uebersetzen, im eigenen Kontext. Bis 2026-09-17
  // stand das vor MakeCurrent: hatte vorher ein 2D-Befehl den Kontext des
  // SDL-Renderers aktiviert (LoadImage, CreateImage), landeten die Programme
  // dort, und das erste Bild blieb weiss (BUG-144).
  if (!bb_shaders_ready_) bb_shaders_init_();

  auto cams = bb_collect_cameras_();

  // Ohne aktive Kamera zeichnet RenderWorld nichts, auch keinen Hintergrund:
  // was vorher im Backbuffer stand (Cls, 2D, das letzte Bild), bleibt stehen.
  // Gemessen am 2026-09-17 ohne jede Kamera, mit versteckter Kamera und mit
  // CameraProjMode 0 (BUG-137). Bis dahin loeschte hier ein erfundener
  // Ersatzzustand das Bild.
  if (cams.empty()) return;

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  // Rueckseiten werden entfernt - am Original gemessen: eine Kamera im Inneren
  // eines Wuerfels sieht dort den Hintergrund, nicht die Innenseiten.
  // EntityFX 16 schaltet es je Entity wieder ab (3D-10).
  //
  // Vorn ist im Original die Seite, auf die (b-a)x(c-a) zeigt - im
  // linkshaendigen Blitz-Koordinatensystem gerechnet, also ein Dreieck, das
  // von vorn gesehen im Uhrzeigersinn laeuft. Das gilt fuer jedes Netz gleich,
  // ob aus AddTriangle, aus einer Datei oder aus einem Create-Befehl
  // (BUG-126, gemessen 2026-09-17). bb_cam_view_ spiegelt z, laesst x und y
  // auf dem Bildschirm aber stehen: der Uhrzeigersinn bleibt Uhrzeigersinn,
  // und GL muss ihn als Vorderseite nehmen. Bis 2026-09-17 stand hier GL_CCW;
  // die Primitive und UpdateNormals waren zum Ausgleich umgedreht, geladene
  // Modelle zeigten ihre Innenseiten.
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CW);
  glEnable(GL_SCISSOR_TEST);

  for (auto* cam : cams) {
    // Viewport — Blitz3D y=0 is top-left; GL y=0 is bottom-left, so flip.
    int vw = (cam->vpW > 0) ? cam->vpW : bb_gfx_width_;
    int vh = (cam->vpH > 0) ? cam->vpH : bb_gfx_height_;
    int gl_y = bb_gfx_height_ - cam->vpY - vh;
    glViewport(cam->vpX, gl_y, vw, vh);
    // glClear beachtet den Viewport nicht, die Schere schon. Im Original
    // loescht jede Kamera nur ihren Viewport; ausserhalb bleibt stehen, was
    // vorher dort war (BUG-129).
    glScissor(cam->vpX, gl_y, vw, vh);

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

    // Pixelmitte wie Direct3D 7: dort liegt sie auf ganzen Koordinaten, in
    // OpenGL auf .5. Am Original gemessen (BUG-152, 2026-09-17): jede Kante,
    // die nicht auf einer Pixelgrenze liegt, faellt dort einen halben Pixel
    // weiter rechts und weiter unten. Deshalb das ganze Bild um einen halben
    // Pixel nach rechts und unten schieben - im Bildraum, damit Tiefe und
    // Clipping unberuehrt bleiben. Senkrecht knapp weniger: eine Kante auf
    // ganzer Zeile laege sonst genau auf der Pixelmitte, und dort entscheidet
    // die Fuellregel - in GL mit dem Ursprung unten andersherum als in D3D
    // (die obere Zeile gehoert im Original dazu, die untere nicht).
    // Der Abstand ist gemessen: 1/64 Pixel verfehlt eine fast genau auf der
    // Mitte liegende Kante, 1/1024 geht in der Subpixel-Rasterung des
    // Treibers unter (NVIDIA: 8 Bit). Bei groeberer Rasterung koennen solche
    // Grenzfaelle wieder kippen.
    {
      const float dx = 1.0f / (float)vw;
      const float dy = -(1.0f - 1.0f / 128.0f) / (float)vh;   // 1/2 - 1/256 Pixel
      if (cam->projMode == 2) { cam->proj[12] += dx; cam->proj[13] += dy; }
      else                    { cam->proj[8]  -= dx; cam->proj[9]  -= dy; }
    }

    // Netze zeichnet immer der LIT-Shader, auch ohne ein einziges Licht -
    // sonst faellt das Umgebungslicht unter den Tisch. Am Original gemessen
    // (3D-10): ein weisser Wuerfel ohne jedes Licht und ohne AmbientLight
    // kommt als **127,127,127** heraus, nicht weiss; das ist genau die
    // Vorgabe 127,127,127 von AmbientLight. Mit AmbientLight 0,0,0 ist er
    // schwarz, mit 255,255,255 weiss. Bis 3D-11 zeichnete hier ohne Licht der
    // TEXTURED-Shader, der das Umgebungslicht gar nicht kennt.
    int n = bb_collect_lights_();
    bb_Shader_ *sh = bb_shader_lit_;
    if (!sh) sh = bb_shader_textured_ ? bb_shader_textured_ : bb_shader_unlit_;
    if (sh) {
      bb_shader_bind_(sh);
      if (sh == bb_shader_lit_) bb_upload_lights_(sh, n, cam);
      const float cam_pos[3] = { cam->world[12], cam->world[13], cam->world[14] };
      bb_render_meshes_(sh, cam->view, cam->proj, cam_pos);
    }
  }
  glDisable(GL_SCISSOR_TEST);
}

// ============================================================
// ClearWorld / CaptureWorld
// ============================================================

// Die drei Schalter sind optional - im Original
// `ClearWorld [entities][,brushes][,textures]` (BUG-44), und jeder gibt nur
// seine Art frei (bbClearWorld, BUG-120). Gemessen am 2026-09-17: mit 0,0,0
// bleibt alles stehen; freigegebene Brushes und Texturen nehmen bemalten
// Entities ihr Aussehen nicht - die Entities halten eigene Kopien bzw.
// Verweise. Die Handle-Zaehlung laeuft weiter, anders als am Programmende.
inline void bb_ClearWorld(int entities = 1, int brushes = 1, int textures = 1) {
  if (entities) bb_entities_.clear();
  if (brushes)  bb_brushes_.clear();
  if (textures) bb_textures_.clear();
}

inline const bool bb_world_close_reg_ =
    (bb_world_close_hook_ = [] { bb_ClearWorld(1, 1, 1); }, true);

inline void bb_CaptureWorld() { /* stub — rarely used */ }

// ============================================================
// Queries & scene settings
// ============================================================

inline int bb_TrisRendered() { return bb_tris_rendered_; }

inline void bb_AmbientLight(float r, float g, float b) {
  bb_ambient_r_ = r;
  bb_ambient_g_ = g;
  bb_ambient_b_ = b;
}

// ---- Render-state toggles ----

inline void bb_Dither(int)    { /* driver-controlled in modern GL */ }
inline void bb_WBuffer(int)   { /* not applicable in GL 3.3 Core   */ }
inline void bb_AntiAlias(int) { /* use multisampling via SDL attrs  */ }

inline void bb_Wireframe(int on) {
  bb_gl_use_(); // sonst traefe der Zustand den Kontext des 2D-Renderers (BUG-63)
  if (glPolygonMode)
    glPolygonMode(GL_FRONT_AND_BACK, on ? GL_LINE : GL_FILL);
}

inline void bb_HWMultiTex(int) { /* always available in GL 3.3 */ }

#endif // BB_GRAPHICS3D_H
