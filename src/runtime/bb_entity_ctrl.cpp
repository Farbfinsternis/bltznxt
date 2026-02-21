// =============================================================================
// bb_entity_ctrl.cpp — Entity Control (Parent, Free, CreatePivot).
// =============================================================================

#include "bb_globals.h"
#include "bb_math.h"

#include <algorithm>

void bbFreeEntity(bb_int ent_handle) {
  bb_entity *ent = lookup_entity(ent_handle);
  if (!ent)
    return;

  // Remove from parent
  if (ent->parent) {
    ent->parent->removeChild(ent);
  }

  // Remove from light list if applicable
  if (ent->type == ENTITY_LIGHT) {
    g_lights.erase(
        std::remove(g_lights.begin(), g_lights.end(), (bb_light *)ent),
        g_lights.end());
  }

  if (ent->type == ENTITY_MESH) {
    unregister_all_surfaces((bb_mesh *)ent);
  }

  unregister_entity(ent_handle);
  delete ent;
}

void freeentity(bb_int ent) { bbFreeEntity(ent); }

void entityparent(bb_int ent_handle, bb_int parent_handle, bb_int global) {
  bb_entity *ent = lookup_entity(ent_handle);
  bb_entity *newParent = lookup_entity(parent_handle);
  if (!ent)
    return;

  // Save world position if global flag is set
  float wx = 0, wy = 0, wz = 0;
  float wp = 0, wyaw = 0, wr = 0;
  if (global) {
    bbMatrix world = get_world_matrix(ent);
    wx = world.m[12];
    wy = world.m[13];
    wz = world.m[14];
    world.getEuler(wp, wyaw, wr);
  }

  // Remove from old parent
  if (ent->parent) {
    ent->parent->removeChild(ent);
  }

  // Attach to new parent (or world root)
  if (newParent) {
    newParent->addChild(ent);
  } else {
    ensure_world_root();
    g_worldRoot->addChild(ent);
  }

  // Restore world position if global flag is set
  if (global && newParent) {
    bbMatrix parentWorld = get_world_matrix(newParent);
    bbMatrix parentInv = bbMatrix::inverse(parentWorld);
    float lx = wx, ly = wy, lz = wz;
    parentInv.transform(lx, ly, lz);
    ent->x = lx;
    ent->y = ly;
    ent->z = lz;
    ent->pitch = wp;
    ent->yaw = wyaw;
    ent->roll = wr;
  } else if (global && !newParent) {
    ent->x = wx;
    ent->y = wy;
    ent->z = wz;
    ent->pitch = wp;
    ent->yaw = wyaw;
    ent->roll = wr;
  }
}

bb_int createpivot(bb_int parent_handle) {
  bb_entity *pivot = new bb_entity(ENTITY_PIVOT);
  ensure_world_root();
  bb_entity *parent = lookup_entity(parent_handle);
  if (parent)
    parent->addChild(pivot);
  else
    g_worldRoot->addChild(pivot);
  return register_entity(pivot);
}
