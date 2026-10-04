#ifndef BB_TERRAIN_H
#define BB_TERRAIN_H

// Terrain: CreateTerrain, LoadTerrain, ModifyTerrain, TerrainHeight,
// TerrainSize, TerrainDetail, TerrainShading, TerrainX/Y/Z (BUG-195).
//
// Uebersetzt aus blitz3d/terrain.cpp, terrainrep.cpp und den Terrain-Befehlen
// in bbruntime/bbblitz3d.cpp. Ein Terrain ist ein Raster aus size x size
// Hoehen (size eine Zweierpotenz), je Zelle ein Byte 0..255 fuer 0..1. Im
// eigenen Raum liegt es in x und z von 0 bis size, in y von 0 bis 1;
// ScaleEntity macht daraus eine Landschaft. Die Hoehe wiederholt sich am
// Rand: Spalte size ist Spalte 0 (x & mask), wie im Original.
//
// Gezeichnet wird es wie im Original je Kamera neu, als ROAM-artiger
// Dreiecksbaum: zwei Dreiecke ueber das ganze Raster, jedes wird entlang
// seiner langen Kante geteilt, solange sein vorab berechneter Fehler (die
// Abweichung der Mitte von der Kante, als Byte) geteilt durch den Abstand
// zum Auge am groessten ist und weniger als `detail` Dreiecke da sind
// (TerrainDetail, Vorgabe 2000). Mit morph werden die zuletzt geteilten
// Punkte zwischen alter und neuer Hoehe gemischt. Nachbarn werden mitgeteilt,
// damit keine Risse entstehen (split). Dreiecke ausserhalb des Sichtkegels
// fallen weg; der Fehlerbaum haelt dafuer auch die hoechste Hoehe darunter.
//
// Kollision und Picking (Methode 2) laufen ueber denselben Baum, nicht ueber
// die gezeichneten Dreiecke: es wird bis zu Dreiecken ohne Fehler geteilt
// (bb_collision.h).
//
// Ein Model (EntityTexture, EntityColor, ...), aber kein Mesh. CopyEntity
// liefert wie im Original nur eine leere Entity: Terrain ueberschreibt
// clone() nicht, kopiert wird das Object darunter.

#include "bb_entity_core.h"
#include "bb_mesh_core.h"
#include <cmath>
#include <cstring>
#include <memory>
#include <queue>
#include <vector>

struct bb_TerrainRep_;

struct bb_TerrainEntity_ : bb_Entity_ {
  std::shared_ptr<bb_TerrainRep_> rep;
  bb_MeshData_ mesh;             // je Zeichnen neu gefuellt, im Raum des Terrains

  bb_EntityKind_ kind() const override { return bb_EntityKind_::Terrain; }

  // clone(): die Vorgabe von bb_Entity_ (ein Pivot), wie im Original.

  ~bb_TerrainEntity_() override { bb_mesh_free_gpu_(&mesh); }
};

// geom.h
inline constexpr float BB_TERRAIN_EPS = .000001f;

// float -> unsigned char wie MSVC: ueber int, dann das untere Byte.
static inline unsigned char bb_terrain_byte_(float f) {
  return static_cast<unsigned char>(static_cast<int>(f));
}

struct bb_TerrainRep_ {
  struct Err { unsigned char error = 0, bound = 0; };

  int cell_shift, cell_size, cell_mask;
  int end_tri_id;
  int detail = 0;
  bool morph = true, shading = false;
  mutable bool errs_valid = true;
  std::vector<unsigned char> cells;
  mutable std::vector<Err> errors;

  explicit bb_TerrainRep_(int n)
    : cell_shift(n), cell_size(1 << n), cell_mask((1 << n) - 1),
      end_tri_id((1 << n) * (1 << n) * 2),
      cells(static_cast<size_t>(1 << n) * (1 << n), 0),
      errors(static_cast<size_t>(end_tri_id)) {
    setDetail(2000, false);
  }

  void setDetail(int n, bool m) { morph = m; detail = n; }

  float getHeight(int x, int z) const {
    return cells[static_cast<size_t>(((z & cell_mask) << cell_shift) | (x & cell_mask))] / 255.0f;
  }

  struct Vert {
    short x = 0, z = 0;
    float vx = 0, vy = 0, vz = 0;   // v
    float src_y = 0;
  };

  Vert mkVert(int x, int z) const {
    Vert v;
    v.x = static_cast<short>(x); v.z = static_cast<short>(z);
    v.vx = static_cast<float>(x); v.vz = static_cast<float>(z);
    v.vy = getHeight(x, z);
    v.src_y = v.vy;
    return v;
  }

  void setHeight(int x, int z, float h, bool realtime) {
    cells[static_cast<size_t>(((z & cell_mask) << cell_shift) | (x & cell_mask))] =
        bb_terrain_byte_(h * 255.0f);
    if (!errs_valid) return;
    if (realtime) {
      const Vert v0 = mkVert(0, 0), v1 = mkVert(cell_size, 0),
                 v2 = mkVert(cell_size, cell_size), v3 = mkVert(0, cell_size);
      calcErrAt(2, x, z, v1, v2, v0);
      calcErrAt(3, x, z, v3, v0, v2);
      return;
    }
    errs_valid = false;
  }

  static unsigned char boundOf(float y) {
    return y >= 1 ? 255 : static_cast<unsigned char>(std::ceil(y * 255.0f));
  }
  static unsigned char errOf(float e) {
    return e >= 1 ? 255 : static_cast<unsigned char>(std::ceil((e - BB_TERRAIN_EPS) * 255.0f));
  }

  Err calcErr(int id, const Vert& v0, const Vert& v1, const Vert& v2) const {
    Err et;
    float y = v0.vy;
    if (v1.vy > y) y = v1.vy;
    if (v2.vy > y) y = v2.vy;
    et.error = 0;
    et.bound = boundOf(y);
    if (id >= end_tri_id) return et;

    const Vert tv = mkVert((v1.x + v2.x) / 2, (v1.z + v2.z) / 2);
    const float e = std::fabs(tv.vy - (v1.vy + v2.vy) / 2);
    et.error = errOf(e);

    const Err el = calcErr(id * 2, tv, v2, v0);
    const Err er = calcErr(id * 2 + 1, tv, v0, v1);
    if (el.error > et.error) et.error = el.error;
    if (er.error > et.error) et.error = er.error;
    if (el.bound > et.bound) et.bound = el.bound;
    if (er.bound > et.bound) et.bound = er.bound;
    return errors[static_cast<size_t>(id)] = et;
  }

  Err calcErrAt(int id, int x, int z, const Vert& v0, const Vert& v1, const Vert& v2) const {
    Err et;
    float y = v0.vy;
    if (v1.vy > y) y = v1.vy;
    if (v2.vy > y) y = v2.vy;
    et.error = 0;
    et.bound = boundOf(y);
    if (id >= end_tri_id) return et;

    // liegt x/z in diesem Dreieck?
    int dx, dz;
    dx = -(v1.z - v0.z); dz = (v1.x - v0.x);
    if ((x - v0.x) * dx + (z - v0.z) * dz < 0) return errors[static_cast<size_t>(id)];
    dx = -(v2.z - v1.z); dz = (v2.x - v1.x);
    if ((x - v1.x) * dx + (z - v1.z) * dz < 0) return errors[static_cast<size_t>(id)];
    dx = -(v0.z - v2.z); dz = (v0.x - v2.x);
    if ((x - v2.x) * dx + (z - v2.z) * dz < 0) return errors[static_cast<size_t>(id)];

    const Vert tv = mkVert((v1.x + v2.x) / 2, (v1.z + v2.z) / 2);
    const float e = std::fabs(tv.vy - (v1.vy + v2.vy) / 2);
    et.error = errOf(e);

    const Err el = calcErrAt(id * 2, x, z, tv, v2, v0);
    const Err er = calcErrAt(id * 2 + 1, x, z, tv, v0, v1);
    if (el.error > et.error) et.error = el.error;
    if (er.error > et.error) et.error = er.error;
    if (el.bound > et.bound) et.bound = el.bound;
    if (er.bound > et.bound) et.bound = er.bound;
    return errors[static_cast<size_t>(id)] = et;
  }

  void validateErrs() const {
    if (errs_valid) return;
    const Vert v0 = mkVert(0, 0), v1 = mkVert(cell_size, 0),
               v2 = mkVert(cell_size, cell_size), v3 = mkVert(0, cell_size);
    calcErr(2, v1, v2, v0);
    calcErr(3, v3, v0, v2);
    errs_valid = true;
  }

  // getNormal: Mittel der vier Flaechennormalen um den Punkt.
  void getNormal(int x, int z, float out[3]) const {
    const float vt[3] = { (float)x, getHeight(x, z), (float)z };
    const float p[4][3] = {
      { (float)x,       getHeight(x, z - 1), (float)(z - 1) },
      { (float)(x + 1), getHeight(x + 1, z), (float)z },
      { (float)x,       getHeight(x, z + 1), (float)(z + 1) },
      { (float)(x - 1), getHeight(x - 1, z), (float)z } };
    // Plane(a,b,c).n = normalized((b-a) x (c-a))
    auto pn = [](const float* a, const float* b, const float* c, float* n) {
      const float u[3] = { b[0] - a[0], b[1] - a[1], b[2] - a[2] };
      const float v[3] = { c[0] - a[0], c[1] - a[1], c[2] - a[2] };
      n[0] = u[1] * v[2] - u[2] * v[1];
      n[1] = u[2] * v[0] - u[0] * v[2];
      n[2] = u[0] * v[1] - u[1] * v[0];
      const float l = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
      if (l > 0) { n[0] /= l; n[1] /= l; n[2] /= l; }
    };
    float s[3] = { 0, 0, 0 }, n[3];
    pn(vt, p[1], p[0], n); s[0] += n[0]; s[1] += n[1]; s[2] += n[2];
    pn(vt, p[2], p[1], n); s[0] += n[0]; s[1] += n[1]; s[2] += n[2];
    pn(vt, p[3], p[2], n); s[0] += n[0]; s[1] += n[1]; s[2] += n[2];
    pn(vt, p[0], p[3], n); s[0] += n[0]; s[1] += n[1]; s[2] += n[2];
    const float l = std::sqrt(s[0] * s[0] + s[1] * s[1] + s[2] * s[2]);
    out[0] = l > 0 ? s[0] / l : 0; out[1] = l > 0 ? s[1] / l : 1; out[2] = l > 0 ? s[2] / l : 0;
  }
};

// Terrain::getHeight: ausserhalb 0..size (einschliesslich) 0, sonst mit Maske.
static inline float bb_terrain_height_(const bb_TerrainRep_& r, int x, int z) {
  return (x >= 0 && z >= 0 && x <= r.cell_size && z <= r.cell_size) ? r.getHeight(x, z) : 0.0f;
}

// ---- ROAM-Zeichnen (TerrainRep::render) ----
//
// Zustand wie die statischen Variablen in terrainrep.cpp.
namespace bb_terrain_r_ {

struct Tri {
  int id = 0;
  short clip = 0, v0 = 0, v1 = 0, v2 = 0;
  Tri *e0 = nullptr, *e1 = nullptr, *e2 = nullptr;
  float proj_err = 0;

  // Eigener Vorrat wie Tri::operator new des Originals.
  static Tri*& pool() { static Tri* p = nullptr; return p; }
  static Tri* make(int id, int clip, int v0, int v1, int v2,
                   Tri* e0 = nullptr, Tri* e1 = nullptr, Tri* e2 = nullptr) {
    if (!pool()) {
      const int GROW = 64;
      Tri* blk = new Tri[GROW];
      for (int k = 0; k < GROW - 1; ++k) blk[k].e0 = &blk[k + 1];
      blk[GROW - 1].e0 = nullptr;
      pool() = blk;
    }
    Tri* t = pool();
    pool() = t->e0;
    t->id = id; t->clip = static_cast<short>(clip);
    t->v0 = static_cast<short>(v0); t->v1 = static_cast<short>(v1); t->v2 = static_cast<short>(v2);
    t->e0 = e0; t->e1 = e1; t->e2 = e2;
    t->proj_err = 0;
    return t;
  }
  static void release(Tri* t) { t->e0 = pool(); pool() = t; }

  void unlink() {
    if (e0) { if (e0->e0 == this) e0->e0 = nullptr; else if (e0->e1 == this) e0->e1 = nullptr; else e0->e2 = nullptr; }
    if (e1) { if (e1->e0 == this) e1->e0 = nullptr; else if (e1->e1 == this) e1->e1 = nullptr; else e1->e2 = nullptr; }
    if (e2) { if (e2->e0 == this) e2->e0 = nullptr; else if (e2->e1 == this) e2->e1 = nullptr; else e2->e2 = nullptr; }
  }
};

struct TriComp {
  bool operator()(const Tri* a, const Tri* b) const { return a->proj_err < b->proj_err; }
};
struct TriQue : std::priority_queue<Tri*, std::vector<Tri*>, TriComp> {
  std::vector<Tri*>& getVector() { return c; }
};

// Ebene im Raum des Terrains: innen, wenn a*x + b*y + c*z + d >= 0.
struct Pl { float a, b, c, d; float dist(float x, float y, float z) const { return a * x + b * y + c * z + d; } };

inline const bb_TerrainRep_* curr = nullptr;
inline Pl planes[6];
inline float eye[3];
inline TriQue tri_que;
inline std::vector<Tri*> tris;
inline std::vector<bb_TerrainRep_::Vert> verts;
inline int vert_cnt = 0;
inline int out_cnt = 0;
inline int stat_mvc = 0, stat_mtc = 0;   // static int mvc,mtc in render

inline void insert(Tri* t) {
  const bb_TerrainRep_& r = *curr;
  // schnellere Pruefung fuer "duenne" Dreiecke
  if (t->id >= r.end_tri_id || !r.errors[static_cast<size_t>(t->id)].error) {
    if (t->clip & 63) {
      const auto &e0 = verts[t->v0], &e1 = verts[t->v1], &e2 = verts[t->v2];
      for (int n = 0; n < 6; ++n) {
        if (!(t->clip & (1 << n))) continue;
        const Pl& p = planes[n];
        if (p.dist(e0.vx, e0.vy, e0.vz) < 0 && p.dist(e1.vx, e1.vy, e1.vz) < 0 &&
            p.dist(e2.vx, e2.vy, e2.vz) < 0) {
          t->unlink();
          Tri::release(t);
          ++out_cnt;
          return;
        }
      }
    }
    t->clip |= 128;
    tris.push_back(t);
    ++out_cnt;
    return;
  }

  // abschneiden?
  if (t->id < r.end_tri_id / 2 && (t->clip & 63)) {
    const auto &a = verts[t->v0], &b = verts[t->v1], &c = verts[t->v2];
    const float top = r.errors[static_cast<size_t>(t->id)].bound / 255.0f;
    const float ex[6] = { a.vx, b.vx, c.vx, a.vx, b.vx, c.vx };
    const float ey[6] = { 0, 0, 0, top, top, top };
    const float ez[6] = { a.vz, b.vz, c.vz, a.vz, b.vz, c.vz };
    for (int n = 0; n < 6; ++n) {
      const int mask = 1 << n;
      if (!(t->clip & mask)) continue;
      const Pl& p = planes[n];
      int q = 0;
      for (int k = 0; k < 6; ++k) q += p.dist(ex[k], ey[k], ez[k]) >= 0;
      if (!q) {
        t->unlink();
        Tri::release(t);
        ++out_cnt;
        return;
      }
      if (q == 6) t->clip = static_cast<short>(t->clip & ~mask);
    }
  }

  if (t->clip & 128) {
    t->clip |= 128;
    tris.push_back(t);
  } else {
    const auto &a = verts[t->v1], &b = verts[t->v2];
    const float mx = (a.vx + b.vx) / 2 - eye[0];
    const float my = (a.vy + b.vy) / 2 - eye[1];
    const float mz = (a.vz + b.vz) / 2 - eye[2];
    float d = std::sqrt(mx * mx + my * my + mz * mz);
    if (d < BB_TERRAIN_EPS) d = BB_TERRAIN_EPS;
    t->proj_err = r.errors[static_cast<size_t>(t->id)].error / d;
    if (t->proj_err > BB_TERRAIN_EPS) {
      tri_que.push(t);
    } else {
      t->clip |= 128;
      tris.push_back(t);
    }
  }
  ++out_cnt;
}

inline void split(Tri* t) {
  if (t->e2 && t->e2->e2 != t) split(t->e2);

  const int tv = vert_cnt++;
  if (static_cast<size_t>(tv) >= verts.size()) verts.resize(verts.size() + verts.size() / 2 + 32);
  bb_TerrainRep_::Vert& vert = verts[static_cast<size_t>(tv)];
  vert.x = static_cast<short>((verts[t->v1].x + verts[t->v2].x) / 2);
  vert.z = static_cast<short>((verts[t->v1].z + verts[t->v2].z) / 2);
  vert.vx = vert.x;
  vert.vz = vert.z;
  vert.src_y = (verts[t->v1].vy + verts[t->v2].vy) / 2;
  vert.vy = curr->getHeight(vert.x, vert.z);

  Tri* tl = Tri::make(t->id * 2, t->clip, tv, t->v2, t->v0, nullptr, nullptr, t->e0);
  if (Tri* p = tl->e2) {
    if (p->e0 == t) p->e0 = tl; else if (p->e1 == t) p->e1 = tl; else p->e2 = tl;
  }
  Tri* tr = Tri::make(t->id * 2 + 1, t->clip, tv, t->v0, t->v1, nullptr, tl, t->e1);
  tl->e0 = tr;
  if (Tri* p = tr->e2) {
    if (p->e0 == t) p->e0 = tr; else if (p->e1 == t) p->e1 = tr; else p->e2 = tr;
  }

  if (Tri* b = t->e2) {
    Tri* br = Tri::make(b->id * 2, b->clip, tv, b->v2, b->v0, nullptr, tr, b->e0);
    tr->e0 = br;
    if (Tri* p = br->e2) {
      if (p->e0 == b) p->e0 = br; else if (p->e1 == b) p->e1 = br; else p->e2 = br;
    }
    Tri* bl = Tri::make(b->id * 2 + 1, b->clip, tv, b->v0, b->v1, tl, br, b->e1);
    tl->e1 = br->e0 = bl;
    if (Tri* p = bl->e2) {
      if (p->e0 == b) p->e0 = bl; else if (p->e1 == b) p->e1 = bl; else p->e2 = bl;
    }
    b->id = 0;
    --out_cnt;
    insert(br);
    insert(bl);
  }
  t->id = 0;
  --out_cnt;
  insert(tl);
  insert(tr);
}

} // namespace bb_terrain_r_

// TerrainRep::render. `view` ist die GL-Sichtmatrix (Kamera schaut nach -z),
// `f` der Kegel wie fuer das Verwerfen. Fuellt tr->mesh im Raum des Terrains;
// false, wenn nichts zu zeichnen ist.
static inline bool bb_terrain_build_(bb_TerrainEntity_* te, const float* view,
                                     const bb_CullFrustum_& f) {
  using namespace bb_terrain_r_;
  bb_TerrainRep_& r = *te->rep;
  curr = &r;
  r.validateErrs();

  // Frustum( rc.getWorldFrustum(), -model->getRenderTform() ): die sechs
  // Ebenen der Kamera in den Raum des Terrains. Mit M = view * world ist ein
  // Punkt p des Terrains in Kamerakoordinaten M*p; eine Kameraebene
  // (n, d) wird damit zu (n, d) * M.
  float M[16];
  mat4_mul_(M, view, te->world);
  const float cam[6][4] = {
    {  0, -1, -f.sy, 0 },     // oben
    {  1,  0, -f.sx, 0 },     // links
    {  0,  1, -f.sy, 0 },     // unten
    { -1,  0, -f.sx, 0 },     // rechts
    {  0,  0, -1, -f.nr },    // nah
    {  0,  0,  1,  f.fr } };  // fern
  for (int k = 0; k < 6; ++k) {
    const float* c = cam[k];
    planes[k].a = c[0] * M[0]  + c[1] * M[1]  + c[2] * M[2]  + c[3] * M[3];
    planes[k].b = c[0] * M[4]  + c[1] * M[5]  + c[2] * M[6]  + c[3] * M[7];
    planes[k].c = c[0] * M[8]  + c[1] * M[9]  + c[2] * M[10] + c[3] * M[11];
    planes[k].d = c[0] * M[12] + c[1] * M[13] + c[2] * M[14] + c[3] * M[15];
  }
  {
    float inv[16];
    if (!mat4_inverse_(inv, M)) return false;
    eye[0] = inv[12]; eye[1] = inv[13]; eye[2] = inv[14];
  }

  const int need = r.detail + 32;
  if (static_cast<int>(verts.size()) < need) verts.resize(static_cast<size_t>(need));
  vert_cnt = 4;
  out_cnt = 0;
  tri_que.getVector().clear();
  tris.clear();

  verts[0] = r.mkVert(0, 0);
  verts[1] = r.mkVert(r.cell_size, 0);
  verts[2] = r.mkVert(r.cell_size, r.cell_size);
  verts[3] = r.mkVert(0, r.cell_size);

  Tri* t0 = Tri::make(2, 0x3f, 1, 2, 0);
  Tri* t1 = Tri::make(3, 0x3f, 3, 0, 2);
  t0->e2 = t1; t1->e2 = t0;

  insert(t0);
  insert(t1);

  while (!tri_que.empty() && out_cnt < r.detail) {
    Tri* t = tri_que.top();
    tri_que.pop();
    if (t->id) split(t);
    Tri::release(t);
  }

  std::vector<Tri*>& q_tris = tri_que.getVector();
  bb_MeshData_& m = te->mesh;
  m.vertices.clear();
  m.indices.clear();
  m.dirty = true;

  if (!out_cnt) {
    for (Tri* t : tris) Tri::release(t);
    for (Tri* t : q_tris) Tri::release(t);
    q_tris.clear();
    tris.clear();
    return false;
  }

  int err_cnt = 0;
  for (Tri* t : q_tris) {
    if (t->id) { tris.push_back(t); ++err_cnt; }
    else Tri::release(t);
  }
  q_tris.clear();

  if (r.morph) {
    if (int morph_cnt = err_cnt / 4) {
      if (morph_cnt > vert_cnt) morph_cnt = vert_cnt;
      float t = 0;
      const float morph_step = 1.0f / morph_cnt;
      for (int vn = vert_cnt - morph_cnt; vn < vert_cnt; ++vn) {
        auto& v = verts[static_cast<size_t>(vn)];
        v.vy += (v.src_y - v.vy) * t;
        t += morph_step;
      }
    }
  }

  m.vertices.reserve(static_cast<size_t>(vert_cnt) * BB_VF);
  const float cs = static_cast<float>(r.cell_size);
  for (int k = 0; k < vert_cnt; ++k) {
    const auto& v = verts[static_cast<size_t>(k)];
    float n[3] = { 0, 1, 0 };
    if (r.shading) r.getNormal(static_cast<int>(v.vx), static_cast<int>(v.vz), n);
    bb_vert_push_(m, v.vx, v.vy, v.vz, n[0], n[1], n[2], v.vx, cs - v.vz);
  }
  int tc = 0;
  m.indices.reserve(tris.size() * 3);
  for (Tri* t : tris) {
    if (t->id) {
      m.indices.push_back(static_cast<unsigned>(t->v0));
      m.indices.push_back(static_cast<unsigned>(t->v2));
      m.indices.push_back(static_cast<unsigned>(t->v1));
      ++tc;
    }
    Tri::release(t);
  }
  tris.clear();

  // static int mvc,mtc: die groessten Zahlen bisher, ueber alle Terrains
  if (vert_cnt > stat_mvc) stat_mvc = vert_cnt;
  if (tc > stat_mtc) stat_mtc = tc;
  bb_stats3d_[1] = static_cast<float>(stat_mvc);
  bb_stats3d_[2] = static_cast<float>(stat_mtc);
  return tc > 0;
}

// ---- Befehle ----

static inline bb_TerrainEntity_* bb_terrain_chk_(int h) {
  bb_Entity_* e = bb_ent_chk_(h);
  if (e->kind() != bb_EntityKind_::Terrain) bb_RuntimeError("Entity is not a terrain");
  return static_cast<bb_TerrainEntity_*>(e);
}

static inline int bb_terrain_make_(int shift, int parent) {
  auto ent = std::make_unique<bb_TerrainEntity_>();
  ent->rep = std::make_shared<bb_TerrainRep_>(shift);
  return bb_entity_register_(std::move(ent), parent);
}

inline int bb_CreateTerrain(int grid_size, int parent = 0) {
  const int n = grid_size;
  bb_parent_chk_(parent);
  int shift = 0;
  while ((1 << shift) < n) ++shift;
  if ((1 << shift) != n) bb_RuntimeError("Illegal terrain size");
  return bb_terrain_make_(shift, parent);
}

// LoadTerrain liest die Hoehenkarte als Canvas: die hellste der drei Farben
// je Pixel ist die Hoehe, die oberste Bildzeile das hintere Ende (z = size-1).
inline int bb_LoadTerrain(const bbString& heightmap_file, int parent = 0) {
  const bbString& file = heightmap_file;
  bb_parent_chk_(parent);
  int w = 0, h = 0, ch = 0;
  unsigned char* data = bb_load_rgba_(file.c_str(), &w, &h, &ch);
  if (!data) bb_RuntimeError("Unable to load heightmap image");
  if (w != h) { stbi_image_free(data); bb_RuntimeError("Terrain must be square"); }
  int shift = 0;
  while ((1 << shift) < w) ++shift;
  if ((1 << shift) != w) { stbi_image_free(data); bb_RuntimeError("Illegal terrain size"); }
  const int handle = bb_terrain_make_(shift, parent);
  bb_TerrainRep_& r = *static_cast<bb_TerrainEntity_*>(bb_entity_get_(handle))->rep;
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const unsigned char* p = data + (static_cast<size_t>(y) * w + x) * 4;
      const int R = p[0], G = p[1], B = p[2];
      const float v = (R > G ? (R > B ? R : B) : (G > B ? G : B)) / 255.0f;
      if (x >= 0 && x <= r.cell_size && h - 1 - y >= 0 && h - 1 - y <= r.cell_size)
        r.setHeight(x, h - 1 - y, v, false);
    }
  }
  stbi_image_free(data);
  return handle;
}

inline void bb_TerrainDetail(int terrain, int detail_level, int morph = 0) {
  bb_terrain_chk_(terrain)->rep->setDetail(detail_level, morph != 0);
}

inline void bb_TerrainShading(int terrain, int enable) {
  bb_terrain_chk_(terrain)->rep->shading = (enable != 0);
}

inline int bb_TerrainSize(int terrain) {
  return bb_terrain_chk_(terrain)->rep->cell_size;
}

inline float bb_TerrainHeight(int terrain, int terrain_x, int terrain_z) {
  return bb_terrain_height_(*bb_terrain_chk_(terrain)->rep, terrain_x, terrain_z);
}

inline void bb_ModifyTerrain(int terrain, int terrain_x, int terrain_z, float height,
                             int realtime = 0) {
  const int x = terrain_x, z = terrain_z;
  const float h = height;
  bb_TerrainRep_& r = *bb_terrain_chk_(terrain)->rep;
  if (x >= 0 && z >= 0 && x <= r.cell_size && z <= r.cell_size) r.setHeight(x, z, h, realtime != 0);
}

// terrainHeight / terrainVector aus bbblitz3d.cpp: der Punkt in den Raum des
// Terrains (volle Umkehrung samt Skalierung), dort bilinear die Hoehe, und
// zurueck in die Welt.
static inline void bb_terrain_vector_(int t, float x, float y, float z, float out[3]) {
  bb_TerrainEntity_* te = bb_terrain_chk_(t);
  const float* w = bb_entity_world_(te);
  float inv[16];
  if (!mat4_inverse_(inv, w)) { out[0] = x; out[1] = y; out[2] = z; return; }
  float v[3];
  mat4_xform_pt_(v, inv, x, y, z);
  const bb_TerrainRep_& r = *te->rep;
  const int ix = static_cast<int>(std::floor(v[0]));
  const int iz = static_cast<int>(std::floor(v[2]));
  const float tx = v[0] - ix, tz = v[2] - iz;
  const float h0 = bb_terrain_height_(r, ix, iz);
  const float h1 = bb_terrain_height_(r, ix + 1, iz);
  const float h2 = bb_terrain_height_(r, ix, iz + 1);
  const float h3 = bb_terrain_height_(r, ix + 1, iz + 1);
  const float ha = (h1 - h0) * tx + h0, hb = (h3 - h2) * tx + h2;
  const float hh = (hb - ha) * tz + ha;
  mat4_xform_pt_(out, w, v[0], hh, v[2]);
}

inline float bb_TerrainX(int terrain, float world_x, float world_y, float world_z) {
  float o[3]; bb_terrain_vector_(terrain, world_x, world_y, world_z, o); return o[0];
}
inline float bb_TerrainY(int terrain, float world_x, float world_y, float world_z) {
  float o[3]; bb_terrain_vector_(terrain, world_x, world_y, world_z, o); return o[1];
}
inline float bb_TerrainZ(int terrain, float world_x, float world_y, float world_z) {
  float o[3]; bb_terrain_vector_(terrain, world_x, world_y, world_z, o); return o[2];
}

#endif // BB_TERRAIN_H
