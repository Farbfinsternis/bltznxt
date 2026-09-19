#ifndef BB_MESH_H
#define BB_MESH_H

// Primitive Mesh Entities (3D-09).
//
// Provides bb_MeshEntity_ (entity with one or more bb_MeshData_ surfaces),
// four generator functions (cube, sphere, cylinder, cone), and the matching
// Blitz3D commands: CreateCube, CreateSphere, CreateCylinder, CreateCone,
// MeshWidth/Height/Depth.
//
// RenderWorld integration: bb_render_meshes_() is called from bb_graphics3d.h
// inside the per-camera loop after view/proj matrices are built.

#include "bb_entity_core.h"
#include "bb_mesh_core.h"
#include "bb_texture.h"
#include "bb_sprite.h"
#include "bb_md2.h"
#include <algorithm>
#include <cmath>
#include <cfloat>

// ============================================================
// bb_MeshEntity_ — entity with surfaces (one MeshData per surface)
// ============================================================

// Die Flaechen liegen hinter einem geteilten Zeiger, weil CopyEntity im
// Original die Geometrie *teilt* statt sie zu vervielfaeltigen: dort haelt
// MeshModel einen refgezaehlten `rep`, und der Kopierkonstruktor uebernimmt
// ihn, statt ihn zu kopieren (`blitz3d/meshmodel.cpp:167`). Am Original
// gemessen (2026-09-11): nach `RotateMesh original` liegt der erste Vertex
// der **Kopie** bei +1 statt -1, und ein `ScaleMesh` auf die Kopie bewegt
// umgekehrt auch das Original. Deshalb trennt CopyMesh (tiefe Kopie) und
// CopyEntity (geteilte Flaechen) sich hier, nicht nur dem Namen nach.
struct bb_MeshRep_ {
  std::vector<bb_MeshData_> surfaces;  // each surface = one draw call

  // Der Dreiecksbaum fuer Kollisionen und Picking (bb_collision.h baut ihn,
  // hier liegt er nur). `stamp` haelt fest, zu welchem Stand der Geometrie
  // er gehoert.
  std::shared_ptr<void>  collider;
  unsigned long long     collider_stamp = 0;

  ~bb_MeshRep_() {
    for (auto& s : surfaces) bb_mesh_free_gpu_(&s);
  }
};

struct bb_MeshEntity_ : bb_Entity_ {
  std::shared_ptr<bb_MeshRep_> rep = std::make_shared<bb_MeshRep_>();

  std::vector<bb_MeshData_>&       surfaces()       { return rep->surfaces; }
  const std::vector<bb_MeshData_>& surfaces() const { return rep->surfaces; }

  bb_EntityKind_ kind() const override { return bb_EntityKind_::Mesh; }

  // Der Klon teilt den `rep`, kopiert aber den Entity-Teil (Lage, Name,
  // Aussehen) fuer sich - genau wie MeshModel::MeshModel(const MeshModel&).
  std::unique_ptr<bb_Entity_> clone() const override {
    auto c = std::make_unique<bb_MeshEntity_>();
    bb_entity_copy_fields_(*c, *this);
    c->rep = rep;
    return c;
  }
};

// Downcast helper
static inline bb_MeshEntity_* bb_mesh_ent_(int h) {
  bb_Entity_* e = bb_entity_get_(h);
  if (!e || e->kind() != bb_EntityKind_::Mesh) return nullptr;
  return static_cast<bb_MeshEntity_*>(e);
}

// ============================================================
// Helper: push a single triangle's worth of vertex data.
// Appends 3 vertices (each BB_VF floats) and 3 indices.
// ============================================================

static inline void bb_mesh_push_tri_(bb_MeshData_& m,
    float x0,float y0,float z0, float nx0,float ny0,float nz0, float u0,float v0,
    float x1,float y1,float z1, float nx1,float ny1,float nz1, float u1,float v1,
    float x2,float y2,float z2, float nx2,float ny2,float nz2, float u2,float v2) {
  unsigned int base = static_cast<unsigned int>(m.vertices.size() / BB_VF);
  bb_vert_push_(m, x0,y0,z0, nx0,ny0,nz0, u0,v0);
  bb_vert_push_(m, x1,y1,z1, nx1,ny1,nz1, u1,v1);
  bb_vert_push_(m, x2,y2,z2, nx2,ny2,nz2, u2,v2);
  m.indices.push_back(base);
  m.indices.push_back(base+1);
  m.indices.push_back(base+2);
}

// Push a quad as two triangles (shared flat normal, simple UV)
static inline void bb_mesh_push_quad_(bb_MeshData_& m,
    float x0,float y0,float z0, float u0,float v0,
    float x1,float y1,float z1, float u1,float v1,
    float x2,float y2,float z2, float u2,float v2,
    float x3,float y3,float z3, float u3,float v3,
    float nx,float ny,float nz) {
  unsigned int base = static_cast<unsigned int>(m.vertices.size() / BB_VF);
  bb_vert_push_(m, x0,y0,z0, nx,ny,nz, u0,v0);
  bb_vert_push_(m, x1,y1,z1, nx,ny,nz, u1,v1);
  bb_vert_push_(m, x2,y2,z2, nx,ny,nz, u2,v2);
  bb_vert_push_(m, x3,y3,z3, nx,ny,nz, u3,v3);
  m.indices.push_back(base);   m.indices.push_back(base+1); m.indices.push_back(base+2);
  m.indices.push_back(base);   m.indices.push_back(base+2); m.indices.push_back(base+3);
}

// ============================================================
// bb_gen_cube_ — unit cube [-1,+1] on each axis, 6 quads
// ============================================================

// Die 24 Vertices stehen hier Zeile fuer Zeile so, wie das Original sie
// meldet - Reihenfolge der Flaechen, Reihenfolge der Ecken, Normalen und
// Texturkoordinaten (BUG-65, gemessen ueber GetSurface und die
// Vertex-Getter, siehe Buglist). Jede Flaeche laeuft von der oberen linken
// Ecke im Uhrzeigersinn: (0,0), (1,0), (1,1), (0,1).
//
// Die Dreiecke laufen wie im Original: TriangleVertex meldet (0,1,2) und
// (0,2,3). Bis 2026-09-17 liefen sie hier andersherum, um eine falsch
// gesetzte Vorderseite im Renderer auszugleichen (BUG-126, siehe
// bb_RenderWorld). Die Vertextabelle selbst stimmt Zahl fuer Zahl.
static inline bb_MeshData_ bb_gen_cube_() {
  bb_MeshData_ m;

  struct Ecke { float x, y, z, u, v; };
  struct Flaeche { float nx, ny, nz; Ecke e[4]; };
  static const Flaeche flaechen[6] = {
    { 0,0,-1, { {-1, 1,-1, 0,0}, { 1, 1,-1, 1,0}, { 1,-1,-1, 1,1}, {-1,-1,-1, 0,1} } },
    { 1,0, 0, { { 1, 1,-1, 0,0}, { 1, 1, 1, 1,0}, { 1,-1, 1, 1,1}, { 1,-1,-1, 0,1} } },
    { 0,0, 1, { { 1, 1, 1, 0,0}, {-1, 1, 1, 1,0}, {-1,-1, 1, 1,1}, { 1,-1, 1, 0,1} } },
    {-1,0, 0, { {-1, 1, 1, 0,0}, {-1, 1,-1, 1,0}, {-1,-1,-1, 1,1}, {-1,-1, 1, 0,1} } },
    { 0,1, 0, { {-1, 1, 1, 0,0}, { 1, 1, 1, 1,0}, { 1, 1,-1, 1,1}, {-1, 1,-1, 0,1} } },
    { 0,-1,0, { {-1,-1,-1, 0,0}, { 1,-1,-1, 1,0}, { 1,-1, 1, 1,1}, {-1,-1, 1, 0,1} } },
  };

  for (const Flaeche& f : flaechen) {
    const unsigned base = static_cast<unsigned>(m.vertices.size() / BB_VF);
    for (const Ecke& e : f.e)
      bb_vert_push_(m, e.x, e.y, e.z, f.nx, f.ny, f.nz, e.u, e.v);
    m.indices.push_back(base);     m.indices.push_back(base + 1);
    m.indices.push_back(base + 2);
    m.indices.push_back(base);     m.indices.push_back(base + 2);
    m.indices.push_back(base + 3);
  }

  m.dirty = true;
  return m;
}

// ============================================================
// Kugel, Zylinder, Kegel - nach MeshUtil::createSphere/createCylinder/
// createCone (blitz3d/meshutil.cpp) und am Original Vertex fuer Vertex
// gemessen (BUG-69, BUG-93, BUG-133, 2026-09-17).
//
// Die Winkel rechnen mit denselben float-Konstanten wie geom.h, und die
// Richtungen entstehen wie dort aus rotationMatrix(pitch,yaw,0).k bzw.
// yawMatrix(yaw).k. Beide zeigen bei yaw=0 nach +z und laufen mit wachsendem
// yaw nach -x.
//
// Die Segmentzahl pruefte das Original nur im Debug-Modus (Kugel 2..100,
// Zylinder und Kegel 3..100, sonst "Illegal number of segments"). Hier wird
// nach unten auf dieselbe Grenze angehoben, damit kein ungueltiger Index
// entsteht; nach oben gibt es keine Grenze.
// ============================================================

static const float bb_geom_pi_     = 3.14159265359f;
static const float bb_geom_twopi_  = bb_geom_pi_ * 2.0f;
static const float bb_geom_halfpi_ = bb_geom_pi_ * .5f;

// rotationMatrix(p,y,0).k = yawMatrix(y) * pitchMatrix(p).k, ausmultipliziert
// in der Reihenfolge von Matrix::operator*(Vector) - damit auch die
// Vorzeichen der Nullen dieselben sind.
static inline void bb_geom_rot_k_(float pitch, float yaw, float* o) {
  const float vx = 0.0f, vy = -sinf(pitch), vz = cosf(pitch);
  const float cy = cosf(yaw), sy = sinf(yaw);
  // yawMatrix: i=(cy,0,sy) j=(0,1,0) k=(-sy,0,cy)
  o[0] = cy * vx + 0.0f * vy + (-sy) * vz;
  o[1] = 0.0f * vx + 1.0f * vy + 0.0f * vz;
  o[2] = sy * vx + 0.0f * vy + cy * vz;
}

static inline void bb_geom_vert_(bb_MeshData_& m, float x, float y, float z,
                                 float nx, float ny, float nz, float u, float v) {
  bb_vert_push_(m, x, y, z, nx, ny, nz, u, v);
}

static inline void bb_geom_tri_(bb_MeshData_& m, int a, int b, int c) {
  m.indices.push_back(static_cast<unsigned>(a));
  m.indices.push_back(static_cast<unsigned>(b));
  m.indices.push_back(static_cast<unsigned>(c));
}

// Eine Flaeche: h_segs Vertices am Nordpol (je einer pro Segment, damit jedes
// Poldreieck sein eigenes u hat), v_segs-1 Ringe mit h_segs+1 Vertices (die
// Naht doppelt), h_segs Vertices am Suedpol. Normale = Position.
static inline bb_MeshData_ bb_gen_sphere_(int segs) {
  if (segs < 2) segs = 2;
  const int h_segs = segs * 2, v_segs = segs;
  bb_MeshData_ m;

  for (int k = 0; k < h_segs; ++k)
    bb_geom_vert_(m, 0, 1, 0, 0, 1, 0, (k + .5f) / h_segs, 0);
  for (int k = 1; k < v_segs; ++k) {
    const float pitch = k * bb_geom_pi_ / v_segs - bb_geom_halfpi_;
    for (int j = 0; j <= h_segs; ++j) {
      const float yaw = (j % h_segs) * bb_geom_twopi_ / h_segs;
      float p[3];
      bb_geom_rot_k_(pitch, yaw, p);
      bb_geom_vert_(m, p[0], p[1], p[2], p[0], p[1], p[2],
                    float(j) / float(h_segs), float(k) / float(v_segs));
    }
  }
  for (int k = 0; k < h_segs; ++k)
    bb_geom_vert_(m, 0, -1, 0, 0, -1, 0, (k + .5f) / h_segs, 1);

  for (int k = 0; k < h_segs; ++k)
    bb_geom_tri_(m, k, k + h_segs + 1, k + h_segs);
  for (int k = 1; k < v_segs - 1; ++k) {
    for (int j = 0; j < h_segs; ++j) {
      const int a = k * (h_segs + 1) + j - 1;
      bb_geom_tri_(m, a, a + 1, a + 1 + h_segs + 1);
      bb_geom_tri_(m, a, a + 1 + h_segs + 1, a + h_segs + 1);
    }
  }
  for (int k = 0; k < h_segs; ++k) {
    const int a = (h_segs + 1) * (v_segs - 1) + k - 1;
    bb_geom_tri_(m, a, a + 1, a + 1 + h_segs);
  }

  m.dirty = true;
  return m;
}

// Mantel: je Segmentkante ein Vertex oben und einer unten, die Naht doppelt.
// Deckel (solid): eigene Flaeche, je Segment ein Vertex oben und einer unten,
// beide Faecher ab Vertex 0 bzw. 1.
static inline std::vector<bb_MeshData_> bb_gen_cylinder_(int segs, bool solid) {
  if (segs < 3) segs = 3;
  std::vector<bb_MeshData_> out(solid ? 2 : 1);

  bb_MeshData_& s = out[0];
  for (int k = 0; k <= segs; ++k) {
    const float yaw = (k % segs) * bb_geom_twopi_ / segs;
    float p[3];
    bb_geom_rot_k_(0.0f, yaw, p);
    const float u = float(k) / segs;
    bb_geom_vert_(s, p[0], 1, p[2], p[0], 0, p[2], u, 0);
    bb_geom_vert_(s, p[0], -1, p[2], p[0], 0, p[2], u, 1);
  }
  for (int k = 0; k < segs; ++k) {
    const int a = k * 2;
    bb_geom_tri_(s, a, a + 2, a + 3);
    bb_geom_tri_(s, a, a + 3, a + 1);
  }
  s.dirty = true;
  if (!solid) return out;

  bb_MeshData_& c = out[1];
  for (int k = 0; k < segs; ++k) {
    const float yaw = k * bb_geom_twopi_ / segs;
    float p[3];
    bb_geom_rot_k_(0.0f, yaw, p);
    const float u = p[0] * .5f + .5f, v = p[2] * .5f + .5f;
    bb_geom_vert_(c, p[0], 1, p[2], 0, 1, 0, u, v);
    bb_geom_vert_(c, p[0], -1, p[2], 0, -1, 0, u, v);
  }
  for (int k = 2; k < segs; ++k) {
    bb_geom_tri_(c, 0, k * 2, (k - 1) * 2);
    bb_geom_tri_(c, 1, (k - 1) * 2 + 1, k * 2 + 1);
  }
  c.dirty = true;
  return out;
}

// Mantel: segs Spitzenvertices (je Segment eigenes u), segs+1 Randvertices mit
// waagerechter Normale. Boden (solid): eigene Flaeche, Normalen wie der Rand
// des Mantels - so steht es im Original, nicht (0,-1,0).
static inline std::vector<bb_MeshData_> bb_gen_cone_(int segs, bool solid) {
  if (segs < 3) segs = 3;
  std::vector<bb_MeshData_> out(solid ? 2 : 1);

  bb_MeshData_& s = out[0];
  for (int k = 0; k < segs; ++k)
    bb_geom_vert_(s, 0, 1, 0, 0, 1, 0, (k + .5f) / segs, 0);
  for (int k = 0; k <= segs; ++k) {
    const float yaw = (k % segs) * bb_geom_twopi_ / segs;
    const float x = -sinf(yaw), z = cosf(yaw);    // yawMatrix(yaw).k
    bb_geom_vert_(s, x, -1, z, x, 0, z, float(k) / segs, 1);
  }
  for (int k = 0; k < segs; ++k)
    bb_geom_tri_(s, k, k + segs + 1, k + segs);
  s.dirty = true;
  if (!solid) return out;

  bb_MeshData_& b = out[1];
  for (int k = 0; k < segs; ++k) {
    const float yaw = k * bb_geom_twopi_ / segs;
    const float x = -sinf(yaw), z = cosf(yaw);
    bb_geom_vert_(b, x, -1, z, x, 0, z, x * .5f + .5f, z * .5f + .5f);
  }
  for (int k = 2; k < segs; ++k)
    bb_geom_tri_(b, 0, k - 1, k);
  b.dirty = true;
  return out;
}

// ============================================================
// Create commands
// ============================================================

static inline int bb_mesh_create_(std::vector<bb_MeshData_> surfs, int parent) {
  auto ent = std::make_unique<bb_MeshEntity_>();
  for (auto& s : surfs) ent->surfaces().push_back(std::move(s));
  return bb_entity_register_(std::move(ent), parent);
}

static inline int bb_mesh_create_(bb_MeshData_ surf, int parent) {
  std::vector<bb_MeshData_> v;
  v.push_back(std::move(surf));
  return bb_mesh_create_(std::move(v), parent);
}

inline int bb_CreateCube(int parent = 0) {
  return bb_mesh_create_(bb_gen_cube_(), parent);
}

inline int bb_CreateSphere(int segs = 8, int parent = 0) {
  return bb_mesh_create_(bb_gen_sphere_(segs), parent);
}

// Der zweite Parameter heisst im Original `solid` und steht auf 1: mit 0
// fehlen die Deckel. Bis 2026-09-17 hiess er hier `open` mit Vorgabe 0 und
// bedeutete das Gegenteil (BUG-133).
inline int bb_CreateCylinder(int segs = 8, int solid = 1, int parent = 0) {
  return bb_mesh_create_(bb_gen_cylinder_(segs, solid != 0), parent);
}

inline int bb_CreateCone(int segs = 8, int solid = 1, int parent = 0) {
  return bb_mesh_create_(bb_gen_cone_(segs, solid != 0), parent);
}

// ============================================================
// EntityTexture (3D-11)
// ============================================================

// EntityTexture entity,texture[,frame][,index] - der Index ist laut Doku
// 0-7 und dient dem Multitexturing (siehe TextureBlend). Ein Texturhandle
// von 0 raeumt die Lage wieder ab.
inline void bb_EntityTexture(int entity, int texture, int frame = 0, int index = 0) {
  // Im Original ein Model: Netze, Sprites (3D-16) und MD2 (3D-23), keine
  // Pivots.
  bb_Entity_* me = bb_mesh_ent_(entity);
  if (!me) me = bb_sprite_ent_(entity);
  if (!me) me = bb_md2_ent_(entity);
  if (!me) return;
  if (index < 0 || index >= BB_TEX_SLOTS) return;
  // Seit 3D-15 landet sie im Brush der **Entity** und nicht mehr in dem
  // jeder Flaeche. Im Bild ist das dasselbe - beim Verrechnen ueberschreibt
  // die Entity die Lage der Flaeche -, aber die Flaeche behaelt ihre eigene
  // Textur. Vorher hat EntityTexture sie ueberschrieben, und ein
  // GetSurfaceBrush danach haette die falsche gemeldet.
  me->brush.tex.tex[index]   = bb_texture_ref_(texture);
  me->brush.tex.frame[index] = frame;
}

// ============================================================
// PaintMesh (3D-15)
// ============================================================
//
// Legt den Brush in **jede** Flaeche des Netzes; im Original ist das
// Rep::paint, eine Schleife ueber alle Surfaces mit setBrush. Auch hier
// eine Kopie - spaetere Aenderungen am Brush erreichen das Netz nicht mehr.
//
// Der Unterschied zu PaintEntity ist genau der, den die Doku zu CreateBrush
// beschreibt: PaintEntity faerbt das Ganze auf einen Schlag, PaintMesh
// setzt das Aussehen jeder einzelnen Flaeche neu und macht damit
// unterschiedliche Flaechen gleich.
inline void bb_PaintMesh(int mesh, int brush) {
  auto*      me = bb_mesh_ent_(mesh);
  bb_Brush_* b  = bb_brush_get_(brush);
  if (!me || !b) return;
  for (auto& s : me->surfaces()) s.brush = *b;
}

// ============================================================
// AABB queries (MeshWidth/Height/Depth)
// ============================================================

static inline void bb_mesh_aabb_(const bb_MeshEntity_* me,
                                  float& minX, float& maxX,
                                  float& minY, float& maxY,
                                  float& minZ, float& maxZ) {
  minX = minY = minZ =  FLT_MAX;
  maxX = maxY = maxZ = -FLT_MAX;
  for (const auto& s : me->surfaces()) {
    const auto& v = s.vertices;
    for (size_t i = 0; i + BB_VF - 1 < v.size(); i += BB_VF) {
      if (v[i]   < minX) minX = v[i];
      if (v[i]   > maxX) maxX = v[i];
      if (v[i+1] < minY) minY = v[i+1];
      if (v[i+1] > maxY) maxY = v[i+1];
      if (v[i+2] < minZ) minZ = v[i+2];
      if (v[i+2] > maxZ) maxZ = v[i+2];
    }
  }
}

inline float bb_MeshWidth(int h) {
  auto* me = bb_mesh_ent_(h);
  if (!me) return 0.0f;
  float x0,x1,y0,y1,z0,z1;
  bb_mesh_aabb_(me, x0,x1,y0,y1,z0,z1);
  return (x1 > x0) ? (x1 - x0) : 0.0f;
}

inline float bb_MeshHeight(int h) {
  auto* me = bb_mesh_ent_(h);
  if (!me) return 0.0f;
  float x0,x1,y0,y1,z0,z1;
  bb_mesh_aabb_(me, x0,x1,y0,y1,z0,z1);
  return (y1 > y0) ? (y1 - y0) : 0.0f;
}

inline float bb_MeshDepth(int h) {
  auto* me = bb_mesh_ent_(h);
  if (!me) return 0.0f;
  float x0,x1,y0,y1,z0,z1;
  bb_mesh_aabb_(me, x0,x1,y0,y1,z0,z1);
  return (z1 > z0) ? (z1 - z0) : 0.0f;
}

// ============================================================
// Meshbefehle (3D-13)
//
// Alle diese Befehle arbeiten auf den **Vertices** und nicht auf der
// Transformation der Entity - "Scales all vertices of a mesh" - und rechnen
// laut Doku vom globalen Ursprung 0,0,0 aus. ScaleEntity und PositionEntity
// lassen die Geometrie dagegen unberuehrt.
//
// Was die Doku offenlaesst, ist am laufenden Original nachgemessen; die
// Ausmasse liefert MeshWidth/Height/Depth als Zahl, Lage und Beleuchtung
// kommen aus ReadPixel:
//
//   ScaleMesh  ist **kumulativ**: zweimal 2 ergibt den vierfachen Wuerfel
//              (2.0 -> 4.0 -> 8.0 gemessen).
//   FitMesh    setzt die **Mindestecke** der Box auf x,y,z - nicht die Mitte.
//              Mit uniform gilt der **kleinste** der drei Faktoren: ein
//              2x2x2-Wuerfel in eine Box 4x2x6 gepasst bleibt 2x2x2.
//   FlipMesh   kehrt Umlaufsinn **und Normalen** um (gemessen: die Flaeche
//              wird unbeleuchtet, wenn man die Rueckseitenentfernung
//              abschaltet).
//   LightMesh  addiert auf die Vertexfarben und klemmt:
//                 Farbe * (range / Abstand) * max(N.L, 0)
//              Ohne Reichweite oder mit Reichweite 0 wird gleichmaessig
//              addiert, ohne Abstand und ohne N.L. Fuenf Messpunkte
//              bestaetigen die Formel (siehe DEVLOG).
//   AddMesh    fasst die Geometrie in die **vorhandene** Flaeche zusammen;
//              die Zahl der Flaechen bleibt 1, die Quelle bleibt erhalten.
//
// Nicht angetastet werden die Normalen bei ScaleMesh: die Doku nennt
// UpdateNormals ausdruecklich als das Mittel, sie nach solchen Eingriffen
// wieder richtigzustellen ("This is necessary for correct lighting if you
// have not set surface normals").
// ============================================================

// Jede Aenderung an den Vertices muss neu auf die Grafikkarte.
static inline void bb_mesh_touch_(bb_MeshEntity_* me) {
  for (auto& s : me->surfaces()) s.dirty = true;
  ++bb_mesh_geom_version_;
}

// ---- CreateMesh: leeres Netz, Geometrie kommt mit AddMesh oder 3D-15 ----

inline int bb_CreateMesh(int parent = 0) {
  auto ent = std::make_unique<bb_MeshEntity_>();
  return bb_entity_register_(std::move(ent), parent);
}

inline int bb_CountSurfaces(int h) {
  auto* me = bb_mesh_ent_(h);
  return me ? static_cast<int>(me->surfaces().size()) : 0;
}

// ---- ScaleMesh / PositionMesh / RotateMesh ----

inline void bb_ScaleMesh(int h, float x_scale, float y_scale, float z_scale) {
  auto* me = bb_mesh_ent_(h);
  if (!me) return;
  for (auto& s : me->surfaces())
    for (size_t i = 0; i + BB_VF - 1 < s.vertices.size(); i += BB_VF) {
      s.vertices[i]     *= x_scale;
      s.vertices[i + 1] *= y_scale;
      s.vertices[i + 2] *= z_scale;
    }
  bb_mesh_touch_(me);
}

inline void bb_PositionMesh(int h, float x, float y, float z) {
  auto* me = bb_mesh_ent_(h);
  if (!me) return;
  for (auto& s : me->surfaces())
    for (size_t i = 0; i + BB_VF - 1 < s.vertices.size(); i += BB_VF) {
      s.vertices[i]     += x;
      s.vertices[i + 1] += y;
      s.vertices[i + 2] += z;
    }
  bb_mesh_touch_(me);
}

// Dieselbe YXZ-Reihenfolge wie RotateEntity - die Drehung soll fuer Netz und
// Entity dieselbe Bedeutung haben. Die Normalen drehen mit; ohne das waere
// ein gedrehtes Netz von der falschen Seite beleuchtet, und davor warnt die
// Doku im Gegensatz zu ScaleMesh nicht.
inline void bb_RotateMesh(int h, float pitch, float yaw, float roll) {
  auto* me = bb_mesh_ent_(h);
  if (!me) return;
  float R[16];
  mat4_make_euler_YXZ_(R, pitch, yaw, roll);
  auto turn = [&R](float& a, float& b, float& c) {
    float x = a, y = b, z = c;
    a = R[0] * x + R[4] * y + R[8]  * z;
    b = R[1] * x + R[5] * y + R[9]  * z;
    c = R[2] * x + R[6] * y + R[10] * z;
  };
  for (auto& s : me->surfaces())
    for (size_t i = 0; i + BB_VF - 1 < s.vertices.size(); i += BB_VF) {
      turn(s.vertices[i],     s.vertices[i + 1], s.vertices[i + 2]);
      turn(s.vertices[i + 3], s.vertices[i + 4], s.vertices[i + 5]);
    }
  bb_mesh_touch_(me);
}

// Gemessen: x,y,z ist die **Mindestecke** der Zielbox, nicht deren Mitte, und
// uniform nimmt den kleinsten der drei Faktoren.
inline void bb_FitMesh(int h, float x, float y, float z,
                       float width, float height, float depth,
                       int uniform = 0) {
  auto* me = bb_mesh_ent_(h);
  if (!me || me->surfaces().empty()) return;

  float x0, x1, y0, y1, z0, z1;
  bb_mesh_aabb_(me, x0, x1, y0, y1, z0, z1);
  if (x1 < x0) return;                       // keine Vertices

  float ex = x1 - x0, ey = y1 - y0, ez = z1 - z0;
  float sx = (ex > 1e-9f) ? width  / ex : 1.0f;
  float sy = (ey > 1e-9f) ? height / ey : 1.0f;
  float sz = (ez > 1e-9f) ? depth  / ez : 1.0f;
  if (uniform) {
    float s = sx;
    if (sy < s) s = sy;
    if (sz < s) s = sz;
    sx = sy = sz = s;
  }

  for (auto& s : me->surfaces())
    for (size_t i = 0; i + BB_VF - 1 < s.vertices.size(); i += BB_VF) {
      s.vertices[i]     = (s.vertices[i]     - x0) * sx + x;
      s.vertices[i + 1] = (s.vertices[i + 1] - y0) * sy + y;
      s.vertices[i + 2] = (s.vertices[i + 2] - z0) * sz + z;
    }
  bb_mesh_touch_(me);
}

// ---- FlipMesh ----

// Gemessen: der Umlaufsinn **und** die Normalen kehren sich um. Mit
// abgeschalteter Rueckseitenentfernung wird die vorher beleuchtete Flaeche
// danach schwarz - das geht nur, wenn auch die Normale kippt.
inline void bb_FlipMesh(int h) {
  auto* me = bb_mesh_ent_(h);
  if (!me) return;
  for (auto& s : me->surfaces()) {
    for (size_t i = 0; i + 2 < s.indices.size(); i += 3)
      std::swap(s.indices[i + 1], s.indices[i + 2]);
    for (size_t i = 0; i + BB_VF - 1 < s.vertices.size(); i += BB_VF) {
      s.vertices[i + 3] = -s.vertices[i + 3];
      s.vertices[i + 4] = -s.vertices[i + 4];
      s.vertices[i + 5] = -s.vertices[i + 5];
    }
  }
  bb_mesh_touch_(me);
}

// ---- UpdateNormals ----

// Mittelt die Flaechennormalen ueber die Dreiecke, die sich einen Vertex
// **teilen**. Unsere Primitiven legen fuer jede Flaeche eigene Vertices an,
// ein Wuerfel bleibt dadurch kantig - gemessen aendert UpdateNormals auch im
// Original am Wuerfelbild nichts.
inline void bb_UpdateNormals(int h) {
  auto* me = bb_mesh_ent_(h);
  if (!me) return;
  for (auto& s : me->surfaces()) {
    const size_t n = s.vertices.size() / BB_VF;
    if (!n) continue;
    std::vector<float> acc(n * 3, 0.0f);

    for (size_t t = 0; t + 2 < s.indices.size(); t += 3) {
      unsigned a = s.indices[t], b = s.indices[t + 1], c = s.indices[t + 2];
      if (a >= n || b >= n || c >= n) continue;
      const float* pa = &s.vertices[a * BB_VF];
      const float* pb = &s.vertices[b * BB_VF];
      const float* pc = &s.vertices[c * BB_VF];
      float ux = pb[0] - pa[0], uy = pb[1] - pa[1], uz = pb[2] - pa[2];
      float vx = pc[0] - pa[0], vy = pc[1] - pa[1], vz = pc[2] - pa[2];
      // (b-a)x(c-a): zeigt zur Vorderseite, wie im Original (BUG-126,
      // gemessen an einem Dreieck aus einer .x-Datei ohne MeshNormals).
      float nx = uy * vz - uz * vy;
      float ny = uz * vx - ux * vz;
      float nz = ux * vy - uy * vx;
      for (unsigned idx : { a, b, c }) {
        acc[idx * 3]     += nx;
        acc[idx * 3 + 1] += ny;
        acc[idx * 3 + 2] += nz;
      }
    }

    for (size_t v = 0; v < n; ++v) {
      float nx = acc[v * 3], ny = acc[v * 3 + 1], nz = acc[v * 3 + 2];
      float len = std::sqrt(nx * nx + ny * ny + nz * nz);
      if (len > 1e-9f) { nx /= len; ny /= len; nz /= len; }
      s.vertices[v * BB_VF + 3] = nx;
      s.vertices[v * BB_VF + 4] = ny;
      s.vertices[v * BB_VF + 5] = nz;
    }
  }
  bb_mesh_touch_(me);
}

// Vertexfarben liegen im Original als Byte vor. Gemessen: bei Reichweite 1
// steht dort 69 und nicht 70 - der Wert wird also **abgeschnitten**, nicht
// gerundet, und wiederholte LightMesh-Aufrufe rechnen mit dem
// abgeschnittenen Wert weiter. Ohne diese Quantisierung wichen fuenf der
// Messpunkte um eins ab.
static inline float bb_vcol_(float v) {
  if (v <= 0.0f) return 0.0f;
  if (v >= 1.0f) return 1.0f;
  return std::floor(v * 255.0f) / 255.0f;
}

// ---- LightMesh ----

// Am Original ausgemessen (fuenf Messpunkte, siehe DEVLOG):
//
//     Vertexfarbe += Farbe * (range / Abstand) * max(N.L, 0)
//
// mit dem Abstand vom Licht zum jeweiligen Vertex. Ohne Reichweite - oder mit
// Reichweite 0 - wird die Farbe **gleichmaessig** addiert, ohne Abstand und
// ohne N.L; so setzt "LightMesh mesh,-255,-255,-255" die Vertexfarben auf 0
// zurueck, wie es die Doku beschreibt.
//
// Sichtbar wird das Ergebnis erst mit EntityFX 2 (Vertexfarben).
inline void bb_LightMesh(int h, float red, float green, float blue,
                         float range = 0.0f,
                         float light_x = 0.0f, float light_y = 0.0f,
                         float light_z = 0.0f) {
  auto* me = bb_mesh_ent_(h);
  if (!me) return;
  const float r = red / 255.0f, g = green / 255.0f, b = blue / 255.0f;

  for (auto& s : me->surfaces())
    for (size_t i = 0; i + BB_VF - 1 < s.vertices.size(); i += BB_VF) {
      float f = 1.0f;
      if (range > 0.0f) {
        float dx = light_x - s.vertices[i];
        float dy = light_y - s.vertices[i + 1];
        float dz = light_z - s.vertices[i + 2];
        float d  = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (d < 1e-6f) {
          f = 1.0f;
        } else {
          float nl = (dx * s.vertices[i + 3] + dy * s.vertices[i + 4] +
                      dz * s.vertices[i + 5]) / d;
          if (nl < 0.0f) nl = 0.0f;
          f = (range / d) * nl;
        }
      }
      float* c = &s.vertices[i + 10];   // r,g,b der Vertexfarbe
      c[0] = bb_vcol_(c[0] + r * f);
      c[1] = bb_vcol_(c[1] + g * f);
      c[2] = bb_vcol_(c[2] + b * f);
    }
  bb_mesh_touch_(me);
}

// ---- AddMesh / CopyMesh ----

// Gemessen: die Geometrie kommt in die **vorhandene** Flaeche, die Zahl der
// Flaechen bleibt also 1, und die Quelle bleibt danach bestehen. Ein
// Brush-System, das ein Zusammenfassen verhindern koennte, gibt es noch nicht
// (3D-15); sobald es das gibt, darf nur bei gleichem Brush zusammengefasst
// werden.
inline void bb_AddMesh(int source_mesh, int dest_mesh) {
  auto* src = bb_mesh_ent_(source_mesh);
  auto* dst = bb_mesh_ent_(dest_mesh);
  if (!src || !dst || src == dst) return;
  if (dst->surfaces().empty()) dst->surfaces().emplace_back();
  bb_MeshData_& into = dst->surfaces()[0];

  for (const auto& s : src->surfaces()) {
    const unsigned base = static_cast<unsigned>(into.vertices.size() / BB_VF);
    into.vertices.insert(into.vertices.end(), s.vertices.begin(), s.vertices.end());
    for (unsigned idx : s.indices) into.indices.push_back(base + idx);
  }
  into.dirty = true;
}

// Laut Doku "identical to performing new_mesh=CreateMesh() : AddMesh mesh,new_mesh".
inline int bb_CopyMesh(int mesh, int parent = 0) {
  if (!bb_mesh_ent_(mesh)) return 0;
  int h = bb_CreateMesh(parent);
  bb_AddMesh(mesh, h);
  return h;
}

// ---- MeshesIntersect ----

// Erst die Huellkoerper, dann Dreieck gegen Dreieck - die Doku nennt den
// Befehl selbst "a fairly slow routine".
static inline bool bb_tri_tri_hit_(const float* a0, const float* a1, const float* a2,
                                    const float* b0, const float* b1, const float* b2);

inline int bb_MeshesIntersect(int mesh_a, int mesh_b) {
  auto* A = bb_mesh_ent_(mesh_a);
  auto* B = bb_mesh_ent_(mesh_b);
  if (!A || !B) return 0;

  float ax0, ax1, ay0, ay1, az0, az1, bx0, bx1, by0, by1, bz0, bz1;
  bb_mesh_aabb_(A, ax0, ax1, ay0, ay1, az0, az1);
  bb_mesh_aabb_(B, bx0, bx1, by0, by1, bz0, bz1);
  // Beide Weltmatrizen auffrischen, bevor sie gelesen werden - sonst rechnet
  // der Schnitttest mit der Lage vor dem letzten PositionEntity (BUG-71).
  bb_entity_refresh_world_(A);
  bb_entity_refresh_world_(B);
  // Die Huellkoerper stehen in Modellkoordinaten; die Weltmatrix der Entity
  // kommt dazu.
  auto to_world = [](const bb_MeshEntity_* e, const float* p, float* o) {
    const float* m = e->world;
    o[0] = m[0] * p[0] + m[4] * p[1] + m[8]  * p[2] + m[12];
    o[1] = m[1] * p[0] + m[5] * p[1] + m[9]  * p[2] + m[13];
    o[2] = m[2] * p[0] + m[6] * p[1] + m[10] * p[2] + m[14];
  };

  // Weltraum-AABB als Vortest: die acht Ecken beider Huellkoerper umrechnen.
  float wa0[3] = { 1e30f, 1e30f, 1e30f }, wa1[3] = { -1e30f, -1e30f, -1e30f };
  float wb0[3] = { 1e30f, 1e30f, 1e30f }, wb1[3] = { -1e30f, -1e30f, -1e30f };
  auto grow = [](float* lo, float* hi, const float* p) {
    for (int i = 0; i < 3; ++i) {
      if (p[i] < lo[i]) lo[i] = p[i];
      if (p[i] > hi[i]) hi[i] = p[i];
    }
  };
  for (int c = 0; c < 8; ++c) {
    float pa[3] = { (c & 1) ? ax1 : ax0, (c & 2) ? ay1 : ay0, (c & 4) ? az1 : az0 };
    float pb[3] = { (c & 1) ? bx1 : bx0, (c & 2) ? by1 : by0, (c & 4) ? bz1 : bz0 };
    float o[3];
    to_world(A, pa, o); grow(wa0, wa1, o);
    to_world(B, pb, o); grow(wb0, wb1, o);
  }
  for (int i = 0; i < 3; ++i)
    if (wa1[i] < wb0[i] || wb1[i] < wa0[i]) return 0;

  // Dreieck gegen Dreieck in Weltkoordinaten.
  std::vector<float> TA, TB;
  auto collect = [&to_world](const bb_MeshEntity_* e, std::vector<float>& out) {
    for (const auto& s : e->surfaces())
      for (size_t t = 0; t + 2 < s.indices.size(); t += 3)
        for (int k = 0; k < 3; ++k) {
          const float* p = &s.vertices[s.indices[t + k] * BB_VF];
          float o[3]; to_world(e, p, o);
          out.insert(out.end(), o, o + 3);
        }
  };
  collect(A, TA);
  collect(B, TB);

  for (size_t i = 0; i + 8 < TA.size(); i += 9)
    for (size_t j = 0; j + 8 < TB.size(); j += 9)
      if (bb_tri_tri_hit_(&TA[i], &TA[i + 3], &TA[i + 6],
                          &TB[j], &TB[j + 3], &TB[j + 6]))
        return 1;
  return 0;
}

// Trennachsentest fuer zwei Dreiecke: die beiden Flaechennormalen und die
// neun Kreuzprodukte der Kanten. Findet sich eine Achse, auf der sich die
// Projektionen nicht ueberlappen, beruehren sich die Dreiecke nicht.
static inline bool bb_tri_tri_hit_(const float* a0, const float* a1, const float* a2,
                                    const float* b0, const float* b1, const float* b2) {
  const float* A[3] = { a0, a1, a2 };
  const float* B[3] = { b0, b1, b2 };
  float ea[3][3], eb[3][3];
  for (int i = 0; i < 3; ++i)
    for (int k = 0; k < 3; ++k) {
      ea[i][k] = A[(i + 1) % 3][k] - A[i][k];
      eb[i][k] = B[(i + 1) % 3][k] - B[i][k];
    }

  auto separated = [&](const float* ax) {
    float len2 = ax[0] * ax[0] + ax[1] * ax[1] + ax[2] * ax[2];
    if (len2 < 1e-12f) return false;          // entartete Achse sagt nichts
    float amin = 1e30f, amax = -1e30f, bmin = 1e30f, bmax = -1e30f;
    for (int i = 0; i < 3; ++i) {
      float pa = A[i][0] * ax[0] + A[i][1] * ax[1] + A[i][2] * ax[2];
      float pb = B[i][0] * ax[0] + B[i][1] * ax[1] + B[i][2] * ax[2];
      if (pa < amin) amin = pa;
      if (pa > amax) amax = pa;
      if (pb < bmin) bmin = pb;
      if (pb > bmax) bmax = pb;
    }
    return amax < bmin || bmax < amin;
  };

  auto cross = [](const float* u, const float* v, float* o) {
    o[0] = u[1] * v[2] - u[2] * v[1];
    o[1] = u[2] * v[0] - u[0] * v[2];
    o[2] = u[0] * v[1] - u[1] * v[0];
  };

  float n[3];
  cross(ea[0], ea[1], n); if (separated(n)) return false;
  cross(eb[0], eb[1], n); if (separated(n)) return false;
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) {
      cross(ea[i], eb[j], n);
      if (separated(n)) return false;
    }
  return true;
}

// ============================================================
// Render helper — called from bb_graphics3d.h per camera pass
//
// Zeichenreihenfolge (3D-10), am Original nachgemessen:
//   1. EntityOrder absteigend - ein Wert > 0 wird zuerst und damit hinter
//      allem gezeichnet, ein Wert < 0 zuletzt und damit vor allem. Bei einem
//      Wert ungleich 0 ist ausserdem der Z-Puffer abgeschaltet.
//   2. Innerhalb derselben Ordnung erst die deckenden, dann die
//      durchscheinenden Flaechen von hinten nach vorn. Ohne das zeigt eine
//      Glasscheibe, was zufaellig vor ihr gezeichnet wurde.
// ============================================================

// Braucht dieses Entity Blending? Alles, was nicht deckend Alpha 1 im
// Vorgabemodus ist: ein Alphawert unter 1, ein anderer Blendmodus,
// EntityFX 32 oder eine Texturlage mit Alphaflag.
static inline bool bb_brush_translucent_(const bb_Brush_& br) {
  if (br.alpha < 1.0f) return true;
  if (br.blend >= 2)   return true;
  if (br.fx & 32)      return true;
  for (int i = 0; i < BB_TEX_SLOTS; ++i)
    if (br.tex.tex[i] && (br.tex.tex[i]->flags & BB_TEX_ALPHA))
      return true;
  return false;
}

static inline bool bb_ent_translucent_(const bb_MeshEntity_* me) {
  if (me->brush.alpha < 1.0f) return true;
  if (me->brush.blend >= 2)   return true;
  if (me->brush.fx & 32)      return true;
  for (const auto& s : me->surfaces()) {
    const bb_Brush_ br = bb_brush_combine_(s.brush, me->brush);
    if (br.alpha < 1.0f) return true;
    if (br.blend >= 2)   return true;
    if (br.fx & 32)      return true;
    for (int i = 0; i < BB_TEX_SLOTS; ++i)
      if (br.tex.tex[i] && (br.tex.tex[i]->flags & BB_TEX_ALPHA))
        return true;
  }
  return false;
}

static inline void bb_render_meshes_(bb_Shader_* shader,
                                      const float* view,
                                      const float* proj,
                                      const float* cam_world) {
  const float cam_pos[3] = { cam_world[12], cam_world[13], cam_world[14] };
  // "fade" ist nur der Faktor aus EntityAutoFade; die Deckkraft entsteht
  // erst je Flaeche aus dem verrechneten Brush. Ein Sprite (3D-16) hat
  // keine Flaechen, nur seinen Brush und das je Kamera gebaute Quadrat.
  struct Item { bb_Entity_* e; bb_MeshEntity_* me; bb_SpriteEntity_* sp;
                float fade; float dist; bool translucent;
                bb_Md2Entity_* md = nullptr; };   // MD2 (3D-23): ein Netz, Brush der Entity
  std::vector<Item> items;
  items.reserve(bb_entities_.size());

  for (auto& [h, ent] : bb_entities_) {
    const bool is_mesh   = ent->kind() == bb_EntityKind_::Mesh;
    const bool is_sprite = ent->kind() == bb_EntityKind_::Sprite;
    const bool is_md2    = ent->kind() == bb_EntityKind_::Md2;
    if (!is_mesh && !is_sprite && !is_md2) continue;
    if (!bb_entity_shown_(ent.get())) continue;
    bb_Entity_* me = ent.get();

    float dx = me->world[12] - cam_pos[0];
    float dy = me->world[13] - cam_pos[1];
    float dz = me->world[14] - cam_pos[2];
    float dist = sqrtf(dx*dx + dy*dy + dz*dz);

    // EntityAutoFade: gemessen alpha = (far - Abstand) / (far - near),
    // geklemmt, mit dem Abstand zum Ursprung des Entity.
    float fade = 1.0f;
    if (me->fadeFar > me->fadeNear) {
      float f = (me->fadeFar - dist) / (me->fadeFar - me->fadeNear);
      fade = (f < 0.0f) ? 0.0f : (f > 1.0f ? 1.0f : f);
    }
    // Alpha 0 wird laut Doku gar nicht gezeichnet - und bleibt trotzdem fuer
    // Kollisionen vorhanden, anders als HideEntity.
    const float ent_alpha = me->brush.alpha * fade;
    if (ent_alpha <= 0.0f) continue;

    if (is_mesh) {
      auto* mm = static_cast<bb_MeshEntity_*>(me);
      items.push_back({ me, mm, nullptr, fade, dist,
                        bb_ent_translucent_(mm) || ent_alpha < 1.0f });
    } else if (is_md2) {
      auto* md = static_cast<bb_Md2Entity_*>(me);
      items.push_back({ me, nullptr, nullptr, fade, dist,
                        bb_brush_translucent_(md->brush) || ent_alpha < 1.0f, md });
    } else {
      auto* sp = static_cast<bb_SpriteEntity_*>(me);
      items.push_back({ me, nullptr, sp, fade, dist,
                        bb_brush_translucent_(sp->brush) || ent_alpha < 1.0f });
    }
  }

  std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) {
    if (a.e->order != b.e->order) return a.e->order > b.e->order;
    if (a.translucent != b.translucent) return !a.translucent;
    if (a.translucent) return a.dist > b.dist;   // hinten zuerst
    return false;
  });

  // Durchscheinende Flaechen der Ordnung 0 wie World::flushTransparent: die
  // Modelle kommen in Aufzaehlungsreihenfolge (Wurzeln nach Erzeugung, je
  // Entity erst es selbst, dann die Kinder) in eine priority_queue nach
  // Abstand, der weiteste zuerst. Bei gleichem Abstand entscheidet die
  // Heap-Mechanik der STL des Originals - dieselbe wie bei den Kameras
  // (bb_collect_cameras_, BUG-129). Am Original gemessen: Funke vor grossem
  // Sprite in gleicher Tiefe, der zuerst erzeugte wird zuerst gezeichnet
  // (BUG-169).
  {
    auto lo = std::find_if(items.begin(), items.end(), [](const Item& it) {
      return it.e->order == 0 && it.translucent; });
    auto hi = std::find_if(lo, items.end(), [](const Item& it) {
      return !(it.e->order == 0 && it.translucent); });
    if (hi - lo > 1) {
      std::unordered_map<const bb_Entity_*, int> rank;
      std::vector<bb_Entity_*> roots;
      for (auto& [h, e] : bb_entities_)
        if (e->parent == 0) roots.push_back(e.get());
      std::sort(roots.begin(), roots.end(),
                [](bb_Entity_* a, bb_Entity_* b) { return a->seq < b->seq; });
      std::vector<bb_Entity_*> stack(roots.rbegin(), roots.rend());
      while (!stack.empty()) {
        bb_Entity_* e = stack.back();
        stack.pop_back();
        rank.emplace(e, (int)rank.size());
        for (auto k = e->children.rbegin(); k != e->children.rend(); ++k)
          if (bb_Entity_* c = bb_entity_get_(*k)) stack.push_back(c);
      }
      std::vector<Item> found(lo, hi);
      std::sort(found.begin(), found.end(), [&rank](const Item& a, const Item& b) {
        return rank[a.e] < rank[b.e]; });
      std::vector<Item> heap;
      for (const Item& it : found) {
        heap.push_back(it);
        for (size_t i = heap.size() - 1; i > 0;) {
          size_t j = (i - 1) / 2;
          if (!(heap[j].dist < heap[i].dist)) break;
          std::swap(heap[i], heap[j]);
          i = j;
        }
      }
      auto out = lo;
      while (!heap.empty()) {
        *out++ = heap[0];
        heap[0] = heap.back();
        heap.pop_back();
        const size_t z = heap.size();
        for (size_t i = 0; 2 * i + 1 < z;) {
          size_t j = 2 * i + 1;
          if (j + 1 < z && !(heap[j + 1].dist < heap[j].dist)) ++j;
          if (heap[j].dist < heap[i].dist) break;
          std::swap(heap[i], heap[j]);
          i = j;
        }
      }
    }
  }

  bool  blend_on   = false;
  int   blend_mode = 0;
  bool  cull_on    = true;
  bool  depth_on   = true;
  bool  zwrite_on  = true;
  float vm[16];
  mat4_mul_(vm, proj, view);

  static const float identity[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };

  for (const Item& it : items) {
    bb_Entity_* me = it.e;

    // Ein Sprite liegt schon in Weltkoordinaten.
    if (it.sp) bb_sprite_build_(it.sp, cam_world);
    const float* model = it.sp ? identity : me->world;
    float mvp[16];
    mat4_mul_(mvp, vm, model);

    // MD2Model::render: ausserhalb des Sichtkegels weder gezeichnet noch
    // gezaehlt; sonst das Netz fuer den aktuellen Animationsstand fuellen.
    if (it.md) {
      if (!bb_md2_box_visible_(*it.md->rep, mvp)) continue;
      bb_md2_build_(it.md);
    }

    // ---- Blending ----
    // Am Original gemessen: 1 = Alpha (Vorgabe), 2 = Multiply, 3 = Add. Der
    // eigene Roadmap-Entwurf hatte 2 und 3 vertauscht. Der Modus selbst
    // kommt seit 3D-15 je Flaeche aus dem verrechneten Brush.
    const bool want_blend = it.translucent;
    if (want_blend != blend_on) {
      blend_on = want_blend;
      if (want_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
      // Unbekannt, damit die Mischfunktion sicher gesetzt wird. Bis
      // 2026-09-17 stand hier 0 - der Wert des Vorgabe-Brush -, und fuer ihn
      // wurde glBlendFunc nie gerufen: EntityAlpha mischte gar nicht oder mit
      // der Funktion des vorigen Objekts (BUG-142).
      blend_mode = -1;
    }

    // ---- Z-Puffer ----
    // Laut Doku schaltet eine Ordnung ungleich 0 das Z-Buffering ab.
    const bool want_depth = (me->order == 0);
    if (want_depth != depth_on) {
      depth_on = want_depth;
      if (want_depth) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    }

    // Jede Flaeche bringt ihren eigenen Brush mit; der wird mit dem der
    // Entity verrechnet - Farbe und Deckkraft mal, Glanz plus, FX oder,
    // Texturen von der Entity ueberschrieben. Die Formel steht in
    // bb_brush.h und stammt aus blitz3d/brush.cpp.
    const size_t nsurf = (it.sp || it.md) ? 1 : it.me->surfaces().size();
    for (size_t si = 0; si < nsurf; ++si) {
      bb_MeshData_& surf = it.sp ? it.sp->quad
                         : it.md ? it.md->mesh
                                 : it.me->surfaces()[si];
      const bb_Brush_ br = (it.sp || it.md) ? me->brush
                                            : bb_brush_combine_(surf.brush, me->brush);

      if (want_blend && br.blend != blend_mode) {
        blend_mode = br.blend;
        switch (blend_mode) {
          case 2:  glBlendFunc(GL_DST_COLOR, GL_ZERO);            break;
          case 3:  glBlendFunc(GL_SRC_ALPHA, GL_ONE);             break;
          default: glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // 0 und 1: Alpha
        }
      }

      // Rueckseitenentfernung: das Original entfernt Rueckseiten
      // (die Kamera im Wuerfel sieht den Hintergrund), FX 16 schaltet das
      // ab - und ein zweiseitiges Material aus der Datei je Flaeche (3D-13).
      const bool want_cull = ((br.fx & 16) == 0) && !br.twosided;
      if (want_cull != cull_on) {
        cull_on = want_cull;
        if (want_cull) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
      }
      bb_shader_uniform_i(shader, "u_fx", br.fx);
      float color[4] = { br.r / 255.0f,
                         br.g / 255.0f,
                         br.b / 255.0f,
                         br.alpha * it.fade };

      // Durchscheinende Flaechen pruefen den Z-Puffer, schreiben aber nicht
      // hinein: im Original landet jede Flaeche, deren Brush nicht "ersetzen"
      // mischt, im durchsichtigen Durchgang mit ZMODE_CMPONLY (world.cpp,
      // model.cpp). Sonst verdeckt ein fast unsichtbares Sprite alles, was
      // danach in gleicher Tiefe kommt - die Funken im Menue von
      // blox-n-balls hinter den Sprites mit Alpha 0.05 (BUG-169).
      const bool want_zwrite = !(bb_brush_translucent_(br) || color[3] < 1.0f);
      if (want_zwrite != zwrite_on) {
        zwrite_on = want_zwrite;
        glDepthMask(want_zwrite ? GL_TRUE : GL_FALSE);
      }
      bb_shader_uniform_f(shader, "u_shininess", br.shininess);
      bb_texture_bind_(shader, br.tex);
      bb_mesh_draw_(&surf, shader, mvp, model, color, nullptr);
      bb_tris_rendered_ += surf.triCount;
    }
  }

  if (blend_on)  glDisable(GL_BLEND);
  if (!zwrite_on) glDepthMask(GL_TRUE);  // sonst loescht glClear den Z-Puffer nicht
  if (!cull_on)  glEnable(GL_CULL_FACE);
  if (!depth_on) glEnable(GL_DEPTH_TEST);
}

#endif // BB_MESH_H
