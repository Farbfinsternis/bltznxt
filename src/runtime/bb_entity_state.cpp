// =============================================================================
// bb_entity_state.cpp — Entity State (getters for position/rotation/color).
// =============================================================================

#include "bb_entity_state.h"
#include "bb_globals.h"
#include "bb_math.h"

bb_float entityx(bb_int ent_handle, bb_int global) {
  bb_entity *ent = lookup_entity(ent_handle);
  if (!ent)
    return 0.0f;
  if (global) {
    bbMatrix world = get_world_matrix(ent);
    return world.m[12];
  }
  return ent->x;
}

bb_float entityy(bb_int ent_handle, bb_int global) {
  bb_entity *ent = lookup_entity(ent_handle);
  if (!ent)
    return 0.0f;
  if (global) {
    bbMatrix world = get_world_matrix(ent);
    return world.m[13];
  }
  return ent->y;
}

bb_float entityz(bb_int ent_handle, bb_int global) {
  bb_entity *ent = lookup_entity(ent_handle);
  if (!ent)
    return 0.0f;
  if (global) {
    bbMatrix world = get_world_matrix(ent);
    return world.m[14];
  }
  return ent->z;
}

bb_float entitypitch(bb_int ent_handle, bb_int global) {
  bb_entity *ent = lookup_entity(ent_handle);
  if (!ent)
    return 0.0f;
  if (global) {
    bbMatrix world = get_world_matrix(ent);
    float p, y, r;
    world.getEuler(p, y, r);
    return p;
  }
  return ent->pitch;
}

bb_float entityyaw(bb_int ent_handle, bb_int global) {
  bb_entity *ent = lookup_entity(ent_handle);
  if (!ent)
    return 0.0f;
  if (global) {
    bbMatrix world = get_world_matrix(ent);
    float p, y, r;
    world.getEuler(p, y, r);
    return y;
  }
  return ent->yaw;
}

bb_float entityroll(bb_int ent_handle, bb_int global) {
  bb_entity *ent = lookup_entity(ent_handle);
  if (!ent)
    return 0.0f;
  if (global) {
    bbMatrix world = get_world_matrix(ent);
    float p, y, r;
    world.getEuler(p, y, r);
    return r;
  }
  return ent->roll;
}

void entitycolor(bb_int ent_handle, bb_float r, bb_float g, bb_float b) {
  bb_entity *ent = lookup_entity(ent_handle);
  if (ent) {
    ent->er = (float)r / 255.0f;
    ent->eg = (float)g / 255.0f;
    ent->eb = (float)b / 255.0f;
  }
}

void entityalpha(bb_int ent, bb_float alpha) { /* stub */ }
void entityshininess(bb_int ent, bb_float shininess) { /* stub */ }
void entityfx(bb_int ent, bb_int fx) { /* stub */ }
