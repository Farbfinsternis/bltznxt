#pragma once
// =============================================================================
// bb_globals.h — Shared mutable runtime state (extern declarations).
// =============================================================================

#include "bb_types.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL3/SDL.h>

#include <map>

// Window & GL context
extern SDL_Window *g_window;
extern SDL_GLContext g_glContext;
extern int g_width;
extern int g_height;
extern bb_string g_appTitle;

// 2D drawing color
extern Uint8 g_drawR, g_drawG, g_drawB;

// Input state
extern Uint8 g_keyState[SDL_SCANCODE_COUNT];
extern Uint8 g_keyHits[SDL_SCANCODE_COUNT];

extern int g_mouseX;
extern int g_mouseY;
extern Uint8 g_mouseState[4]; // 1=Left, 2=Right, 3=Middle
extern Uint8 g_mouseHits[4];

// Ambient light
extern float g_ambientR, g_ambientG, g_ambientB;

// Fonts
extern std::vector<bb_font *> g_fonts;
extern bb_font *g_currentFont;

// Arrays
extern std::map<std::string, bb_array> g_arrays;

// Data Management
extern std::vector<bb_data_value> g_data;
extern std::map<std::string, size_t> g_data_labels;
extern size_t g_data_ptr;

// Resources
extern std::vector<bb_image *> g_images;
extern std::vector<bb_texture *> g_textures;

// Scene graph
extern bb_entity *g_worldRoot;
extern bb_camera *g_activeCamera;
extern std::vector<bb_light *> g_lights;

// Entity Management
extern std::map<bb_int, bb_entity *> g_entities;
extern bb_int g_lastEntityHandle;

// Picking Results
extern bb_int g_pickedEntity;
extern float g_pickedX, g_pickedY, g_pickedZ;
extern float g_pickedNX, g_pickedNY, g_pickedNZ;
extern float g_pickedTime;
extern bb_int g_pickedSurface;
extern bb_int g_pickedTriangle;

struct CollisionDef {
  int src_type;
  int dest_type;
  int method;
  int response;
};
extern std::vector<CollisionDef> g_collisionDefs;

// Camera Projection Results
extern float g_projectedX;
extern float g_projectedY;
extern float g_projectedZ;

bb_int register_entity(bb_entity *ent);
bb_entity *lookup_entity(bb_int handle);
void unregister_entity(bb_int handle);

// Surface Management (Surfaces are NOT entities in B3D)
extern std::map<bb_int, bb_surface *> g_surfaces;
extern bb_int g_lastSurfaceHandle;

// Timer Management
extern std::map<bb_int, bb_timer *> g_timers;
extern bb_int g_lastTimerHandle;

bb_int register_surface(bb_surface *surf);
bb_surface *lookup_surface(bb_int handle);
void unregister_surface(bb_int handle);
void unregister_all_surfaces(bb_mesh *mesh);

// Helper: ensure world root exists
void ensure_world_root();
