#ifndef BB_COLLISION_H
#define BB_COLLISION_H

// Kollisionen (3D-18) und Picking (3D-17).
//
// Uebersetzt aus blitz3d/collision.cpp (Kugel gegen Dreieck, Kante, Ecke),
// blitz3d/world.cpp (World::collide, traceRay, checkLOS) und den Befehlen in
// bbruntime/bbblitz3d.cpp. Die Namen und die Reihenfolge der Rechenschritte
// sind absichtlich dieselben - die Rueckgabewerte (CollisionX/NX/Time,
// PickedX/NX/Time) sind beobachtbar und muessen stimmen.
//
// Drei Methoden: 1 Kugel gegen Kugel, 2 Kugel gegen Dreiecke, 3 Kugel gegen
// Box. Vier Reaktionen: 0 nichts, 1 anhalten, 2 gleiten, 3 gleiten ohne Y.
//
// Nicht uebernommen ist der Dreiecksbaum des Originals (MeshCollider): er
// spart nur Arbeit, am Ergebnis aendert er nichts. Hier laufen alle Dreiecke
// der Reihe nach.

#include "bb_entity_core.h"
#include "bb_mesh.h"
#include "bb_surface.h"
#include <cmath>
#include <map>
#include <vector>

// ============================================================
// Kleine Geometrie - wie geom.h, nur mit float[3]
// ============================================================

inline constexpr float BB_COLL_EPSILON_ = 0.001f;   // COLLISION_EPSILON
inline constexpr float BB_GEOM_EPSILON_ = 0.000001f; // EPSILON

struct bb_V3_ { float x = 0, y = 0, z = 0; };

static inline bb_V3_ bb_v3_(float x, float y, float z) { return { x, y, z }; }
static inline bb_V3_ operator+(const bb_V3_& a, const bb_V3_& b) { return { a.x+b.x, a.y+b.y, a.z+b.z }; }
static inline bb_V3_ operator-(const bb_V3_& a, const bb_V3_& b) { return { a.x-b.x, a.y-b.y, a.z-b.z }; }
static inline bb_V3_ operator*(const bb_V3_& a, float s)         { return { a.x*s, a.y*s, a.z*s }; }
static inline bb_V3_ operator-(const bb_V3_& a)                  { return { -a.x, -a.y, -a.z }; }
static inline float  bb_dot_(const bb_V3_& a, const bb_V3_& b)   { return a.x*b.x + a.y*b.y + a.z*b.z; }
static inline bb_V3_ bb_cross_(const bb_V3_& a, const bb_V3_& b) {
  return { a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x };
}
static inline float  bb_len_(const bb_V3_& a) { return sqrtf(bb_dot_(a, a)); }
static inline bb_V3_ bb_norm_(const bb_V3_& a) {
  const float l = bb_len_(a);
  return l ? bb_V3_{ a.x/l, a.y/l, a.z/l } : a;
}
static inline float bb_dist_(const bb_V3_& a, const bb_V3_& b) { return bb_len_(b - a); }

// Punkt bzw. Richtung mit einer Weltmatrix (spaltenweise) abbilden
static inline bb_V3_ bb_xf_pt_(const float* m, const bb_V3_& v) {
  return { m[0]*v.x + m[4]*v.y + m[8]*v.z  + m[12],
           m[1]*v.x + m[5]*v.y + m[9]*v.z  + m[13],
           m[2]*v.x + m[6]*v.y + m[10]*v.z + m[14] };
}

struct bb_Line_ { bb_V3_ o, d; };
static inline bb_V3_ bb_line_at_(const bb_Line_& l, float t) { return l.o + l.d * t; }

struct bb_Plane_ { bb_V3_ n; float d = 0; };

// Plane(v0,v1,v2): n = ((v1-v0) x (v2-v0)).normalized, d = -n.v0
static inline bb_Plane_ bb_plane_tri_(const bb_V3_& v0, const bb_V3_& v1, const bb_V3_& v2) {
  bb_Plane_ p;
  p.n = bb_norm_(bb_cross_(v1 - v0, v2 - v0));
  p.d = -bb_dot_(p.n, v0);
  return p;
}
static inline bb_Plane_ bb_plane_pt_(const bb_V3_& pt, const bb_V3_& n) {
  return { n, -bb_dot_(n, pt) };
}
static inline float bb_plane_dist_(const bb_Plane_& p, const bb_V3_& q) {
  return bb_dot_(p.n, q) + p.d;
}
static inline float bb_plane_t_(const bb_Plane_& p, const bb_Line_& q) {
  return -bb_plane_dist_(p, q.o) / bb_dot_(p.n, q.d);
}
static inline bb_V3_ bb_plane_nearest_(const bb_Plane_& p, const bb_V3_& q) {
  return q - p.n * bb_plane_dist_(p, q);
}
static inline bb_V3_ bb_line_nearest_(const bb_Line_& l, const bb_V3_& q) {
  return l.o + l.d * (bb_dot_(l.d, q - l.o) / bb_dot_(l.d, l.d));
}
// Schnittgerade zweier Ebenen, wie Plane::intersect(Plane)
static inline bb_Line_ bb_plane_cross_(const bb_Plane_& a, const bb_Plane_& b) {
  const bb_V3_ lv = bb_norm_(bb_cross_(a.n, b.n));
  const bb_Line_ helper{ bb_plane_nearest_(a, a.n * -a.d), bb_cross_(a.n, lv) };
  return { bb_line_at_(helper, bb_plane_t_(b, helper)), lv };
}

// ============================================================
// Collision - der Zustand eines laufenden Tests
// ============================================================

struct bb_Coll_ {
  float  time = 1;
  bb_V3_ normal;
  int    surface = 0;   // Flaechenhandle wie von GetSurface, 0 = keine
  // Collision::index ist im Original ein unsigned short und startet auf ~0;
  // CollisionTriangle meldet nach einem Kugel- oder Boxtreffer darum 65535.
  int    index = 65535; // Dreieck in dieser Flaeche
};

// Collision::update
static inline bool bb_coll_update_(bb_Coll_& c, const bb_Line_& line, float t, const bb_V3_& n) {
  if (t > c.time) return false;
  const bb_Plane_ p = bb_plane_pt_(bb_line_at_(line, t), n);
  if (bb_dot_(p.n, line.d) >= 0) return false;
  if (bb_plane_dist_(p, line.o) < -BB_COLL_EPSILON_) return false;
  c.time = t;
  c.normal = n;
  return true;
}

// Collision::sphereCollide
static inline bool bb_coll_sphere_(bb_Coll_& c, const bb_Line_& line, float radius,
                                   const bb_V3_& dest, float dest_radius) {
  radius += dest_radius;
  const bb_Line_ l{ line.o - dest, line.d };
  const float a = bb_dot_(l.d, l.d);
  if (!a) return false;
  const float b = bb_dot_(l.o, l.d) * 2;
  const float cc = bb_dot_(l.o, l.o) - radius * radius;
  const float d = b*b - 4*a*cc;
  if (d < 0) return false;
  const float t1 = (-b + sqrtf(d)) / (2*a);
  const float t2 = (-b - sqrtf(d)) / (2*a);
  const float t = t1 < t2 ? t1 : t2;
  if (t > c.time) return false;
  return bb_coll_update_(c, line, t, bb_norm_(bb_line_at_(l, t)));
}

// edgeTest: Zylinder um die Kante, darunter eine Kugel um die Ecke.
// tm ist die transponierte Basis (Kantennormale, Kantenrichtung, Flaechennormale).
static inline bool bb_coll_edge_(bb_Coll_& c, const bb_V3_& v0, const bb_V3_& v1,
                                 const bb_V3_& pn, const bb_V3_& en,
                                 const bb_Line_& line, float radius) {
  const bb_V3_ ex = en, ey = bb_norm_(v1 - v0), ez = pn;
  // ~Matrix(en, dir, pn) * v: Zeilen der Basis als Skalarprodukte
  auto tm = [&](const bb_V3_& v) {
    return bb_V3_{ ex.x*v.x + ex.y*v.y + ex.z*v.z,
                   ey.x*v.x + ey.y*v.y + ey.z*v.z,
                   ez.x*v.x + ez.y*v.y + ez.z*v.z };
  };
  // ~tm * v: zurueck, also die Basis selbst
  auto itm = [&](const bb_V3_& v) {
    return bb_V3_{ ex.x*v.x + ey.x*v.y + ez.x*v.z,
                   ex.y*v.x + ey.y*v.y + ez.y*v.z,
                   ex.z*v.x + ey.z*v.y + ez.z*v.z };
  };
  const bb_V3_ sv = tm(line.o - v0);
  const bb_V3_ dv = tm(line.o + line.d - v0);
  const bb_Line_ l{ sv, dv - sv };

  float a = l.d.x*l.d.x + l.d.z*l.d.z;
  if (!a) return false;                       // parallel zum Zylinder
  float b = (l.o.x*l.d.x + l.o.z*l.d.z) * 2;
  float cc = (l.o.x*l.o.x + l.o.z*l.o.z) - radius * radius;
  float d = b*b - 4*a*cc;
  if (d < 0) return false;                    // verfehlt den Zylinder
  float t1 = (-b + sqrtf(d)) / (2*a);
  float t2 = (-b - sqrtf(d)) / (2*a);
  float t = t1 < t2 ? t1 : t2;
  if (t > c.time) return false;
  bb_V3_ i = bb_line_at_(l, t), p;
  if (i.y > bb_dist_(v0, v1)) return false;   // ueber dem Zylinder
  if (i.y >= 0) {
    p.y = i.y;
  } else {
    // unter dem Zylinder: Kugeltest um die Ecke
    a = bb_dot_(l.d, l.d);
    if (!a) return false;
    b = bb_dot_(l.o, l.d) * 2;
    cc = bb_dot_(l.o, l.o) - radius * radius;
    d = b*b - 4*a*cc;
    if (d < 0) return false;
    t1 = (-b + sqrtf(d)) / (2*a);
    t2 = (-b - sqrtf(d)) / (2*a);
    t = t1 < t2 ? t1 : t2;
    if (t > c.time) return false;
    i = bb_line_at_(l, t);
  }
  return bb_coll_update_(c, line, t, bb_norm_(itm(i - p)));
}

// Collision::triangleCollide
static inline bool bb_coll_tri_(bb_Coll_& c, const bb_Line_& line, float radius,
                                const bb_V3_& v0, const bb_V3_& v1, const bb_V3_& v2) {
  bb_Plane_ p = bb_plane_tri_(v0, v1, v2);
  if (bb_dot_(p.n, line.d) >= 0) return false;

  p.d -= radius;                              // Ebene nach aussen schieben
  const float t = bb_plane_t_(p, line);
  if (t > c.time) return false;

  const bb_Plane_ p0 = bb_plane_tri_(v0 + p.n, v1, v0);
  const bb_Plane_ p1 = bb_plane_tri_(v1 + p.n, v2, v1);
  const bb_Plane_ p2 = bb_plane_tri_(v2 + p.n, v0, v2);

  const bb_V3_ i = bb_line_at_(line, t);
  if (bb_plane_dist_(p0, i) >= 0 && bb_plane_dist_(p1, i) >= 0 && bb_plane_dist_(p2, i) >= 0)
    return bb_coll_update_(c, line, t, p.n);

  if (radius <= 0) return false;

  const bool a = bb_coll_edge_(c, v0, v1, p.n, p0.n, line, radius);
  const bool b = bb_coll_edge_(c, v1, v2, p.n, p1.n, line, radius);
  const bool d = bb_coll_edge_(c, v2, v0, p.n, p2.n, line, radius);
  return a | b | d;
}

// Collision::boxCollide - die sechs Seiten der Box, Ecken wie im Original
static inline bb_V3_ bb_box_corner_(const float* a, const float* b, int n) {
  return { (n & 1) ? b[0] : a[0], (n & 2) ? b[1] : a[1], (n & 4) ? b[2] : a[2] };
}

static inline bool bb_coll_box_(bb_Coll_& c, const bb_Line_& line, float radius,
                                const float* boxA, const float* boxB) {
  static const int quads[] = {
    2,3,1,0, 3,7,5,1, 7,6,4,5, 6,2,0,4, 6,7,3,2, 0,1,5,4
  };
  bool hit = false;
  for (int n = 0; n < 24; n += 4) {
    const bb_V3_ v0 = bb_box_corner_(boxA, boxB, quads[n]);
    const bb_V3_ v1 = bb_box_corner_(boxA, boxB, quads[n+1]);
    const bb_V3_ v2 = bb_box_corner_(boxA, boxB, quads[n+2]);
    const bb_V3_ v3 = bb_box_corner_(boxA, boxB, quads[n+3]);

    bb_Plane_ p = bb_plane_tri_(v0, v1, v2);
    if (bb_dot_(p.n, line.d) >= 0) continue;

    p.d -= radius;
    const float t = bb_plane_t_(p, line);
    if (t > c.time) return hit;               // wie im Original: return, nicht continue

    const bb_Plane_ p0 = bb_plane_tri_(v0 + p.n, v1, v0);
    const bb_Plane_ p1 = bb_plane_tri_(v1 + p.n, v2, v1);
    const bb_Plane_ p2 = bb_plane_tri_(v2 + p.n, v3, v2);
    const bb_Plane_ p3 = bb_plane_tri_(v3 + p.n, v0, v3);

    const bb_V3_ i = bb_line_at_(line, t);
    if (bb_plane_dist_(p0, i) >= 0 && bb_plane_dist_(p1, i) >= 0 &&
        bb_plane_dist_(p2, i) >= 0 && bb_plane_dist_(p3, i) >= 0) {
      hit |= bb_coll_update_(c, line, t, p.n);
      continue;
    }
    if (radius <= 0) continue;
    const bool e0 = bb_coll_edge_(c, v0, v1, p.n, p0.n, line, radius);
    const bool e1 = bb_coll_edge_(c, v1, v2, p.n, p1.n, line, radius);
    const bool e2 = bb_coll_edge_(c, v2, v3, p.n, p2.n, line, radius);
    const bool e3 = bb_coll_edge_(c, v3, v0, p.n, p3.n, line, radius);
    hit |= (e0 | e1 | e2 | e3);
  }
  return hit;
}

// ============================================================
// Ein Ziel pruefen (World::hitTest)
// ============================================================

// ============================================================
// Dreiecksbaum (MeshCollider)
// ============================================================
//
// Der Baum spart nicht nur Arbeit, er ist auch beobachtbar: treffen mehrere
// Dreiecke gleich frueh, gewinnt das **zuletzt gepruefte**, und genau das
// meldet CollisionTriangle. Deshalb ist er wie im Original gebaut - Blaetter
// mit hoechstens 16 Dreiecken, geteilt an der laengsten Achse der Huellbox,
// nach den Schwerpunkten sortiert (multimap: gleiche Schluessel behalten ihre
// Reihenfolge), linke Haelfte = die ersten size/2 Eintraege.
struct bb_CollTri_ { int v[3]; int surf; int index; };

struct bb_CollNode_ {
  float a[3] = {  1e7f,  1e7f,  1e7f };
  float b[3] = { -1e7f, -1e7f, -1e7f };
  int left = -1, right = -1;
  std::vector<int> tris;
};

struct bb_Collider_ {
  std::vector<bb_V3_>       verts;
  std::vector<bb_CollTri_>  tris;
  std::vector<bb_V3_>       centres;
  std::vector<bb_CollNode_> nodes;
  int root = -1;
};

static inline void bb_box_update_(bb_CollNode_& n, const bb_V3_& q) {
  if (q.x < n.a[0]) n.a[0] = q.x;
  if (q.y < n.a[1]) n.a[1] = q.y;
  if (q.z < n.a[2]) n.a[2] = q.z;
  if (q.x > n.b[0]) n.b[0] = q.x;
  if (q.y > n.b[1]) n.b[1] = q.y;
  if (q.z > n.b[2]) n.b[2] = q.z;
}

static inline bool bb_box_overlap_(const float* a0, const float* b0,
                                   const float* a1, const float* b1) {
  return a0[0] <= b1[0] && b0[0] >= a1[0] &&
         a0[1] <= b1[1] && b0[1] >= a1[1] &&
         a0[2] <= b1[2] && b0[2] >= a1[2];
}

static inline int bb_collider_node_(bb_Collider_& c, const std::vector<int>& tris) {
  const int me = static_cast<int>(c.nodes.size());
  c.nodes.push_back({});
  {
    bb_CollNode_& n = c.nodes[me];
    for (int t : tris)
      for (int j = 0; j < 3; ++j) bb_box_update_(n, c.verts[c.tris[t].v[j]]);
  }
  if (tris.size() <= 16) {                   // MAX_COLL_TRIS
    c.nodes[me].tris = tris;
    return me;
  }
  const float w = c.nodes[me].b[0] - c.nodes[me].a[0];
  const float h = c.nodes[me].b[1] - c.nodes[me].a[1];
  const float d = c.nodes[me].b[2] - c.nodes[me].a[2];
  float mx = w;
  if (h > mx) mx = h;
  if (d > mx) mx = d;
  int axis = 0;
  if (mx == h) axis = 1;
  else if (mx == d) axis = 2;

  std::multimap<float, int> axis_map;
  for (int t : tris) {
    const bb_V3_& ctr = c.centres[t];
    const float key = (axis == 0) ? ctr.x : (axis == 1 ? ctr.y : ctr.z);
    axis_map.insert({ key, t });
  }
  std::vector<int> part;
  auto it = axis_map.begin();
  for (size_t k = axis_map.size() / 2; k--; ++it) part.push_back(it->second);
  const int l = bb_collider_node_(c, part);
  part.clear();
  for (; it != axis_map.end(); ++it) part.push_back(it->second);
  const int r = bb_collider_node_(c, part);
  c.nodes[me].left  = l;
  c.nodes[me].right = r;
  return me;
}

// Den Baum dieses Netzes holen; nach einer Aenderung an der Geometrie neu.
static inline bb_Collider_* bb_collider_for_(bb_MeshEntity_* me) {
  auto& rep = *me->rep;
  if (rep.collider && rep.collider_stamp == bb_mesh_geom_version_)
    return static_cast<bb_Collider_*>(rep.collider.get());

  auto c = std::make_shared<bb_Collider_>();
  const size_t nsurf = me->surfaces().size();
  for (size_t si = 0; si < nsurf; ++si) {
    bb_MeshData_& surf = me->surfaces()[si];
    const int base = static_cast<int>(c->verts.size());
    const size_t nv = surf.vertices.size() / BB_VF;
    for (size_t k = 0; k < nv; ++k) {
      const float* vd = &surf.vertices[k * BB_VF];
      c->verts.push_back({ vd[0], vd[1], vd[2] });
    }
    const int sh = bb_surface_handle_(me->handle, static_cast<int>(si));
    const size_t ntri = surf.indices.size() / 3;
    for (size_t k = 0; k < ntri; ++k) {
      const unsigned int* idx = &surf.indices[k * 3];
      bb_CollTri_ t;
      t.v[0] = base + static_cast<int>(idx[0]);
      t.v[1] = base + static_cast<int>(idx[1]);
      t.v[2] = base + static_cast<int>(idx[2]);
      t.surf  = sh;
      t.index = static_cast<int>(k);
      c->tris.push_back(t);
    }
  }
  std::vector<int> all;
  for (size_t k = 0; k < c->tris.size(); ++k) {
    const bb_CollTri_& t = c->tris[k];
    c->centres.push_back((c->verts[t.v[0]] + c->verts[t.v[1]] + c->verts[t.v[2]]) * (1.0f/3.0f));
    all.push_back(static_cast<int>(k));
  }
  if (!all.empty()) c->root = bb_collider_node_(*c, all);

  rep.collider = c;
  rep.collider_stamp = bb_mesh_geom_version_;
  return c.get();
}

static inline bool bb_collider_walk_(bb_Coll_& coll, const bb_Collider_& c, int node,
                                     const float* lbA, const float* lbB,
                                     const bb_Line_& line, float radius, const float* tf) {
  const bb_CollNode_& n = c.nodes[node];
  if (!bb_box_overlap_(lbA, lbB, n.a, n.b)) return false;

  bool hit = false;
  if (n.tris.empty()) {
    if (n.left  >= 0) hit |= bb_collider_walk_(coll, c, n.left,  lbA, lbB, line, radius, tf);
    if (n.right >= 0) hit |= bb_collider_walk_(coll, c, n.right, lbA, lbB, line, radius, tf);
    return hit;
  }

  for (int ti : n.tris) {
    const bb_CollTri_& tri = c.tris[ti];
    const bb_V3_& v0 = c.verts[tri.v[0]];
    const bb_V3_& v1 = c.verts[tri.v[1]];
    const bb_V3_& v2 = c.verts[tri.v[2]];
    bb_CollNode_ tb;
    bb_box_update_(tb, v0);
    bb_box_update_(tb, v1);
    bb_box_update_(tb, v2);
    if (!bb_box_overlap_(tb.a, tb.b, lbA, lbB)) continue;
    if (!bb_coll_tri_(coll, line, radius,
                      bb_xf_pt_(tf, v0), bb_xf_pt_(tf, v1), bb_xf_pt_(tf, v2))) continue;
    coll.surface = tri.surf;
    coll.index   = tri.index;
    hit = true;
  }
  return hit;
}

// Die Dreiecke eines Netzes, mit `tf` in den Raum der Linie gebracht
// (MeshCollider::collide).
static inline bool bb_coll_mesh_(bb_Coll_& c, const bb_Line_& line, float radius,
                                 bb_MeshEntity_* me, const float* tf) {
  bb_Collider_* col = bb_collider_for_(me);
  if (!col || col->root < 0) return false;

  // Huellbox der Linie, um den Radius aufgeblasen und in den Raum des Netzes
  // gebracht (local_box = -t * box).
  bb_CollNode_ wb;
  bb_box_update_(wb, line.o);
  bb_box_update_(wb, line.o + line.d);
  for (int k = 0; k < 3; ++k) { wb.a[k] -= radius; wb.b[k] += radius; }
  float inv[16];
  if (!mat4_inverse_(inv, tf)) return false;
  bb_CollNode_ lb;
  for (int n = 0; n < 8; ++n) {
    const bb_V3_ corner{ (n & 1) ? wb.b[0] : wb.a[0],
                         (n & 2) ? wb.b[1] : wb.a[1],
                         (n & 4) ? wb.b[2] : wb.a[2] };
    bb_box_update_(lb, bb_xf_pt_(inv, corner));
  }
  return bb_collider_walk_(c, *col, col->root, lb.a, lb.b, line, radius, tf);
}

// Die Weltmatrix ohne Verschiebung umkehren, wie ~Transform (transponierte
// Drehung); die Skalierung bleibt dabei aussen vor - so auch im Original.
static inline void bb_tf_invert_(const float* m, float* out) {
  for (int c = 0; c < 3; ++c)
    for (int r = 0; r < 3; ++r) out[c*4+r] = m[r*4+c];
  out[3] = out[7] = out[11] = 0; out[15] = 1;
  const bb_V3_ v{ -m[12], -m[13], -m[14] };
  out[12] = out[0]*v.x + out[4]*v.y + out[8]*v.z;
  out[13] = out[1]*v.x + out[5]*v.y + out[9]*v.z;
  out[14] = out[2]*v.x + out[6]*v.y + out[10]*v.z;
}

static inline bool bb_hit_test_(const bb_Line_& line, float radius, bb_Entity_* obj,
                                const float* tf, int method, bb_Coll_& c) {
  switch (method) {
    case 1:   // Kugel
      return bb_coll_sphere_(c, line, radius, { tf[12], tf[13], tf[14] }, obj->collRadX);
    case 2: { // Dreiecke
      auto* me = (obj->kind() == bb_EntityKind_::Mesh)
                   ? static_cast<bb_MeshEntity_*>(obj) : nullptr;
      if (!me) return false;   // Object::collide liefert sonst false
      return bb_coll_mesh_(c, line, radius, me, tf);
    }
    case 3: { // Box
      // Das Original normiert die drei Achsen, bevor es umkehrt: eine
      // Skalierung des Ziels wirkt auf die Kollisionsbox **nicht** (gemessen
      // 2026-09-17 an einer Wand mit ScaleEntity 5,5,0.5).
      float t[16];
      memcpy(t, tf, sizeof(t));
      for (int col = 0; col < 3; ++col) {
        bb_V3_ a{ t[col*4], t[col*4+1], t[col*4+2] };
        a = bb_norm_(a);
        t[col*4] = a.x; t[col*4+1] = a.y; t[col*4+2] = a.z;
      }
      float inv[16];
      bb_tf_invert_(t, inv);
      const bb_V3_ o = bb_xf_pt_(inv, line.o);
      const bb_V3_ e = bb_xf_pt_(inv, line.o + line.d);
      const bb_Line_ local{ o, e - o };
      bb_Coll_ local_coll = c;
      if (!bb_coll_box_(local_coll, local, radius, obj->collBoxA, obj->collBoxB)) return false;
      c.time = local_coll.time;
      // Die Normale zurueck in den Weltraum drehen
      const bb_V3_ n = local_coll.normal;
      c.normal = { t[0]*n.x + t[4]*n.y + t[8]*n.z,
                   t[1]*n.x + t[5]*n.y + t[9]*n.z,
                   t[2]*n.x + t[6]*n.y + t[10]*n.z };
      return true;
    }
  }
  return false;
}

// ============================================================
// Die Regeln: Collisions / ClearCollisions
// ============================================================

struct bb_CollInfo_ { int dstType, method, response; };

inline std::vector<bb_CollInfo_> bb_coll_rules_[1000];

inline void bb_ClearCollisions() {
  for (int k = 0; k < 1000; ++k) bb_coll_rules_[k].clear();
}

inline void bb_Collisions(int src_type, int dest_type, int method, int response) {
  if (src_type < 0 || src_type > 999 || dest_type < 0 || dest_type > 999) return;
  bb_coll_rules_[src_type].push_back({ dest_type, method, response });
}

// ============================================================
// Befehle am Entity
// ============================================================

static inline void bb_entity_type_rec_(bb_Entity_* e, int type) {
  e->collType = type;
  bb_ResetEntity(e->handle);
  for (int k : e->children)
    if (bb_Entity_* c = bb_entity_get_(k)) bb_entity_type_rec_(c, type);
}

inline void bb_EntityType(int entity, int type, int recursive = 0) {
  bb_Entity_* e = bb_entity_get_(entity);
  if (!e || type < 0 || type > 999) return;
  if (recursive) { bb_entity_type_rec_(e, type); return; }
  e->collType = type;
  bb_ResetEntity(entity);
}

inline int bb_GetEntityType(int entity) {
  bb_Entity_* e = bb_entity_get_(entity);
  return e ? e->collType : 0;
}

// Der Obscurer ist im Original vorbelegt: EntityPickMode%...%obscurer=1.
inline void bb_EntityPickMode(int entity, int mode, int obscurer = 1) {
  bb_Entity_* e = bb_entity_get_(entity);
  if (!e) return;
  e->pickMode = mode;
  e->obscurer = obscurer != 0;
}

// EntityRadius y=0 bedeutet "wie x" (bbEntityRadius).
inline void bb_EntityRadius(int entity, float x_radius, float y_radius = 0) {
  bb_Entity_* e = bb_entity_get_(entity);
  if (!e) return;
  e->collRadX = x_radius;
  e->collRadY = y_radius ? y_radius : x_radius;
}

inline void bb_EntityBox(int entity, float x, float y, float z, float w, float h, float d) {
  bb_Entity_* e = bb_entity_get_(entity);
  if (!e) return;
  const float ax = x, ay = y, az = z, bx = x + w, by = y + h, bz = z + d;
  e->collBoxA[0] = ax < bx ? ax : bx;
  e->collBoxA[1] = ay < by ? ay : by;
  e->collBoxA[2] = az < bz ? az : bz;
  e->collBoxB[0] = ax > bx ? ax : bx;
  e->collBoxB[1] = ay > by ? ay : by;
  e->collBoxB[2] = az > bz ? az : bz;
}

// ============================================================
// Abfragen nach dem UpdateWorld
// ============================================================

inline int bb_CountCollisions(int entity) {
  bb_Entity_* e = bb_entity_get_(entity);
  return e ? static_cast<int>(e->colls.size()) : 0;
}

static inline const bb_ObjColl_* bb_coll_at_(int entity, int index) {
  bb_Entity_* e = bb_entity_get_(entity);
  if (!e || index < 1 || index > static_cast<int>(e->colls.size())) return nullptr;
  return &e->colls[index - 1];
}

inline int bb_EntityCollided(int entity, int type) {
  bb_Entity_* e = bb_entity_get_(entity);
  if (!e) return 0;
  for (const auto& c : e->colls) {
    bb_Entity_* w = bb_entity_get_(c.with);
    if (w && w->collType == type) return c.with;
  }
  return 0;
}

inline float bb_CollisionX(int entity, int index) { auto* c = bb_coll_at_(entity, index); return c ? c->coords[0] : 0; }
inline float bb_CollisionY(int entity, int index) { auto* c = bb_coll_at_(entity, index); return c ? c->coords[1] : 0; }
inline float bb_CollisionZ(int entity, int index) { auto* c = bb_coll_at_(entity, index); return c ? c->coords[2] : 0; }
inline float bb_CollisionNX(int entity, int index) { auto* c = bb_coll_at_(entity, index); return c ? c->normal[0] : 0; }
inline float bb_CollisionNY(int entity, int index) { auto* c = bb_coll_at_(entity, index); return c ? c->normal[1] : 0; }
inline float bb_CollisionNZ(int entity, int index) { auto* c = bb_coll_at_(entity, index); return c ? c->normal[2] : 0; }
inline float bb_CollisionTime(int entity, int index) { auto* c = bb_coll_at_(entity, index); return c ? c->time : 0; }
inline int   bb_CollisionEntity(int entity, int index) { auto* c = bb_coll_at_(entity, index); return c ? c->with : 0; }
inline int   bb_CollisionTriangle(int entity, int index) { auto* c = bb_coll_at_(entity, index); return c ? c->index : 0; }

// CollisionSurface liefert im Original den Zeiger auf die Surface; bei uns
// ist es das Handle, unter dem GetSurface sie meldet (bb_surface.h).
inline int bb_CollisionSurface(int entity, int index) {
  auto* c = bb_coll_at_(entity, index);
  return c ? c->surface : 0;
}

// ============================================================
// World::collide - ein Entity von seiner vorigen Lage zur neuen bewegen
// ============================================================

inline std::vector<bb_Entity_*> bb_coll_by_type_[1000];

static inline void bb_coll_register_(bb_Entity_* src, bb_Entity_* dst,
                                     const bb_Line_& line, const bb_Coll_& coll,
                                     float inv_y_scale) {
  bb_ObjColl_ c;
  c.with = dst->handle;
  const bb_V3_ p = bb_line_at_(line, coll.time) - coll.normal * src->collRadX;
  c.coords[0] = p.x;
  c.coords[1] = p.y * inv_y_scale;
  c.coords[2] = p.z;
  c.time      = coll.time;
  c.normal[0] = coll.normal.x;
  c.normal[1] = coll.normal.y;
  c.normal[2] = coll.normal.z;
  c.surface   = coll.surface;
  c.index     = coll.index;
  src->colls.push_back(c);
  // Das Original traegt denselben Treffer auch beim Ziel ein.
  bb_ObjColl_ d = c;
  d.with = src->handle;
  dst->colls.push_back(d);
}

static inline void bb_world_collide_(bb_Entity_* src) {
  static const int MAX_HITS = 10;

  bb_V3_ dv{ src->world[12], src->world[13], src->world[14] };
  bb_V3_ sv{ src->prev[12],  src->prev[13],  src->prev[14]  };

  if (sv.x == dv.x && sv.y == dv.y && sv.z == dv.z) return;

  const bb_V3_ panic = sv;

  const float radius = src->collRadX;
  float y_scale = 1, inv_y_scale = 1;
  if (src->collRadX != src->collRadY) {
    y_scale = radius / src->collRadY;
    inv_y_scale = 1 / y_scale;
    sv.y *= y_scale;
    dv.y *= y_scale;
  }

  int n_hit = 0;
  bb_Plane_ planes[2];
  bb_Line_ coll_line{ sv, dv - sv };
  const bb_V3_ dir = coll_line.d;

  float td = bb_len_(coll_line.d);
  float td_xz = bb_len_({ coll_line.d.x, 0, coll_line.d.z });

  const std::vector<bb_CollInfo_>& rules = bb_coll_rules_[src->collType];

  int hits = 0;
  for (;;) {
    bb_Coll_ coll;
    bb_Entity_* coll_obj = nullptr;
    const bb_CollInfo_* coll_info = nullptr;

    for (const auto& rule : rules) {
      for (bb_Entity_* dst : bb_coll_by_type_[rule.dstType]) {
        if (src == dst) continue;
        float tf[16];
        memcpy(tf, dst->prev, sizeof(tf));
        if (y_scale != 1) {
          // y_tform * dst_tform: die Y-Zeile der Zielmatrix stauchen
          for (int c = 0; c < 4; ++c) tf[c*4+1] *= y_scale;
        }
        if (bb_hit_test_(coll_line, radius, dst, tf, rule.method, coll)) {
          coll_obj  = dst;
          coll_info = &rule;
        }
      }
    }
    if (!coll_obj) break;

    if (++hits == MAX_HITS) break;

    bb_coll_register_(src, coll_obj, coll_line, coll, inv_y_scale);

    bb_Plane_ coll_plane = bb_plane_pt_(bb_line_at_(coll_line, coll.time), coll.normal);
    coll_plane.d -= BB_COLL_EPSILON_;
    coll.time = bb_plane_t_(coll_plane, coll_line);

    if (coll.time > 0) {
      sv = bb_line_at_(coll_line, coll.time);
      td *= 1 - coll.time;
      td_xz *= 1 - coll.time;
    }

    if (coll_info->response == 1) { dv = sv; break; }        // STOP

    const bb_V3_ nv = bb_plane_nearest_(coll_plane, dv);

    if (n_hit == 0) {
      dv = nv;
    } else if (n_hit == 1) {
      if (bb_plane_dist_(planes[0], nv) >= 0) {
        dv = nv; n_hit = 0;
      } else if (fabsf(bb_dot_(planes[0].n, coll_plane.n)) < 1 - BB_GEOM_EPSILON_) {
        dv = bb_line_nearest_(bb_plane_cross_(coll_plane, planes[0]), dv);
      } else {
        hits = MAX_HITS; break;                              // eingeklemmt
      }
    } else if (bb_plane_dist_(planes[0], nv) >= 0 && bb_plane_dist_(planes[1], nv) >= 0) {
      dv = nv; n_hit = 0;
    } else {
      dv = sv; break;
    }

    bb_V3_ dd = dv - sv;
    if (bb_dot_(dd, dir) <= 0) { dv = sv; break; }

    if (coll_info->response == 2) {                          // SLIDE
      const float d = bb_len_(dd);
      if (d <= BB_GEOM_EPSILON_) { dv = sv; break; }
      if (d > td) dd = dd * (td / d);
    } else if (coll_info->response == 3) {                   // SLIDEXZ
      const float d = bb_len_({ dd.x, 0, dd.z });
      if (d <= BB_GEOM_EPSILON_) { dv = sv; break; }
      if (d > td_xz) dd = dd * (td_xz / d);
    }

    coll_line.o = sv;
    coll_line.d = dd;
    dv = sv + dd;
    planes[n_hit++] = coll_plane;
  }

  if (!hits) return;
  if (hits < MAX_HITS) {
    dv.y *= inv_y_scale;
    bb_PositionEntity(src->handle, dv.x, dv.y, dv.z, 1);
  } else {
    bb_PositionEntity(src->handle, panic.x, panic.y, panic.z, 1);
  }
  // Die eigene Weltmatrix und die der Kinder nachziehen.
  bb_Entity_* p = src->parent ? bb_entity_get_(src->parent) : nullptr;
  bb_update_entity_world_(src, p ? bb_entity_world_(p) : nullptr);
}

// Ein UpdateWorld-Durchlauf (World::update): erst alle Entities mit Typ
// einsortieren, dann **je Entity nacheinander** beginUpdate (Trefferliste
// leeren), collide und endUpdate (vorige Lage = aktuelle).
//
// Diese Reihenfolge ist beobachtbar: das Original traegt jeden Treffer bei
// beiden Beteiligten ein, aber wer spaeter an der Reihe ist, leert seine
// Liste dabei wieder. Am Original gemessen (2026-09-17): das zuerst
// erzeugte Entity bewegt sich und meldet den Treffer, das spaeter erzeugte
// Ziel meldet 0.
static inline void bb_world_update_collisions_() {
  std::vector<bb_Entity_*> all;
  all.reserve(bb_entities_.size());
  for (auto& [h, e] : bb_entities_) all.push_back(e.get());
  // Die Reihenfolge des Originals ist die des Szenenbaums (enumEnabled).
  std::sort(all.begin(), all.end(), [](const bb_Entity_* a, const bb_Entity_* b) {
    return a->seq < b->seq;
  });

  for (bb_Entity_* e : all)
    if (e->collType) bb_coll_by_type_[e->collType].push_back(e);

  for (bb_Entity_* e : all) {
    e->colls.clear();
    if (e->collType) bb_world_collide_(e);
    memcpy(e->prev, bb_entity_world_(e), sizeof(e->prev));
  }

  for (int k = 0; k < 1000; ++k) bb_coll_by_type_[k].clear();
}

// ============================================================
// Picking (3D-17)
// ============================================================

inline bb_ObjColl_ bb_picked_;

static inline int bb_trace_ray_(const bb_Line_& line, float radius) {
  bb_Coll_ coll;
  coll.time = 1;
  bb_Entity_* hit = nullptr;

  std::vector<bb_Entity_*> all;
  all.reserve(bb_entities_.size());
  for (auto& [h, e] : bb_entities_) all.push_back(e.get());
  std::sort(all.begin(), all.end(), [](const bb_Entity_* a, const bb_Entity_* b) {
    return a->seq < b->seq;
  });

  for (bb_Entity_* e : all) {
    if (!e->pickMode) continue;
    if (bb_hit_test_(line, radius, e, bb_entity_world_(e), e->pickMode, coll)) hit = e;
  }

  bb_picked_ = bb_ObjColl_{};
  bb_picked_.index     = 65535;
  bb_picked_.time      = coll.time;
  bb_picked_.normal[0] = coll.normal.x;
  bb_picked_.normal[1] = coll.normal.y;
  bb_picked_.normal[2] = coll.normal.z;
  bb_picked_.surface   = coll.surface;
  bb_picked_.index     = coll.index;
  if (!hit) return 0;
  bb_picked_.with = hit->handle;
  const bb_V3_ p = bb_line_at_(line, coll.time) - coll.normal * radius;
  bb_picked_.coords[0] = p.x;
  bb_picked_.coords[1] = p.y;
  bb_picked_.coords[2] = p.z;
  return hit->handle;
}

inline int bb_LinePick(float x, float y, float z, float dx, float dy, float dz,
                       float radius = 0) {
  return bb_trace_ray_({ { x, y, z }, { dx, dy, dz } }, radius);
}

inline int bb_EntityPick(int entity, float range) {
  bb_Entity_* e = bb_entity_get_(entity);
  if (!e) return 0;
  const float* w = bb_entity_world_(e);
  const bb_Line_ l{ { w[12], w[13], w[14] },
                    { w[8] * range, w[9] * range, w[10] * range } };
  return bb_trace_ray_(l, 0);
}

inline float bb_PickedX()  { return bb_picked_.coords[0]; }
inline float bb_PickedY()  { return bb_picked_.coords[1]; }
inline float bb_PickedZ()  { return bb_picked_.coords[2]; }
inline float bb_PickedNX() { return bb_picked_.normal[0]; }
inline float bb_PickedNY() { return bb_picked_.normal[1]; }
inline float bb_PickedNZ() { return bb_picked_.normal[2]; }
inline float bb_PickedTime()   { return bb_picked_.time; }
inline int   bb_PickedEntity() { return bb_picked_.with; }
inline int   bb_PickedTriangle() { return bb_picked_.index; }
inline int   bb_PickedSurface() { return bb_picked_.surface; }

// EntityVisible: freie Sichtlinie zwischen zwei Entities (World::checkLOS).
// Nur Entities mit Pickmodus **und** Obscurer-Flag stehen im Weg.
inline int bb_EntityVisible(int src, int dest) {
  bb_Entity_* a = bb_entity_get_(src);
  bb_Entity_* b = bb_entity_get_(dest);
  if (!a || !b) return 0;
  const float* wa = bb_entity_world_(a);
  const float* wb = bb_entity_world_(b);
  const bb_Line_ line{ { wa[12], wa[13], wa[14] },
                       { wb[12] - wa[12], wb[13] - wa[13], wb[14] - wa[14] } };
  bb_Coll_ coll;
  for (auto& [h, e] : bb_entities_) {
    bb_Entity_* o = e.get();
    if (o == a || o == b || !o->pickMode || !o->obscurer) continue;
    if (bb_hit_test_(line, 0, o, bb_entity_world_(o), o->pickMode, coll)) return 0;
  }
  return 1;
}

#endif // BB_COLLISION_H
