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
#include <cmath>
#include <cfloat>

// ============================================================
// bb_MeshEntity_ — entity with surfaces (one MeshData per surface)
// ============================================================

struct bb_MeshEntity_ : bb_Entity_ {
  std::vector<bb_MeshData_> surfaces;  // each surface = one draw call
  bb_TexSlots_              tex;       // EntityTexture, Index 0-7 (3D-11)

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
// EntityTexture (3D-11)
// ============================================================

// EntityTexture entity,texture[,frame][,index] - der Index ist laut Doku
// 0-7 und dient dem Multitexturing (siehe TextureBlend). Ein Texturhandle
// von 0 raeumt die Lage wieder ab.
inline void bb_EntityTexture(int entity, int texture, int frame = 0, int index = 0) {
  auto* me = bb_mesh_ent_(entity);
  if (!me) return;
  if (index < 0 || index >= BB_TEX_SLOTS) return;
  me->tex.tex[index]   = bb_texture_ref_(texture);
  me->tex.frame[index] = frame;
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
  if (me->alpha < 1.0f)  return true;
  if (me->blend != 1)    return true;
  if (me->fx & 32)       return true;
  for (int i = 0; i < BB_TEX_SLOTS; ++i)
    if (me->tex.tex[i] && (me->tex.tex[i]->flags & BB_TEX_ALPHA)) return true;
  return false;
}

static inline void bb_render_meshes_(bb_Shader_* shader,
                                      const float* view,
                                      const float* proj,
                                      const float* cam_pos) {
  struct Item { bb_MeshEntity_* me; float alpha; float dist; bool translucent; };
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
    float alpha = me->alpha;
    if (me->fadeFar > me->fadeNear) {
      float f = (me->fadeFar - dist) / (me->fadeFar - me->fadeNear);
      alpha *= (f < 0.0f) ? 0.0f : (f > 1.0f ? 1.0f : f);
    }
    // Alpha 0 wird laut Doku gar nicht gezeichnet - und bleibt trotzdem fuer
    // Kollisionen vorhanden, anders als HideEntity.
    if (alpha <= 0.0f) continue;

    items.push_back({ me, alpha, dist, bb_ent_translucent_(me) || alpha < 1.0f });
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

    // Texturen der Entity auf die Kanaele legen.
    const bool tex_alpha = bb_texture_bind_(shader, me->tex);

    // ---- Blending ----
    // Am Original gemessen: 1 = Alpha (Vorgabe), 2 = Multiply, 3 = Add. Der
    // eigene Roadmap-Entwurf hatte 2 und 3 vertauscht.
    const bool want_blend = it.translucent || tex_alpha;
    if (want_blend != blend_on) {
      blend_on = want_blend;
      if (want_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
      blend_mode = 0;
    }
    if (want_blend && me->blend != blend_mode) {
      blend_mode = me->blend;
      switch (blend_mode) {
        case 2:  glBlendFunc(GL_DST_COLOR, GL_ZERO);            break;
        case 3:  glBlendFunc(GL_SRC_ALPHA, GL_ONE);             break;
        default: glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      }
    }

    // ---- Rueckseitenentfernung ----
    // Gemessen: das Original entfernt Rueckseiten (die Kamera im Wuerfel
    // sieht den Hintergrund), EntityFX 16 schaltet das ab.
    const bool want_cull = (me->fx & 16) == 0;
    if (want_cull != cull_on) {
      cull_on = want_cull;
      if (want_cull) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    }

    // ---- Z-Puffer ----
    // Laut Doku schaltet eine Ordnung ungleich 0 das Z-Buffering ab.
    const bool want_depth = (me->order == 0);
    if (want_depth != depth_on) {
      depth_on = want_depth;
      if (want_depth) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    }

    float color[4] = { me->colR / 255.0f, me->colG / 255.0f,
                       me->colB / 255.0f, it.alpha };
    bb_shader_uniform_i(shader, "u_fx",        me->fx);
    bb_shader_uniform_f(shader, "u_shininess", me->shininess);

    for (auto& surf : me->surfaces) {
      bb_mesh_draw_(&surf, shader, mvp, me->world, color, nullptr);
      bb_tris_rendered_ += surf.triCount;
    }
  }

  if (blend_on)  glDisable(GL_BLEND);
  if (!cull_on)  glEnable(GL_CULL_FACE);
  if (!depth_on) glEnable(GL_DEPTH_TEST);
}

#endif // BB_MESH_H
