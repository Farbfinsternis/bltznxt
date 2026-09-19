#ifndef BB_SHADER_H
#define BB_SHADER_H

// Shader Infrastructure (3D-07).
//
// Three built-in GLSL 3.30 Core shaders compiled once on the first RenderWorld call:
//
//   UNLIT    — solid u_color, no lighting
//   TEXTURED — u_color durch bis zu vier Texturlagen (3D-11), kein Licht;
//              bei u_tex_count == 0 bleibt genau u_color uebrig, das Bild ist
//              dann dasselbe wie mit UNLIT
//   LIT      — Blinn-Phong, up to 8 lights, dieselben Texturlagen;
//              degrades to ambient-only when u_light_count == 0
//
// TEXTURED und LIT teilen sich den Texturteil: BB_GLSL_TEX_VERT und
// BB_GLSL_TEX_FRAG werden in bb_shaders_init_ zwischen Kopf und main() gesetzt,
// damit es die Texturbehandlung nur einmal gibt.
//
// Shared vertex attribute layout (matches 3D-08 interleaved mesh format):
//   location 0 — vec3  a_pos    (x, y, z)
//   location 1 — vec3  a_normal (nx, ny, nz)
//   location 2 — vec2  a_uv    (u, v)
//   location 3 — vec4  a_color (r, g, b, a) — EntityFX Bit 2 und 32 (3D-15)

#include "bb_gl_ctx.h"
#include <iostream>
#include <string>
#include <unordered_map>

// ============================================================
// bb_Shader_ — compiled GL program + cached uniform locations
// ============================================================

struct bb_Shader_ {
  GLuint program = 0;
  std::unordered_map<std::string, GLint> loc_cache;

  // Returns the uniform location (cached after first query).  -1 = not found.
  GLint loc(const char* name) {
    auto it = loc_cache.find(name);
    if (it != loc_cache.end()) return it->second;
    GLint l = glGetUniformLocation(program, name);
    loc_cache[name] = l;
    return l;
  }
};

// ============================================================
// Compile helper
// ============================================================

static inline bool bb_shader_check_(GLuint obj, bool is_prog) {
  GLint ok = GL_FALSE;
  if (is_prog) glGetProgramiv(obj, GL_LINK_STATUS,    &ok);
  else         glGetShaderiv (obj, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    GLint len = 0;
    if (is_prog) glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &len);
    else         glGetShaderiv (obj, GL_INFO_LOG_LENGTH, &len);
    if (len > 1) {
      std::string log(static_cast<size_t>(len), '\0');
      if (is_prog) glGetProgramInfoLog(obj, len, nullptr, &log[0]);
      else         glGetShaderInfoLog (obj, len, nullptr, &log[0]);
      std::cerr << "[shader] " << (is_prog ? "link" : "compile") << " error:\n" << log << "\n";
    }
    return false;
  }
  return true;
}

// Compile vertex + fragment GLSL source into a heap-allocated bb_Shader_.
// Returns nullptr on failure (errors printed to stderr).
inline bb_Shader_* bb_shader_compile_(const char* vert_src, const char* frag_src) {
  GLuint vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs, 1, &vert_src, nullptr);
  glCompileShader(vs);
  if (!bb_shader_check_(vs, false)) { glDeleteShader(vs); return nullptr; }

  GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs, 1, &frag_src, nullptr);
  glCompileShader(fs);
  if (!bb_shader_check_(fs, false)) { glDeleteShader(vs); glDeleteShader(fs); return nullptr; }

  GLuint prog = glCreateProgram();
  glAttachShader(prog, vs);
  glAttachShader(prog, fs);
  glLinkProgram(prog);
  glDetachShader(prog, vs); glDetachShader(prog, fs);
  glDeleteShader(vs);       glDeleteShader(fs);
  if (!bb_shader_check_(prog, true)) { glDeleteProgram(prog); return nullptr; }

  auto* s = new bb_Shader_();
  s->program = prog;
  return s;
}

// ============================================================
// Embedded GLSL 3.30 Core sources
// ============================================================

// ---- UNLIT ----

static constexpr const char* BB_GLSL_UNLIT_VERT = R"glsl(
#version 330 core
layout(location = 0) in vec3 a_pos;
uniform mat4 u_mvp;
void main() {
    gl_Position = u_mvp * vec4(a_pos, 1.0);
}
)glsl";

static constexpr const char* BB_GLSL_UNLIT_FRAG = R"glsl(
#version 330 core
uniform vec4 u_color;
out vec4 frag_color;
void main() {
    frag_color = u_color;
}
)glsl";

// ---- Gemeinsamer Texturteil (3D-11) ----
//
// Bis zu vier Lagen, wie EntityTexture sie ueber die Indizes 0-7 belegt und
// bb_texture_bind_ zusammenschiebt. Jede Lage bringt ihre eigene
// UV-Matrix (ScaleTexture/PositionTexture/RotateTexture), ihren Blendmodus
// (TextureBlend) und ihre Ladeflags mit.
//
// Beide Bausteine werden in bb_shaders_init_ vor das jeweilige main() gesetzt,
// damit TEXTURED und LIT nachweislich dieselbe Texturbehandlung haben statt
// zwei Kopien, die auseinanderlaufen koennen.

static constexpr const char* BB_GLSL_TEX_VERT = R"glsl(
uniform mat3 u_tex_mat[4];
out vec2 v_uv0;
out vec2 v_uv1;
out vec2 v_uv2;
out vec2 v_uv3;
void bb_tex_vert(vec2 uv) {
    vec3 h = vec3(uv, 1.0);
    v_uv0 = (u_tex_mat[0] * h).xy;
    v_uv1 = (u_tex_mat[1] * h).xy;
    v_uv2 = (u_tex_mat[2] * h).xy;
    v_uv3 = (u_tex_mat[3] * h).xy;
}
)glsl";

static constexpr const char* BB_GLSL_TEX_FRAG = R"glsl(
in vec2 v_uv0;
in vec2 v_uv1;
in vec2 v_uv2;
in vec2 v_uv3;
uniform sampler2D u_tex0;
uniform sampler2D u_tex1;
uniform sampler2D u_tex2;
uniform sampler2D u_tex3;
uniform int u_tex_count;
uniform int u_tex_blend[4];
uniform int u_tex_flags[4];

// Blendmodi laut TextureBlend.htm: 1 = kein Blend bzw. Alpha, 2 = Multiply
// (Vorgabe), 3 = Add, 4 = Dot3, 5 = Multiply 2. Dot3 braucht eine
// Lichtrichtung im Tangentenraum, die es hier nicht gibt - es faellt deshalb
// bewusst auf Multiply zurueck, statt etwas Aehnlichsehendes zu erfinden.
vec4 bb_tex_layer(vec4 c, vec4 t, int blend, int flags) {
    if ((flags & 2) == 0) t.a = 1.0;
    if (blend == 1) return vec4(mix(c.rgb, t.rgb, t.a), c.a * t.a);
    if (blend == 3) return vec4(c.rgb + t.rgb, c.a);
    if (blend == 5) return vec4(c.rgb * t.rgb * 2.0, c.a * t.a);
    return c * t;
}

// Flag 4 (Masked): "all areas of a texture coloured 0,0,0 will not be drawn".
vec4 bb_tex_apply(vec4 c) {
    if (u_tex_count > 0) {
        vec4 t = texture(u_tex0, v_uv0);
        if ((u_tex_flags[0] & 4) != 0 && all(lessThan(t.rgb, vec3(0.02)))) discard;
        c = bb_tex_layer(c, t, u_tex_blend[0], u_tex_flags[0]);
    }
    if (u_tex_count > 1) {
        vec4 t = texture(u_tex1, v_uv1);
        if ((u_tex_flags[1] & 4) != 0 && all(lessThan(t.rgb, vec3(0.02)))) discard;
        c = bb_tex_layer(c, t, u_tex_blend[1], u_tex_flags[1]);
    }
    if (u_tex_count > 2) {
        vec4 t = texture(u_tex2, v_uv2);
        if ((u_tex_flags[2] & 4) != 0 && all(lessThan(t.rgb, vec3(0.02)))) discard;
        c = bb_tex_layer(c, t, u_tex_blend[2], u_tex_flags[2]);
    }
    if (u_tex_count > 3) {
        vec4 t = texture(u_tex3, v_uv3);
        if ((u_tex_flags[3] & 4) != 0 && all(lessThan(t.rgb, vec3(0.02)))) discard;
        c = bb_tex_layer(c, t, u_tex_blend[3], u_tex_flags[3]);
    }
    return c;
}
)glsl";

// ---- TEXTURED ----
//
// Ohne Licht zeichnet RenderWorld mit diesem Shader. Bei u_tex_count == 0
// bleibt genau u_color uebrig, das Bild ist also dasselbe wie mit UNLIT.

static constexpr const char* BB_GLSL_TEXTURED_VERT = R"glsl(
#version 330 core
layout(location = 0) in vec3 a_pos;
layout(location = 2) in vec2 a_uv;
layout(location = 3) in vec4 a_color;
uniform mat4 u_mvp;
out vec4 v_color;
)glsl";

static constexpr const char* BB_GLSL_TEXTURED_VERT_MAIN = R"glsl(
void main() {
    v_color = a_color;
    bb_tex_vert(a_uv);
    gl_Position = u_mvp * vec4(a_pos, 1.0);
}
)glsl";

static constexpr const char* BB_GLSL_TEXTURED_FRAG = R"glsl(
#version 330 core
uniform vec4 u_color;
uniform int  u_fx;
in vec4 v_color;
)glsl";

static constexpr const char* BB_GLSL_TEXTURED_FRAG_MAIN = R"glsl(
out vec4 frag_color;
void main() {
    // FX 2: Vertexfarbe statt Entityfarbe, FX 32: Vertexalpha dazu.
    vec4 base = u_color;
    if ((u_fx & 2)  != 0) base.rgb = v_color.rgb;
    if ((u_fx & 32) != 0) base.a  *= v_color.a;
    frag_color = clamp(bb_tex_apply(base), 0.0, 1.0);
}
)glsl";

// ---- LIT (Blinn-Phong, up to 8 lights) ----
//
// Light types: 0 = directional (u_light_pos is direction, pointing away from surface),
//              1 = point       (u_light_pos is world position, u_light_range = LightRange;
//                               attenuation range/distance like Direct3D, BUG-91)
//              2 = spot        (like point, plus the cone)
//
// Directional diffuse is computed per fragment; point/spot diffuse and all
// specular per vertex, as the original's fixed pipeline does (BUG-66, BUG-91).
//
// All colour uniforms (u_ambient, u_light_color) are pre-normalised to [0, 1] by the caller.
// Normal transform uses mat3(u_model): correct for rotation and uniform-scale transforms.
// When u_light_count == 0 the loop is skipped and the result is ambient-only (= effective unlit).

static constexpr const char* BB_GLSL_LIT_VERT = R"glsl(
#version 330 core
layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;
layout(location = 3) in vec4 a_color;
uniform mat4 u_mvp;
uniform mat4 u_model;
out vec3 v_pos;
out vec3 v_normal;
// EntityFX 4 (flatshaded) braucht dieselbe Normale ohne Interpolation.
flat out vec3 v_normal_flat;
out vec4 v_color;

// Je Vertex (BUG-66, BUG-91): das Glanzlicht aller Lichter und das diffuse
// Licht von Punkt- und Spotlichtern.
uniform float u_shininess;
uniform vec3  u_view_pos;
uniform int   u_light_count;
uniform vec3  u_light_pos[8];
uniform vec3  u_light_color[8];
uniform float u_light_range[8];
uniform int   u_light_type[8];
// Spot (Typ 2): Kegelachse und die beiden Winkel als Kosinus des HALBwinkels.
uniform vec3  u_light_dir[8];
uniform float u_light_cos_inner[8];
uniform float u_light_cos_outer[8];
out vec3 v_spec;
flat out vec3 v_spec_flat;
out vec3 v_diff;
flat out vec3 v_diff_flat;
)glsl";

static constexpr const char* BB_GLSL_LIT_VERT_MAIN = R"glsl(
void main() {
    vec4 wp  = u_model * vec4(a_pos, 1.0);
    v_pos    = wp.xyz;
    v_normal = mat3(u_model) * a_normal;
    v_normal_flat = v_normal;
    v_color  = a_color;
    bb_tex_vert(a_uv);
    gl_Position = u_mvp * vec4(a_pos, 1.0);

    // Das Original rechnet das Licht in der festen Direct3D-7-Pipeline je
    // Vertex und interpoliert es. Je Vertex stehen hier:
    //
    // - das Glanzlicht aller Lichter (BUG-66). Das Material setzt gxScene aus
    //   EntityShininess s: Staerke min(s,1), Exponent s*128; das Licht hat
    //   immer weisses Glanzlicht (gxLight: dcvSpecular = 1, LightColor schreibt
    //   nur dcvDiffuse). Der Exponent ist bei 128 gedeckelt - gemessen: s=2
    //   sieht aus wie s=1. Das Ergebnis wird nach Textur und Farbe ADDIERT.
    //
    // - das diffuse Licht von Punkt- und Spotlichtern (BUG-91). gxLight setzt
    //   dvAttenuation1 = 1/range und laesst die beiden anderen Faktoren auf 0,
    //   Direct3D teilt also durch d/range: Abschwaechung range/Abstand, nach
    //   oben offen. Bis dahin stand hier 1 - Abstand/range, je Bildpunkt -
    //   gemessen dunkler bis schwarz (Wuerfel bei range 2: 0 statt 55), und auf
    //   grossen Flaechen ein Lichtfleck, wo das Original die weit entfernten
    //   Ecken interpoliert und gleichmaessig dunkel bleibt (16 statt 255).
    //
    // Das diffuse Licht eines Richtungslichts bleibt im Fragment; auf ebenen
    // Flaechen ist das dasselbe.
    vec3 spec  = vec3(0.0);
    vec3 diffv = vec3(0.0);
    vec3  N     = normalize(v_normal);
    vec3  V     = normalize(u_view_pos - wp.xyz);
    float power = min(u_shininess * 128.0, 128.0);
    float t     = min(u_shininess, 1.0);
    for (int i = 0; i < u_light_count; ++i) {
        vec3  L;
        float atten = 1.0;
        if (u_light_type[i] == 0) {
            L = normalize(u_light_pos[i]);
        } else {
            vec3  delta = u_light_pos[i] - wp.xyz;
            float dist  = length(delta);
            L = (dist > 0.0001) ? delta / dist : vec3(0.0, 1.0, 0.0);
            float r = u_light_range[i];
            atten = (r > 0.0) ? r / max(dist, 0.0001) : 1.0;
            if (u_light_type[i] == 2) {
                // Spot: Winkel zwischen Kegelachse und der Richtung zur
                // Oberflaeche. Innerhalb des inneren Winkels volle Helligkeit,
                // dazwischen weicher Uebergang, ausserhalb nichts.
                float cd = dot(normalize(u_light_dir[i]), -L);
                float ci = u_light_cos_inner[i];
                float co = u_light_cos_outer[i];
                atten *= (ci > co) ? clamp((cd - co) / (ci - co), 0.0, 1.0)
                                   : step(co, cd);
            }
            diffv += max(dot(N, L), 0.0) * u_light_color[i] * atten;
        }
        if (u_shininess > 0.001) {
            vec3 H = normalize(L + V);
            spec += vec3(t * pow(max(dot(N, H), 0.0), power) * atten);
        }
    }
    v_spec      = spec;
    v_spec_flat = spec;
    v_diff      = diffv;
    v_diff_flat = diffv;
}
)glsl";

static constexpr const char* BB_GLSL_LIT_FRAG = R"glsl(
#version 330 core
in vec3 v_pos;
in vec3 v_normal;

uniform vec4  u_color;
uniform vec3  u_ambient;

// Hier nur noch fuer Richtungslichter gebraucht; Punkt und Spot rechnet der
// Vertex-Shader (BUG-91).
uniform int   u_light_count;
uniform vec3  u_light_pos[8];
uniform vec3  u_light_color[8];
uniform int   u_light_type[8];

uniform int   u_fx;          // EntityFX (3D-10)
flat in vec3  v_normal_flat;
in vec4       v_color;
in vec3       v_spec;        // Glanzlicht je Vertex (BUG-66)
flat in vec3  v_spec_flat;
in vec3       v_diff;        // Punkt- und Spotlicht je Vertex (BUG-91)
flat in vec3  v_diff_flat;
)glsl";

static constexpr const char* BB_GLSL_LIT_FRAG_MAIN = R"glsl(
out vec4 frag_color;

void main() {
    // FX 2: Vertexfarbe statt Entityfarbe, FX 32: Vertexalpha dazu. Das
    // Original nennt Bit 32 FX_VERTEXALPHA (blitz3d/brush.cpp) - bis 3D-15
    // hat es bei uns nur das Blending eingeschaltet, weil es gar keine
    // Vertexalpha gab.
    vec4 base = u_color;
    if ((u_fx & 2)  != 0) base.rgb = v_color.rgb;
    if ((u_fx & 32) != 0) base.a  *= v_color.a;

    // EntityFX 1 (full-bright): weder Lichter noch Umgebungslicht. Gemessen:
    // die Flaeche zeigt genau die Entityfarbe, auch bei gesetztem
    // AmbientLight.
    if ((u_fx & 1) != 0) {
        frag_color = clamp(bb_tex_apply(base), 0.0, 1.0);
        return;
    }

    // EntityFX 4 (flatshaded): dieselbe Normale fuer das ganze Dreieck.
    vec3 N      = normalize(((u_fx & 4) != 0) ? v_normal_flat : v_normal);
    vec3 result = u_ambient + (((u_fx & 4) != 0) ? v_diff_flat : v_diff);

    // Richtungslichter: u_light_pos ist die Richtung zum Licht.
    for (int i = 0; i < u_light_count; ++i) {
        if (u_light_type[i] != 0) continue; // Punkt/Spot: v_diff (BUG-91)
        vec3 L = normalize(u_light_pos[i]);
        result += max(dot(N, L), 0.0) * u_light_color[i];
    }

    // Geklemmt wird **nach** der Multiplikation mit der Entityfarbe, nicht
    // davor. Am Original gemessen: Farbe 255,128,0 mit einem vollen Licht und
    // Umgebungslicht 64,32,16 ergibt 255,144,0 - der Gruenanteil steigt also
    // ueber 128 hinaus. Mit der umgekehrten Reihenfolge waere es 128
    // geblieben.
    //
    // Die Texturen kommen erst **danach**, wie in der festen Pipeline: dort
    // ist das beleuchtete Ergebnis die Vertexfarbe, schon auf 1 begrenzt, und
    // die Texturstufen verrechnen sie mit der Textur. Am Original gemessen
    // (BUG-173): Textur 100,150,200 unter Umgebungslicht 128 plus vollem
    // Richtungslicht bleibt 100,150,200; bei uns wurde sie 150,225,255, weil
    // die Textur vor dem Begrenzen mit dem Licht multipliziert wurde. In der
    // BirdDemo waren besonnte Flaechen so 13 % zu hell.
    //
    // Das Glanzlicht kommt aus dem Vertex-Shader und wird danach addiert, wie
    // die feste Pipeline es nach den Texturstufen tut (BUG-66). Bis dahin
    // rechnete dieses Fragment es selbst, je Bildpunkt und mit der Entityfarbe
    // multipliziert: Wuerfel 64,64,64 bei Shininess 1 gab 128 statt 75.
    vec4 lit  = bb_tex_apply(vec4(clamp(result * base.rgb, 0.0, 1.0), base.a));
    vec3 spec = ((u_fx & 4) != 0) ? v_spec_flat : v_spec;
    frag_color = vec4(clamp(lit.rgb + spec, 0.0, 1.0), lit.a);
}
)glsl";

// ============================================================
// Global shader instances
// ============================================================

inline bb_Shader_* bb_shader_unlit_    = nullptr;
inline bb_Shader_* bb_shader_textured_ = nullptr;
inline bb_Shader_* bb_shader_lit_      = nullptr;
inline bb_Shader_* bb_shader_active_   = nullptr;
inline bool        bb_shaders_ready_   = false;

// ============================================================
// Bind
// ============================================================

inline void bb_shader_bind_(bb_Shader_* s) {
  if (s == bb_shader_active_) return;
  bb_shader_active_ = s;
  glUseProgram(s ? s->program : 0);
}

// ============================================================
// Uniform setters
// ============================================================

inline void bb_shader_uniform_i(bb_Shader_* s, const char* n, int v) {
  GLint l = s->loc(n); if (l >= 0) glUniform1i(l, v);
}
inline void bb_shader_uniform_f(bb_Shader_* s, const char* n, float v) {
  GLint l = s->loc(n); if (l >= 0) glUniform1f(l, v);
}
inline void bb_shader_uniform_v3(bb_Shader_* s, const char* n,
                                  float x, float y, float z) {
  GLint l = s->loc(n); if (l >= 0) glUniform3f(l, x, y, z);
}
inline void bb_shader_uniform_v4(bb_Shader_* s, const char* n,
                                  float x, float y, float z, float w) {
  GLint l = s->loc(n); if (l >= 0) glUniform4f(l, x, y, z, w);
}
inline void bb_shader_uniform_mat4(bb_Shader_* s, const char* n, const float* m) {
  GLint l = s->loc(n); if (l >= 0) glUniformMatrix4fv(l, 1, GL_FALSE, m);
}
inline void bb_shader_uniform_mat3(bb_Shader_* s, const char* n, const float* m) {
  GLint l = s->loc(n); if (l >= 0) glUniformMatrix3fv(l, 1, GL_FALSE, m);
}

// Array variants — for uploading light arrays in RenderWorld (3D-12).
inline void bb_shader_uniform_v3v(bb_Shader_* s, const char* n,
                                   int count, const float* v) {
  GLint l = s->loc(n); if (l >= 0) glUniform3fv(l, count, v);
}
inline void bb_shader_uniform_fv(bb_Shader_* s, const char* n,
                                  int count, const float* v) {
  GLint l = s->loc(n); if (l >= 0) glUniform1fv(l, count, v);
}
inline void bb_shader_uniform_iv(bb_Shader_* s, const char* n,
                                  int count, const int* v) {
  GLint l = s->loc(n); if (l >= 0) glUniform1iv(l, count, v);
}
inline void bb_shader_uniform_m3v(bb_Shader_* s, const char* n,
                                   int count, const float* v) {
  GLint l = s->loc(n); if (l >= 0) glUniformMatrix3fv(l, count, GL_FALSE, v);
}

// ============================================================
// Init (lazy — called once from bb_RenderWorld on the first frame)
// ============================================================

inline void bb_shaders_init_() {
  // Der gemeinsame Texturteil (3D-11) wird zwischen Kopf und main() gesetzt.
  auto join = [](const char* head, const char* common, const char* body) {
    return std::string(head) + common + body;
  };
  const std::string tex_vert = join(BB_GLSL_TEXTURED_VERT, BB_GLSL_TEX_VERT,
                                    BB_GLSL_TEXTURED_VERT_MAIN);
  const std::string tex_frag = join(BB_GLSL_TEXTURED_FRAG, BB_GLSL_TEX_FRAG,
                                    BB_GLSL_TEXTURED_FRAG_MAIN);
  const std::string lit_vert = join(BB_GLSL_LIT_VERT, BB_GLSL_TEX_VERT,
                                    BB_GLSL_LIT_VERT_MAIN);
  const std::string lit_frag = join(BB_GLSL_LIT_FRAG, BB_GLSL_TEX_FRAG,
                                    BB_GLSL_LIT_FRAG_MAIN);

  bb_shader_unlit_    = bb_shader_compile_(BB_GLSL_UNLIT_VERT, BB_GLSL_UNLIT_FRAG);
  bb_shader_textured_ = bb_shader_compile_(tex_vert.c_str(), tex_frag.c_str());
  bb_shader_lit_      = bb_shader_compile_(lit_vert.c_str(), lit_frag.c_str());
  bb_shaders_ready_   = true;
  if (bb_shader_unlit_ && bb_shader_textured_ && bb_shader_lit_)
    std::cerr << "[shader] UNLIT, TEXTURED, LIT compiled OK.\n";
  else
    std::cerr << "[shader] WARNING: one or more shaders failed to compile.\n";
}

// ============================================================
// Quit — free all GL program objects
// ============================================================

inline void bb_shader_quit_() {
  bb_gl_use_(); // BUG-63
  auto del = [](bb_Shader_*& s) {
    if (!s) return;
    if (s->program) glDeleteProgram(s->program);
    delete s;
    s = nullptr;
  };
  del(bb_shader_unlit_);
  del(bb_shader_textured_);
  del(bb_shader_lit_);
  bb_shader_active_ = nullptr;
  bb_shaders_ready_ = false;
}

// Register into bb_shader_quit_hook_ (declared in bb_sdl.h).
// Cleanup must run while the GL context is still alive (before bb_gl_quit_hook_).
extern void (*bb_shader_quit_hook_)();
inline const bool bb_shader_hook_reg_ = (bb_shader_quit_hook_ = bb_shader_quit_, true);

#endif // BB_SHADER_H
