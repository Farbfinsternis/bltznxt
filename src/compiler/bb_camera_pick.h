#ifndef BB_CAMERA_PICK_H
#define BB_CAMERA_PICK_H

// Picking und Projektion ueber die Kamera (BUG-195, Gruppe 1).
//
// Included from bb_graphics3d.h, nach bb_collision.h.
// Provides: CameraPick, CameraProject, ProjectedX/Y/Z, EntityInView.
//
// Alles nach bbblitz3d.cpp des Originals (blitz-research/blitz3d) und am
// Original gemessen (2026-10-03, tests/test_bug195_kamera_pick.bb):
//
// - Die Koordinaten beziehen sich auf den Viewport der Kamera, nicht auf den
//   Bildschirm: CameraPick und ProjectedX/Y ziehen vp_x/vp_y nicht ab bzw.
//   rechnen sie nicht dazu.
// - Der Kegel ist so breit wie Camera::getFrustum ihn baut: an der nahen
//   Ebene near*2/zoom, die Hoehe mal vp_h/vp_w.
// - ProjectedZ ist immer die nahe Ebene, nicht die Tiefe des Punkts.

#include "bb_camera.h"
#include "bb_collision.h"
#include "bb_mesh.h"

// Breite und Hoehe des Kegels an der nahen Ebene (Camera::getFrustum).
static inline void bb_cam_frustum_wh_(const bb_CameraEntity_* c, float& w, float& h) {
  const float ar = c->vpW > 0 ? (float)c->vpH / (float)c->vpW : 1.0f;
  w = c->near_ * 2.0f / c->zoom;
  h = w * ar;
}

// ============================================================
// CameraPick
// ============================================================

// bbCameraPick: der Punkt im Viewport wird auf die nahe Ebene umgerechnet.
// Perspektivisch geht der Strahl vom Auge bis zur fernen Ebene, parallel
// von diesem Punkt geradeaus bis zur Tiefe far. Dann wie LinePick.
inline int bb_CameraPick(int camera, float viewport_x, float viewport_y) {
  bb_CameraEntity_* c = bb_cam_chk_(camera);
  if (!c) return 0;
  const float vw = c->vpW > 0 ? (float)c->vpW : 1.0f;
  const float vh = c->vpH > 0 ? (float)c->vpH : 1.0f;
  float nr_w, nr_h;
  bb_cam_frustum_wh_(c, nr_w, nr_h);
  const float nr = c->near_, fr = c->far_;
  float lx = (viewport_x / vw - 0.5f) * nr_w;
  float ly = (0.5f - viewport_y / vh) * nr_h;

  float o[3], d[3];
  if (c->projMode == 2) {
    o[0] = lx; o[1] = ly; o[2] = 0;
    d[0] = 0;  d[1] = 0;  d[2] = fr;
  } else {
    lx /= nr; ly /= nr;
    o[0] = o[1] = o[2] = 0;
    d[0] = lx * fr; d[1] = ly * fr; d[2] = fr;
  }
  const float* w = bb_entity_world_(c);
  float wo[3], wd[3];
  mat4_xform_pt_(wo, w, o[0], o[1], o[2]);
  mat4_xform_vec_(wd, w, d[0], d[1], d[2]);
  return bb_trace_ray_({ { wo[0], wo[1], wo[2] }, { wd[0], wd[1], wd[2] } }, 0);
}

// ============================================================
// CameraProject / ProjectedX/Y/Z
// ============================================================

inline float bb_projected_[3] = { 0, 0, 0 };

// bbCameraProject: der Punkt im Raum der Kamera. Perspektivisch nur, wenn er
// vor dem Auge und nicht hinter der fernen Ebene liegt - sonst alles 0. Die
// nahe Ebene prueft das Original nicht: ein Punkt zwischen Auge und near
// wird trotzdem projiziert.
inline void bb_CameraProject(int camera, float x, float y, float z) {
  bb_CameraEntity_* c = bb_cam_chk_(camera);
  if (!c) return;
  float inv[16];
  bb_projected_[0] = bb_projected_[1] = bb_projected_[2] = 0;
  if (!mat4_inverse_(inv, bb_entity_world_(c))) return;
  float v[3];
  mat4_xform_pt_(v, inv, x, y, z);
  float nr_w, nr_h;
  bb_cam_frustum_wh_(c, nr_w, nr_h);
  const float nr = c->near_;
  const float vw = (float)c->vpW, vh = (float)c->vpH;
  if (c->projMode == 2) {
    bb_projected_[0] = (v[0] / nr_w + 0.5f) * vw;
    bb_projected_[1] = (0.5f - v[1] / nr_h) * vh;
    bb_projected_[2] = nr;
    return;
  }
  if (v[2] > 0 && v[2] <= c->far_) {
    bb_projected_[0] = (v[0] * nr / v[2] / nr_w + 0.5f) * vw;
    bb_projected_[1] = (0.5f - v[1] * nr / v[2] / nr_h) * vh;
    bb_projected_[2] = nr;
  }
}

inline float bb_ProjectedX() { return bb_projected_[0]; }
inline float bb_ProjectedY() { return bb_projected_[1]; }
inline float bb_ProjectedZ() { return bb_projected_[2]; }

// ============================================================
// EntityInView
// ============================================================

// bbEntityInView: ein Netz mit den acht Ecken seiner Huellbox, alles andere
// (Pivot, Sprite, Licht ...) mit seiner Lage - gegen den Kegel der Kamera,
// den das Original fuer beide Projektionen gleich baut. Versteckt oder
// nicht spielt keine Rolle (gemessen).
inline int bb_EntityInView(int entity, int camera) {
  bb_Entity_* e = bb_ent_chk_(entity);
  bb_CameraEntity_* c = bb_cam_chk_(camera);
  if (!e || !c) return 0;
  bb_entity_world_(c);
  bb_cam_view_(c);
  const float vw = c->vpW > 0 ? (float)c->vpW : 1.0f;
  const bb_CullFrustum_ f = { c->near_, c->far_, 1.0f / c->zoom,
                              (float)c->vpH / vw / c->zoom };
  const float* model = bb_entity_world_(e);
  if (e->kind() == bb_EntityKind_::Mesh) {
    // dieselbe zwischengespeicherte Box wie der Sichtkegeltest beim Zeichnen
    bb_MeshRep_& rep = *static_cast<bb_MeshEntity_*>(e)->rep;
    const unsigned long long stand = rep.geom();
    if (rep.box_stamp != stand) {
      float* b = rep.box;
      bb_mesh_aabb_(static_cast<bb_MeshEntity_*>(e), b[0], b[3], b[1], b[4], b[2], b[5]);
      rep.box_empty = b[0] > b[3];
      rep.box_stamp = stand;
    }
    if (rep.box_empty) return 0;
    return bb_frustum_box_visible_(f, c->view, model, rep.box, rep.box + 3) ? 1 : 0;
  }
  const float p[1][3] = { { 0, 0, 0 } };
  return bb_frustum_visible_(f, c->view, model, p, 1) ? 1 : 0;
}

#endif // BB_CAMERA_PICK_H
