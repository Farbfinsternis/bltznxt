#ifndef BB_MIRROR_H
#define BB_MIRROR_H

// CreateMirror (3D-16).
//
// Ein Spiegel hat im Original keine Geometrie (blitz3d/mirror.h: eine leere
// Unterklasse von Object). Die Arbeit macht World::render: **vor** der
// eigentlichen Szene wird sie fuer jeden sichtbaren Spiegel noch einmal
// gezeichnet, mit an der Spiegelebene gespiegelter Kamera und umgekehrter
// Umlaufrichtung. Die Ebene ist die XZ-Ebene des Spiegels (scaleMatrix
// 1,-1,1 in seinem Raum).
//
// Am Original gemessen (2026-09-17, build/mirror20260917/): EntityClass
// meldet "Mirror", TrisRendered zaehlt jeden Durchgang mit, ein versteckter
// Spiegel zeichnet gar nicht, und ein halbdurchsichtiger Boden ueber dem
// Spiegel mischt zweimal - er steckt selbst im gespiegelten Bild.

#include "bb_entity_core.h"
#include <memory>

struct bb_MirrorEntity_ : bb_Entity_ {
  bb_EntityKind_ kind() const override { return bb_EntityKind_::Mirror; }

  std::unique_ptr<bb_Entity_> clone() const override {
    auto c = std::make_unique<bb_MirrorEntity_>();
    bb_entity_copy_fields_(*c, *this);
    return c;
  }
};

inline int bb_CreateMirror(int parent = 0) {
  return bb_entity_register_(std::make_unique<bb_MirrorEntity_>(), parent);
}

// Die Weltlage der Kamera, an der XZ-Ebene des Spiegels gespiegelt:
//   C' = M * scale(1,-1,1) * M^-1 * C      (World::render)
static inline bool bb_mirror_cam_(const bb_Entity_* mirror, const float* cam_world,
                                  float* out) {
  float inv[16];
  if (!mat4_inverse_(inv, mirror->world)) return false;
  float flip[16];
  mat4_make_scale_(flip, 1, -1, 1);
  float t[16];
  mat4_mul_(t, flip, inv);
  mat4_mul_(t, mirror->world, t);
  mat4_mul_(out, t, cam_world);
  return true;
}

#endif // BB_MIRROR_H
