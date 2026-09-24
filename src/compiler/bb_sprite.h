#ifndef BB_SPRITE_H
#define BB_SPRITE_H

// Sprites (3D-16).
//
// Nach blitz3d/sprite.cpp und bbblitz3d.cpp: ein Sprite ist ein Quadrat von
// -1..1 in x und y, das bei jedem Zeichnen neu in Weltkoordinaten aufgebaut
// wird. Die Drehung dafuer haengt vom Modus ab (SpriteViewMode):
//
//   1  frei       die Drehung der **Kamera** - die eigene Lage und
//                 ScaleEntity wirken nicht, nur die Position
//   2  fest       die eigene Weltdrehung samt Skalierung; von hinten
//                 unsichtbar
//   3  aufrecht   die eigene j-Achse, k von der Kamera, orthogonalisiert
//   4  aufrecht2  Gier der Kamera vor die eigene Drehung
//
// Danach RotateSprite (gegen den Uhrzeigersinn) und ScaleSprite, die Ecken
// um HandleSprite verschoben. Am Original gemessen am 2026-09-17
// (build/sprite20260917/); die Zuordnung der Modi im alten Roadmap-Entwurf
// (1 Billboard, 2 Faced, ...) war falsch.
//
// CreateSprite zeichnet voll hell (EntityFX 1). LoadSprite laedt die Textur
// mit den Flags und waehlt die Mischart: Flag 4 deckend (Maske), sonst
// Flag 2 Alpha, sonst additiv.

#include "bb_entity_core.h"
#include "bb_mesh_core.h"
#include "bb_texture.h"
#include <cmath>
#include <memory>

struct bb_SpriteEntity_ : bb_Entity_ {
  float xhandle = 0, yhandle = 0;
  float rot = 0;                 // Bogenmass, wie Sprite::rot
  float xscale = 1, yscale = 1;
  int   viewMode = 1;
  bb_MeshData_ quad;             // je Zeichnen neu gefuellt, Weltkoordinaten

  bb_EntityKind_ kind() const override { return bb_EntityKind_::Sprite; }

  // Sprite::Sprite(const Sprite&) uebernimmt Groesse, Handle, Drehung und
  // Modus; das Quadrat selbst gehoert jeder Kopie allein.
  std::unique_ptr<bb_Entity_> clone() const override {
    auto c = std::make_unique<bb_SpriteEntity_>();
    bb_entity_copy_fields_(*c, *this);
    c->xhandle = xhandle; c->yhandle = yhandle;
    c->rot = rot; c->xscale = xscale; c->yscale = yscale;
    c->viewMode = viewMode;
    return c;
  }

  ~bb_SpriteEntity_() override { bb_mesh_free_gpu_(&quad); }
};

static inline bb_SpriteEntity_* bb_sprite_ent_(int h) {
  bb_Entity_* e = bb_entity_get_(h);
  if (!e || e->kind() != bb_EntityKind_::Sprite) return nullptr;
  return static_cast<bb_SpriteEntity_*>(e);
}

// debugSprite (BUG-170): erst debugModel, eine Kamera meldet also
// "Entity is not a model", ein Wuerfel "Entity is not a sprite".
static inline bb_SpriteEntity_* bb_sprite_chk_(int h) {
  if (bb_model_chk_(h)->kind() != bb_EntityKind_::Sprite)
    bb_RuntimeError("Entity is not a sprite");
  return bb_sprite_ent_(h);
}

inline int bb_CreateSprite(int parent = 0) {
  auto s = std::make_unique<bb_SpriteEntity_>();
  s->brush.fx = 1;               // setFX( FX_FULLBRIGHT )
  return bb_entity_register_(std::move(s), parent);
}

inline int bb_LoadSprite(const bbString& file, int texture_flags = 1, int parent = 0) {
  bb_parent_chk_(parent);   // vor dem Laden (BUG-170)
  int th = bb_LoadTexture(file, texture_flags);
  if (!th) return 0;
  bb_TexRef_ tex = bb_texture_ref_(th);
  // Die Textur ist im Original ein lokales Objekt, kein Handle des Programms.
  bb_FreeTexture(th);

  auto s = std::make_unique<bb_SpriteEntity_>();
  s->brush.tex.tex[0]   = tex;
  s->brush.tex.frame[0] = 0;
  s->brush.fx = 1;
  if (texture_flags & BB_TEX_MASKED)     s->brush.blend = 0;   // BLEND_REPLACE
  else if (texture_flags & BB_TEX_ALPHA) s->brush.blend = 1;   // BLEND_ALPHA
  else                                   s->brush.blend = 3;   // BLEND_ADD
  return bb_entity_register_(std::move(s), parent);
}

inline void bb_RotateSprite(int sprite, float angle) {
  if (auto* s = bb_sprite_chk_(sprite)) s->rot = angle * BB_D2R_;
}

inline void bb_ScaleSprite(int sprite, float x_scale, float y_scale) {
  if (auto* s = bb_sprite_chk_(sprite)) { s->xscale = x_scale; s->yscale = y_scale; }
}

inline void bb_HandleSprite(int sprite, float x_handle, float y_handle) {
  if (auto* s = bb_sprite_chk_(sprite)) { s->xhandle = x_handle; s->yhandle = y_handle; }
}

inline void bb_SpriteViewMode(int sprite, int view_mode) {
  if (auto* s = bb_sprite_chk_(sprite)) s->viewMode = view_mode;
}

// ============================================================
// Quadrat fuer eine Kamera aufbauen (Sprite::render)
// ============================================================

static inline void bb_v3_cross_(const float* a, const float* b, float* o) {
  const float x = a[1]*b[2] - a[2]*b[1];
  const float y = a[2]*b[0] - a[0]*b[2];
  const float z = a[0]*b[1] - a[1]*b[0];
  o[0] = x; o[1] = y; o[2] = z;
}

static inline void bb_v3_normalize_(float* v) {
  const float l = sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
  if (l > 0) { v[0] /= l; v[1] /= l; v[2] /= l; }
}

// cam_world: Weltmatrix der Kamera (Spalten i, j, k, Position).
// reflected: gespiegelter Durchgang (CreateMirror). Die gespiegelte Kamera
// dreht die Achsen des Quadrats mit um, deshalb kehrt das Original dort die
// Dreiecke um (Sprite::render, rc.isReflected(): 0,2,1 und 0,3,2) - sonst
// fiele das Quadrat unter die umgedrehte Rueckseitenpruefung (BUG-168).
static inline void bb_sprite_build_(bb_SpriteEntity_* sp, const float* cam_world,
                                    bool reflected = false) {
  const float* w  = sp->world;
  const float* cw = cam_world;
  float i[3], j[3], k[3];
  for (int n = 0; n < 3; ++n) { i[n] = w[n]; j[n] = w[4+n]; k[n] = w[8+n]; }

  if (sp->viewMode == 1) {
    // t.m = Kameramatrix
    for (int n = 0; n < 3; ++n) { i[n] = cw[n]; j[n] = cw[4+n]; k[n] = cw[8+n]; }
  } else if (sp->viewMode == 3) {
    // t.m.k = Kamera.k; t.m.orthogonalize(): k normieren, i = j x k, j = k x i
    for (int n = 0; n < 3; ++n) k[n] = cw[8+n];
    bb_v3_normalize_(k);
    bb_v3_cross_(j, k, i);
    bb_v3_normalize_(i);
    bb_v3_cross_(k, i, j);
  } else if (sp->viewMode == 4) {
    // t.m = yawMatrix( matrixYaw(Kamera) ) * t.m, matrixYaw = -atan2(k.x, k.z)
    const float q = -atan2f(cw[8], cw[10]);
    const float c = cosf(q), s = sinf(q);
    // yawMatrix: Spalten (c,0,s), (0,1,0), (-s,0,c)
    auto yaw = [c, s](float* v) {
      const float x = c*v[0] - s*v[2];
      const float z = s*v[0] + c*v[2];
      v[0] = x; v[2] = z;
    };
    yaw(i); yaw(j); yaw(k);
  }

  // t.m = t.m * rollMatrix(rot) * scaleMatrix(xscale, yscale, 1)
  const float c = cosf(sp->rot), s = sinf(sp->rot);
  float I[3], J[3];
  for (int n = 0; n < 3; ++n) {
    I[n] = ( i[n]*c + j[n]*s) * sp->xscale;
    J[n] = (-i[n]*s + j[n]*c) * sp->yscale;
  }

  // Die Normale zeigt zur Betrachterseite (-k). Das Original gibt keine
  // (null), gezeichnet wird ohnehin voll hell; fuer EntityFX 0 ist das die
  // sinnvolle Richtung.
  float nrm[3];
  bb_v3_cross_(I, J, nrm);
  bb_v3_normalize_(nrm);
  nrm[0] = -nrm[0]; nrm[1] = -nrm[1]; nrm[2] = -nrm[2];

  static const float corner[4][2] = { {-1, 1}, {1, 1}, {1, -1}, {-1, -1} };
  static const float uv[4][2]     = { {0, 0}, {1, 0}, {1, 1}, {0, 1} };
  bb_MeshData_& m = sp->quad;
  m.vertices.clear();
  for (int v = 0; v < 4; ++v) {
    const float x = corner[v][0] - sp->xhandle;
    const float y = corner[v][1] - sp->yhandle;
    bb_vert_push_(m,
                  I[0]*x + J[0]*y + w[12],
                  I[1]*x + J[1]*y + w[13],
                  I[2]*x + J[2]*y + w[14],
                  nrm[0], nrm[1], nrm[2], uv[v][0], uv[v][1]);
  }
  if (reflected) m.indices = { 0, 2, 1, 0, 3, 2 };
  else           m.indices = { 0, 1, 2, 0, 2, 3 };
  m.dirty = true;
}

#endif // BB_SPRITE_H
