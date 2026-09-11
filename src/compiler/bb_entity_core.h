#ifndef BB_ENTITY_CORE_H
#define BB_ENTITY_CORE_H

// Entity Handle System (3D-03) + Transform System (3D-04).
//
// Defines bb_Entity_ base struct, global handle map, Pivot entity, matrix math
// helpers, and all transform API (PositionEntity, RotateEntity, MoveEntity, …).
// All other entity types (Mesh, Camera, Light, Sprite) are defined in their
// own headers and inherit from bb_Entity_.

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <unordered_map>
#include <vector>
#include "bb_string.h"
#include "bb_brush.h"   // bb_Brush_ - das Aussehen als Wert (3D-15)

// ============================================================
// Entity kind tag
// ============================================================

enum class bb_EntityKind_ {
  Pivot, Mesh, Camera, Light, Sprite
};

// ============================================================
// bb_Entity_ — polymorphic base for all 3D scene objects
// ============================================================

struct bb_Entity_ {
  int      handle  = 0;
  bbString name;
  int      parent  = 0;         // handle of parent entity, 0 = root
  std::vector<int> children;    // handles of direct children
  bool     visible = true;
  // Zeichenreihenfolge (3D-10). Am Original gemessen: > 0 zuerst und damit
  // hinter allem, < 0 zuletzt und damit vor allem; bei einem Wert ungleich 0
  // ist der Z-Puffer fuer dieses Entity abgeschaltet.
  int      order   = 0;

  // ---- Aussehen (3D-10, seit 3D-15 ein Brush) ----
  // Im Original ist das Aussehen einer Entity genau ein Brush: EntityColor
  // ruft m->setColor, und das schreibt in den Brush des Modells. Hier steht
  // es deshalb auch als einer - dann sind PaintEntity und GetEntityBrush
  // nichts weiter als Zuweisung und Kopie.
  bb_Brush_ brush;
  float    fadeNear = 0.0f, fadeFar = 0.0f;   // EntityAutoFade, 0/0 = aus

  // Local transform
  float px = 0, py = 0, pz = 0;   // position
  float rx = 0, ry = 0, rz = 0;   // rotation (Euler degrees, YXZ — same as Blitz3D)
  float sx = 1, sy = 1, sz = 1;   // scale

  // World matrix (column-major 4×4), updated by UpdateWorld
  float world[16] = {
    1,0,0,0,
    0,1,0,0,
    0,0,1,0,
    0,0,0,1
  };

  virtual ~bb_Entity_() = default;
  virtual bb_EntityKind_ kind() const = 0;

  // Eine Kopie dieser Entity ohne Verwandtschaft (kein Handle, kein Parent,
  // keine Kinder) - das Gegenstueck zu Entity::clone() im Original.
  //
  // Die Vorgabe liefert einen **Pivot**, und das ist kein Notbehelf, sondern
  // die gemessene Regel: im Original ueberschreiben Camera, Light und Terrain
  // `clone()` nicht, erben also `Object::clone()`, das ein nacktes `Object`
  // baut (`blitz3d/object.h:29`). `EntityClass$(CopyEntity(light))` meldet im
  // Original darum "Pivot", ebenso fuer eine Kamera (gemessen 2026-09-11).
  // Wer eine Art wirklich kopierbar machen will, ueberschreibt hier.
  virtual std::unique_ptr<bb_Entity_> clone() const;
};

// ============================================================
// PivotEntity — empty transform node (no geometry)
// ============================================================

struct bb_PivotEntity_ : bb_Entity_ {
  bb_EntityKind_ kind() const override { return bb_EntityKind_::Pivot; }
};

// Den Entity-Teil von `src` nach `dst` uebernehmen: Name, Sichtbarkeit,
// Reihenfolge, Aussehen und die **lokale** Lage. Handle, Parent und Kinder
// bleiben aus, die setzt der Aufrufer.
//
// Das Original kopiert beim Pivot-Klon streng genommen kein Aussehen (der
// Brush sitzt dort in Model, nicht in Entity). Bei uns liegt er in
// bb_Entity_; ihn mitzunehmen ist nicht beobachtbar, weil ein Pivot nicht
// gezeichnet wird.
inline void bb_entity_copy_fields_(bb_Entity_& dst, const bb_Entity_& src) {
  dst.name     = src.name;
  dst.visible  = src.visible;
  dst.order    = src.order;
  dst.brush    = src.brush;
  dst.fadeNear = src.fadeNear;
  dst.fadeFar  = src.fadeFar;
  dst.px = src.px; dst.py = src.py; dst.pz = src.pz;
  dst.rx = src.rx; dst.ry = src.ry; dst.rz = src.rz;
  dst.sx = src.sx; dst.sy = src.sy; dst.sz = src.sz;
  memcpy(dst.world, src.world, sizeof(dst.world));
}

inline std::unique_ptr<bb_Entity_> bb_Entity_::clone() const {
  auto c = std::make_unique<bb_PivotEntity_>();
  bb_entity_copy_fields_(*c, *this);
  return c;
}

// ============================================================
// Global entity registry
// ============================================================

inline std::unordered_map<int, std::unique_ptr<bb_Entity_>> bb_entities_;
inline int bb_entity_next_id_ = 1;

// Returns a raw pointer to the entity, or nullptr if handle is invalid.
inline bb_Entity_* bb_entity_get_(int h) {
  auto it = bb_entities_.find(h);
  return (it != bb_entities_.end()) ? it->second.get() : nullptr;
}

// ============================================================
// Internal: register a freshly-created entity with an optional parent.
// Returns the assigned handle.
// ============================================================

inline int bb_entity_register_(std::unique_ptr<bb_Entity_> ent, int parent) {
  int h = bb_entity_next_id_++;
  ent->handle = h;
  ent->parent = parent;
  if (parent) {
    bb_Entity_* p = bb_entity_get_(parent);
    if (p) p->children.push_back(h);
  }
  bb_entities_[h] = std::move(ent);
  return h;
}

// ============================================================
// FreeEntity — recursively destroys an entity and all its descendants.
// ============================================================

inline void bb_FreeEntity(int h) {
  bb_Entity_* e = bb_entity_get_(h);
  if (!e) return;

  // Recurse into children first (copy vector — it gets modified during recursion)
  std::vector<int> kids = e->children;
  for (int c : kids) bb_FreeEntity(c);

  // Detach from parent's child list
  if (e->parent) {
    bb_Entity_* p = bb_entity_get_(e->parent);
    if (p) {
      auto& ch = p->children;
      ch.erase(std::remove(ch.begin(), ch.end(), h), ch.end());
    }
  }

  bb_entities_.erase(h);
}

// ============================================================
// CreatePivot
// ============================================================

inline int bb_CreatePivot(int parent = 0) {
  return bb_entity_register_(std::make_unique<bb_PivotEntity_>(), parent);
}

// ============================================================
// Visibility
// ============================================================

inline void bb_HideEntity(int h) {
  bb_Entity_* e = bb_entity_get_(h);
  if (e) e->visible = false;
}

inline void bb_ShowEntity(int h) {
  bb_Entity_* e = bb_entity_get_(h);
  if (e) e->visible = true;
}

// ============================================================
// Name (NameEntity = setter, EntityName = getter, like Blitz3D)
// ============================================================

inline void bb_NameEntity(int h, const bbString& name) {
  bb_Entity_* e = bb_entity_get_(h);
  if (e) e->name = name;
}

inline bbString bb_EntityName(int h) {
  bb_Entity_* e = bb_entity_get_(h);
  return e ? e->name : bbString{};
}

// ============================================================
// Matrix math helpers (column-major 4×4)
// Convention: m[col*4 + row], so m[12/13/14] = translation.
// ============================================================

static const float BB_PI_ = 3.14159265358979323846f;
static const float BB_D2R_ = BB_PI_ / 180.0f;
static const float BB_R2D_ = 180.0f / BB_PI_;

static inline void mat4_identity_(float m[16]) {
  memset(m, 0, 64);
  m[0] = m[5] = m[10] = m[15] = 1.0f;
}

// C = A * B  (both column-major)
static inline void mat4_mul_(float out[16], const float a[16], const float b[16]) {
  float tmp[16];
  for (int col = 0; col < 4; ++col)
    for (int row = 0; row < 4; ++row) {
      float v = 0;
      for (int k = 0; k < 4; ++k) v += a[k*4+row] * b[col*4+k];
      tmp[col*4+row] = v;
    }
  memcpy(out, tmp, 64);
}

static inline void mat4_make_translate_(float m[16], float x, float y, float z) {
  mat4_identity_(m);
  m[12] = x; m[13] = y; m[14] = z;
}

static inline void mat4_make_scale_(float m[16], float x, float y, float z) {
  mat4_identity_(m);
  m[0] = x; m[5] = y; m[10] = z;
}

// Combined Euler rotation YXZ (Blitz3D convention): R = Ry * Rx * Rz
// Derived analytically to avoid 3× matmul overhead.
static inline void mat4_make_euler_YXZ_(float m[16],
                                         float rx_d, float ry_d, float rz_d) {
  float rx = rx_d * BB_D2R_, ry = ry_d * BB_D2R_, rz = rz_d * BB_D2R_;
  float cx = cosf(rx), sx = sinf(rx);
  float cy = cosf(ry), sy = sinf(ry);
  float cz = cosf(rz), sz = sinf(rz);
  // Column 0
  m[0] = cy*cz + sy*sx*sz;   m[1] = cx*sz;  m[2]  = -sy*cz + cy*sx*sz;  m[3]  = 0;
  // Column 1
  m[4] = -cy*sz + sy*sx*cz;  m[5] = cx*cz;  m[6]  = sy*sz  + cy*sx*cz;  m[7]  = 0;
  // Column 2
  m[8] = sy*cx;               m[9] = -sx;    m[10] = cy*cx;               m[11] = 0;
  // Column 3
  m[12] = 0; m[13] = 0; m[14] = 0; m[15] = 1;
}

// Local TRS matrix:  T * (Ry*Rx*Rz) * S
static inline void mat4_local_(float out[16], const bb_Entity_* e) {
  float R[16], S[16], RS[16], T[16];
  mat4_make_euler_YXZ_(R, e->rx, e->ry, e->rz);
  mat4_make_scale_(S, e->sx, e->sy, e->sz);
  mat4_mul_(RS, R, S);
  mat4_make_translate_(T, e->px, e->py, e->pz);
  mat4_mul_(out, T, RS);
}

// General 4×4 inverse (Mesa GLU algorithm, column-major).
// Returns false if matrix is singular.
static inline bool mat4_inverse_(float inv[16], const float m[16]) {
  inv[0]  =  m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15]
           + m[9]*m[7]*m[14]  + m[13]*m[6]*m[11]  - m[13]*m[7]*m[10];
  inv[4]  = -m[4]*m[10]*m[15] + m[4]*m[11]*m[14]  + m[8]*m[6]*m[15]
           - m[8]*m[7]*m[14]  - m[12]*m[6]*m[11]  + m[12]*m[7]*m[10];
  inv[8]  =  m[4]*m[9]*m[15]  - m[4]*m[11]*m[13]  - m[8]*m[5]*m[15]
           + m[8]*m[7]*m[13]  + m[12]*m[5]*m[11]  - m[12]*m[7]*m[9];
  inv[12] = -m[4]*m[9]*m[14]  + m[4]*m[10]*m[13]  + m[8]*m[5]*m[14]
           - m[8]*m[6]*m[13]  - m[12]*m[5]*m[10]  + m[12]*m[6]*m[9];
  inv[1]  = -m[1]*m[10]*m[15] + m[1]*m[11]*m[14]  + m[9]*m[2]*m[15]
           - m[9]*m[3]*m[14]  - m[13]*m[2]*m[11]  + m[13]*m[3]*m[10];
  inv[5]  =  m[0]*m[10]*m[15] - m[0]*m[11]*m[14]  - m[8]*m[2]*m[15]
           + m[8]*m[3]*m[14]  + m[12]*m[2]*m[11]  - m[12]*m[3]*m[10];
  inv[9]  = -m[0]*m[9]*m[15]  + m[0]*m[11]*m[13]  + m[8]*m[1]*m[15]
           - m[8]*m[3]*m[13]  - m[12]*m[1]*m[11]  + m[12]*m[3]*m[9];
  inv[13] =  m[0]*m[9]*m[14]  - m[0]*m[10]*m[13]  - m[8]*m[1]*m[14]
           + m[8]*m[2]*m[13]  + m[12]*m[1]*m[10]  - m[12]*m[2]*m[9];
  inv[2]  =  m[1]*m[6]*m[15]  - m[1]*m[7]*m[14]   - m[5]*m[2]*m[15]
           + m[5]*m[3]*m[14]  + m[13]*m[2]*m[7]   - m[13]*m[3]*m[6];
  inv[6]  = -m[0]*m[6]*m[15]  + m[0]*m[7]*m[14]   + m[4]*m[2]*m[15]
           - m[4]*m[3]*m[14]  - m[12]*m[2]*m[7]   + m[12]*m[3]*m[6];
  inv[10] =  m[0]*m[5]*m[15]  - m[0]*m[7]*m[13]   - m[4]*m[1]*m[15]
           + m[4]*m[3]*m[13]  + m[12]*m[1]*m[7]   - m[12]*m[3]*m[5];
  inv[14] = -m[0]*m[5]*m[14]  + m[0]*m[6]*m[13]   + m[4]*m[1]*m[14]
           - m[4]*m[2]*m[13]  - m[12]*m[1]*m[6]   + m[12]*m[2]*m[5];
  inv[3]  = -m[1]*m[6]*m[11]  + m[1]*m[7]*m[10]   + m[5]*m[2]*m[11]
           - m[5]*m[3]*m[10]  - m[9]*m[2]*m[7]    + m[9]*m[3]*m[6];
  inv[7]  =  m[0]*m[6]*m[11]  - m[0]*m[7]*m[10]   - m[4]*m[2]*m[11]
           + m[4]*m[3]*m[10]  + m[8]*m[2]*m[7]    - m[8]*m[3]*m[6];
  inv[11] = -m[0]*m[5]*m[11]  + m[0]*m[7]*m[9]    + m[4]*m[1]*m[11]
           - m[4]*m[3]*m[9]   - m[8]*m[1]*m[7]    + m[8]*m[3]*m[5];
  inv[15] =  m[0]*m[5]*m[10]  - m[0]*m[6]*m[9]    - m[4]*m[1]*m[10]
           + m[4]*m[2]*m[9]   + m[8]*m[1]*m[6]    - m[8]*m[2]*m[5];
  float det = m[0]*inv[0] + m[1]*inv[4] + m[2]*inv[8] + m[3]*inv[12];
  if (fabsf(det) < 1e-10f) return false;
  float idet = 1.0f / det;
  for (int i = 0; i < 16; ++i) inv[i] *= idet;
  return true;
}

// Transform a point (w=1) by a column-major 4×4 matrix.
static inline void mat4_xform_pt_(float out[3], const float m[16],
                                   float x, float y, float z) {
  out[0] = m[0]*x + m[4]*y + m[8]*z  + m[12];
  out[1] = m[1]*x + m[5]*y + m[9]*z  + m[13];
  out[2] = m[2]*x + m[6]*y + m[10]*z + m[14];
}

// Extract YXZ Euler angles (degrees) from a column-major world matrix
// (upper 3×3 may include uniform or non-uniform scale — columns are normalized).
static inline void mat4_extract_euler_YXZ_(const float m[16],
                                            float& rx, float& ry, float& rz) {
  float sx = sqrtf(m[0]*m[0] + m[1]*m[1] + m[2]*m[2]);
  float sy = sqrtf(m[4]*m[4] + m[5]*m[5] + m[6]*m[6]);
  float sz = sqrtf(m[8]*m[8] + m[9]*m[9] + m[10]*m[10]);
  if (sx < 1e-8f) sx = 1;
  if (sy < 1e-8f) sy = 1;
  if (sz < 1e-8f) sz = 1;
  // Normalised rotation elements (row 1)
  float r10 = m[1]/sx;   // cx*sz
  // Bei einer halben Drehung ist dieser Zaehler eine Null, und ihr
  // VORZEICHEN entscheidet im atan2 unten, ob der Rollwert +180 oder
  // -180 wird - zwei Namen fuer dieselbe Lage. Am Original gemessen
  // (2026-09-10) ist es dort +180: "RotateEntity e,200,0,0" liefert
  // Roll 180, ebenso "RotateEntity e,100,30,0". Eine negative Null aus
  // der Matrixmultiplikation wuerde uns -180 liefern, deshalb wird sie
  // hier eingeebnet. Der YAW-Zaehler bleibt unberuehrt: dort meldet das
  // Original im selben Kippfall -180, hat also dieselbe negative Null.
  if (r10 == 0.0f) r10 = 0.0f;
  float r11 = m[5]/sy;   // cx*cz
  float r12 = m[9]/sz;   // -sx  → rx = asin(-r12)
  float r02 = m[8]/sz;   // sy*cx
  float r22 = m[10]/sz;  // cy*cx
  float pit = asinf(std::max(-1.0f, std::min(1.0f, -r12)));
  rx = pit * BB_R2D_;
  float cp = cosf(pit);
  if (cp > 1e-4f) {
    ry = atan2f(r02/cp, r22/cp) * BB_R2D_;
    rz = atan2f(r10/cp, r11/cp) * BB_R2D_;
  } else {
    // Gimbal lock — roll assigned arbitrarily, yaw=0
    ry = 0;
    rz = atan2f(-m[4]/sy, m[0]/sx) * BB_R2D_;
  }
}

// ============================================================
// Scene-graph update helpers
// ============================================================

// Recursive DFS: compute world matrix for e and all its descendants.
static inline void bb_update_entity_world_(bb_Entity_* e,
                                            const float* parent_world) {
  float local[16];
  mat4_local_(local, e);
  if (parent_world) mat4_mul_(e->world, parent_world, local);
  else              memcpy(e->world, local, 64);
  for (int ch : e->children) {
    bb_Entity_* c = bb_entity_get_(ch);
    if (c) bb_update_entity_world_(c, e->world);
  }
}

// Called by bb_UpdateWorld() in bb_graphics3d.h.
inline void bb_entity_update_all_() {
  for (auto& [h, e] : bb_entities_)
    if (e->parent == 0)
      bb_update_entity_world_(e.get(), nullptr);
}

// ============================================================
// CopyEntity
// ============================================================

// Steht hier unten und nicht bei CreatePivot, weil die frische Kopie ihre
// Weltmatrix braucht: unsere Getter lesen `world` direkt und nur
// bb_UpdateWorld schreibt es, im Original rechnet der Getter selbst nach.
// Ohne das traegt eine Kopie mit Parent bis zum naechsten UpdateWorld die
// Weltlage des Originals.

// Den Teilbaum kopieren - erst die Entity selbst, dann rekursiv die Kinder
// an die Kopie. Genau die Reihenfolge von Object::copy() im Original
// (`blitz3d/object.cpp:28`), damit auch die Handles in derselben Folge
// vergeben werden.
inline int bb_copy_entity_tree_(int h, int parent) {
  const bb_Entity_* e = bb_entity_get_(h);
  if (!e) return 0;

  std::unique_ptr<bb_Entity_> c = e->clone();
  if (!c) return 0;

  // Kopie der Kinderliste: die Rekursion haengt an die **Kopie** an, aber
  // ein Kind koennte im Prinzip dieselbe Liste beruehren.
  const std::vector<int> kids = e->children;

  int nh = bb_entity_register_(std::move(c), parent);
  for (int k : kids) bb_copy_entity_tree_(k, nh);
  return nh;
}

// Am Original gemessen (2026-09-11): Name, lokale Lage und Winkel wandern
// mit, die Kinder werden rekursiv mitkopiert (samt Enkeln), ohne Parent ist
// die Kopie eine Wurzel, und mit Parent bleibt die **lokale** Lage stehen -
// die Doku ("created at the parent entity's position") beschreibt nur den
// Fall, dass das Original lokal auf 0,0,0 sitzt.
inline int bb_CopyEntity(int h, int parent = 0) {
  int nh = bb_copy_entity_tree_(h, parent);
  if (!nh) return 0;

  bb_Entity_* ne = bb_entity_get_(nh);
  bb_Entity_* p  = parent ? bb_entity_get_(parent) : nullptr;
  if (ne) bb_update_entity_world_(ne, p ? p->world : nullptr);
  return nh;
}

// ============================================================
// Position / Move / Translate
// ============================================================

// Set local (glob=0) or world (glob=1) position.
inline void bb_PositionEntity(int h, float x, float y, float z, int glob = 0) {
  bb_Entity_* e = bb_entity_get_(h);
  if (!e) return;
  if (!glob || e->parent == 0) {
    e->px = x; e->py = y; e->pz = z;
  } else {
    // Convert world position to parent-local space.
    bb_Entity_* p = bb_entity_get_(e->parent);
    if (!p) { e->px = x; e->py = y; e->pz = z; return; }
    float inv[16];
    if (mat4_inverse_(inv, p->world)) {
      float lp[3];
      mat4_xform_pt_(lp, inv, x, y, z);
      e->px = lp[0]; e->py = lp[1]; e->pz = lp[2];
    } else {
      e->px = x; e->py = y; e->pz = z;
    }
  }
}

// Move in entity-local orientation (forward/up/right relative to entity).
inline void bb_MoveEntity(int h, float dx, float dy, float dz) {
  bb_Entity_* e = bb_entity_get_(h);
  if (!e) return;
  float R[16];
  mat4_make_euler_YXZ_(R, e->rx, e->ry, e->rz);
  // R columns 0/1/2 are entity X/Y/Z axes in parent-local space
  e->px += dx * R[0] + dy * R[4] + dz * R[8];
  e->py += dx * R[1] + dy * R[5] + dz * R[9];
  e->pz += dx * R[2] + dy * R[6] + dz * R[10];
}

// Translate by delta.  glob=0 → entity-local space; glob=1 → world space.
inline void bb_TranslateEntity(int h, float dx, float dy, float dz, int glob = 0) {
  bb_Entity_* e = bb_entity_get_(h);
  if (!e) return;
  if (glob == 0) {
    // Local space: same as MoveEntity
    float R[16];
    mat4_make_euler_YXZ_(R, e->rx, e->ry, e->rz);
    e->px += dx * R[0] + dy * R[4] + dz * R[8];
    e->py += dx * R[1] + dy * R[5] + dz * R[9];
    e->pz += dx * R[2] + dy * R[6] + dz * R[10];
  } else {
    // World space delta → convert to parent-local space
    if (e->parent == 0) {
      e->px += dx; e->py += dy; e->pz += dz;
    } else {
      bb_Entity_* p = bb_entity_get_(e->parent);
      if (!p) { e->px += dx; e->py += dy; e->pz += dz; return; }
      float inv[16];
      if (mat4_inverse_(inv, p->world)) {
        float d[3];
        // Transform delta as a vector (no translation)
        d[0] = inv[0]*dx + inv[4]*dy + inv[8]*dz;
        d[1] = inv[1]*dx + inv[5]*dy + inv[9]*dz;
        d[2] = inv[2]*dx + inv[6]*dy + inv[10]*dz;
        e->px += d[0]; e->py += d[1]; e->pz += d[2];
      } else {
        e->px += dx; e->py += dy; e->pz += dz;
      }
    }
  }
}

// ============================================================
// Rotation
// ============================================================

// Set absolute rotation.  glob=0 → set local; glob=1 → set world rotation.
inline void bb_RotateEntity(int h, float rx, float ry, float rz, int glob = 0) {
  bb_Entity_* e = bb_entity_get_(h);
  if (!e) return;
  if (!glob || e->parent == 0) {
    e->rx = rx; e->ry = ry; e->rz = rz;
  } else {
    // Set world rotation: compute the local rotation needed so that
    // parent.world * local_rot = desired_world_rot
    bb_Entity_* p = bb_entity_get_(e->parent);
    if (!p) { e->rx = rx; e->ry = ry; e->rz = rz; return; }
    // Build desired world rotation matrix
    float Rw[16]; mat4_make_euler_YXZ_(Rw, rx, ry, rz);
    // Invert parent world matrix
    float inv_pw[16];
    if (!mat4_inverse_(inv_pw, p->world)) { e->rx = rx; e->ry = ry; e->rz = rz; return; }
    // local_rot = inv_pw * Rw
    float Rl[16]; mat4_mul_(Rl, inv_pw, Rw);
    mat4_extract_euler_YXZ_(Rl, e->rx, e->ry, e->rz);
  }
}

// Apply relative rotation delta.  glob=0 → entity-local; glob=1 → world space.
inline void bb_TurnEntity(int h, float drx, float dry, float drz, int glob = 0) {
  bb_Entity_* e = bb_entity_get_(h);
  if (!e) return;
  if (glob == 0) {
    // Local: just add Euler deltas (approximate for large angles)
    e->rx += drx; e->ry += dry; e->rz += drz;
  } else {
    // World-space turn: apply world delta rotation to current world orientation
    // Build current world rot + delta rot and extract new local Euler
    float Rw[16], Rd[16], Rnew[16];
    mat4_make_euler_YXZ_(Rw, e->rx, e->ry, e->rz);  // approx (no parent)
    mat4_make_euler_YXZ_(Rd, drx, dry, drz);
    mat4_mul_(Rnew, Rd, Rw);
    mat4_extract_euler_YXZ_(Rnew, e->rx, e->ry, e->rz);
  }
}

// ============================================================
// Scale
// ============================================================

inline void bb_ScaleEntity(int h, float sx, float sy, float sz, int /*glob*/ = 0) {
  bb_Entity_* e = bb_entity_get_(h);
  if (e) { e->sx = sx; e->sy = sy; e->sz = sz; }
}

// ============================================================
// PointEntity — rotate entity to face target
// ============================================================

inline void bb_PointEntity(int h, int target, float roll = 0.0f) {
  bb_Entity_* e  = bb_entity_get_(h);
  bb_Entity_* tg = bb_entity_get_(target);
  if (!e || !tg) return;
  float dx = tg->world[12] - e->world[12];
  float dy = tg->world[13] - e->world[13];
  float dz = tg->world[14] - e->world[14];
  float xz = sqrtf(dx*dx + dz*dz);
  e->ry = atan2f(dx, dz) * BB_R2D_;
  e->rx = atan2f(-dy, xz) * BB_R2D_;
  e->rz = roll;
}

// ============================================================
// AlignToVector — smoothly align entity axis to a world-space vector
// axis: 1=X, 2=Y, 3=Z  |  rate: 0=no change, 1=immediate
// ============================================================

inline void bb_AlignToVector(int h, float nx, float ny, float nz,
                              int axis, float rate = 1.0f) {
  bb_Entity_* e = bb_entity_get_(h);
  if (!e || rate <= 0.0f) return;
  float len = sqrtf(nx*nx + ny*ny + nz*nz);
  if (len < 1e-8f) return;
  nx /= len; ny /= len; nz /= len;

  // For rate=1 (immediate) we set the entity axis directly;
  // for 0<rate<1 we blend current and target (approximation via Euler lerp).
  if (axis == 3) {          // align Z axis → similar to PointEntity
    float xz = sqrtf(nx*nx + nz*nz);
    float tgy = atan2f(nx, nz) * BB_R2D_;
    float tgx = atan2f(-ny, xz) * BB_R2D_;
    if (rate >= 1.0f) { e->ry = tgy; e->rx = tgx; }
    else { e->ry += (tgy - e->ry) * rate; e->rx += (tgx - e->rx) * rate; }
  } else if (axis == 2) {   // align Y axis
    float xz = sqrtf(nx*nx + nz*nz);
    float tgx = atan2f(ny, xz) * BB_R2D_;
    float tgy = atan2f(-nx, nz) * BB_R2D_;
    if (rate >= 1.0f) { e->rx = tgx; e->ry = tgy; }
    else { e->rx += (tgx - e->rx) * rate; e->ry += (tgy - e->ry) * rate; }
  } else {                   // align X axis (axis=1)
    float yz = sqrtf(ny*ny + nz*nz);
    float tgz = atan2f(-ny, yz) * BB_R2D_;
    float tgy = atan2f(nx, nz) * BB_R2D_;
    if (rate >= 1.0f) { e->rz = tgz; e->ry = tgy; }
    else { e->rz += (tgz - e->rz) * rate; e->ry += (tgy - e->ry) * rate; }
  }
}

// ============================================================
// Reset
// ============================================================

inline void bb_ResetEntity(int h) {
  bb_Entity_* e = bb_entity_get_(h);
  if (!e) return;
  e->px = e->py = e->pz = 0;
  e->rx = e->ry = e->rz = 0;
  e->sx = e->sy = e->sz = 1;
}

// ============================================================
// Transform queries
// ============================================================

inline float bb_EntityX(int h, int glob = 0) {
  bb_Entity_* e = bb_entity_get_(h);
  if (!e) return 0;
  return glob ? e->world[12] : e->px;
}
inline float bb_EntityY(int h, int glob = 0) {
  bb_Entity_* e = bb_entity_get_(h);
  if (!e) return 0;
  return glob ? e->world[13] : e->py;
}
inline float bb_EntityZ(int h, int glob = 0) {
  bb_Entity_* e = bb_entity_get_(h);
  if (!e) return 0;
  return glob ? e->world[14] : e->pz;
}

// Die drei Winkel-Getter lesen die LAGE zurueck, nicht das Hineingegebene
// (BUG-83). Am Original gemessen (2026-09-10):
//
//   RotateEntity e,0,370,0   ->  Yaw 10        (nicht 370)
//   RotateEntity e,0,181.2,0 ->  Yaw -178.8    (Bereich (-180,180])
//   RotateEntity e,200,0,0   ->  -20/-180/180  (jenseits 90 Grad Nick kippt
//                                               die Zerlegung, es wird nicht
//                                               nur der Wert umgeschlagen)
//   RotateEntity e,100,30,0  ->  80/-150/180
//
// Bis dahin gaben wir fuer den lokalen Fall e->rx/ry/rz zurueck, also genau
// das, was RotateEntity hineingeschrieben hatte. Solange alle drei Winkel im
// Bereich liegen, ist das dasselbe - deshalb faellt es erst an einem echten
// Programm auf. Jedes Programm, das einen Winkel ZURUECKLIEST und damit
// rechnet, bekam davor ein stilles Falschergebnis.
//
// Der gespeicherte Wert bleibt absichtlich unberuehrt: normalisiert wird nur
// beim Lesen. Wuerde RotateEntity selbst normalisieren, ginge jede Transform
// durch eine zusaetzliche Zerlegung, und die Bahnen wuerden sich um
// Rundungsstellen verschieben - die Gegenprobe an der BirdDemo haengt genau
// daran.
static inline void bb_entity_local_euler_(const bb_Entity_* e,
                                          float& rx, float& ry, float& rz) {
  float R[16];
  mat4_make_euler_YXZ_(R, e->rx, e->ry, e->rz);
  mat4_extract_euler_YXZ_(R, rx, ry, rz);
}


inline float bb_EntityPitch(int h, int glob = 0) {
  bb_Entity_* e = bb_entity_get_(h);
  if (!e) return 0;
  float rx, ry, rz;
  if (glob) mat4_extract_euler_YXZ_(e->world, rx, ry, rz);
  else      bb_entity_local_euler_(e, rx, ry, rz);
  return rx;
}
inline float bb_EntityYaw(int h, int glob = 0) {
  bb_Entity_* e = bb_entity_get_(h);
  if (!e) return 0;
  float rx, ry, rz;
  if (glob) mat4_extract_euler_YXZ_(e->world, rx, ry, rz);
  else      bb_entity_local_euler_(e, rx, ry, rz);
  return ry;
}
inline float bb_EntityRoll(int h, int glob = 0) {
  bb_Entity_* e = bb_entity_get_(h);
  if (!e) return 0;
  float rx, ry, rz;
  if (glob) mat4_extract_euler_YXZ_(e->world, rx, ry, rz);
  else      bb_entity_local_euler_(e, rx, ry, rz);
  return rz;
}

inline float bb_EntityDistance(int h1, int h2) {
  bb_Entity_* a = bb_entity_get_(h1);
  bb_Entity_* b = bb_entity_get_(h2);
  if (!a || !b) return 0;
  float dx = a->world[12] - b->world[12];
  float dy = a->world[13] - b->world[13];
  float dz = a->world[14] - b->world[14];
  return sqrtf(dx*dx + dy*dy + dz*dz);
}

// ============================================================
// Hierarchy (3D-05)
// ============================================================

// Re-parent entity.  glob=1 preserves world-space position/rotation/scale.
inline void bb_EntityParent(int h, int new_parent, int glob = 0) {
  bb_Entity_* e = bb_entity_get_(h);
  if (!e || e->handle == new_parent) return;

  // If preserving world coords, snapshot world matrix before any change.
  float saved_world[16];
  if (glob) memcpy(saved_world, e->world, 64);

  // Detach from current parent
  if (e->parent) {
    bb_Entity_* old_p = bb_entity_get_(e->parent);
    if (old_p) {
      auto& ch = old_p->children;
      ch.erase(std::remove(ch.begin(), ch.end(), h), ch.end());
    }
  }

  // Attach to new parent
  e->parent = new_parent;
  if (new_parent) {
    bb_Entity_* np = bb_entity_get_(new_parent);
    if (np) np->children.push_back(h);
  }

  if (glob) {
    // Compute new local matrix: inv(new_parent_world) * old_world
    float local[16];
    if (new_parent) {
      bb_Entity_* np = bb_entity_get_(new_parent);
      float inv_pw[16];
      if (np && mat4_inverse_(inv_pw, np->world))
        mat4_mul_(local, inv_pw, saved_world);
      else
        memcpy(local, saved_world, 64);
    } else {
      memcpy(local, saved_world, 64);
    }
    // Extract T, R, S from the local matrix
    e->px = local[12]; e->py = local[13]; e->pz = local[14];
    e->sx = sqrtf(local[0]*local[0] + local[1]*local[1] + local[2]*local[2]);
    e->sy = sqrtf(local[4]*local[4] + local[5]*local[5] + local[6]*local[6]);
    e->sz = sqrtf(local[8]*local[8] + local[9]*local[9] + local[10]*local[10]);
    mat4_extract_euler_YXZ_(local, e->rx, e->ry, e->rz);
  }
}

inline int bb_GetParent(int h) {
  bb_Entity_* e = bb_entity_get_(h);
  return e ? e->parent : 0;
}

// Recursive depth-first search for first child with matching name.
static inline int bb_find_child_rec_(int h, const bbString& name) {
  bb_Entity_* e = bb_entity_get_(h);
  if (!e) return 0;
  for (int c : e->children) {
    bb_Entity_* ch = bb_entity_get_(c);
    if (!ch) continue;
    if (ch->name == name) return c;
    int found = bb_find_child_rec_(c, name);
    if (found) return found;
  }
  return 0;
}

inline int bb_FindChild(int h, const bbString& name) {
  return bb_find_child_rec_(h, name);
}

inline int bb_CountChildren(int h) {
  bb_Entity_* e = bb_entity_get_(h);
  return e ? static_cast<int>(e->children.size()) : 0;
}

// 1-based child index.
inline int bb_GetChild(int h, int index) {
  bb_Entity_* e = bb_entity_get_(h);
  if (!e || index < 1 || index > static_cast<int>(e->children.size())) return 0;
  return e->children[index - 1];
}

inline void bb_EntityOrder(int h, int order) {
  bb_Entity_* e = bb_entity_get_(h);
  if (e) e->order = order;
}

// ============================================================
// Aussehen (3D-10)
//
// Alle Zusagen sind am laufenden Original nachgemessen, nicht dem eigenen
// Roadmap-Entwurf entnommen - der hatte bei EntityBlend die Modi 2 und 3
// vertauscht (siehe DEVLOG). Gemessen wurde jeweils der Bildpunkt in der
// Mitte einer Wuerfelflaeche ueber bekanntem Hintergrund.
// ============================================================

// 0-1, Vorgabe 1. Ein Wert von 0 wird laut Doku gar nicht gezeichnet, bleibt
// aber im Gegensatz zu HideEntity fuer Kollisionen vorhanden - gemessen:
// der Hintergrund steht unveraendert da.
inline void bb_EntityAlpha(int h, float alpha) {
  bb_Entity_* e = bb_entity_get_(h);
  if (!e) return;
  e->brush.alpha = (alpha < 0.0f) ? 0.0f : (alpha > 1.0f ? 1.0f : alpha);
}

// 0-255, Vorgabe 255,255,255. Die Farbe wird mit dem Beleuchtungsergebnis
// und der Textur multipliziert; geklemmt wird erst danach.
inline void bb_EntityColor(int h, float r, float g, float b) {
  bb_Entity_* e = bb_entity_get_(h);
  if (!e) return;
  e->brush.r = r; e->brush.g = g; e->brush.b = b;
}

inline void bb_EntityShininess(int h, float shininess) {
  bb_Entity_* e = bb_entity_get_(h);
  if (e) e->brush.shininess = shininess;
}

// 1 = Alpha, 2 = Multiply, 3 = Add. Die Vorgabe ist 0 - "nicht gesetzt";
// das Original leitet daraus "deckend" ab, solange weder die Deckkraft noch
// eine Textur etwas anderes verlangt.
inline void bb_EntityBlend(int h, int blend) {
  bb_Entity_* e = bb_entity_get_(h);
  if (e) e->brush.blend = blend;
}

inline void bb_EntityFX(int h, int fx) {
  bb_Entity_* e = bb_entity_get_(h);
  if (e) e->brush.fx = fx;
}

// ============================================================
// Brush einer Entity (3D-15)
// ============================================================

// Legt den Brush **ab**, er wird nicht gemerkt: das Original ruft
// m->setBrush( *b ) mit einer Wertklasse. Wer den Brush danach aendert,
// aendert diese Entity nicht mehr. Und weil es der ganze Brush ist, setzt
// PaintEntity auch Deckkraft, Glanz, Blend, FX und Texturen neu - ein
// vorher gesetztes EntityColor ist danach weg.
inline void bb_PaintEntity(int entity, int brush) {
  bb_Entity_* e = bb_entity_get_(entity);
  bb_Brush_*  b = bb_brush_get_(brush);
  if (!e || !b) return;
  e->brush = *b;
}

// Eine Kopie, kein Handle auf das Original - deshalb sagt die Doku, man
// solle sie mit FreeBrush wieder loswerden.
inline int bb_GetEntityBrush(int entity) {
  bb_Entity_* e = bb_entity_get_(entity);
  if (!e) return 0;
  return bb_brush_register_(e->brush);
}

// Gemessen: alpha = (far - Abstand) / (far - near), geklemmt auf 0..1, wobei
// der Abstand von der Kamera zum **Ursprung** des Entity zaehlt. Bei near
// und naeher ist es deckend, bei far und weiter unsichtbar.
inline void bb_EntityAutoFade(int h, float near_dist, float far_dist) {
  bb_Entity_* e = bb_entity_get_(h);
  if (!e) return;
  e->fadeNear = near_dist;
  e->fadeFar  = far_dist;
}

inline bbString bb_EntityClass(int h) {
  bb_Entity_* e = bb_entity_get_(h);
  if (!e) return "";
  switch (e->kind()) {
    case bb_EntityKind_::Pivot:  return "Pivot";
    case bb_EntityKind_::Mesh:   return "Mesh";
    case bb_EntityKind_::Camera: return "Camera";
    case bb_EntityKind_::Light:  return "Light";
    case bb_EntityKind_::Sprite: return "Sprite";
  }
  return "";
}

// ============================================================
// Quit hook — free all entities on shutdown
// ============================================================

inline void bb_entity_quit_() {
  bb_entities_.clear();
  bb_entity_next_id_ = 1;
}

// Registered into bb_entity_quit_hook_ (declared in bb_sdl.h, set here).
// The extern declaration avoids including bb_sdl.h from this header.
extern void (*bb_entity_quit_hook_)();
inline const bool bb_entity_hook_reg_ = (bb_entity_quit_hook_ = bb_entity_quit_, true);

#endif // BB_ENTITY_CORE_H
