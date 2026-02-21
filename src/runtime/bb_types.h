#pragma once
// =============================================================================
// bb_types.h — Entity type definitions for the BltzNxt runtime.
// =============================================================================

#include "api.h"

#include <algorithm>
#include <string>
#include <variant>
#include <vector>

// =============================================================================
// Entity Type Enum
// =============================================================================

enum EntityType { ENTITY_PIVOT, ENTITY_MESH, ENTITY_CAMERA, ENTITY_LIGHT };

struct bb_surface;

// =============================================================================
// Collision Result
// =============================================================================

struct CollisionResult {
  float x, y, z;       // Impact point
  float nx, ny, nz;    // Collision normal
  bb_entity *entity;   // Entity collided with
  bb_surface *surface; // Surface (if mesh)
  int triangle;        // Triangle index (if mesh)
};

// =============================================================================
// Base Entity
// =============================================================================

struct bb_entity {
  EntityType type;

  std::vector<CollisionResult> collisionResults;

  // Transform
  float x = 0, y = 0, z = 0;
  float pitch = 0, yaw = 0, roll = 0;
  float sx = 1, sy = 1, sz = 1;

  // Movement Tracking (for collisions)
  float prevX = 0, prevY = 0, prevZ = 0;

  // Entity color (used as material diffuse for meshes, light color for lights)
  float er = 1.0f, eg = 1.0f, eb = 1.0f;

  // Hierarchy
  bb_entity *parent = nullptr;
  std::vector<bb_entity *> children;

  // Picking / Collision
  int pickMode = 0; // 0=None, 1=Sphere, 2=Mesh, 3=Box
  int pickObs = 0;  // Obscurer?
  float radiusX = 1.0f, radiusY = 1.0f;
  float boxX = 0, boxY = 0, boxZ = 0, boxW = 0, boxH = 0, boxD = 0;
  int collisionType = 0; // 0 = no collision

  explicit bb_entity(EntityType t) : type(t) {}
  virtual ~bb_entity() {}

  void addChild(bb_entity *child) {
    child->parent = this;
    children.push_back(child);
  }

  void removeChild(bb_entity *child) {
    children.erase(std::remove(children.begin(), children.end(), child),
                   children.end());
    child->parent = nullptr;
  }
};

// =============================================================================
// Mesh Surface
// =============================================================================

struct bb_surface {
  struct Vertex {
    float x, y, z;    // Position
    float nx, ny, nz; // Normal
    float u, v;       // Texture coordinate
  };

  struct Triangle {
    int v0, v1, v2;
  };

  std::vector<Vertex> vertices;
  std::vector<Triangle> triangles;

  // Render Cache (Optimization)
  std::vector<unsigned int> indexCache;
  bool indicesValid = false;

  unsigned int textureID = 0; // Primary texture

  // Axis-Aligned Bounding Box
  float minX = 1e10, minY = 1e10, minZ = 1e10;
  float maxX = -1e10, maxY = -1e10, maxZ = -1e10;

  void invalidate() {
    indicesValid = false;
    recalculate_aabb();
  }

  void recalculate_aabb() {
    minX = minY = minZ = 1e10;
    maxX = maxY = maxZ = -1e10;
    for (const auto &v : vertices) {
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
    }
  }
};

// =============================================================================
// Mesh Entity
// =============================================================================

struct bb_mesh : public bb_entity {
  std::vector<bb_surface *> surfaces;

  // Axis-Aligned Bounding Box (Local space)
  float minX = 1e10, minY = 1e10, minZ = 1e10;
  float maxX = -1e10, maxY = -1e10, maxZ = -1e10;

  void recalculate_aabb() {
    minX = minY = minZ = 1e10;
    maxX = maxY = maxZ = -1e10;
    for (auto surf : surfaces) {
      surf->recalculate_aabb();
      if (surf->minX < minX)
        minX = surf->minX;
      if (surf->minY < minY)
        minY = surf->minY;
      if (surf->minZ < minZ)
        minZ = surf->minZ;
      if (surf->maxX > maxX)
        maxX = surf->maxX;
      if (surf->maxY > maxY)
        maxY = surf->maxY;
      if (surf->maxZ > maxZ)
        maxZ = surf->maxZ;
    }
  }

  bb_mesh() : bb_entity(ENTITY_MESH) {}
  virtual ~bb_mesh() {
    for (auto s : surfaces)
      delete s;
  }
};

// =============================================================================
// Camera Entity
// =============================================================================

struct bb_camera : public bb_entity {
  float zoom = 1.0f; // Zoom factor (1.0 = 90 degrees FOV)
  float nearPlane = 0.1f;
  float farPlane = 1000.0f;

  // Viewport
  int vX = 0, vY = 0, vW = 0, vH = 0; // 0 means use screen width/height

  // Fog
  int fogMode = 0; // 0 = None, 1 = Linear
  float fogRangeNear = 0, fogRangeFar = 1000.0f;
  float fogR = 0, fogG = 0, fogB = 0;

  // Camera clear color
  float clsR = 0, clsG = 0, clsB = 0;

  bb_camera() : bb_entity(ENTITY_CAMERA) {}
};

// =============================================================================
// Light Entity — Blitz3D types: 1 = Directional, 2 = Point, 3 = Spot
// =============================================================================

struct bb_light : public bb_entity {
  int lightType = 2; // Default: point light

  bb_light(int type = 2) : bb_entity(ENTITY_LIGHT), lightType(type) {}
};

// =============================================================================
// Array support
// =============================================================================

struct bb_array {
  std::vector<bb_int> dims;
  std::vector<bb_value> data;
};

// =============================================================================
// Images & Textures
// =============================================================================

struct bb_image {
  int width, height;
  int frames;
  std::vector<unsigned int> textureIDs; // GL textures for frames
  int mask_r = 0, mask_g = 0, mask_b = 0;
  int handle_x = 0, handle_y = 0;

  bb_image(int w, int h, int f = 1) : width(w), height(h), frames(f) {
    textureIDs.resize(f, 0);
  }
};

struct bb_texture {
  unsigned int textureID;
  int width, height, flags;

  bb_texture(unsigned int id, int w, int h, int f)
      : textureID(id), width(w), height(h), flags(f) {}
};

struct bb_font {
  unsigned int textureID = 0;
  int size = 0;
  float height = 0; // Total line height (ascent - descent + lineGap)
  float ascent = 0;
  float maxAdvance = 0;
  void *charData = nullptr; // Pointer to stbtt_packedchar array

  bb_font() = default;
};

// =============================================================================
// Timer Support
// =============================================================================

struct bb_timer {
  double hertz;
  double period;    // target period in ms
  double last_tick; // SDL_GetTicks() of last tick
  int ticks_accum;  // ticks accrued since last WaitTimer
};
