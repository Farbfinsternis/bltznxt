// =============================================================================
// bb_entity_move.cpp — Entity Movement (Position, Move, Turn, Scale, Point).
// =============================================================================

#include "bb_globals.h"
#include "bb_math.h"

#define _USE_MATH_DEFINES
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void positionentity(bb_int ent_handle, bb_float x, bb_float y, bb_float z,
                    bb_int global) {
  bb_entity *ent = lookup_entity(ent_handle);
  if (!ent)
    return;

  if (global && ent->parent) {
    bbMatrix pInv = bbMatrix::inverse(get_world_matrix(ent->parent));
    float lx = (float)x, ly = (float)y, lz = (float)z;
    pInv.transform(lx, ly, lz);
    ent->x = lx;
    ent->y = ly;
    ent->z = lz;
  } else {
    ent->x = (float)x;
    ent->y = (float)y;
    ent->z = (float)z;
  }
}

void moveentity(bb_int ent_handle, bb_float x, bb_float y, bb_float z) {
  bb_entity *ent = lookup_entity(ent_handle);
  if (ent) {
    // Move along local axes
    bbMatrix rot = bbMatrix::rotation(ent->pitch, ent->yaw, ent->roll);
    float lx = (float)x, ly = (float)y, lz = (float)z;

    float wx = rot.m[0] * lx + rot.m[4] * ly + rot.m[8] * lz;
    float wy = rot.m[1] * lx + rot.m[5] * ly + rot.m[9] * lz;
    float wz = rot.m[2] * lx + rot.m[6] * ly + rot.m[10] * lz;

    ent->x += wx;
    ent->y += wy;
    ent->z += wz;
  }
}

void rotateentity(bb_int ent_handle, bb_float p, bb_float y, bb_float r,
                  bb_int global) {
  bb_entity *ent = lookup_entity(ent_handle);
  if (!ent)
    return;

  if (global && ent->parent) {
    bbMatrix wRot = bbMatrix::rotation((float)p, (float)y, (float)r);
    bbMatrix pWorld = get_world_matrix(ent->parent);

    // Remove parent scaling from pWorld to get pRot
    float sx = sqrtf(pWorld.m[0] * pWorld.m[0] + pWorld.m[1] * pWorld.m[1] +
                     pWorld.m[2] * pWorld.m[2]);
    float sy = sqrtf(pWorld.m[4] * pWorld.m[4] + pWorld.m[5] * pWorld.m[5] +
                     pWorld.m[6] * pWorld.m[6]);
    float sz = sqrtf(pWorld.m[8] * pWorld.m[8] + pWorld.m[9] * pWorld.m[9] +
                     pWorld.m[10] * pWorld.m[10]);

    bbMatrix pRot = pWorld;
    pRot.m[0] /= sx;
    pRot.m[1] /= sx;
    pRot.m[2] /= sx;
    pRot.m[4] /= sy;
    pRot.m[5] /= sy;
    pRot.m[6] /= sy;
    pRot.m[8] /= sz;
    pRot.m[9] /= sz;
    pRot.m[10] /= sz;
    pRot.m[12] = 0;
    pRot.m[13] = 0;
    pRot.m[14] = 0;

    bbMatrix lRot = bbMatrix::inverse(pRot) * wRot;
    lRot.getEuler(ent->pitch, ent->yaw, ent->roll);
  } else {
    ent->pitch = (float)p;
    ent->yaw = (float)y;
    ent->roll = (float)r;
  }
}

void turnentity(bb_int ent_handle, bb_float p, bb_float y, bb_float r,
                bb_int global) {
  bb_entity *ent = lookup_entity(ent_handle);
  if (!ent)
    return;

  bbMatrix rel = bbMatrix::rotation((float)p, (float)y, (float)r);

  if (global) {
    // Rotate around world axes
    bbMatrix world = get_world_matrix(ent);
    bbMatrix newWorld = rel * world;

    if (ent->parent) {
      bbMatrix pInv = bbMatrix::inverse(get_world_matrix(ent->parent));
      bbMatrix newLocal = pInv * newWorld;
      newLocal.getEuler(ent->pitch, ent->yaw, ent->roll);
      // Note: translation might be updated too if we wanted,
      // but TurnEntity usually only affects rotation.
      // In B3D, TurnEntity with global=true keeps world position constant.
    } else {
      newWorld.getEuler(ent->pitch, ent->yaw, ent->roll);
    }
  } else {
    // Additive local rotation
    bbMatrix local = bbMatrix::rotation(ent->pitch, ent->yaw, ent->roll);
    bbMatrix newLocal = local * rel;
    newLocal.getEuler(ent->pitch, ent->yaw, ent->roll);
  }
}

void translateentity(bb_int ent_handle, bb_float x, bb_float y, bb_float z,
                     bb_int global) {
  bb_entity *ent = lookup_entity(ent_handle);
  if (!ent)
    return;

  if (global && ent->parent) {
    bbMatrix pInv = bbMatrix::inverse(get_world_matrix(ent->parent));
    // Only translate, so we don't want the full inverse if it includes
    // rotation? No, TranslateEntity(global=true) means displacement (x,y,z) in
    // world space. We need to transform the vector (x,y,z) into parent space.
    float dx = (float)x, dy = (float)y, dz = (float)z;

    // Remove translation from pInv to just get the vector transform
    // (rotation+scale inverse)
    bbMatrix pInvVec = pInv;
    pInvVec.m[12] = 0;
    pInvVec.m[13] = 0;
    pInvVec.m[14] = 0;
    pInvVec.transform(dx, dy, dz);

    ent->x += dx;
    ent->y += dy;
    ent->z += dz;
  } else {
    ent->x += (float)x;
    ent->y += (float)y;
    ent->z += (float)z;
  }
}

void scaleentity(bb_int ent_handle, bb_float x, bb_float y, bb_float z,
                 bb_int global) {
  bb_entity *ent = lookup_entity(ent_handle);
  if (ent) {
    ent->sx = (float)x;
    ent->sy = (float)y;
    ent->sz = (float)z;
  }
}

void pointentity(bb_int ent_handle, bb_int target_handle, bb_float roll) {
  bb_entity *ent = lookup_entity(ent_handle);
  bb_entity *target = lookup_entity(target_handle);
  if (!ent || !target)
    return;

  // Get world positions
  bbMatrix entWorld = get_world_matrix(ent);
  bbMatrix tgtWorld = get_world_matrix(target);

  float ex = entWorld.m[12], ey = entWorld.m[13], ez = entWorld.m[14];
  float tx = tgtWorld.m[12], ty = tgtWorld.m[13], tz = tgtWorld.m[14];

  float dx = tx - ex;
  float dy = ty - ey;
  float dz = tz - ez;

  float dist = sqrtf(dx * dx + dy * dy + dz * dz);
  if (dist < 0.0001f)
    return;

  // Calculate pitch and yaw to face target
  float pitch = -asinf(dy / dist) * 180.0f / (float)M_PI;
  float yaw = atan2f(dx, dz) * 180.0f / (float)M_PI;

  ent->pitch = pitch;
  ent->yaw = yaw;
  ent->roll = (float)roll;
}
