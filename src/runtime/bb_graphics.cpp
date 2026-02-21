// =============================================================================
// bb_graphics.cpp — Graphics Mode, 2D Overlay, Window management.
// =============================================================================

#include "bb_graphics.h"
#include "bb_globals.h"

#include <cmath>
#include <cstring>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// =============================================================================
// Initialization & Window
// =============================================================================

void bbRuntimeInit() {
  std::cout << std::unitbuf;
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
    exit(1);
  }
  SDL_PumpEvents();
  std::cout << "[Runtime] SDL3 Initialized" << std::endl;
}

void apptitle(bb_string title, bb_string close_prompt) {
  g_appTitle = title;
  if (g_window) {
    SDL_SetWindowTitle(g_window, g_appTitle.c_str());
  }
}

static void bbGraphics(bb_int width, bb_int height, bb_int depth, bb_int mode,
                       bool threeD) {
  if (g_window)
    return;

  if (width == 0 && height == 0) {
    SDL_DisplayID displayID = SDL_GetPrimaryDisplay();
    const SDL_DisplayMode *modeInfo = SDL_GetDesktopDisplayMode(displayID);
    if (modeInfo) {
      width = modeInfo->w;
      height = modeInfo->h;
    } else {
      // Fallback
      width = 1024;
      height = 768;
    }
  }

  g_width = (int)width;
  g_height = (int)height;

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

  Uint32 flags = SDL_WINDOW_OPENGL;
  if (mode == 1) {
    flags |= SDL_WINDOW_FULLSCREEN;
  }

  g_window = SDL_CreateWindow(g_appTitle.c_str(), width, height, flags);
  if (!g_window) {
    std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
    exit(1);
  }

  g_glContext = SDL_GL_CreateContext(g_window);
  if (!g_glContext) {
    std::cerr << "GL Context creation failed: " << SDL_GetError() << std::endl;
    exit(1);
  }

  if (SDL_GL_MakeCurrent(g_window, g_glContext) < 0) {
    std::cerr << "SDL_GL_MakeCurrent failed: " << SDL_GetError() << std::endl;
  }

  SDL_GL_SetSwapInterval(1);
  std::cout << "[Runtime] OpenGL Window Opened (" << width << "x" << height
            << ")" << std::endl;

  // Initial GL state
  if (threeD) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
  } else {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glShadeModel(GL_FLAT);
  }

  glViewport(0, 0, width, height);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void graphics(bb_int width, bb_int height, bb_int depth, bb_int mode) {
  bbGraphics(width, height, depth, mode, false);
}

void graphics3d(bb_int width, bb_int height, bb_int depth, bb_int mode) {
  bbGraphics(width, height, depth, mode, true);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glEnable(GL_NORMALIZE);
  glViewport(0, 0, width, height);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

  glShadeModel(GL_SMOOTH);

  glEnable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

  // Build font texture atlas (Phase 3: handled by bb_font.cpp)
}

bb_int graphicswidth() { return g_width; }
bb_int graphicsheight() { return g_height; }

#define GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX 0x9048
#define GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX 0x9049
#define GL_VBO_FREE_MEMORY_ATI 0x87FB

bb_int totalvidmem() {
  GLint total_kb = 0;
  // Try NVIDIA
  glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &total_kb);
  if (glGetError() == GL_NO_ERROR && total_kb > 0) {
    return (bb_int)total_kb * 1024;
  }
  // Try AMD/ATI (Partial support)
  glGetIntegerv(GL_VBO_FREE_MEMORY_ATI, &total_kb);
  if (glGetError() == GL_NO_ERROR && total_kb > 0) {
    return (bb_int)total_kb * 1024;
  }
  // Fallback: 4GB
  return (bb_int)4096 * 1024 * 1024;
}

bb_int availvidmem() {
  GLint avail_kb = 0;
  // Try NVIDIA
  glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &avail_kb);
  if (glGetError() == GL_NO_ERROR && avail_kb > 0) {
    return (bb_int)avail_kb * 1024;
  }
  // Fallback: Just return total
  return totalvidmem();
}

bb_int backbuffer() { return 1; }

void setbuffer(bb_int buf) {
  // Stub for now: Always rendering to the one and only GL backbuffer.
}

// =============================================================================
// Clear & Flip
// =============================================================================

void cls() {
  if (!g_window)
    return;
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void clscolor(bb_int r, bb_int g, bb_int b) {
  glClearColor(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
}

void endgraphics() {
  if (g_glContext) {
    SDL_GL_DestroyContext(g_glContext);
    g_glContext = nullptr;
  }
  if (g_window) {
    SDL_DestroyWindow(g_window);
    g_window = nullptr;
  }
  SDL_Quit();
}

// =============================================================================
// 2D Overlay Commands
// =============================================================================

void color(bb_int r, bb_int g, bb_int b) {
  g_drawR = (Uint8)r;
  g_drawG = (Uint8)g;
  g_drawB = (Uint8)b;
  glColor3ub(g_drawR, g_drawG, g_drawB);
}

// Text implementation moved to bb_font.cpp

void rect(bb_int x, bb_int y, bb_int w, bb_int h, bb_int solid) {
  if (!g_window)
    return;

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  glDisable(GL_TEXTURE_2D);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, g_width, g_height, 0, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glColor3ub(g_drawR, g_drawG, g_drawB);

  if (solid) {
    glBegin(GL_QUADS);
    glVertex2i(x, y);
    glVertex2i(x + w, y);
    glVertex2i(x + w, y + h);
    glVertex2i(x, y + h);
    glEnd();
  } else {
    glBegin(GL_LINE_LOOP);
    glVertex2i(x, y);
    glVertex2i(x + w, y);
    glVertex2i(x + w, y + h);
    glVertex2i(x, y + h);
    glEnd();
  }

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
}

void line(bb_int x1, bb_int y1, bb_int x2, bb_int y2) {
  if (!g_window)
    return;

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  glDisable(GL_TEXTURE_2D);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, g_width, g_height, 0, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glColor3ub(g_drawR, g_drawG, g_drawB);

  glBegin(GL_LINES);
  glVertex2i(x1, y1);
  glVertex2i(x2, y2);
  glEnd();

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
}

void oval(bb_int x, bb_int y, bb_int w, bb_int h, bb_int solid) {
  if (!g_window)
    return;

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  glDisable(GL_TEXTURE_2D);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, g_width, g_height, 0, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glColor3ub(g_drawR, g_drawG, g_drawB);

  float cx = x + w * 0.5f;
  float cy = y + h * 0.5f;
  float rx = w * 0.5f;
  float ry = h * 0.5f;
  int segs = 32;

  glBegin(solid ? GL_POLYGON : GL_LINE_LOOP);
  for (int i = 0; i < segs; i++) {
    float theta = 2.0f * 3.1415926f * float(i) / float(segs);
    float dx = rx * cosf(theta);
    float dy = ry * sinf(theta);
    glVertex2f(cx + dx, cy + dy);
  }
  glEnd();

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
}

void flip(bb_int vwait) {
  if (!g_window)
    return;

  SDL_GL_SwapWindow(g_window);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Poll events
  SDL_PumpEvents();
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT) {
      exit(0);
    } else if (event.type == SDL_EVENT_KEY_DOWN) {
      if (event.key.scancode < SDL_SCANCODE_COUNT) {
        g_keyState[event.key.scancode] = 1;
        g_keyHits[event.key.scancode] = 1;
      }
    } else if (event.type == SDL_EVENT_KEY_UP) {
      if (event.key.scancode < SDL_SCANCODE_COUNT) {
        g_keyState[event.key.scancode] = 0;
      }
    } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
      g_mouseX = (int)event.motion.x;
      g_mouseY = (int)event.motion.y;
    } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
      int btn = 0;
      if (event.button.button == SDL_BUTTON_LEFT)
        btn = 1;
      else if (event.button.button == SDL_BUTTON_RIGHT)
        btn = 2;
      else if (event.button.button == SDL_BUTTON_MIDDLE)
        btn = 3;

      if (btn) {
        g_mouseState[btn] = 1;
        g_mouseHits[btn] = 1;
      }
    } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
      int btn = 0;
      if (event.button.button == SDL_BUTTON_LEFT)
        btn = 1;
      else if (event.button.button == SDL_BUTTON_RIGHT)
        btn = 2;
      else if (event.button.button == SDL_BUTTON_MIDDLE)
        btn = 3;

      if (btn) {
        g_mouseState[btn] = 0;
      }
    }
  }
}

bb_int waitkey() {
  SDL_Event event;
  while (true) {
    if (SDL_WaitEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT)
        exit(0);
      if (event.type == SDL_EVENT_KEY_DOWN)
        return 1;
    }
  }
}
// =============================================================================
// Images
// =============================================================================

bb_int loadimage(bb_string file) {
  int w, h, n;
  stbi_set_flip_vertically_on_load(false);
  unsigned char *data = stbi_load(file.c_str(), &w, &h, &n, 4);
  if (!data) {
    std::cerr << "Failed to load image: " << file << std::endl;
    return 0;
  }

  GLuint tex;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               data);
  glBindTexture(GL_TEXTURE_2D, 0);

  stbi_image_free(data);

  bb_image *img = new bb_image(w, h, 1);
  img->textureIDs[0] = tex;
  g_images.push_back(img);
  return (bb_int)g_images.size();
}

void drawimage(bb_int handle, bb_int x, bb_int y, bb_int frame) {
  if (handle < 1 || handle > (bb_int)g_images.size())
    return;
  bb_image *img = g_images[handle - 1];
  if (!img || frame < 0 || frame >= img->frames)
    return;

  GLuint tex = img->textureIDs[frame];
  if (!tex)
    return;

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, tex);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, g_width, g_height, 0, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glColor4f(1, 1, 1, 1);
  float fx = (float)x - img->handle_x;
  float fy = (float)y - img->handle_y;
  float fw = (float)img->width;
  float fh = (float)img->height;

  glBegin(GL_QUADS);
  glTexCoord2f(0, 0);
  glVertex2f(fx, fy);
  glTexCoord2f(1, 0);
  glVertex2f(fx + fw, fy);
  glTexCoord2f(1, 1);
  glVertex2f(fx + fw, fy + fh);
  glTexCoord2f(0, 1);
  glVertex2f(fx, fy + fh);
  glEnd();

  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);

  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_BLEND);
  glEnable(GL_LIGHTING);
  glEnable(GL_DEPTH_TEST);
}

void maskimage(bb_int handle, bb_int r, bb_int g, bb_int b) {
  if (handle < 1 || handle > (bb_int)g_images.size())
    return;
  bb_image *img = g_images[handle - 1];
  if (!img)
    return;
  img->mask_r = (int)r;
  img->mask_g = (int)g;
  img->mask_b = (int)b;
}

void freeimage(bb_int handle) {
  if (handle < 1 || handle > (bb_int)g_images.size())
    return;
  bb_image *img = g_images[handle - 1];
  if (!img)
    return;
  for (auto tex : img->textureIDs) {
    if (tex)
      glDeleteTextures(1, &tex);
  }
  delete img;
  g_images[handle - 1] = nullptr;
}

// ---------------------------------------------------------------------------
// Textures (3D)
// ---------------------------------------------------------------------------

bb_int loadtexture(bb_string file, bb_int flags) {
  int w, h, n;
  stbi_set_flip_vertically_on_load(true); // 3D usually needs flip
  unsigned char *data = stbi_load(file.c_str(), &w, &h, &n, 4);
  if (!data)
    return 0;

  GLuint tex;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, w, h, GL_RGBA, GL_UNSIGNED_BYTE,
                    data);

  stbi_image_free(data);

  bb_texture *bt = new bb_texture(tex, w, h, (int)flags);
  g_textures.push_back(bt);
  return (bb_int)g_textures.size();
}

void freetexture(bb_int handle) {
  if (handle < 1 || handle > (bb_int)g_textures.size())
    return;
  bb_texture *bt = g_textures[handle - 1];
  if (bt) {
    if (bt->textureID)
      glDeleteTextures(1, &bt->textureID);
    delete bt;
    g_textures[handle - 1] = nullptr;
  }
}

void entitytexture(bb_int ent_handle, bb_int tex_handle, bb_int frame,
                   bb_int index) {
  bb_entity *ent = lookup_entity(ent_handle);
  if (!ent || ent->type != ENTITY_MESH)
    return;

  bb_texture *bt = nullptr;
  if (tex_handle >= 1 && tex_handle <= (bb_int)g_textures.size()) {
    bt = g_textures[tex_handle - 1];
  }

  bb_mesh *mesh = static_cast<bb_mesh *>(ent);
  for (auto surf : mesh->surfaces) {
    surf->textureID = bt ? bt->textureID : 0;
  }
}
