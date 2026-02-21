// =============================================================================
// bb_mesh.cpp — Mesh/Surface commands (CreateCube, CreateSphere).
// =============================================================================

#define NOMINMAX
#include "bb_globals.h"
#include "bb_math.h"

#define _USE_MATH_DEFINES
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <cmath>

// Helper: attach entity to parent or world root
static void attach_to_parent(bb_entity *entity, bb_int parent_handle) {
  ensure_world_root();
  bb_entity *parent = lookup_entity(parent_handle);
  if (parent)
    parent->addChild(entity);
  else
    g_worldRoot->addChild(entity);
}

bb_int createmesh(bb_int parent_handle) {
  bb_mesh *mesh = new bb_mesh();
  attach_to_parent(mesh, parent_handle);
  return register_entity(mesh);
}

bb_int createsurface(bb_int mesh_handle) {
  bb_mesh *mesh = (bb_mesh *)lookup_entity(mesh_handle);
  if (!mesh || mesh->type != ENTITY_MESH)
    return 0;
  bb_surface *surf = new bb_surface();
  mesh->surfaces.push_back(surf);
  return register_surface(surf);
}

bb_int addvertex(bb_int surf_handle, bb_float x, bb_float y, bb_float z,
                 bb_float u, bb_float v) {
  bb_surface *surf = lookup_surface(surf_handle);
  if (!surf)
    return -1;
  bb_surface::Vertex vert;
  vert.x = (float)x;
  vert.y = (float)y;
  vert.z = (float)z;
  vert.nx = 0;
  vert.ny = 0;
  vert.nz = -1;
  vert.u = (float)u;
  vert.v = (float)v;
  surf->vertices.push_back(vert);
  surf->invalidate();
  return (bb_int)surf->vertices.size() - 1;
}

bb_int addtriangle(bb_int surf_handle, bb_int v0, bb_int v1, bb_int v2) {
  bb_surface *surf = lookup_surface(surf_handle);
  if (!surf)
    return -1;
  surf->triangles.push_back({(int)v0, (int)v1, (int)v2});
  surf->invalidate();
  return (bb_int)surf->triangles.size() - 1;
}

bb_int countsurfaces(bb_int mesh_handle) {
  bb_mesh *mesh = (bb_mesh *)lookup_entity(mesh_handle);
  if (!mesh || mesh->type != ENTITY_MESH)
    return 0;
  return (bb_int)mesh->surfaces.size();
}

bb_int getsurface(bb_int mesh_handle, bb_int index) {
  bb_mesh *mesh = (bb_mesh *)lookup_entity(mesh_handle);
  if (!mesh || mesh->type != ENTITY_MESH)
    return 0;
  if (index < 1 || index > (bb_int)mesh->surfaces.size())
    return 0;
  bb_surface *surf = mesh->surfaces[index - 1];
  for (auto const &[handle, s] : g_surfaces) {
    if (s == surf)
      return handle;
  }
  return 0;
}

void updatenormals(bb_int mesh_handle) {
  bb_mesh *mesh = (bb_mesh *)lookup_entity(mesh_handle);
  if (!mesh || mesh->type != ENTITY_MESH)
    return;

  for (auto surf : mesh->surfaces) {
    // Reset normals
    for (auto &v : surf->vertices) {
      v.nx = 0;
      v.ny = 0;
      v.nz = 0;
    }
    // Sum face normals
    for (const auto &t : surf->triangles) {
      if (t.v0 >= surf->vertices.size() || t.v1 >= surf->vertices.size() ||
          t.v2 >= surf->vertices.size())
        continue;
      auto &v0 = surf->vertices[t.v0];
      auto &v1 = surf->vertices[t.v1];
      auto &v2 = surf->vertices[t.v2];

      float ux = v1.x - v0.x, uy = v1.y - v0.y, uz = v1.z - v0.z;
      float vx = v2.x - v0.x, vy = v2.y - v0.y, vz = v2.z - v0.z;
      float nx = uy * vz - uz * vy;
      float ny = uz * vx - ux * vz;
      float nz = ux * vy - uy * vx;

      v0.nx += nx;
      v0.ny += ny;
      v0.nz += nz;
      v1.nx += nx;
      v1.ny += ny;
      v1.nz += nz;
      v2.nx += nx;
      v2.ny += ny;
      v2.nz += nz;
    }
    // Normalize
    for (auto &v : surf->vertices) {
      float mag = sqrtf(v.nx * v.nx + v.ny * v.ny + v.nz * v.nz);
      if (mag > 0.0001f) {
        v.nx /= mag;
        v.ny /= mag;
        v.nz /= mag;
      }
    }
    surf->invalidate();
  }
}

bb_int createcube(bb_int parent_handle) {
  bb_int mesh_handle = createmesh(parent_handle);
  bb_int surf_handle = createsurface(mesh_handle);

  struct FaceDef {
    float nx, ny, nz;
    float positions[4][3];
    float uvs[4][2];
  };

  const FaceDef faces[6] = {
      {0,
       0,
       -1,
       {{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1}},
       {{0, 0}, {1, 0}, {1, 1}, {0, 1}}},
      {0,
       0,
       1,
       {{1, -1, 1}, {-1, -1, 1}, {-1, 1, 1}, {1, 1, 1}},
       {{0, 0}, {1, 0}, {1, 1}, {0, 1}}},
      {-1,
       0,
       0,
       {{-1, -1, 1}, {-1, -1, -1}, {-1, 1, -1}, {-1, 1, 1}},
       {{0, 0}, {1, 0}, {1, 1}, {0, 1}}},
      {1,
       0,
       0,
       {{1, -1, -1}, {1, -1, 1}, {1, 1, 1}, {1, 1, -1}},
       {{0, 0}, {1, 0}, {1, 1}, {0, 1}}},
      {0,
       1,
       0,
       {{-1, 1, -1}, {1, 1, -1}, {1, 1, 1}, {-1, 1, 1}},
       {{0, 0}, {1, 0}, {1, 1}, {0, 1}}},
      {0,
       -1,
       0,
       {{-1, -1, 1}, {1, -1, 1}, {1, -1, -1}, {-1, -1, -1}},
       {{0, 0}, {1, 0}, {1, 1}, {0, 1}}},
  };

  for (int f = 0; f < 6; ++f) {
    int v0 = (int)addvertex(surf_handle, faces[f].positions[0][0],
                            faces[f].positions[0][1], faces[f].positions[0][2],
                            faces[f].uvs[0][0], faces[f].uvs[0][1]);
    int v1 = (int)addvertex(surf_handle, faces[f].positions[1][0],
                            faces[f].positions[1][1], faces[f].positions[1][2],
                            faces[f].uvs[1][0], faces[f].uvs[1][1]);
    int v2 = (int)addvertex(surf_handle, faces[f].positions[2][0],
                            faces[f].positions[2][1], faces[f].positions[2][2],
                            faces[f].uvs[2][0], faces[f].uvs[2][1]);
    int v3 = (int)addvertex(surf_handle, faces[f].positions[3][0],
                            faces[f].positions[3][1], faces[f].positions[3][2],
                            faces[f].uvs[3][0], faces[f].uvs[3][1]);

    // Apply normals manually for cube (since they are flat)
    bb_surface *surf = lookup_surface(surf_handle);
    surf->vertices[v0].nx = surf->vertices[v1].nx = surf->vertices[v2].nx =
        surf->vertices[v3].nx = faces[f].nx;
    surf->vertices[v0].ny = surf->vertices[v1].ny = surf->vertices[v2].ny =
        surf->vertices[v3].ny = faces[f].ny;
    surf->vertices[v0].nz = surf->vertices[v1].nz = surf->vertices[v2].nz =
        surf->vertices[v3].nz = faces[f].nz;

    addtriangle(surf_handle, v0, v1, v2);
    addtriangle(surf_handle, v2, v3, v0);
  }
  return mesh_handle;
}

bb_int createsphere(bb_int segs, bb_int parent_handle) {
  bb_int mesh_handle = createmesh(parent_handle);
  bb_int surf_handle = createsurface(mesh_handle);

  int stacks = std::max((int)segs, 4);
  int slices = std::max(stacks * 2, 8);

  for (int i = 0; i <= stacks; ++i) {
    float phi = (float)M_PI * i / stacks;
    float y = cosf(phi);
    float r = sinf(phi);
    for (int j = 0; j <= slices; ++j) {
      float theta = 2.0f * (float)M_PI * j / slices;
      float x = r * cosf(theta);
      float z = r * sinf(theta);
      int v = (int)addvertex(surf_handle, x, y, z, (float)j / slices,
                             (float)i / stacks);
      bb_surface *surf = lookup_surface(surf_handle);
      surf->vertices[v].nx = x;
      surf->vertices[v].ny = y;
      surf->vertices[v].nz = z;
    }
  }

  for (int i = 0; i < stacks; ++i) {
    for (int j = 0; j < slices; ++j) {
      int a = i * (slices + 1) + j;
      int b = a + slices + 1;
      addtriangle(surf_handle, a, b, a + 1);
      addtriangle(surf_handle, a + 1, b, b + 1);
    }
  }
  return mesh_handle;
}

void rotatemesh(bb_int mesh_handle, bb_float p, bb_float y, bb_float r) {
  bb_mesh *mesh = (bb_mesh *)lookup_entity(mesh_handle);
  if (!mesh || mesh->type != ENTITY_MESH)
    return;
  bbMatrix rot = bbMatrix::rotation((float)p, (float)y, (float)r);
  for (auto surf : mesh->surfaces) {
    for (auto &v : surf->vertices) {
      rot.transform(v.x, v.y, v.z);
      rot.rotate_normal(v.nx, v.ny, v.nz);
    }
    surf->invalidate();
  }
}

void scalemesh(bb_int mesh_handle, bb_float x, bb_float y, bb_float z) {
  bb_mesh *mesh = (bb_mesh *)lookup_entity(mesh_handle);
  if (!mesh || mesh->type != ENTITY_MESH)
    return;
  bbMatrix s = bbMatrix::scale((float)x, (float)y, (float)z);
  for (auto surf : mesh->surfaces) {
    for (auto &v : surf->vertices) {
      s.transform(v.x, v.y, v.z);
    }
    surf->invalidate();
  }
  if (fabsf((float)(x - y)) > 0.001f || fabsf((float)(x - z)) > 0.001f) {
    updatenormals(mesh_handle);
  }
}

void translatemesh(bb_int mesh_handle, bb_float x, bb_float y, bb_float z) {
  bb_mesh *mesh = (bb_mesh *)lookup_entity(mesh_handle);
  if (!mesh || mesh->type != ENTITY_MESH)
    return;
  bbMatrix t = bbMatrix::translation((float)x, (float)y, (float)z);
  for (auto surf : mesh->surfaces) {
    for (auto &v : surf->vertices) {
      t.transform(v.x, v.y, v.z);
    }
    surf->invalidate();
  }
}

void fitmesh(bb_int mesh_handle, bb_float x, bb_float y, bb_float z, bb_float w,
             bb_float h, bb_float d, bb_int uniform) {
  bb_mesh *mesh = (bb_mesh *)lookup_entity(mesh_handle);
  if (!mesh || mesh->type != ENTITY_MESH)
    return;

  float minX = 1e10, minY = 1e10, minZ = 1e10;
  float maxX = -1e10, maxY = -1e10, maxZ = -1e10;
  bool found = false;

  for (auto surf : mesh->surfaces) {
    for (const auto &v : surf->vertices) {
      if (v.x < minX)
        minX = v.x;
      if (v.y < minY)
        minY = v.y;
      if (v.z < minZ)
        minZ = v.z;
      if (v.x > maxX)
        maxX = v.x;
      if (v.y > maxY)
        maxY = v.y;
      if (v.z > maxZ)
        maxZ = v.z;
      found = true;
    }
  }

  if (!found)
    return;

  float curW = maxX - minX;
  float curH = maxY - minY;
  float curD = maxZ - minZ;

  if (curW < 0.0001f)
    curW = 0.0001f;
  if (curH < 0.0001f)
    curH = 0.0001f;
  if (curD < 0.0001f)
    curD = 0.0001f;

  float sx = (float)w / curW;
  float sy = (float)h / curH;
  float sz = (float)d / curD;

  if (uniform) {
    sx = sy = sz = std::min({sx, sy, sz});
  }

  translatemesh(mesh_handle, -minX - curW * 0.5f, -minY - curH * 0.5f,
                -minZ - curD * 0.5f);
  scalemesh(mesh_handle, sx, sy, sz);
  translatemesh(mesh_handle, (float)x + (float)w * 0.5f,
                (float)y + (float)h * 0.5f, (float)z + (float)d * 0.5f);
}

bb_int createcylinder(bb_int segs, bb_int solid, bb_int parent_handle) {
  bb_int mesh_handle = createmesh(parent_handle);
  bb_int surf_handle = createsurface(mesh_handle);
  int slices = std::max((int)segs, 3);
  for (int i = 0; i <= slices; ++i) {
    float theta = 2.0f * (float)M_PI * i / slices;
    float x = cosf(theta);
    float z = sinf(theta);
    float u = (float)i / slices;
    int v0 = (int)addvertex(surf_handle, x, 1, z, u, 0);
    int v1 = (int)addvertex(surf_handle, x, -1, z, u, 1);
    bb_surface *surf = lookup_surface(surf_handle);
    surf->vertices[v0].nx = x;
    surf->vertices[v0].ny = 0;
    surf->vertices[v0].nz = z;
    surf->vertices[v1].nx = x;
    surf->vertices[v1].ny = 0;
    surf->vertices[v1].nz = z;
  }
  for (int i = 0; i < slices; ++i) {
    int v0 = i * 2;
    int v1 = i * 2 + 1;
    int v2 = (i + 1) * 2;
    int v3 = (i + 1) * 2 + 1;
    addtriangle(surf_handle, v0, v2, v1);
    addtriangle(surf_handle, v1, v2, v3);
  }
  if (solid) {
    int top_center = (int)addvertex(surf_handle, 0, 1, 0, 0.5f, 0.5f);
    bb_surface *surf = lookup_surface(surf_handle);
    surf->vertices[top_center].nx = 0;
    surf->vertices[top_center].ny = 1;
    surf->vertices[top_center].nz = 0;
    int top_start = (int)surf->vertices.size();
    for (int i = 0; i <= slices; ++i) {
      float theta = 2.0f * (float)M_PI * i / slices;
      int v =
          (int)addvertex(surf_handle, cosf(theta), 1, sinf(theta),
                         0.5f + 0.5f * cosf(theta), 0.5f + 0.5f * sinf(theta));
      surf = lookup_surface(surf_handle);
      surf->vertices[v].nx = 0;
      surf->vertices[v].ny = 1;
      surf->vertices[v].nz = 0;
    }
    for (int i = 0; i < slices; ++i) {
      addtriangle(surf_handle, top_center, top_start + i + 1, top_start + i);
    }
    int bot_center = (int)addvertex(surf_handle, 0, -1, 0, 0.5f, 0.5f);
    surf = lookup_surface(surf_handle);
    surf->vertices[bot_center].nx = 0;
    surf->vertices[bot_center].ny = -1;
    surf->vertices[bot_center].nz = 0;
    int bot_start = (int)surf->vertices.size();
    for (int i = 0; i <= slices; ++i) {
      float theta = 2.0f * (float)M_PI * i / slices;
      int v =
          (int)addvertex(surf_handle, cosf(theta), -1, sinf(theta),
                         0.5f + 0.5f * cosf(theta), 0.5f + 0.5f * sinf(theta));
      surf = lookup_surface(surf_handle);
      surf->vertices[v].nx = 0;
      surf->vertices[v].ny = -1;
      surf->vertices[v].nz = 0;
    }
    for (int i = 0; i < slices; ++i) {
      addtriangle(surf_handle, bot_center, bot_start + i, bot_start + i + 1);
    }
  }
  return mesh_handle;
}

bb_int createcone(bb_int segs, bb_int solid, bb_int parent_handle) {
  bb_int mesh_handle = createmesh(parent_handle);
  bb_int surf_handle = createsurface(mesh_handle);
  int slices = std::max((int)segs, 3);
  int tip = (int)addvertex(surf_handle, 0, 1, 0, 0.5f, 0);
  for (int i = 0; i <= slices; ++i) {
    float theta = 2.0f * (float)M_PI * i / slices;
    float x = cosf(theta);
    float z = sinf(theta);
    addvertex(surf_handle, x, -1, z, (float)i / slices, 1);
  }
  for (int i = 0; i < slices; ++i) {
    addtriangle(surf_handle, tip, i + 2, i + 1);
  }
  if (solid) {
    int bot_center = (int)addvertex(surf_handle, 0, -1, 0, 0.5f, 0.5f);
    bb_surface *surf = lookup_surface(surf_handle);
    surf->vertices[bot_center].ny = -1;
    int bot_start = (int)surf->vertices.size();
    for (int i = 0; i <= slices; ++i) {
      float theta = 2.0f * (float)M_PI * i / slices;
      int v =
          (int)addvertex(surf_handle, cosf(theta), -1, sinf(theta),
                         0.5f + 0.5f * cosf(theta), 0.5f + 0.5f * sinf(theta));
      surf = lookup_surface(surf_handle);
      surf->vertices[v].ny = -1;
    }
    for (int i = 0; i < slices; ++i) {
      addtriangle(surf_handle, bot_center, bot_start + i, bot_start + i + 1);
    }
  }
  updatenormals(mesh_handle);
  return mesh_handle;
}

bb_int createplane(bb_int segs, bb_int parent_handle) {
  bb_int mesh_handle = createmesh(parent_handle);
  bb_int surf_handle = createsurface(mesh_handle);

  int slices = std::max((int)segs, 1);
  float size = 1000.0f; // Large enough for a plane

  for (int i = 0; i <= slices; ++i) {
    float z = size * (0.5f - (float)i / slices);
    float v = (float)i;
    for (int j = 0; j <= slices; ++j) {
      float x = size * ((float)j / slices - 0.5f);
      float u = (float)j;
      int v_idx = (int)addvertex(surf_handle, x, 0, z, u, v);
      bb_surface *surf = lookup_surface(surf_handle);
      surf->vertices[v_idx].nx = 0;
      surf->vertices[v_idx].ny = 1;
      surf->vertices[v_idx].nz = 0;
    }
  }

  for (int i = 0; i < slices; ++i) {
    for (int j = 0; j < slices; ++j) {
      int v0 = i * (slices + 1) + j;
      int v1 = v0 + 1;
      int v2 = (i + 1) * (slices + 1) + j;
      int v3 = v2 + 1;
      addtriangle(surf_handle, v0, v1, v2);
      addtriangle(surf_handle, v1, v3, v2);
    }
  }

  return mesh_handle;
}
