#ifndef BB_PLANE_H
#define BB_PLANE_H

// Ebenen: CreatePlane (nach blitz3d/planemodel.cpp und bbblitz3d.cpp).
//
// Eine Ebene ist unendlich: y = 0 im eigenen Raum, Normale +y. Sie hat keine
// feste Geometrie - bei jedem Zeichnen wird der Sichtkegel der Kamera in den
// Raum der Ebene gebracht, seine ferne Flaeche in segs x segs Felder
// geteilt, und jedes Feld von der Kamera aus auf die Ebene projiziert:
// Ecken unter der Ebene wandern auf den Blickstrahl, Kanten, die die Ebene
// kreuzen, werden dort geschnitten (PlaneModel::Rep::render). Steht das
// Auge nicht ueber der Ebene, wird nichts gezeichnet - von unten ist sie
// unsichtbar. Die Texturkoordinaten sind x und z im Raum der Ebene, eine
// Textur liegt also einmal je Einheit und waechst mit ScaleEntity.
//
// Kollision und Picking (Methode 2, Dreiecke) treffen die Ebene selbst,
// nicht ihre gezeichneten Dreiecke: PlaneModel::collide schneidet die
// Bewegungslinie mit der um den Radius angehobenen Ebene (bb_collision.h).
//
// Ein Model wie Mesh und Sprite (EntityTexture, EntityColor, ...), aber kein
// Mesh: die Mesh-Befehle melden "Entity is not a mesh".

#include "bb_entity_core.h"
#include "bb_mesh_core.h"
#include <cmath>

struct bb_PlaneEntity_ : bb_Entity_ {
  int          segs = 1;
  bb_MeshData_ mesh;             // je Zeichnen neu gefuellt, im Raum der Ebene

  bb_EntityKind_ kind() const override { return bb_EntityKind_::Plane; }

  std::unique_ptr<bb_Entity_> clone() const override {
    auto c = std::make_unique<bb_PlaneEntity_>();
    bb_entity_copy_fields_(*c, *this);
    c->segs = segs;
    return c;
  }

  ~bb_PlaneEntity_() override { bb_mesh_free_gpu_(&mesh); }
};

// Das Original prueft 1..20 im Debug-Modus. Sein Raster fasst nur 17 x 17
// Punkte (static Vector vts[17][17]) - mehr als 16 schreibt dort ueber das
// Feld hinaus; hier ist Platz fuer 20.
inline int bb_CreatePlane(int segs = 1, int parent = 0) {
  bb_parent_chk_(parent);
  if (segs < 1 || segs > 20) bb_RuntimeError("Illegal number of segments");
  auto ent = std::make_unique<bb_PlaneEntity_>();
  ent->segs = segs;
  return bb_entity_register_(std::move(ent), parent);
}

// PlaneModel::Rep::render. `view` ist die GL-Sichtmatrix (Kamera schaut nach
// -z), `f` der Kegel wie fuer das Verwerfen. Liefert false, wenn nichts zu
// zeichnen ist.
static inline bool bb_plane_build_(bb_PlaneEntity_* pl, const float* view,
                                   const bb_CullFrustum_& f) {
  bb_MeshData_& m = pl->mesh;
  m.vertices.clear();
  m.indices.clear();
  m.dirty = true;

  // Kamera -> Welt -> Ebene
  float cam[16], inv[16], tf[16];
  if (!mat4_inverse_(cam, view) || !mat4_inverse_(inv, pl->world)) return false;
  mat4_mul_(tf, inv, cam);
  struct V { float x, y, z; };
  auto xf = [&](float x, float y, float z) {
    float p[3];
    mat4_xform_pt_(p, tf, x, y, z);
    return V{ p[0], p[1], p[2] };
  };
  auto lerp = [](const V& a, const V& b, float t) {
    return V{ (b.x - a.x) * t + a.x, (b.y - a.y) * t + a.y, (b.z - a.z) * t + a.z };
  };

  const V eye = xf(0, 0, 0);
  if (eye.y <= 0) return false;

  // Frustum(nr, fr, w, h): die fernen Ecken, im Kameraraum des Originals
  // (z nach vorn) - hier GL, also z = -fr.
  const float w = f.fr * f.sx, h = f.fr * f.sy;
  const V tl = xf(-w,  h, -f.fr), tr = xf(w,  h, -f.fr);
  const V br = xf( w, -h, -f.fr), bl = xf(-w, -h, -f.fr);

  const int n = pl->segs;
  V vts[21][21];
  for (int x = 0; x <= n; ++x) {
    const float t = float(x) / float(n);
    const V tx = lerp(tl, tr, t), bx = lerp(bl, br, t);
    for (int y = 0; y <= n; ++y) vts[x][y] = lerp(tx, bx, float(y) / float(n));
  }

  unsigned int vc = 0;
  for (int x = 0; x < n; ++x) {
    for (int y = 0; y < n; ++y) {
      const V in[4] = { vts[x][y], vts[x + 1][y], vts[x + 1][y + 1], vts[x][y + 1] };
      V out[8];
      int cnt = 0;
      for (int k = 0; k < 4; ++k) {
        const V& v = in[k];
        const V& p = in[(k - 1) & 3];
        if (v.y > 0) {
          if (p.y <= 0) out[cnt++] = lerp(p, v, p.y / (p.y - v.y));
        } else {
          if (p.y > 0) out[cnt++] = lerp(p, v, p.y / (p.y - v.y));
          // Plane::intersect(Line(eye, v - eye)) mit y = 0
          const V d{ v.x - eye.x, v.y - eye.y, v.z - eye.z };
          out[cnt++] = lerp(eye, V{ eye.x + d.x, eye.y + d.y, eye.z + d.z }, -eye.y / d.y);
        }
      }
      if (cnt < 3 || cnt > 5) continue;
      for (int k = 0; k < cnt; ++k)
        bb_vert_push_(m, out[k].x, out[k].y, out[k].z, 0, 1, 0, out[k].x, out[k].z);
      for (int k = 2; k < cnt; ++k) {
        m.indices.push_back(vc);
        m.indices.push_back(vc + k - 1);
        m.indices.push_back(vc + k);
      }
      vc += cnt;
    }
  }
  return vc >= 3;
}

#endif // BB_PLANE_H
