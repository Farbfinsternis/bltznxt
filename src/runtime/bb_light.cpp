// =============================================================================
// bb_light.cpp — Lighting commands.
// =============================================================================

#include "bb_light.h"
#include "bb_globals.h"

#include <iostream>

bb_int createlight(bb_int lightType, bb_int parent_handle) {
  bb_light *light = new bb_light(lightType == 0 ? 2 : (int)lightType);
  ensure_world_root();

  bb_entity *parent = lookup_entity(parent_handle);
  if (parent)
    parent->addChild(light);
  else
    g_worldRoot->addChild(light);

  if (g_lights.size() < 8) {
    g_lights.push_back(light);
  } else {
    std::cerr << "[Runtime] Warning: Max 8 lights supported." << std::endl;
  }

  return register_entity(light);
}

void ambientlight(bb_float r, bb_float g, bb_float b) {
  g_ambientR = (float)r / 255.0f;
  g_ambientG = (float)g / 255.0f;
  g_ambientB = (float)b / 255.0f;
}

void lightcolor(bb_int light_handle, bb_float r, bb_float g, bb_float b) {
  bb_light *light = (bb_light *)lookup_entity(light_handle);
  if (light && light->type == ENTITY_LIGHT) {
    light->er = (float)r / 255.0f;
    light->eg = (float)g / 255.0f;
    light->eb = (float)b / 255.0f;
  }
}

void setup_gl_lights() {
  bool hasLights = !g_lights.empty();

  if (hasLights) {
    glEnable(GL_LIGHTING);
  } else {
    glDisable(GL_LIGHTING);
    return;
  }

  // Global ambient
  float ambient[4] = {g_ambientR, g_ambientG, g_ambientB, 1.0f};
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);

  // Configure each light (GL_LIGHT0 .. GL_LIGHT7)
  for (int i = 0; i < (int)g_lights.size() && i < 8; ++i) {
    GLenum glLight = GL_LIGHT0 + i;
    bb_light *light = g_lights[i];

    glEnable(glLight);

    // Diffuse color
    float diffuse[4] = {light->er, light->eg, light->eb, 1.0f};
    glLightfv(glLight, GL_DIFFUSE, diffuse);
    glLightfv(glLight, GL_SPECULAR, diffuse);

    if (light->lightType == 1) {
      // Directional light: position.w = 0 means direction
      float dir[4] = {light->x, light->y, light->z, 0.0f};
      glLightfv(glLight, GL_POSITION, dir);
    } else {
      // Point light: position.w = 1
      float pos[4] = {light->x, light->y, light->z, 1.0f};
      glLightfv(glLight, GL_POSITION, pos);

      // Attenuation (gentle falloff)
      glLightf(glLight, GL_CONSTANT_ATTENUATION, 1.0f);
      glLightf(glLight, GL_LINEAR_ATTENUATION, 0.01f);
      glLightf(glLight, GL_QUADRATIC_ATTENUATION, 0.001f);
    }
  }

  // Disable unused light slots
  for (int i = (int)g_lights.size(); i < 8; ++i) {
    glDisable(GL_LIGHT0 + i);
  }
}
