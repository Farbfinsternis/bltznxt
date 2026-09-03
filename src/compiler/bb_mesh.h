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
// Appends 3 vertices (each 11 floats) and 3 indices.
// ============================================================

static inline void bb_mesh_push_tri_(bb_MeshData_& m,
    float x0,float y0,float z0, float nx0,float ny0,float nz0, float u0,float v0,
    float x1,float y1,float z1, float nx1,float ny1,float nz1, float u1,float v1,
    float x2,float y2,float z2, float nx2,float ny2,float nz2, float u2,float v2) {
  unsigned int base = static_cast<unsigned int>(m.vertices.size() / 11);
  float vd[] = {
    x0,y0,z0, nx0,ny0,nz0, u0,v0, 1,1,1,
    x1,y1,z1, nx1,ny1,nz1, u1,v1, 1,1,1,
    x2,y2,z2, nx2,ny2,nz2, u2,v2, 1,1,1
  };
  m.vertices.insert(m.vertices.end(), vd, vd + 33);
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
  unsigned int base = static_cast<unsigned int>(m.vertices.size() / 11);
  float vd[] = {
    x0,y0,z0, nx,ny,nz, u0,v0, 1,1,1,
    x1,y1,z1, nx,ny,nz, u1,v1, 1,1,1,
    x2,y2,z2, nx,ny,nz, u2,v2, 1,1,1,
    x3,y3,z3, nx,ny,nz, u3,v3, 1,1,1
  };
  m.vertices.insert(m.vertices.end(), vd, vd + 44);
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
        float nx = vx[i], ny = vy[i], nz = vz[i];
        float vdata[11] = { nx, ny, nz, nx, ny, nz, uu[i], vv[i], 1,1,1 };
        m.vertices.insert(m.vertices.end(), vdata, vdata + 11);
      };

      base = static_cast<unsigned int>(m.vertices.size() / 11);
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
    unsigned int base = static_cast<unsigned int>(m.vertices.size() / 11);
    float vd[] = {
      x0,-1,z0, x0,0,z0, u0,1, 1,1,1,
      x1,-1,z1, x1,0,z1, u1,1, 1,1,1,
      x1, 1,z1, x1,0,z1, u1,0, 1,1,1,
      x0, 1,z0, x0,0,z0, u0,0, 1,1,1
    };
    m.vertices.insert(m.vertices.end(), vd, vd + 44);
    m.indices.push_back(base); m.indices.push_back(base+1); m.indices.push_back(base+2);
    m.indices.push_back(base); m.indices.push_back(base+2); m.indices.push_back(base+3);
  }

  if (!open) {
    // Top cap (+Y) and bottom cap (-Y) as triangle fans
    unsigned int center;
    float vd_c[11];

    // Top cap
    center = static_cast<unsigned int>(m.vertices.size() / 11);
    float tc[11] = { 0,1,0, 0,1,0, 0.5f,0.5f, 1,1,1 };
    m.vertices.insert(m.vertices.end(), tc, tc+11);
    for (int i = 0; i < segs; ++i) {
      float a0 = two_pi * i / segs;
      float a1 = two_pi * (i+1) / segs;
      float x0=cosf(a0), z0=sinf(a0), x1=cosf(a1), z1=sinf(a1);
      unsigned int b = static_cast<unsigned int>(m.vertices.size() / 11);
      float vd[] = {
        x0,1,z0, 0,1,0, 0.5f+0.5f*x0,0.5f-0.5f*z0, 1,1,1,
        x1,1,z1, 0,1,0, 0.5f+0.5f*x1,0.5f-0.5f*z1, 1,1,1
      };
      m.vertices.insert(m.vertices.end(), vd, vd+22);
      m.indices.push_back(center); m.indices.push_back(b+1); m.indices.push_back(b);
    }

    // Bottom cap (-Y)
    center = static_cast<unsigned int>(m.vertices.size() / 11);
    float bc[11] = { 0,-1,0, 0,-1,0, 0.5f,0.5f, 1,1,1 };
    m.vertices.insert(m.vertices.end(), bc, bc+11);
    for (int i = 0; i < segs; ++i) {
      float a0 = two_pi * i / segs;
      float a1 = two_pi * (i+1) / segs;
      float x0=cosf(a0), z0=sinf(a0), x1=cosf(a1), z1=sinf(a1);
      unsigned int b = static_cast<unsigned int>(m.vertices.size() / 11);
      float vd[] = {
        x0,-1,z0, 0,-1,0, 0.5f+0.5f*x0,0.5f+0.5f*z0, 1,1,1,
        x1,-1,z1, 0,-1,0, 0.5f+0.5f*x1,0.5f+0.5f*z1, 1,1,1
      };
      m.vertices.insert(m.vertices.end(), vd, vd+22);
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

    unsigned int base = static_cast<unsigned int>(m.vertices.size() / 11);
    float vd[] = {
      x0,-1,z0, nx0,ny0,nz0, u0,1, 1,1,1,
      x1,-1,z1, nx1,ny1,nz1, u1,1, 1,1,1,
      0,  1, 0, nxa,nya,nza, um,0, 1,1,1
    };
    m.vertices.insert(m.vertices.end(), vd, vd+33);
    m.indices.push_back(base); m.indices.push_back(base+1); m.indices.push_back(base+2);
  }

  if (!open) {
    // Bottom cap (-Y)
    unsigned int center = static_cast<unsigned int>(m.vertices.size() / 11);
    float bc[11] = { 0,-1,0, 0,-1,0, 0.5f,0.5f, 1,1,1 };
    m.vertices.insert(m.vertices.end(), bc, bc+11);
    for (int i = 0; i < segs; ++i) {
      float a0 = two_pi * i / segs;
      float a1 = two_pi * (i+1) / segs;
      float x0=cosf(a0), z0=sinf(a0), x1=cosf(a1), z1=sinf(a1);
      unsigned int b = static_cast<unsigned int>(m.vertices.size() / 11);
      float vd[] = {
        x0,-1,z0, 0,-1,0, 0.5f+0.5f*x0,0.5f+0.5f*z0, 1,1,1,
        x1,-1,z1, 0,-1,0, 0.5f+0.5f*x1,0.5f+0.5f*z1, 1,1,1
      };
      m.vertices.insert(m.vertices.end(), vd, vd+22);
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
    for (size_t i = 0; i + 10 < v.size(); i += 11) {
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
// Render helper — called from bb_graphics3d.h per camera pass
// ============================================================

static inline void bb_render_meshes_(bb_Shader_* shader,
                                      const float* view,
                                      const float* proj) {
  float color[4] = { 1, 1, 1, 1 };

  for (auto& [h, ent] : bb_entities_) {
    if (!ent->visible) continue;
    if (ent->kind() != bb_EntityKind_::Mesh) continue;
    auto* me = static_cast<bb_MeshEntity_*>(ent.get());

    // MVP = proj * view * model
    float vm[16], mvp[16];
    mat4_mul_(vm,  proj, view);
    mat4_mul_(mvp, vm,   me->world);

    for (auto& surf : me->surfaces) {
      bb_mesh_draw_(&surf, shader, mvp, me->world, color, 0, nullptr);
      bb_tris_rendered_ += surf.triCount;
    }
  }
}

#endif // BB_MESH_H
