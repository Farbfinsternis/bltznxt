// =============================================================================
// bb_render.cpp — Rendering (RenderWorld, render_node).
// =============================================================================

#include "bb_render.h"
#include "bb_globals.h"
#include "bb_light.h"

#define _USE_MATH_DEFINES
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void render_node(bb_entity *node) {
  if (!node)
    return;

  glPushMatrix();
  glTranslatef(node->x, node->y, node->z);
  glRotatef(node->yaw, 0, 1, 0);
  glRotatef(node->pitch, 1, 0, 0);
  glRotatef(node->roll, 0, 0, 1);
  glScalef(node->sx, node->sy, node->sz);

  if (node->type == ENTITY_MESH) {
    bb_mesh *mesh = (bb_mesh *)node;
    for (auto surf : mesh->surfaces) {
      if (surf->vertices.empty() || surf->triangles.empty())
        continue;

      // Set material color from entity color
      float matDiffuse[4] = {mesh->er, mesh->eg, mesh->eb, 1.0f};
      glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, matDiffuse);
      glColor3f(mesh->er, mesh->eg, mesh->eb);

      // Rebuild index buffer only if invalid
      if (!surf->indicesValid) {
        surf->indexCache.clear();
        surf->indexCache.reserve(surf->triangles.size() * 3);
        for (auto &tri : surf->triangles) {
          surf->indexCache.push_back((unsigned int)tri.v0);
          surf->indexCache.push_back((unsigned int)tri.v1);
          surf->indexCache.push_back((unsigned int)tri.v2);
        }
        surf->indicesValid = true;
      }

      const int stride = sizeof(bb_surface::Vertex);
      const char *base = (const char *)&surf->vertices[0];

      if (surf->textureID) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, surf->textureID);
      } else {
        glDisable(GL_TEXTURE_2D);
      }

      glEnableClientState(GL_VERTEX_ARRAY);
      glEnableClientState(GL_NORMAL_ARRAY);
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);

      glVertexPointer(3, GL_FLOAT, stride,
                      base + offsetof(bb_surface::Vertex, x));
      glNormalPointer(GL_FLOAT, stride,
                      base + offsetof(bb_surface::Vertex, nx));
      glTexCoordPointer(2, GL_FLOAT, stride,
                        base + offsetof(bb_surface::Vertex, u));

      glDrawElements(GL_TRIANGLES, (GLsizei)surf->indexCache.size(),
                     GL_UNSIGNED_INT, surf->indexCache.data());

      glDisableClientState(GL_TEXTURE_COORD_ARRAY);
      glDisableClientState(GL_NORMAL_ARRAY);
      glDisableClientState(GL_VERTEX_ARRAY);

      if (surf->textureID) {
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
      }
    }
  }

  // Recurse into children
  for (auto *child : node->children) {
    render_node(child);
  }
  glPopMatrix();
}

void renderworld(bb_float tween) {
  int w = g_width;
  int h = g_height;
  int vx = 0, vy = 0, vw = w, vh = h;

  if (g_activeCamera) {
    if (g_activeCamera->vW > 0) {
      vx = g_activeCamera->vX;
      vy = g_activeCamera->vY;
      vw = g_activeCamera->vW;
      vh = g_activeCamera->vH;
    }
    // Set clear color for this camera
    glClearColor(g_activeCamera->clsR, g_activeCamera->clsG,
                 g_activeCamera->clsB, 1.0f);
  }

  glViewport(vx, vy, vw, vh);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Fog setup
  if (g_activeCamera && g_activeCamera->fogMode > 0) {
    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    float fogColor[4] = {g_activeCamera->fogR, g_activeCamera->fogG,
                         g_activeCamera->fogB, 1.0f};
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogf(GL_FOG_START, g_activeCamera->fogRangeNear);
    glFogf(GL_FOG_END, g_activeCamera->fogRangeFar);
  } else {
    glDisable(GL_FOG);
  }

  // Projection
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  float aspect = (vh > 0) ? (float)vw / (float)vh : 1.0f;

  float fov = 90.0f;
  float nearPlane = 0.1f;
  float farPlane = 1000.0f;

  if (g_activeCamera) {
    float zoom = g_activeCamera->zoom;
    if (zoom < 0.0001f)
      zoom = 0.0001f;
    float fovX = 2.0f * (float)atan(1.0 / zoom);
    float fovY = 2.0f * (float)atan(tan(fovX / 2.0f) / aspect);
    fov = fovY * (180.0f / (float)M_PI);

    nearPlane = g_activeCamera->nearPlane;
    farPlane = g_activeCamera->farPlane;
  }
  gluPerspective(fov, aspect, nearPlane, farPlane);

  // ModelView
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glScalef(1.0f, 1.0f, -1.0f); // Flip Z to match Blitz3D (+Z forward)

  if (g_activeCamera) {
    glRotatef(-g_activeCamera->roll, 0, 0, 1);
    glRotatef(-g_activeCamera->pitch, 1, 0, 0);
    glRotatef(-g_activeCamera->yaw, 0, 1, 0);
    glTranslatef(-g_activeCamera->x, -g_activeCamera->y, -g_activeCamera->z);
  } else {
    glTranslatef(0, 0, -5);
  }

  // Lights
  setup_gl_lights();

  // Scene
  if (g_worldRoot)
    render_node(g_worldRoot);
}
