// =============================================================================
// bb_camera.cpp — Camera commands.
// =============================================================================

#include "bb_globals.h"
#include "bb_math.h"
#include <cmath>

bb_int createcamera(bb_int parent_handle) {
  bb_camera *cam = new bb_camera();
  ensure_world_root();

  bb_entity *parent = lookup_entity(parent_handle);
  if (parent)
    parent->addChild(cam);
  else
    g_worldRoot->addChild(cam);

  if (!g_activeCamera)
    g_activeCamera = cam;

  return register_entity(cam);
}

void camerazoom(bb_int cam_handle, bb_float zoom) {
  bb_camera *cam = (bb_camera *)lookup_entity(cam_handle);
  if (cam && cam->type == ENTITY_CAMERA) {
    cam->zoom = (float)zoom;
  }
}

void camerarange(bb_int cam_handle, bb_float n, bb_float f) {
  bb_camera *cam = (bb_camera *)lookup_entity(cam_handle);
  if (cam && cam->type == ENTITY_CAMERA) {
    cam->nearPlane = (float)n;
    cam->farPlane = (float)f;
  }
}

void camerafogmode(bb_int cam_handle, bb_int mode) {
  bb_camera *cam = (bb_camera *)lookup_entity(cam_handle);
  if (cam && cam->type == ENTITY_CAMERA) {
    cam->fogMode = (int)mode;
  }
}

void camerafogcolor(bb_int cam_handle, bb_float r, bb_float g, bb_float b) {
  bb_camera *cam = (bb_camera *)lookup_entity(cam_handle);
  if (cam && cam->type == ENTITY_CAMERA) {
    cam->fogR = (float)r / 255.0f;
    cam->fogG = (float)g / 255.0f;
    cam->fogB = (float)b / 255.0f;
  }
}

void camerafogrange(bb_int cam_handle, bb_float n, bb_float f) {
  bb_camera *cam = (bb_camera *)lookup_entity(cam_handle);
  if (cam && cam->type == ENTITY_CAMERA) {
    cam->fogRangeNear = (float)n;
    cam->fogRangeFar = (float)f;
  }
}

void cameraviewport(bb_int cam_handle, bb_int x, bb_int y, bb_int width,
                    bb_int height) {
  bb_camera *cam = (bb_camera *)lookup_entity(cam_handle);
  if (cam && cam->type == ENTITY_CAMERA) {
    cam->vX = (int)x;
    cam->vY = (int)y;
    cam->vW = (int)width;
    cam->vH = (int)height;
  }
}

// =============================================================================
// Camera Project
// =============================================================================

void cameraproject(bb_int cam_handle, bb_float x, bb_float y, bb_float z) {
  bb_camera *cam = (bb_camera *)lookup_entity(cam_handle);
  if (!cam || cam->type != ENTITY_CAMERA)
    return;

  // Viewport
  GLint viewport[4];
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
  GLdouble projection[16];
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
  GLdouble modelview[16];
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glScalef(1.0f, 1.0f, -1.0f);

  // Apply Camera Transform (Inverse of Camera Matrix)
  // We can use the global matrix of the camera and invert it.
  // Or since we have p/y/r and x/y/z, let's just reverse them as in camerapick.
  // Note: This only works perfectly if the camera has no parent scaling or
  // complex hierarchy that isn't captured by simple TRS. But for standard B3D
  // cameras, this is usually how it's done. OPTIMIZATION: Use inverse of
  // get_world_matrix(cam) is more robust for hierarchies.

  // Let's stick to the manual way matching bb_collisions.cpp for consistency
  // first.
  glRotatef(-cam->roll, 0, 0, 1);
  glRotatef(-cam->pitch, 1, 0, 0);
  glRotatef(-cam->yaw, 0, 1, 0);
  glTranslatef(-cam->x, -cam->y, -cam->z);

  glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
  glPopMatrix();

  GLdouble winX, winY, winZ;
  if (gluProject(x, y, z, modelview, projection, viewport, &winX, &winY,
                 &winZ) == GL_TRUE) {
    g_projectedX = (float)winX;
    g_projectedY = (float)vH - (float)winY; // Invert Y
    g_projectedZ = (float)winZ;
  } else {
    g_projectedX = 0;
    g_projectedY = 0;
    g_projectedZ = 0;
  }
}

bb_float projectedx() { return g_projectedX; }
bb_float projectedy() { return g_projectedY; }
bb_float projectedz() { return g_projectedZ; }

void cameraclscolor(bb_int cam_handle, bb_float r, bb_float g, bb_float b) {
  bb_camera *cam = (bb_camera *)lookup_entity(cam_handle);
  if (cam && cam->type == ENTITY_CAMERA) {
    cam->clsR = (float)r / 255.0f;
    cam->clsG = (float)g / 255.0f;
    cam->clsB = (float)b / 255.0f;
  }
}
