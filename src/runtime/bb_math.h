#pragma once
// =============================================================================
// bb_math.h — 3D Math helpers (header-only).
// =============================================================================

#define _USE_MATH_DEFINES
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "bb_types.h"

// =============================================================================
// 4x4 Matrix (column-major, matching OpenGL)
// =============================================================================

struct bbMatrix {
  float m[16]; // Column-major to match OpenGL

  static bbMatrix identity() {
    bbMatrix res = {0};
    res.m[0] = res.m[5] = res.m[10] = res.m[15] = 1.0f;
    return res;
  }

  static bbMatrix translation(float x, float y, float z) {
    bbMatrix res = identity();
    res.m[12] = x;
    res.m[13] = y;
    res.m[14] = z;
    return res;
  }

  static bbMatrix rotation(float p, float y, float r) {
    // Blitz3D Order: Pitch (X), then Yaw (Y), then Roll (Z)
    // v' = RZ * RY * RX * v
    float cp = cosf(p * (float)M_PI / 180.0f),
          sp = sinf(p * (float)M_PI / 180.0f);
    float cy = cosf(y * (float)M_PI / 180.0f),
          sy = sinf(y * (float)M_PI / 180.0f);
    float cr = cosf(r * (float)M_PI / 180.0f),
          sr = sinf(r * (float)M_PI / 180.0f);

    bbMatrix RX = identity();
    RX.m[5] = cp;
    RX.m[6] = sp;
    RX.m[9] = -sp;
    RX.m[10] = cp;

    bbMatrix RY = identity();
    RY.m[0] = cy;
    RY.m[2] = -sy;
    RY.m[8] = sy;
    RY.m[10] = cy;

    bbMatrix RZ = identity();
    RZ.m[0] = cr;
    RZ.m[1] = sr;
    RZ.m[4] = -sr;
    RZ.m[5] = cr;

    return RZ * RY * RX;
  }

  static bbMatrix scale(float sx, float sy, float sz) {
    bbMatrix res = identity();
    res.m[0] = sx;
    res.m[5] = sy;
    res.m[10] = sz;
    return res;
  }

  bbMatrix operator*(const bbMatrix &other) const {
    bbMatrix res;
    for (int i = 0; i < 4; ++i) {
      for (int j = 0; j < 4; ++j) {
        res.m[i + j * 4] = 0;
        for (int k = 0; k < 4; ++k) {
          res.m[i + j * 4] += m[i + k * 4] * other.m[k + j * 4];
        }
      }
    }
    return res;
  }

  void transform(float &x, float &y, float &z) const {
    float tx = x, ty = y, tz = z;
    x = m[0] * tx + m[4] * ty + m[8] * tz + m[12];
    y = m[1] * tx + m[5] * ty + m[9] * tz + m[13];
    z = m[2] * tx + m[6] * ty + m[10] * tz + m[14];
  }

  void rotate_normal(float &x, float &y, float &z) const {
    float tx = x, ty = y, tz = z;
    x = m[0] * tx + m[4] * ty + m[8] * tz;
    y = m[1] * tx + m[5] * ty + m[9] * tz;
    z = m[2] * tx + m[6] * ty + m[10] * tz;
    float mag = sqrtf(x * x + y * y + z * z);
    if (mag > 0.0001f) {
      x /= mag;
      y /= mag;
      z /= mag;
    }
  }

  void getEuler(float &p, float &y, float &r) const {
    // Extract scale
    float sx = sqrtf(m[0] * m[0] + m[1] * m[1] + m[2] * m[2]);
    float sy = sqrtf(m[4] * m[4] + m[5] * m[5] + m[6] * m[6]);
    float sz = sqrtf(m[8] * m[8] + m[9] * m[9] + m[10] * m[10]);

    // Normalize for rotation extraction
    float m01 = m[4] / sy, m11 = m[5] / sy, m21 = m[6] / sy;
    float m02 = m[8] / sz, m12 = m[9] / sz, m22 = m[10] / sz;
    float m00 = m[0] / sx, m10 = m[1] / sx, m20 = m[2] / sx;

    // For RZ * RY * RX:
    // M = [ cr*cy  -sr*cp+cr*sy*sp   sr*sp+cr*sy*cp ]
    //     [ sr*cy   cr*cp+sr*sy*sp  -cr*sp+sr*sy*cp ]
    //     [ -sy     cy*sp            cy*cp          ]

    y = asinf(-m20) * 180.0f / (float)M_PI;
    if (cosf(y * (float)M_PI / 180.0f) > 0.0001f) {
      p = atan2f(m21, m22) * 180.0f / (float)M_PI;
      r = atan2f(m10, m00) * 180.0f / (float)M_PI;
    } else {
      p = 0;
      r = atan2f(m01, m11) * 180.0f / (float)M_PI;
    }
  }

  static bbMatrix inverse(const bbMatrix &mat) {
    // Proper inverse for TRS matrices
    // M = T * R * S
    // M^-1 = S^-1 * R^T * T^-1

    float sx =
        sqrtf(mat.m[0] * mat.m[0] + mat.m[1] * mat.m[1] + mat.m[2] * mat.m[2]);
    float sy =
        sqrtf(mat.m[4] * mat.m[4] + mat.m[5] * mat.m[5] + mat.m[6] * mat.m[6]);
    float sz = sqrtf(mat.m[8] * mat.m[8] + mat.m[9] * mat.m[9] +
                     mat.m[10] * mat.m[10]);

    bbMatrix res = identity();
    // R^T / S
    res.m[0] = mat.m[0] / (sx * sx);
    res.m[4] = mat.m[1] / (sx * sx);
    res.m[8] = mat.m[2] / (sx * sx);
    res.m[1] = mat.m[4] / (sy * sy);
    res.m[5] = mat.m[5] / (sy * sy);
    res.m[9] = mat.m[6] / (sy * sy);
    res.m[2] = mat.m[8] / (sz * sz);
    res.m[6] = mat.m[9] / (sz * sz);
    res.m[10] = mat.m[10] / (sz * sz);

    // - (R^T/S) * T
    res.m[12] =
        -(res.m[0] * mat.m[12] + res.m[4] * mat.m[13] + res.m[8] * mat.m[14]);
    res.m[13] =
        -(res.m[1] * mat.m[12] + res.m[5] * mat.m[13] + res.m[9] * mat.m[14]);
    res.m[14] =
        -(res.m[2] * mat.m[12] + res.m[6] * mat.m[13] + res.m[10] * mat.m[14]);

    return res;
  }
};

// =============================================================================
// Helper functions
// =============================================================================

inline bbMatrix get_local_matrix(bb_entity *ent) {
  if (!ent)
    return bbMatrix::identity();
  return bbMatrix::translation(ent->x, ent->y, ent->z) *
         bbMatrix::rotation(ent->pitch, ent->yaw, ent->roll) *
         bbMatrix::scale(ent->sx, ent->sy, ent->sz);
}

// Forward-declared: needs g_worldRoot from bb_globals.h
// Implemented in bb_globals.cpp
bbMatrix get_world_matrix(bb_entity *ent);
