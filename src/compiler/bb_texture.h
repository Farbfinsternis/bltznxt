#ifndef BLITZNEXT_BB_TEXTURE_H
#define BLITZNEXT_BB_TEXTURE_H

#include "bb_image.h"   // stbi_load - die Implementierung liegt dort
#include "bb_shader.h"
#include <filesystem>
#include <memory>
#include <unordered_map>
#include <vector>

// ============================================================
//  Texturen  -  bb_texture.h   (3D-11)
//
//  EntityTexture steht in 54, LoadTexture in 48 der 130 mitgelieferten
//  Beispielprogramme - beides haeufiger als CreateLight (39), das 3D-12
//  ausgeloest hat. Ohne Texturen bleibt jede dieser Szenen einfarbig.
//
//  Signaturen aus "blitzcc +k" der Originalinstallation:
//
//    LoadTexture     ( file$[,flags] )
//    LoadAnimTexture ( file$,flags,width,height,first,count )
//    CreateTexture   ( width,height[,flags][,frames] )
//    FreeTexture     texture
//    TextureWidth    ( texture ) / TextureHeight / TextureName$
//    EntityTexture   entity,texture[,frame][,index]
//    TextureBlend    texture,blend      TextureCoords texture,coords
//    ScaleTexture    texture,u_scale#,v_scale#
//    PositionTexture texture,u_offset#,v_offset#
//    RotateTexture   texture,angle#
//    TextureFilter   match_text$[,texture_flags]   ClearTextureFilters
//    TextureBuffer   ( texture[,frame] )
//    ActiveTextures  ( )   HWTexUnits ( )
//
//  Die Flags stehen in help/commands/3d_commands/CreateTexture.htm und sind
//  NICHT die aus dem Roadmap-Entwurf ("Bit0 Mipmaps, Bit1 Clamp, Bit2
//  Nearest") - jedes geladene Bild waere damit falsch behandelt worden:
//
//      1 Color (Vorgabe)    2 Alpha        4 Masked       8 Mipmapped
//     16 Clamp U           32 Clamp V     64 Sphere-Map 128 Cube-Map
//    256 VRAM             512 High-Color
//
//  Dazu die Vorgabe der Filterliste: TextureFilter "",1+8. Jede geladene
//  Textur ist also mipmapped, auch wenn LoadTexture nur Flag 1 sieht - und
//  Graphics3D stellt diese Vorgabe wieder her.
// ============================================================

// ---- Flagbits ----
inline constexpr int BB_TEX_COLOR  = 1;
inline constexpr int BB_TEX_ALPHA  = 2;
inline constexpr int BB_TEX_MASKED = 4;
inline constexpr int BB_TEX_MIPMAP = 8;
inline constexpr int BB_TEX_CLAMPU = 16;
inline constexpr int BB_TEX_CLAMPV = 32;

// EntityTexture nimmt laut Doku Index 0-7. Gemischt werden im Shader die
// ersten vier belegten Lagen; HWTexUnits() meldet genau diese Zahl.
inline constexpr int BB_TEX_SLOTS     = 8;
inline constexpr int BB_MAX_TEX_UNITS = 4;

// ============================================================
// bb_Texture_ - eine Textur mit einem oder mehreren Frames
// ============================================================

struct bb_TexFrame_ {
  GLuint               id = 0;   // GL-Objekt, 0 = noch nicht hochgeladen
  std::vector<uint8_t> px;       // RGBA, w*h*4 - bleibt liegen fuer Re-Upload
};

struct bb_Texture_ {
  int      w = 0, h = 0;
  int      flags = BB_TEX_COLOR;
  bbString name;              // absoluter Dateiname (TextureName$)
  int      blend  = 2;        // TextureBlend, Vorgabe Multiply
  int      coords = 0;        // TextureCoords
  float    uScale = 1.0f, vScale = 1.0f;
  float    uPos   = 0.0f, vPos   = 0.0f;
  float    angle  = 0.0f;     // Grad
  std::vector<bb_TexFrame_> frames;
  bool     dirty = true;      // GPU-Upload noetig

  ~bb_Texture_() {
    // Nur solange der Kontext lebt. Ein Handle, das das Programmende
    // ueberdauert, zeigte sonst in einen toten Treiber.
    if (!bb_gl_active_ || !glDeleteTextures) return;
    bb_gl_use_(); // FreeTexture kommt oft nach 2D-Zeichnen (BUG-63)
    for (auto &f : frames)
      if (f.id) { glDeleteTextures(1, &f.id); f.id = 0; }
  }
};

// FreeTexture nimmt die Textur aus der Handle-Tabelle, aber laut Doku
// "entities already textured with it will not lose the texture". Genau das
// leistet gemeinsamer Besitz: die Entity haelt ihre eigene Referenz, das
// GL-Objekt stirbt erst mit der letzten.
using bb_TexRef_ = std::shared_ptr<bb_Texture_>;

inline std::unordered_map<int, bb_TexRef_> bb_textures_;
inline int bb_texture_next_id_ = 1;

inline bb_Texture_ *bb_texture_get_(int h) {
  auto it = bb_textures_.find(h);
  return (it != bb_textures_.end()) ? it->second.get() : nullptr;
}

inline bb_TexRef_ bb_texture_ref_(int h) {
  auto it = bb_textures_.find(h);
  return (it != bb_textures_.end()) ? it->second : bb_TexRef_();
}

// ============================================================
// Texturfilter - TextureFilter / ClearTextureFilters
// ============================================================

struct bb_TexFilter_ { bbString match; int flags; };

// Vorgabe laut Doku: TextureFilter "",1+8 - Farbe und Mipmaps fuer alles.
inline std::vector<bb_TexFilter_> bb_tex_filters_ = { { "", 9 } };

inline void bb_ClearTextureFilters() { bb_tex_filters_.clear(); }

inline void bb_TextureFilter(const bbString &match_text, int texture_flags = 1) {
  bb_tex_filters_.push_back({ match_text, texture_flags });
}

// Graphics3D stellt die Vorgabe wieder her ("setting the graphics mode
// restores the default texture filters").
inline void bb_texture_gfxreset_() { bb_tex_filters_ = { { "", 9 } }; }

extern void (*bb_texture_gfxreset_hook_)();
inline const bool bb_texture_gfxreset_reg_ =
    (bb_texture_gfxreset_hook_ = bb_texture_gfxreset_, true);

// Passende Filter zu den angegebenen Flags hinzunehmen. Der Vergleich achtet
// nicht auf Gross- und Kleinschreibung - bei Dateinamen tut das keine der
// beiden Zielplattformen zuverlaessig.
inline int bb_tex_apply_filters_(const bbString &file, int flags) {
  bbString lf = file;
  for (auto &c : lf) c = static_cast<char>(::tolower((unsigned char)c));
  for (const auto &f : bb_tex_filters_) {
    bbString m = f.match;
    for (auto &c : m) c = static_cast<char>(::tolower((unsigned char)c));
    if (m.empty() || lf.find(m) != bbString::npos) flags |= f.flags;
  }
  return flags;
}

// ============================================================
// Laden und Erzeugen
// ============================================================

// Alphakanal nach den Flags herrichten. Ohne Flag 2 gilt "what you see is
// what you get", also volle Deckkraft; mit Flag 2 und einer Vorlage ohne
// eigenen Alphakanal dient laut Doku die Helligkeit als Alphamaske.
inline void bb_tex_fix_alpha_(std::vector<uint8_t> &px, int flags, int src_channels) {
  if (!(flags & BB_TEX_ALPHA)) {
    for (size_t i = 3; i < px.size(); i += 4) px[i] = 255;
  } else if (src_channels < 4) {
    for (size_t i = 0; i + 3 < px.size(); i += 4) {
      int lum = (px[i] * 30 + px[i + 1] * 59 + px[i + 2] * 11) / 100;
      px[i + 3] = static_cast<uint8_t>(lum);
    }
  }
}

inline bbString bb_tex_abs_path_(const bbString &file) {
  std::error_code ec;
  auto p = std::filesystem::absolute(std::filesystem::path(file), ec);
  if (ec) return file;
  return p.string();
}

inline int bb_texture_register_(bb_TexRef_ t) {
  int h = bb_texture_next_id_++;
  bb_textures_[h] = std::move(t);
  return h;
}

inline int bb_LoadTexture(const bbString &file, int flags = 1) {
  int w = 0, h = 0, ch = 0;
  unsigned char *data = bb_load_rgba_(file.c_str(), &w, &h, &ch);
  if (!data) {
    std::cerr << "[runtime] LoadTexture: cannot load '" << file << "'\n";
    return 0;
  }

  auto t   = std::make_shared<bb_Texture_>();
  t->w     = w;
  t->h     = h;
  t->flags = bb_tex_apply_filters_(file, flags);
  t->name  = bb_tex_abs_path_(file);

  bb_TexFrame_ f;
  f.px.assign(data, data + static_cast<size_t>(w) * h * 4);
  stbi_image_free(data);
  bb_tex_fix_alpha_(f.px, t->flags, ch);
  t->frames.push_back(std::move(f));

  return bb_texture_register_(std::move(t));
}

// Die Frames liegen links-nach-rechts, oben-nach-unten im Bild; "first" ist
// der erste genutzte, "count" die Anzahl.
inline int bb_LoadAnimTexture(const bbString &file, int flags, int width,
                              int height, int first, int count) {
  if (width <= 0 || height <= 0 || count <= 0) return 0;

  int w = 0, h = 0, ch = 0;
  unsigned char *data = bb_load_rgba_(file.c_str(), &w, &h, &ch);
  if (!data) {
    std::cerr << "[runtime] LoadAnimTexture: cannot load '" << file << "'\n";
    return 0;
  }

  int cols = w / width;
  int rows = h / height;
  if (cols <= 0 || rows <= 0) {
    std::cerr << "[runtime] LoadAnimTexture: '" << file << "' ist " << w << "x" << h
              << ", zu klein fuer Frames von " << width << "x" << height << "\n";
    stbi_image_free(data);
    return 0;
  }

  auto t   = std::make_shared<bb_Texture_>();
  t->w     = width;
  t->h     = height;
  t->flags = bb_tex_apply_filters_(file, flags);
  t->name  = bb_tex_abs_path_(file);

  for (int i = 0; i < count; ++i) {
    int idx = first + i;
    bb_TexFrame_ f;
    f.px.assign(static_cast<size_t>(width) * height * 4, 0);
    if (idx >= 0 && idx < cols * rows) {
      int ox = (idx % cols) * width;
      int oy = (idx / cols) * height;
      for (int y = 0; y < height; ++y)
        std::memcpy(&f.px[static_cast<size_t>(y) * width * 4],
                    data + ((static_cast<size_t>(oy + y) * w + ox) * 4),
                    static_cast<size_t>(width) * 4);
    }
    bb_tex_fix_alpha_(f.px, t->flags, ch);
    t->frames.push_back(std::move(f));
  }
  stbi_image_free(data);

  return bb_texture_register_(std::move(t));
}

inline int bb_CreateTexture(int width, int height, int flags = 1, int frames = 1) {
  if (width <= 0 || height <= 0 || frames < 1) return 0;

  auto t   = std::make_shared<bb_Texture_>();
  t->w     = width;
  t->h     = height;
  t->flags = flags;              // CreateTexture geht nicht durch die Filter

  for (int i = 0; i < frames; ++i) {
    bb_TexFrame_ f;
    f.px.assign(static_cast<size_t>(width) * height * 4, 0);
    for (size_t p = 3; p < f.px.size(); p += 4) f.px[p] = 255;  // deckendes Schwarz
    t->frames.push_back(std::move(f));
  }

  return bb_texture_register_(std::move(t));
}

inline void bb_FreeTexture(int texture) { bb_textures_.erase(texture); }

// ============================================================
// Abfragen
// ============================================================

// Das Original meldet hier die tatsaechliche Groesse, die vom Wunsch
// abweichen darf (Zweierpotenzen alter Hardware). GL 3.3 nimmt jede Groesse,
// also ist die tatsaechliche gleich der gewuenschten.
inline int bb_TextureWidth(int texture) {
  auto *t = bb_texture_get_(texture);
  return t ? t->w : 0;
}

inline int bb_TextureHeight(int texture) {
  auto *t = bb_texture_get_(texture);
  return t ? t->h : 0;
}

inline bbString bb_TextureName(int texture) {
  auto *t = bb_texture_get_(texture);
  return t ? t->name : bbString();
}

inline int bb_ActiveTextures() { return static_cast<int>(bb_textures_.size()); }

// Wieviele Lagen der Shader wirklich mischt. Eine groessere Zahl zu melden
// waere eine Zusage, die das Bild nicht einloest.
inline int bb_HWTexUnits() { return BB_MAX_TEX_UNITS; }

// ============================================================
// UV-Transformation
//
//  Am laufenden Original ausgemessen - abgelesen ueber ReadPixel statt am
//  Bild, damit die Antwort eine Zahl ist und keine Einschaetzung:
//
//      u' = ( cos a * u - sin a * v ) / u_scale - u_offset
//      v' = ( sin a * u + cos a * v ) / v_scale - v_offset
//
//  Drei Dinge, die man ohne diese Messung falsch gemacht haette:
//  * ScaleTexture *teilt* die Koordinaten. "ScaleTexture t,2,2" zeigt einen
//    Ausschnitt (Textur wirkt doppelt so gross), nicht zwei Kacheln.
//  * PositionTexture *zieht ab*; 0.25 schiebt das Bild in Richtung +u.
//  * RotateTexture dreht um den Ursprung (0,0), nicht um die Mitte - bei 90
//    und 180 Grad ist das nicht zu unterscheiden, bei 45 schon.
//  Die Reihenfolge ist Drehung, dann Skalierung, dann Verschiebung.
// ============================================================

inline void bb_ScaleTexture(int texture, float u_scale, float v_scale) {
  if (auto *t = bb_texture_get_(texture)) { t->uScale = u_scale; t->vScale = v_scale; }
}

inline void bb_PositionTexture(int texture, float u_offset, float v_offset) {
  if (auto *t = bb_texture_get_(texture)) { t->uPos = u_offset; t->vPos = v_offset; }
}

inline void bb_RotateTexture(int texture, float angle) {
  if (auto *t = bb_texture_get_(texture)) t->angle = angle;
}

inline void bb_TextureBlend(int texture, int blend) {
  if (auto *t = bb_texture_get_(texture)) t->blend = blend;
}

// Es gibt bei uns nur einen UV-Satz je Vertex; der zweite kommt mit den
// Vertexbefehlen (3D-15). Der Wert wird gemerkt, damit er nicht verloren geht.
inline void bb_TextureCoords(int texture, int coords) {
  if (auto *t = bb_texture_get_(texture)) t->coords = coords;
}

// Spaltenweise 3x3-Matrix fuer den Shader.
inline void bb_texture_matrix_(const bb_Texture_ *t, float *m) {
  float a  = t->angle * 3.14159265358979f / 180.0f;
  float c  = std::cos(a), s = std::sin(a);
  float su = (t->uScale != 0.0f) ? t->uScale : 1.0f;
  float sv = (t->vScale != 0.0f) ? t->vScale : 1.0f;
  // Spalte 0: Beitrag von u, Spalte 1: von v, Spalte 2: die Verschiebung.
  m[0] =  c / su;  m[1] =  s / sv;  m[2] = 0.0f;
  m[3] = -s / su;  m[4] =  c / sv;  m[5] = 0.0f;
  m[6] = -t->uPos; m[7] = -t->vPos; m[8] = 1.0f;
}

// ============================================================
// GPU-Upload
// ============================================================

inline void bb_texture_upload_(bb_Texture_ *t) {
  if (!t || !bb_gl_active_ || !glGenTextures) return;

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  for (auto &f : t->frames) {
    if (f.px.empty()) continue;
    if (!f.id) glGenTextures(1, &f.id);
    glBindTexture(GL_TEXTURE_2D, f.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->w, t->h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, f.px.data());

    const bool mip = (t->flags & BB_TEX_MIPMAP) != 0;
    if (mip && glGenerateMipmap) glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    mip ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                    (t->flags & BB_TEX_CLAMPU) ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                    (t->flags & BB_TEX_CLAMPV) ? GL_CLAMP_TO_EDGE : GL_REPEAT);
  }
  glBindTexture(GL_TEXTURE_2D, 0);
  t->dirty = false;
}

// ============================================================
// Bindung an eine Entity
// ============================================================

struct bb_TexSlots_ {
  bb_TexRef_ tex[BB_TEX_SLOTS];
  int        frame[BB_TEX_SLOTS] = {};

  bool any() const {
    for (int i = 0; i < BB_TEX_SLOTS; ++i)
      if (tex[i]) return true;
    return false;
  }
};

// Legt die belegten Lagen auf die Texturkanaele und schreibt die Uniforms.
// Rueckgabe: true, wenn eine Lage Alpha mitbringt und der Aufrufer deshalb
// Blending einschalten sollte.
inline bool bb_texture_bind_(bb_Shader_ *sh, const bb_TexSlots_ &slots) {
  int   n = 0;
  int   blend[BB_MAX_TEX_UNITS]     = {};
  int   flags[BB_MAX_TEX_UNITS]     = {};
  float mats[BB_MAX_TEX_UNITS * 9]  = {};
  bool  needs_blend = false;
  static const char *unit_name[BB_MAX_TEX_UNITS] = { "u_tex0", "u_tex1",
                                                     "u_tex2", "u_tex3" };

  for (int i = 0; i < BB_TEX_SLOTS && n < BB_MAX_TEX_UNITS; ++i) {
    bb_Texture_ *t = slots.tex[i].get();
    if (!t) continue;
    if (t->blend == 0) continue;          // 0 = "do not blend", Lage ist aus
    if (t->dirty) bb_texture_upload_(t);
    if (t->frames.empty()) continue;

    int fi = slots.frame[i];
    if (fi < 0 || fi >= static_cast<int>(t->frames.size())) fi = 0;
    GLuint id = t->frames[fi].id;
    if (!id) continue;

    glActiveTexture(GL_TEXTURE0 + n);
    glBindTexture(GL_TEXTURE_2D, id);
    bb_shader_uniform_i(sh, unit_name[n], n);

    blend[n] = t->blend;
    flags[n] = t->flags;
    bb_texture_matrix_(t, &mats[n * 9]);
    if (t->flags & BB_TEX_ALPHA) needs_blend = true;
    ++n;
  }

  bb_shader_uniform_i(sh, "u_tex_count", n);
  if (n > 0) {
    bb_shader_uniform_iv (sh, "u_tex_blend", n, blend);
    bb_shader_uniform_iv (sh, "u_tex_flags", n, flags);
    bb_shader_uniform_m3v(sh, "u_tex_mat",   n, mats);
  }
  glActiveTexture(GL_TEXTURE0);
  return needs_blend;
}

// ============================================================
// Aufraeumen
// ============================================================

inline void bb_texture_quit_() {
  bb_textures_.clear();
  bb_texture_next_id_ = 1;
  bb_tex_filters_     = { { "", 9 } };
}

extern void (*bb_texture_quit_hook_)();
inline const bool bb_texture_hook_reg_ =
    (bb_texture_quit_hook_ = bb_texture_quit_, true);

// ============================================================
// Noch nicht eingeloest
// ============================================================

// TextureBuffer setzt 2D-Zeichnen in eine Textur voraus (SetBuffer, Cls,
// Text). Das kommt mit dem Pixelzugriff; bis dahin ist 0 die ehrliche
// Antwort - ein erfundenes Pufferhandle wuerde still in ein fremdes Bild
// zeichnen.
inline int bb_TextureBuffer(int /*texture*/, int /*frame*/ = 0) {
  static bool warned = false;
  if (!warned) {
    warned = true;
    std::cerr << "[runtime] TextureBuffer: Zeichnen in Texturen ist noch nicht "
                 "umgesetzt - liefert 0 (3D-11)\n";
  }
  return 0;
}

// Wuerfelumgebungskarten (Flag 128) mischt der Shader nicht.
inline void bb_SetCubeFace(int /*texture*/, int /*face*/) {}
inline void bb_SetCubeMode(int /*texture*/, int /*mode*/) {}

#endif // BLITZNEXT_BB_TEXTURE_H
