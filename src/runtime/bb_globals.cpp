// =============================================================================
// bb_globals.cpp — Definitions of all shared mutable runtime state.
// =============================================================================

#include "bb_globals.h"
#include "bb_math.h"

// Window & GL context
SDL_Window *g_window = nullptr;
SDL_GLContext g_glContext = nullptr;
int g_width = 800;
int g_height = 600;
bb_string g_appTitle = "BltzNxt App";

// 2D drawing color
Uint8 g_drawR = 255, g_drawG = 255, g_drawB = 255;

// Input state
Uint8 g_keyState[SDL_SCANCODE_COUNT] = {0};
Uint8 g_keyHits[SDL_SCANCODE_COUNT] = {0};

int g_mouseX = 0;
int g_mouseY = 0;
Uint8 g_mouseState[4] = {0};
Uint8 g_mouseHits[4] = {0};

// Ambient light
float g_ambientR = 0.5f, g_ambientG = 0.5f, g_ambientB = 0.5f;

// Fonts
std::vector<bb_font *> g_fonts;
bb_font *g_currentFont = nullptr;

// Arrays
std::map<std::string, bb_array> g_arrays;

// Data Management
std::vector<bb_data_value> g_data;
std::map<std::string, size_t> g_data_labels;
size_t g_data_ptr = 0;

// Resources
std::vector<bb_image *> g_images;
std::vector<bb_texture *> g_textures;

// Scene graph
bb_entity *g_worldRoot = nullptr;
bb_camera *g_activeCamera = nullptr;
std::vector<bb_light *> g_lights;

// Entity Management
std::map<bb_int, bb_entity *> g_entities;
bb_int g_lastEntityHandle = 0;

bb_int register_entity(bb_entity *ent) {
  if (!ent)
    return 0;
  bb_int handle = ++g_lastEntityHandle;
  g_entities[handle] = ent;

  // Initialize prev positions for collision
  bbMatrix world = get_world_matrix(ent);
  ent->prevX = world.m[12];
  ent->prevY = world.m[13];
  ent->prevZ = world.m[14];

  return handle;
}

bb_entity *lookup_entity(bb_int handle) {
  auto it = g_entities.find(handle);
  if (it != g_entities.end())
    return it->second;
  return nullptr;
}

void unregister_entity(bb_int handle) { g_entities.erase(handle); }

// Surface Management
std::map<bb_int, bb_surface *> g_surfaces;
bb_int g_lastSurfaceHandle = 0;

// Picking Results
bb_int g_pickedEntity = 0;
float g_pickedX = 0, g_pickedY = 0, g_pickedZ = 0;
float g_pickedNX = 0, g_pickedNY = 0, g_pickedNZ = 0;
float g_pickedTime = 0;
bb_int g_pickedSurface = 0;
bb_int g_pickedTriangle = 0;

std::vector<CollisionDef> g_collisionDefs;

// Timer Management
std::map<bb_int, bb_timer *> g_timers;
bb_int g_lastTimerHandle = 0;

// Camera Projection Results
float g_projectedX = 0.0f;
float g_projectedY = 0.0f;
float g_projectedZ = 0.0f;

bb_int register_surface(bb_surface *surf) {
  if (!surf)
    return 0;
  bb_int handle = ++g_lastSurfaceHandle;
  g_surfaces[handle] = surf;
  return handle;
}

bb_surface *lookup_surface(bb_int handle) {
  auto it = g_surfaces.find(handle);
  if (it != g_surfaces.end())
    return it->second;
  return nullptr;
}

void unregister_surface(bb_int handle) { g_surfaces.erase(handle); }

void unregister_all_surfaces(bb_mesh *mesh) {
  if (!mesh)
    return;
  for (auto s : mesh->surfaces) {
    // We need to find the handle for this surface to unregister it
    for (auto it = g_surfaces.begin(); it != g_surfaces.end(); ++it) {
      if (it->second == s) {
        g_surfaces.erase(it);
        break;
      }
    }
  }
}

// Helper
void ensure_world_root() {
  if (!g_worldRoot) {
    g_worldRoot = new bb_entity(ENTITY_PIVOT);
  }
}

// get_world_matrix needs g_worldRoot, so it lives here
bbMatrix get_world_matrix(bb_entity *ent) {
  if (!ent || ent == g_worldRoot)
    return bbMatrix::identity();
  return get_world_matrix(ent->parent) * get_local_matrix(ent);
}
