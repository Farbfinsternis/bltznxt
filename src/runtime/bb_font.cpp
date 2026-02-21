#include "bb_globals.h"
#include "bb_types.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <fstream>
#include <iostream>
#include <vector>

// Helper to get font from handle
static bb_font *get_font(bb_int handle) {
  if (handle < 1 || handle > (bb_int)g_fonts.size())
    return nullptr;
  return g_fonts[handle - 1];
}

bb_int loadfont(bb_string file, bb_int size, bb_int bold, bb_int italic,
                bb_int underline) {
  // Read TTF file
  std::ifstream in(file, std::ios::binary | std::ios::ate);
  if (!in) {
    // Try C:\Windows\Fonts
    std::string sysFont = "C:\\Windows\\Fonts\\" + file;
    if (file.find('.') == std::string::npos)
      sysFont += ".ttf";
    in.open(sysFont, std::ios::binary | std::ios::ate);

    if (!in && file.find('.') == std::string::npos) {
      // Try .otf too? B3D usually just does TTF
    }
  }

  if (!in) {
    std::cerr << "Failed to open font: " << file << std::endl;
    return 0;
  }
  std::streamsize ttf_size = in.tellg();
  in.seekg(0, std::ios::beg);
  std::vector<unsigned char> ttf_buffer(ttf_size);
  if (!in.read((char *)ttf_buffer.data(), ttf_size))
    return 0;

  // Create font object
  bb_font *font = new bb_font();
  font->size = (int)size;

  // Bake font into a texture
  const int atlas_w = 512;
  const int atlas_h = 512;
  std::vector<unsigned char> atlas_data(atlas_w * atlas_h);

  stbtt_pack_context pc;
  if (!stbtt_PackBegin(&pc, atlas_data.data(), atlas_w, atlas_h, 0, 1, NULL)) {
    delete font;
    return 0;
  }

  stbtt_packedchar *p_chars = new stbtt_packedchar[96]; // ASCII 32..126
  if (!stbtt_PackFontRange(&pc, ttf_buffer.data(), 0, (float)size, 32, 96,
                           p_chars)) {
    stbtt_PackEnd(&pc);
    delete[] p_chars;
    delete font;
    return 0;
  }
  stbtt_PackEnd(&pc);
  font->charData = p_chars;

  // Get font metrics
  stbtt_fontinfo info;
  if (stbtt_InitFont(&info, ttf_buffer.data(), 0)) {
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);
    float scale = stbtt_ScaleForPixelHeight(&info, (float)size);
    font->ascent = ascent * scale;
    font->height = (ascent - descent + lineGap) * scale;

    // Calculate max advance
    float maxA = 0;
    stbtt_packedchar *p_chars = (stbtt_packedchar *)font->charData;
    for (int i = 0; i < 96; ++i) {
      if (p_chars[i].xadvance > maxA)
        maxA = p_chars[i].xadvance;
    }
    font->maxAdvance = maxA;
  }

  // Create GL texture
  glGenTextures(1, &font->textureID);
  glBindTexture(GL_TEXTURE_2D, font->textureID);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, atlas_w, atlas_h, 0, GL_ALPHA,
               GL_UNSIGNED_BYTE, atlas_data.data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);

  g_fonts.push_back(font);
  return (bb_int)g_fonts.size();
}

void setfont(bb_int handle) {
  bb_font *font = get_font(handle);
  if (font) {
    g_currentFont = font;
  } else {
    g_currentFont = nullptr;
  }
}

void freefont(bb_int handle) {
  if (handle < 1 || handle > (bb_int)g_fonts.size())
    return;
  bb_font *font = g_fonts[handle - 1];
  if (!font)
    return;

  if (font->textureID)
    glDeleteTextures(1, &font->textureID);
  if (font->charData)
    delete[] (stbtt_packedchar *)font->charData;

  if (g_currentFont == font)
    g_currentFont = nullptr;

  delete font;
  g_fonts[handle - 1] = nullptr;
}

void text(bb_int x, bb_int y, bb_string txt, bb_int cx, bb_int cy) {
  if (!g_currentFont || !g_currentFont->textureID)
    return;

  float fx = (float)x;
  float fy = (float)y;

  if (cx) {
    fx -= (float)stringwidth(txt) / 2.0f;
  }
  if (cy) {
    fy -= (float)stringheight(txt) / 2.0f;
  }

  // Add ascent to Y to align with Blitz3D baseline?
  // Actually Blitz3D Text(x,y) draws with (x,y) as top-left of the line box
  // usually. In stbtt, y is baseline. So we add ascent.
  fy += g_currentFont->ascent;

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, g_currentFont->textureID);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, g_width, g_height, 0, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glColor4ub(g_drawR, g_drawG, g_drawB, 255);

  stbtt_packedchar *p_chars = (stbtt_packedchar *)g_currentFont->charData;

  glBegin(GL_QUADS);
  for (char c : txt) {
    if (c >= 32 && c <= 126) {
      stbtt_aligned_quad q;
      stbtt_GetPackedQuad(p_chars, 512, 512, c - 32, &fx, &fy, &q, 1);

      glTexCoord2f(q.s0, q.t0);
      glVertex2f(q.x0, q.y0);
      glTexCoord2f(q.s1, q.t0);
      glVertex2f(q.x1, q.y0);
      glTexCoord2f(q.s1, q.t1);
      glVertex2f(q.x1, q.y1);
      glTexCoord2f(q.s0, q.t1);
      glVertex2f(q.x0, q.y1);
    }
  }
  glEnd();

  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);

  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_BLEND);
}

bb_int stringwidth(bb_string txt) {
  if (!g_currentFont || !g_currentFont->charData)
    return 0;
  stbtt_packedchar *p_chars = (stbtt_packedchar *)g_currentFont->charData;
  float w = 0;
  for (char c : txt) {
    if (c >= 32 && c <= 126) {
      w += p_chars[c - 32].xadvance;
    }
  }
  return (bb_int)w;
}

bb_int stringheight(bb_string txt) {
  if (!g_currentFont)
    return 0;
  return (bb_int)g_currentFont->height;
}

bb_int fontwidth() {
  if (!g_currentFont)
    return 0;
  return (bb_int)g_currentFont->maxAdvance;
}

bb_int fontheight() {
  if (!g_currentFont)
    return 0;
  return (bb_int)g_currentFont->height;
}

void bbFontsCleanup() {
  for (size_t i = 0; i < g_fonts.size(); ++i) {
    if (g_fonts[i]) {
      freefont((bb_int)i + 1);
    }
  }
  g_fonts.clear();
  g_currentFont = nullptr;
}
