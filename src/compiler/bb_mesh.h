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
#include <algorithm>
#include <cmath>
#include <cfloat>

// ============================================================
// bb_MeshEntity_ — entity with surfaces (one MeshData per surface)
// ============================================================

struct bb_MeshEntity_ : bb_Entity_ {
  std::vector<bb_MeshData_> surfaces;  // each surface = one draw call

  bb_EntityKind_ kind() const override { return bb_EntityKind_::Mesh; }

  ~bb_MeshEntity_() override {
    for (auto& s : surfaces) bb_mesh_free_gpu_(&s);
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

static inline bb_MeshData_ bb_gen_cube_() {
  bb_MeshData_ m;

  // +X face
  bb_mesh_push_quad_(m,  1,-1,-1, 0,1,  1, 1,-1, 0,0,  1, 1, 1, 1,0,  1,-1, 1, 1,1,  1,0,0);
  // -X face
  bb_mesh_push_quad_(m, -1,-1, 1, 0,1, -1, 1, 1, 0,0, -1, 1,-1, 1,0, -1,-1,-1, 1,1, -1,0,0);
  // +Y face
  bb_mesh_push_quad_(m, -1, 1,-1, 0,1,  1, 1,-1, 0,0,  1, 1, 1, 1,0, -1, 1, 1, 1,1,  0,1,0);
  // -Y face
  bb_mesh_push_quad_(m, -1,-1, 1, 0,1,  1,-1, 1, 0,0,  1,-1,-1, 1,0, -1,-1,-1, 1,1,  0,-1,0);
  // +Z face
  bb_mesh_push_quad_(m, -1,-1, 1, 0,1, -1, 1, 1, 0,0,  1, 1, 1, 1,0,  1,-1, 1, 1,1,  0,0,1);
  // -Z face
  bb_mesh_push_quad_(m,  1,-1,-1, 0,1,  1, 1,-1, 0,0, -1, 1,-1, 1,0, -1,-1,-1, 1,1,  0,0,-1);

  m.dirty = true;
  return m;
}

// ============================================================
// bb_gen_sphere_ — UV sphere, radius=1
// ============================================================

static inline bb_MeshData_ bb_gen_sphere_(int segs) {
  if (segs < 3) segs = 3;
  bb_MeshData_ m;

  int rings = segs;    // latitudinal rings (excluding poles)
  int slices = segs * 2;

  const float pi  = 3.14159265358979323846f;
  const float two_pi = 2.0f * pi;

  // Build grid of vertices: (rings+2) rows × (slices+1) cols
  // Row 0 = north pole, row rings+1 = south pole
  int rows = rings + 2;
  int cols = slices + 1;
  std::vector<float> vx(rows * cols), vy(rows * cols), vz(rows * cols);
  std::vector<float> uu(rows * cols), vv(rows * cols);

  for (int r = 0; r < rows; ++r) {
    float phi = pi * r / (rows - 1);   // 0 = top, pi = bottom
    float cp  = cosf(phi), sp = sinf(phi);
    for (int c = 0; c < cols; ++c) {
      float theta = two_pi * c / slices;
      float ct = cosf(theta), st = sinf(theta);
      int i = r * cols + c;
      vx[i] = sp * ct;
      vy[i] = cp;
      vz[i] = sp * st;
      uu[i] = (float)c / slices;
      vv[i] = (float)r / (rows - 1);
    }
  }

  // Emit quads for each cell
  unsigned int base = 0;
  for (int r = 0; r < rows - 1; ++r) {
    for (int c = 0; c < cols - 1; ++c) {
      int i00 = r * cols + c;
      int i10 = r * cols + c + 1;
      int i01 = (r+1) * cols + c;
      int i11 = (r+1) * cols + c + 1;

      auto push_v = [&](int i) {
        bb_vert_push_(m, vx[i], vy[i], vz[i], vx[i], vy[i], vz[i], uu[i], vv[i]);
      };

      base = static_cast<unsigned int>(m.vertices.size() / BB_VF);
      push_v(i00); push_v(i10); push_v(i11); push_v(i01);
      m.indices.push_back(base);   m.indices.push_back(base+1); m.indices.push_back(base+2);
      m.indices.push_back(base);   m.indices.push_back(base+2); m.indices.push_back(base+3);
    }
  }

  m.dirty = true;
  return m;
}

// ============================================================
// bb_gen_cylinder_ — Y-axis, radius=1, height=2 (-1 to +1)
// ============================================================

static inline bb_MeshData_ bb_gen_cylinder_(int segs, bool open) {
  if (segs < 3) segs = 3;
  bb_MeshData_ m;

  const float pi  = 3.14159265358979323846f;
  const float two_pi = 2.0f * pi;

  // Side faces
  for (int i = 0; i < segs; ++i) {
    float a0 = two_pi * i / segs;
    float a1 = two_pi * (i + 1) / segs;
    float x0 = cosf(a0), z0 = sinf(a0);
    float x1 = cosf(a1), z1 = sinf(a1);
    float u0 = (float)i / segs;
    float u1 = (float)(i+1) / segs;
    // Smooth normals on the sides
    unsigned int base = static_cast<unsigned int>(m.vertices.size() / BB_VF);
    bb_vert_push_(m, x0,-1,z0, x0,0,z0, u0,1);
    bb_vert_push_(m, x1,-1,z1, x1,0,z1, u1,1);
    bb_vert_push_(m, x1, 1,z1, x1,0,z1, u1,0);
    bb_vert_push_(m, x0, 1,z0, x0,0,z0, u0,0);
    m.indices.push_back(base); m.indices.push_back(base+1); m.indices.push_back(base+2);
    m.indices.push_back(base); m.indices.push_back(base+2); m.indices.push_back(base+3);
  }

  if (!open) {
    // Top cap (+Y) and bottom cap (-Y) as triangle fans
    unsigned int center;

    // Top cap
    center = static_cast<unsigned int>(m.vertices.size() / BB_VF);
    bb_vert_push_(m, 0,1,0, 0,1,0, 0.5f,0.5f);
    for (int i = 0; i < segs; ++i) {
      float a0 = two_pi * i / segs;
      float a1 = two_pi * (i+1) / segs;
      float x0=cosf(a0), z0=sinf(a0), x1=cosf(a1), z1=sinf(a1);
      unsigned int b = static_cast<unsigned int>(m.vertices.size() / BB_VF);
      bb_vert_push_(m, x0,1,z0, 0,1,0, 0.5f+0.5f*x0, 0.5f-0.5f*z0);
      bb_vert_push_(m, x1,1,z1, 0,1,0, 0.5f+0.5f*x1, 0.5f-0.5f*z1);
      m.indices.push_back(center); m.indices.push_back(b+1); m.indices.push_back(b);
    }

    // Bottom cap (-Y)
    center = static_cast<unsigned int>(m.vertices.size() / BB_VF);
    bb_vert_push_(m, 0,-1,0, 0,-1,0, 0.5f,0.5f);
    for (int i = 0; i < segs; ++i) {
      float a0 = two_pi * i / segs;
      float a1 = two_pi * (i+1) / segs;
      float x0=cosf(a0), z0=sinf(a0), x1=cosf(a1), z1=sinf(a1);
      unsigned int b = static_cast<unsigned int>(m.vertices.size() / BB_VF);
      bb_vert_push_(m, x0,-1,z0, 0,-1,0, 0.5f+0.5f*x0, 0.5f+0.5f*z0);
      bb_vert_push_(m, x1,-1,z1, 0,-1,0, 0.5f+0.5f*x1, 0.5f+0.5f*z1);
      m.indices.push_back(center); m.indices.push_back(b); m.indices.push_back(b+1);
    }
  }

  m.dirty = true;
  return m;
}

// ============================================================
// bb_gen_cone_ — Y-axis apex at +1, base at -1, radius=1
// ============================================================

static inline bb_MeshData_ bb_gen_cone_(int segs, bool open) {
  if (segs < 3) segs = 3;
  bb_MeshData_ m;

  const float pi  = 3.14159265358979323846f;
  const float two_pi = 2.0f * pi;
  // Normal tilt for the lateral face: angle of slant from horizontal
  const float slope_n = 1.0f / sqrtf(2.0f); // 45° (height=2, radius=1 → slope 1:1)

  // Lateral faces: apex at (0,1,0)
  for (int i = 0; i < segs; ++i) {
    float a0 = two_pi * i / segs;
    float a1 = two_pi * (i+1) / segs;
    float x0=cosf(a0), z0=sinf(a0);
    float x1=cosf(a1), z1=sinf(a1);
    // Average normal direction for this segment
    float xm = (x0+x1)*0.5f, zm = (z0+z1)*0.5f;
    float nlen = sqrtf(xm*xm + zm*zm);
    if (nlen > 1e-6f) { xm/=nlen; zm/=nlen; }
    float nx0=x0*slope_n, ny0=slope_n, nz0=z0*slope_n;
    float nx1=x1*slope_n, ny1=slope_n, nz1=z1*slope_n;
    float nxa=xm*slope_n, nya=slope_n, nza=zm*slope_n;
    float u0=(float)i/segs, u1=(float)(i+1)/segs, um=(u0+u1)*0.5f;

    unsigned int base = static_cast<unsigned int>(m.vertices.size() / BB_VF);
    bb_vert_push_(m, x0,-1,z0, nx0,ny0,nz0, u0,1);
    bb_vert_push_(m, x1,-1,z1, nx1,ny1,nz1, u1,1);
    bb_vert_push_(m, 0,  1, 0, nxa,nya,nza, um,0);
    m.indices.push_back(base); m.indices.push_back(base+1); m.indices.push_back(base+2);
  }

  if (!open) {
    // Bottom cap (-Y)
    unsigned int center = static_cast<unsigned int>(m.vertices.size() / BB_VF);
    bb_vert_push_(m, 0,-1,0, 0,-1,0, 0.5f,0.5f);
    for (int i = 0; i < segs; ++i) {
      float a0 = two_pi * i / segs;
      float a1 = two_pi * (i+1) / segs;
      float x0=cosf(a0), z0=sinf(a0), x1=cosf(a1), z1=sinf(a1);
      unsigned int b = static_cast<unsigned int>(m.vertices.size() / BB_VF);
      bb_vert_push_(m, x0,-1,z0, 0,-1,0, 0.5f+0.5f*x0, 0.5f+0.5f*z0);
      bb_vert_push_(m, x1,-1,z1, 0,-1,0, 0.5f+0.5f*x1, 0.5f+0.5f*z1);
      m.indices.push_back(center); m.indices.push_back(b); m.indices.push_back(b+1);
    }
  }

  m.dirty = true;
  return m;
}

// ============================================================
// Create commands
// ============================================================

static inline int bb_mesh_create_(bb_MeshData_ surf, int parent) {
  auto ent = std::make_unique<bb_MeshEntity_>();
  ent->surfaces.push_back(std::move(surf));
  return bb_entity_register_(std::move(ent), parent);
}

inline int bb_CreateCube(int parent = 0) {
  return bb_mesh_create_(bb_gen_cube_(), parent);
}

inline int bb_CreateSphere(int segs = 8, int parent = 0) {
  return bb_mesh_create_(bb_gen_sphere_(segs), parent);
}

inline int bb_CreateCylinder(int segs = 8, int open = 0, int parent = 0) {
  return bb_mesh_create_(bb_gen_cylinder_(segs, open != 0), parent);
}

inline int bb_CreateCone(int segs = 8, int open = 0, int parent = 0) {
  return bb_mesh_create_(bb_gen_cone_(segs, open != 0), parent);
}

// ============================================================
// EntityTexture (3D-11)
// ============================================================

// EntityTexture entity,texture[,frame][,index] - der Index ist laut Doku
// 0-7 und dient dem Multitexturing (siehe TextureBlend). Ein Texturhandle
// von 0 raeumt die Lage wieder ab.
inline void bb_EntityTexture(int entity, int texture, int frame = 0, int index = 0) {
  auto* me = bb_mesh_ent_(entity);
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
  for (auto& s : me->surfaces) s.brush = *b;
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
  for (const auto& s : me->surfaces) {
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
  for (auto& s : me->surfaces) s.dirty = true;
}

// ---- CreateMesh: leeres Netz, Geometrie kommt mit AddMesh oder 3D-15 ----

inline int bb_CreateMesh(int parent = 0) {
  auto ent = std::make_unique<bb_MeshEntity_>();
  return bb_entity_register_(std::move(ent), parent);
}

inline int bb_CountSurfaces(int h) {
  auto* me = bb_mesh_ent_(h);
  return me ? static_cast<int>(me->surfaces.size()) : 0;
}

// ---- ScaleMesh / PositionMesh / RotateMesh ----

inline void bb_ScaleMesh(int h, float x_scale, float y_scale, float z_scale) {
  auto* me = bb_mesh_ent_(h);
  if (!me) return;
  for (auto& s : me->surfaces)
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
  for (auto& s : me->surfaces)
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
  for (auto& s : me->surfaces)
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
  if (!me || me->surfaces.empty()) return;

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

  for (auto& s : me->surfaces)
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
  for (auto& s : me->surfaces) {
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
  for (auto& s : me->surfaces) {
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
      // Kreuzprodukt in der Reihenfolge, die zur Umlaufrichtung unserer
      // Primitiven passt (siehe bb_gen_cube_).
      float nx = uz * vy - uy * vz;
      float ny = ux * vz - uz * vx;
      float nz = uy * vx - ux * vy;
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

  for (auto& s : me->surfaces)
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
  if (dst->surfaces.empty()) dst->surfaces.emplace_back();
  bb_MeshData_& into = dst->surfaces[0];

  for (const auto& s : src->surfaces) {
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
    for (const auto& s : e->surfaces)
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
static inline bool bb_ent_translucent_(const bb_MeshEntity_* me) {
  if (me->brush.alpha < 1.0f) return true;
  if (me->brush.blend >= 2)   return true;
  if (me->brush.fx & 32)      return true;
  for (const auto& s : me->surfaces) {
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
                                      const float* cam_pos) {
  // "fade" ist nur der Faktor aus EntityAutoFade; die Deckkraft entsteht
  // erst je Flaeche aus dem verrechneten Brush.
  struct Item { bb_MeshEntity_* me; float fade; float dist; bool translucent; };
  std::vector<Item> items;
  items.reserve(bb_entities_.size());

  for (auto& [h, ent] : bb_entities_) {
    if (!ent->visible) continue;
    if (ent->kind() != bb_EntityKind_::Mesh) continue;
    auto* me = static_cast<bb_MeshEntity_*>(ent.get());

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

    items.push_back({ me, fade, dist,
                      bb_ent_translucent_(me) || ent_alpha < 1.0f });
  }

  std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) {
    if (a.me->order != b.me->order) return a.me->order > b.me->order;
    if (a.translucent != b.translucent) return !a.translucent;
    if (a.translucent) return a.dist > b.dist;   // hinten zuerst
    return false;
  });

  bool  blend_on   = false;
  int   blend_mode = 0;
  bool  cull_on    = true;
  bool  depth_on   = true;
  float vm[16];
  mat4_mul_(vm, proj, view);

  for (const Item& it : items) {
    bb_MeshEntity_* me = it.me;

    float mvp[16];
    mat4_mul_(mvp, vm, me->world);

    // ---- Blending ----
    // Am Original gemessen: 1 = Alpha (Vorgabe), 2 = Multiply, 3 = Add. Der
    // eigene Roadmap-Entwurf hatte 2 und 3 vertauscht. Der Modus selbst
    // kommt seit 3D-15 je Flaeche aus dem verrechneten Brush.
    const bool want_blend = it.translucent;
    if (want_blend != blend_on) {
      blend_on = want_blend;
      if (want_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
      blend_mode = 0;
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
    for (auto& surf : me->surfaces) {
      const bb_Brush_ br = bb_brush_combine_(surf.brush, me->brush);

      if (want_blend && br.blend != blend_mode) {
        blend_mode = br.blend;
        switch (blend_mode) {
          case 2:  glBlendFunc(GL_DST_COLOR, GL_ZERO);            break;
          case 3:  glBlendFunc(GL_SRC_ALPHA, GL_ONE);             break;
          default: glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
      bb_shader_uniform_f(shader, "u_shininess", br.shininess);
      bb_texture_bind_(shader, br.tex);
      bb_mesh_draw_(&surf, shader, mvp, me->world, color, nullptr);
      bb_tris_rendered_ += surf.triCount;
    }
  }

  if (blend_on)  glDisable(GL_BLEND);
  if (!cull_on)  glEnable(GL_CULL_FACE);
  if (!depth_on) glEnable(GL_DEPTH_TEST);
}

#endif // BB_MESH_H
