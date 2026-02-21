#define NOMINMAX
#include "api.h"
#include "bb_globals.h"
#include "bb_math.h"
#include <algorithm>
#include <cmath>
#include <limits>

// Ray structure for internal use
struct Ray {
  float ox, oy, oz; // Origin
  float dx, dy, dz; // Direction (normalized)
  float length;     // Max length
};

// Internal recursive pick function
static void recursive_pick(bb_entity *ent, const Ray &ray,
                           float &closest_dist) {
  if (!ent)
    return;

  // Only check entities with pick mode set
  if (ent->pickMode > 0) {
    // 1. Transform ray to local space of entity
    bbMatrix world = get_world_matrix(ent);
    bbMatrix inv = bbMatrix::inverse(world);

    float lox = ray.ox, loy = ray.oy, loz = ray.oz;
    float ldx = ray.dx, ldy = ray.dy, ldz = ray.dz;

    // Transform origin
    inv.transform(lox, loy, loz);

    // Transform direction (as vector, ignore translation)
    // We can use transform() on (ox+dx) then subtract new origin, or
    // rotate_normal if scale is 1. Inverse matrix handles scale too.
    float tdx = ray.ox + ray.dx;
    float tdy = ray.oy + ray.dy;
    float tdz = ray.oz + ray.dz;
    inv.transform(tdx, tdy, tdz);
    ldx = tdx - lox;
    ldy = tdy - loy;
    ldz = tdz - loz;

    // Normalize local direction and adjust length
    float local_len_scale = sqrtf(ldx * ldx + ldy * ldy + ldz * ldz);

    // Avoid division by zero
    if (local_len_scale > 0.000001f) {
      ldx /= local_len_scale;
      ldy /= local_len_scale;
      ldz /= local_len_scale;
    }

    // Sphere Picking (Mode 1)
    if (ent->pickMode == 1 ||
        ent->pickMode ==
            2) { // Treating Mesh as sphere for now if Mode 2 unimplemented
      // Ray-Sphere Intersection
      // Sphere at (0,0,0) in local space with radiusX (assuming uniform for
      // now)
      float radius = ent->radiusX;

      // Vector from ray origin to sphere center (0,0,0)
      // sphere_center - ray_origin = (0,0,0) - (lox,loy,loz) = (-lox, -loy,
      // -loz)
      float cx = -lox;
      float cy = -loy;
      float cz = -loz;

      float t =
          cx * ldx + cy * ldy + cz * ldz; // Project sphere center onto ray

      // Closest point on ray line to center
      float px = lox + ldx * t;
      float py = loy + ldy * t;
      float pz = loz + ldz * t;

      float dist_sq = px * px + py * py + pz * pz;

      if (dist_sq <= radius * radius) {
        // Intersection
        float half_chord = sqrtf(radius * radius - dist_sq);
        float t_hit = t - half_chord; // First hit point

        if (t_hit < 0)
          t_hit = t + half_chord; // Inside or behind?

        if (t_hit > 0.0001f) {
          // Check if within ray length (in local scale)
          // Ray length in local units is ray.length * local_len_scale?
          // No, logic check:
          // Global Ray Length = ray.length
          // We traversed t_hit units in LOCAL space directions.
          // How much is that in GLOBAL units?
          // We need to transform the hit point back to world to check distance.

          float hx = lox + ldx * t_hit;
          float hy = loy + ldy * t_hit;
          float hz = loz + ldz * t_hit;

          bbMatrix mat = get_world_matrix(ent);
          mat.transform(hx, hy, hz);

          // Distance from global ray origin
          float dx = hx - ray.ox;
          float dy = hy - ray.oy;
          float dz = hz - ray.oz;
          float dist_global = sqrtf(dx * dx + dy * dy + dz * dz);

          if (dist_global < closest_dist && dist_global <= ray.length) {
            closest_dist = dist_global;

            // Register Hit
            bb_int handle = 0;
            for (auto const &[h, e] : g_entities) {
              if (e == ent) {
                handle = h;
                break;
              }
            }
            g_pickedEntity = handle;
            g_pickedX = hx;
            g_pickedY = hy;
            g_pickedZ = hz;
            g_pickedTime = dist_global;

            // Calculate Normal (Sphere: HitPoint - Center (0,0,0)) -> Rotate to
            // World
            float nx = lox + ldx * t_hit; // Center is 0,0,0
            float ny = loy + ldy * t_hit;
            float nz = loz + ldz * t_hit;
            mat.rotate_normal(nx, ny, nz);
            g_pickedNX = nx;
            g_pickedNY = ny;
            g_pickedNZ = nz;

            g_pickedSurface = 0;
            g_pickedTriangle = -1;
          }
        }
      }
    }
    // TODO: Mode 2 (Mesh Picking) - Iterate triangles
  }

  // Recurse
  for (auto child : ent->children) {
    recursive_pick(child, ray, closest_dist);
  }
}

// =============================================================================
// Commands
// =============================================================================

void entitypickmode(bb_int ent_handle, bb_int mode, bb_int obs) {
  bb_entity *ent = lookup_entity(ent_handle);
  if (ent) {
    ent->pickMode = (int)mode;
    ent->pickObs = (int)obs;
  }
}

void entityradius(bb_int ent_handle, bb_float rx, bb_float ry) {
  bb_entity *ent = lookup_entity(ent_handle);
  if (ent) {
    ent->radiusX = (float)rx;
    ent->radiusY = (ry == 0.0f) ? (float)rx : (float)ry; // If ry=0, use rx
  }
}

static void entitytype_recursive(bb_entity *ent, int type) {
  if (!ent)
    return;
  ent->collisionType = type;
  for (auto child : ent->children) {
    entitytype_recursive(child, type);
  }
}

void entitytype(bb_int ent_handle, bb_int type, bb_int recursive) {
  bb_entity *ent = lookup_entity(ent_handle);
  if (!ent)
    return;
  if (recursive) {
    entitytype_recursive(ent, (int)type);
  } else {
    ent->collisionType = (int)type;
  }
}

void entitybox(bb_int ent_handle, bb_float x, bb_float y, bb_float z,
               bb_float w, bb_float h, bb_float d) {
  bb_entity *ent = lookup_entity(ent_handle);
  if (ent) {
    ent->boxX = (float)x;
    ent->boxY = (float)y;
    ent->boxZ = (float)z;
    ent->boxW = (float)w;
    ent->boxH = (float)h;
    ent->boxD = (float)d;
  }
}

// Helper: Check if an entity is an ancestor of another
static bool is_ancestor_of(bb_entity *pot_ancestor, bb_entity *ent) {
  if (!pot_ancestor || !ent)
    return false;
  bb_entity *curr = ent->parent;
  while (curr) {
    if (curr == pot_ancestor)
      return true;
    curr = curr->parent;
  }
  return false;
}

// Helper: Closest point on triangle to a point p
static void closest_point_on_triangle(float px, float py, float pz, float v0x,
                                      float v0y, float v0z, float v1x,
                                      float v1y, float v1z, float v2x,
                                      float v2y, float v2z, float &cx,
                                      float &cy, float &cz) {
  float abX = v1x - v0x, abY = v1y - v0y, abZ = v1z - v0z;
  float acX = v2x - v0x, acY = v2y - v0y, acZ = v2z - v0z;
  float apX = px - v0x, apY = py - v0y, apZ = pz - v0z;

  float d1 = abX * apX + abY * apY + abZ * apZ;
  float d2 = acX * apX + acY * apY + acZ * apZ;
  if (d1 <= 0.0f && d2 <= 0.0f) {
    cx = v0x;
    cy = v0y;
    cz = v0z;
    return;
  }

  float bpX = px - v1x, bpY = py - v1y, bpZ = pz - v1z;
  float d3 = abX * bpX + abY * bpY + abZ * bpZ;
  float d4 = acX * bpX + acY * bpY + acZ * bpZ;
  if (d3 >= 0.0f && d4 <= d3) {
    cx = v1x;
    cy = v1y;
    cz = v1z;
    return;
  }

  float vc = d1 * d4 - d3 * d2;
  if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
    float v = d1 / (d1 - d3);
    cx = v0x + v * abX;
    cy = v0y + v * abY;
    cz = v0z + v * abZ;
    return;
  }

  float cpX = px - v2x, cpY = py - v2y, cpZ = pz - v2z;
  float d5 = abX * cpX + abY * cpY + abZ * cpZ;
  float d6 = acX * cpX + acY * cpY + acZ * cpZ;
  if (d6 >= 0.0f && d5 <= d6) {
    cx = v2x;
    cy = v2y;
    cz = v2z;
    return;
  }

  float vb = d5 * d2 - d1 * d6;
  if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
    float w = d2 / (d2 - d6);
    cx = v0x + w * acX;
    cy = v0y + w * acY;
    cz = v0z + w * acZ;
    return;
  }

  float va = d3 * d6 - d5 * d4;
  if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
    float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
    cx = v1x + w * (v2x - v1x);
    cy = v1y + w * (v2y - v1y);
    cz = v1z + w * (v2z - v1z);
    return;
  }

  float denom = 1.0f / (va + vb + vc);
  float v = vb * denom;
  float w = vc * denom;
  cx = v0x + abX * v + acX * w;
  cy = v0y + abY * v + acY * w;
  cz = v0z + abZ * v + acZ * w;
}

void resetentity(bb_int ent_handle) {
  bb_entity *ent = lookup_entity(ent_handle);
  if (ent) {
    bbMatrix world = get_world_matrix(ent);
    ent->prevX = world.m[12];
    ent->prevY = world.m[13];
    ent->prevZ = world.m[14];
  }
}

void collisions(bb_int src_type, bb_int dest_type, bb_int method,
                bb_int response) {
  // Check if this rule already exists
  for (auto &def : g_collisionDefs) {
    if (def.src_type == (int)src_type && def.dest_type == (int)dest_type) {
      def.method = (int)method;
      def.response = (int)response;
      return;
    }
  }
  CollisionDef def;
  def.src_type = (int)src_type;
  def.dest_type = (int)dest_type;
  def.method = (int)method;
  def.response = (int)response;
  g_collisionDefs.push_back(def);
}

void updateworld(bb_float tween) {
  // 1. Collect all entities that participate in collisions
  struct Collidable {
    bb_entity *ent;
    float wx, wy, wz; // Current World Pos
    float radius;
  };
  std::vector<Collidable> collidables;

  for (auto const &[handle, ent] : g_entities) {
    ent->collisionResults.clear();
    if (ent->collisionType > 0) {
      bbMatrix world = get_world_matrix(ent);
      collidables.push_back(
          {ent, world.m[12], world.m[13], world.m[14], ent->radiusX});
    }
  }

  // 2. Process collisions based on defined rules
  // Multiple iterations to handle corner cases
  for (int iter = 0; iter < 3; ++iter) {
    bool shifted = false;

    // For each src entity
    for (auto &src : collidables) {
      for (auto const &def : g_collisionDefs) {
        if (src.ent->collisionType != def.src_type)
          continue;

        // Find entities of dest_type
        for (auto const &[handle, dest_ent] : g_entities) {
          if (dest_ent->collisionType != def.dest_type || src.ent == dest_ent)
            continue;

          // Exclude hierarchy (parent-child) collisions - Standard Blitz3D
          // behavior
          if (is_ancestor_of(src.ent, dest_ent) ||
              is_ancestor_of(dest_ent, src.ent))
            continue;

          float nx, ny, nz, overlap;
          bool collision = false;

          // Sphere-Sphere (Method 1)
          if (def.method == 1 && dest_ent->type != ENTITY_MESH) {
            bbMatrix destWorld = get_world_matrix(dest_ent);
            float dx = src.wx - destWorld.m[12];
            float dy = src.wy - destWorld.m[13];
            float dz = src.wz - destWorld.m[14];
            float d_sq = dx * dx + dy * dy + dz * dz;
            float r_sum = src.radius + dest_ent->radiusX;

            if (d_sq < r_sum * r_sum && d_sq > 0.000001f) {
              float dist = sqrtf(d_sq);
              overlap = r_sum - dist;
              nx = dx / dist;
              ny = dy / dist;
              nz = dz / dist;
              collision = true;
            }
          }
          // Sphere-Mesh (Method 2)
          else if (def.method == 2 && dest_ent->type == ENTITY_MESH) {
            bb_mesh *mesh = (bb_mesh *)dest_ent;
            bbMatrix meshWorld = get_world_matrix(mesh);
            bbMatrix invMeshWorld = bbMatrix::inverse(meshWorld);

            float lpx = src.wx, lpy = src.wy, lpz = src.wz;
            invMeshWorld.transform(lpx, lpy, lpz);

            // Rough scale for local radius
            float s =
                std::max({meshWorld.m[0], meshWorld.m[5], meshWorld.m[10]});
            float lr = src.radius / s;

            if (iter == 0)
              mesh->recalculate_aabb();
            if (lpx + lr < mesh->minX || lpx - lr > mesh->maxX ||
                lpy + lr < mesh->minY || lpy - lr > mesh->maxY ||
                lpz + lr < mesh->minZ || lpz - lr > mesh->maxZ)
              continue;

            for (auto surf : mesh->surfaces) {
              if (lpx + lr < surf->minX || lpx - lr > surf->maxX ||
                  lpy + lr < surf->minY || lpy - lr > surf->maxY ||
                  lpz + lr < surf->minZ || lpz - lr > surf->maxZ)
                continue;

              for (const auto &tri : surf->triangles) {
                const auto &v0 = surf->vertices[tri.v0];
                const auto &v1 = surf->vertices[tri.v1];
                const auto &v2 = surf->vertices[tri.v2];

                float cx, cy, cz;
                closest_point_on_triangle(lpx, lpy, lpz, v0.x, v0.y, v0.z, v1.x,
                                          v1.y, v1.z, v2.x, v2.y, v2.z, cx, cy,
                                          cz);

                float ldx = lpx - cx, ldy = lpy - cy, ldz = lpz - cz;
                float ld_sq = ldx * ldx + ldy * ldy + ldz * ldz;

                if (ld_sq < lr * lr && ld_sq > 0.000001f) {
                  float ldist = sqrtf(ld_sq);
                  overlap = (lr - ldist) * s;
                  nx = ldx / ldist;
                  ny = ldy / ldist;
                  nz = ldz / ldist;
                  meshWorld.rotate_normal(nx, ny, nz);
                  float n_mag = sqrtf(nx * nx + ny * ny + nz * nz);
                  nx /= n_mag;
                  ny /= n_mag;
                  nz /= n_mag;
                  collision = true;

                  // Record collision
                  {
                    CollisionResult res;
                    res.nx = nx;
                    res.ny = ny;
                    res.nz = nz;
                    res.x = cx;
                    res.y = cy;
                    res.z = cz;
                    meshWorld.transform(res.x, res.y, res.z); // Local to World
                    res.entity = mesh;
                    res.surface = surf;
                    res.triangle = -1;
                    // Find triangle index
                    for (int i = 0; i < (int)surf->triangles.size(); ++i) {
                      if (&surf->triangles[i] == &tri) {
                        res.triangle = i;
                        break;
                      }
                    }
                    src.ent->collisionResults.push_back(res);
                  }

                  // For meshes, we apply response per triangle to allow sliding
                  // out of complex areas
                  goto apply_response;
                }
              }
            }
          }

          if (collision) {
            // Record sphere-sphere collision
            if (def.method == 1 && dest_ent->type != ENTITY_MESH) {
              CollisionResult res;
              res.nx = nx;
              res.ny = ny;
              res.nz = nz;
              res.x = dest_ent->x + nx * dest_ent->radiusX; // Approximate
              res.y = dest_ent->y + ny * dest_ent->radiusX;
              res.z = dest_ent->z + nz * dest_ent->radiusX;
              res.entity = dest_ent;
              res.surface = nullptr;
              res.triangle = -1;
              src.ent->collisionResults.push_back(res);
            }

          apply_response:
            float moveX, moveY, moveZ;
            if (def.response == 1) { // Stop
              moveX = src.ent->prevX - src.wx;
              moveY = src.ent->prevY - src.wy;
              moveZ = src.ent->prevZ - src.wz;
            } else { // Slide (Response 2)
              float pushX = nx * overlap, pushY = ny * overlap,
                    pushZ = nz * overlap;
              float vx = src.wx - src.ent->prevX, vy = src.wy - src.ent->prevY,
                    vz = src.wz - src.ent->prevZ;
              float dot = vx * nx + vy * ny + vz * nz;
              float slideX = 0, slideY = 0, slideZ = 0;
              if (dot < 0) {
                slideX = nx * -dot;
                slideY = ny * -dot;
                slideZ = nz * -dot;
              }
              moveX = pushX + slideX;
              moveY = pushY + slideY;
              moveZ = pushZ + slideZ;
            }

            if (src.ent->parent && src.ent->parent != g_worldRoot) {
              bbMatrix invParent =
                  bbMatrix::inverse(get_world_matrix(src.ent->parent));
              invParent.rotate_normal(moveX, moveY, moveZ);
            }
            src.ent->x += moveX;
            src.ent->y += moveY;
            src.ent->z += moveZ;
            bbMatrix newWorld = get_world_matrix(src.ent);
            src.wx = newWorld.m[12];
            src.wy = newWorld.m[13];
            src.wz = newWorld.m[14];
            shifted = true;
          }
        }
      }
    }
    if (!shifted)
      break;
  }

  // 3. Update prevX/Y/Z for the next frame
  for (auto const &[handle, ent] : g_entities) {
    if (ent->collisionType > 0) {
      bbMatrix world = get_world_matrix(ent);
      ent->prevX = world.m[12];
      ent->prevY = world.m[13];
      ent->prevZ = world.m[14];
    }
  }
}

bb_int entitypick(bb_int ent_handle, bb_float range) {
  bb_entity *ent = lookup_entity(ent_handle);
  if (!ent)
    return 0;

  bbMatrix mat = get_world_matrix(ent);
  // Forward vector from matrix (third column)
  // m[0]..m[2] = right, m[4]..m[6] = up, m[8]..m[10] = forward?
  // bbMatrix rot = RZ * RY * RX.
  // Standard OpenGL: -Z is forward.
  // bb_math.h:
  // Rx=... Rz=...

  float ox = ent->x, oy = ent->y, oz = ent->z;
  mat.transform(
      ox, oy,
      oz); // Actually, get_world_matrix includes translation in m[12-14]
  ox = mat.m[12];
  oy = mat.m[13];
  oz = mat.m[14];

  // In B3D, Forward is +Z. In OGL it's -Z.
  // Our View Matrix does glScalef(1,1,-1).
  // Let's assume Entity Forward is the Z axis of its matrix.
  float dx = mat.m[8];
  float dy = mat.m[9];
  float dz = mat.m[10];
  // Normalize
  float mag = sqrtf(dx * dx + dy * dy + dz * dz);
  if (mag > 0) {
    dx /= mag;
    dy /= mag;
    dz /= mag;
  }

  return linepick(ox, oy, oz, dx * range, dy * range, dz * range, 0);
}

bb_int linepick(bb_float x, bb_float y, bb_float z, bb_float dx, bb_float dy,
                bb_float dz, bb_float radius) {
  float len = sqrtf(dx * dx + dy * dy + dz * dz);
  if (len < 0.0001f)
    return 0;

  Ray ray;
  ray.ox = x;
  ray.oy = y;
  ray.oz = z;
  ray.dx = dx / len;
  ray.dy = dy / len;
  ray.dz = dz / len;
  ray.length = len;

  g_pickedEntity = 0;
  float closest = std::numeric_limits<float>::max();

  ensure_world_root();
  recursive_pick(g_worldRoot, ray, closest);

  return g_pickedEntity;
}

bb_int camerapick(bb_int cam_handle, bb_float screen_x, bb_float screen_y) {
  bb_camera *cam = (bb_camera *)lookup_entity(cam_handle);
  if (!cam || cam->type != ENTITY_CAMERA)
    return 0;

  // Unproject
  GLint viewport[4];
  GLdouble modelview[16];
  GLdouble projection[16];

  // We need the ACTUAL current matrices.
  // Since we are likely calling this inside current GL context flow, BUT
  // RenderWorld sets up matrices. If we are outside RenderWorld, matrices might
  // be identity. We should manually reconstruct the Camera matrices.

  // Viewport
  int vx = cam->vX, vy = cam->vY, vw = cam->vW, vH = cam->vH;
  if (vw == 0)
    vw = g_width;
  if (vH == 0)
    vH = g_height;
  viewport[0] = vx;
  viewport[1] = vy;
  viewport[2] = vw;
  viewport[3] = vH;

  // Projection
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  float aspect = (float)vw / (float)vH;
  float zoom = cam->zoom;
  if (zoom < 0.0001f)
    zoom = 0.0001f;
  float fovX = 2.0f * atan(1.0f / zoom);
  float fovY = 2.0f * atan(tan(fovX / 2.0f) / aspect);
  float fovDeg = fovY * (180.0f / 3.14159265f);
  gluPerspective(fovDeg, aspect, cam->nearPlane, cam->farPlane);
  glGetDoublev(GL_PROJECTION_MATRIX, projection);
  glPopMatrix();

  // ModelView
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glScalef(1.0f, 1.0f, -1.0f);
  glRotatef(-cam->roll, 0, 0, 1);
  glRotatef(-cam->pitch, 1, 0, 0);
  glRotatef(-cam->yaw, 0, 1, 0);
  glTranslatef(-cam->x, -cam->y, -cam->z);
  glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
  glPopMatrix();

  GLdouble nearX, nearY, nearZ;
  GLdouble farX, farY, farZ;

  // Invert Y for OpenGL (bottom-up) vs Screen (top-down)
  // SDL relative mouse is top-down.
  gluUnProject(screen_x, (float)vH - screen_y, 0.0, modelview, projection,
               viewport, &nearX, &nearY, &nearZ);
  gluUnProject(screen_x, (float)vH - screen_y, 1.0, modelview, projection,
               viewport, &farX, &farY, &farZ);

  float dx = (float)(farX - nearX);
  float dy = (float)(farY - nearY);
  float dz = (float)(farZ - nearZ);

  return linepick((float)nearX, (float)nearY, (float)nearZ, dx, dy, dz, 0);
}

bb_int pickedentity() { return g_pickedEntity; }
bb_float pickedx() { return g_pickedX; }
bb_float pickedy() { return g_pickedY; }
bb_float pickedz() { return g_pickedZ; }
bb_float pickednx() { return g_pickedNX; }
bb_float pickedny() { return g_pickedNY; }
bb_float pickednz() { return g_pickedNZ; }
bb_float pickedtime() { return g_pickedTime; }
bb_int pickedtriangle() { return g_pickedTriangle; }
bb_int pickedsurface() { return g_pickedSurface; }
bb_int countcollisions(bb_int ent_handle) {
  bb_entity *ent = lookup_entity(ent_handle);
  return ent ? (bb_int)ent->collisionResults.size() : 0;
}

bb_float collisionx(bb_int ent_handle, bb_int index) {
  bb_entity *ent = lookup_entity(ent_handle);
  if (!ent || index < 1 || index > (bb_int)ent->collisionResults.size())
    return 0.0f;
  return ent->collisionResults[index - 1].x;
}

bb_float collisiony(bb_int ent_handle, bb_int index) {
  bb_entity *ent = lookup_entity(ent_handle);
  if (!ent || index < 1 || index > (bb_int)ent->collisionResults.size())
    return 0.0f;
  return ent->collisionResults[index - 1].y;
}

bb_float collisionz(bb_int ent_handle, bb_int index) {
  bb_entity *ent = lookup_entity(ent_handle);
  if (!ent || index < 1 || index > (bb_int)ent->collisionResults.size())
    return 0.0f;
  return ent->collisionResults[index - 1].z;
}

bb_float collisionnx(bb_int ent_handle, bb_int index) {
  bb_entity *ent = lookup_entity(ent_handle);
  if (!ent || index < 1 || index > (bb_int)ent->collisionResults.size())
    return 0.0f;
  return ent->collisionResults[index - 1].nx;
}

bb_float collisionny(bb_int ent_handle, bb_int index) {
  bb_entity *ent = lookup_entity(ent_handle);
  if (!ent || index < 1 || index > (bb_int)ent->collisionResults.size())
    return 0.0f;
  return ent->collisionResults[index - 1].ny;
}

bb_float collisionnz(bb_int ent_handle, bb_int index) {
  bb_entity *ent = lookup_entity(ent_handle);
  if (!ent || index < 1 || index > (bb_int)ent->collisionResults.size())
    return 0.0f;
  return ent->collisionResults[index - 1].nz;
}

bb_int collisionentity(bb_int ent_handle, bb_int index) {
  bb_entity *ent = lookup_entity(ent_handle);
  if (!ent || index < 1 || index > (bb_int)ent->collisionResults.size())
    return 0;
  bb_entity *other = ent->collisionResults[index - 1].entity;
  for (auto const &[handle, e] : g_entities) {
    if (e == other)
      return handle;
  }
  return 0;
}

bb_int collisionsurface(bb_int ent_handle, bb_int index) {
  bb_entity *ent = lookup_entity(ent_handle);
  if (!ent || index < 1 || index > (bb_int)ent->collisionResults.size())
    return 0;
  bb_surface *surf = ent->collisionResults[index - 1].surface;
  if (!surf)
    return 0;
  for (auto const &[handle, s] : g_surfaces) {
    if (s == surf)
      return handle;
  }
  return 0;
}

bb_int collisiontriangle(bb_int ent_handle, bb_int index) {
  bb_entity *ent = lookup_entity(ent_handle);
  if (!ent || index < 1 || index > (bb_int)ent->collisionResults.size())
    return -1;
  return (bb_int)ent->collisionResults[index - 1].triangle;
}
