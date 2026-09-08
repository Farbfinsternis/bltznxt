#ifndef BB_SHADER_H
#define BB_SHADER_H

// Shader Infrastructure (3D-07).
//
// Three built-in GLSL 3.30 Core shaders compiled once on the first RenderWorld call:
//
//   UNLIT    — solid u_color, no lighting
//   TEXTURED — sampler2D * u_color, no lighting
//   LIT      — Blinn-Phong, up to 8 lights, optional texture;
//              degrades to ambient-only when u_light_count == 0
//
// Shared vertex attribute layout (matches 3D-08 interleaved mesh format):
//   location 0 — vec3  a_pos    (x, y, z)
//   location 1 — vec3  a_normal (nx, ny, nz)
//   location 2 — vec2  a_uv    (u, v)
//   location 3 — vec3  a_color (r, g, b)  — used by EntityFX Bit2 (3D-10)

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

// ---- TEXTURED ----

static constexpr const char* BB_GLSL_TEXTURED_VERT = R"glsl(
#version 330 core
layout(location = 0) in vec3 a_pos;
layout(location = 2) in vec2 a_uv;
uniform mat4 u_mvp;
out vec2 v_uv;
void main() {
    v_uv        = a_uv;
    gl_Position = u_mvp * vec4(a_pos, 1.0);
}
)glsl";

static constexpr const char* BB_GLSL_TEXTURED_FRAG = R"glsl(
#version 330 core
in vec2 v_uv;
uniform sampler2D u_tex;
uniform vec4      u_color;
out vec4 frag_color;
void main() {
    frag_color = texture(u_tex, v_uv) * u_color;
}
)glsl";

// ---- LIT (Blinn-Phong, up to 8 lights) ----
//
// Light types: 0 = directional (u_light_pos is direction, pointing away from surface),
//              1 = point       (u_light_pos is world position, u_light_range = falloff radius)
//
// All colour uniforms (u_ambient, u_light_color) are pre-normalised to [0, 1] by the caller.
// Normal transform uses mat3(u_model): correct for rotation and uniform-scale transforms.
// When u_light_count == 0 the loop is skipped and the result is ambient-only (= effective unlit).

static constexpr const char* BB_GLSL_LIT_VERT = R"glsl(
#version 330 core
layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;
uniform mat4 u_mvp;
uniform mat4 u_model;
out vec3 v_pos;
out vec3 v_normal;
out vec2 v_uv;
void main() {
    vec4 wp  = u_model * vec4(a_pos, 1.0);
    v_pos    = wp.xyz;
    v_normal = mat3(u_model) * a_normal;
    v_uv     = a_uv;
    gl_Position = u_mvp * vec4(a_pos, 1.0);
}
)glsl";

static constexpr const char* BB_GLSL_LIT_FRAG = R"glsl(
#version 330 core
in vec3 v_pos;
in vec3 v_normal;
in vec2 v_uv;

uniform vec4  u_color;
uniform vec3  u_ambient;
uniform float u_shininess;
uniform vec3  u_view_pos;

uniform int   u_light_count;
uniform vec3  u_light_pos[8];
uniform vec3  u_light_color[8];
uniform float u_light_range[8];
uniform int   u_light_type[8];
// Spot (Typ 2): Kegelachse und die beiden Winkel als Kosinus des HALBwinkels,
// damit der Vergleich im Fragment ohne Trigonometrie auskommt.
uniform vec3  u_light_dir[8];
uniform float u_light_cos_inner[8];
uniform float u_light_cos_outer[8];

uniform sampler2D u_tex;
uniform int       u_use_tex;

out vec4 frag_color;

void main() {
    vec3 N      = normalize(v_normal);
    vec3 V      = normalize(u_view_pos - v_pos);
    vec3 result = u_ambient;

    for (int i = 0; i < u_light_count; ++i) {
        vec3  L;
        float atten = 1.0;
        if (u_light_type[i] == 0) {
            // Directional: u_light_pos is the direction vector (towards light source)
            L = normalize(u_light_pos[i]);
        } else {
            // Point und Spot teilen sich Abstand und Reichweite
            vec3  delta = u_light_pos[i] - v_pos;
            float dist  = length(delta);
            L = (dist > 0.0001) ? delta / dist : vec3(0.0, 1.0, 0.0);
            float r = u_light_range[i];
            atten = (r > 0.0) ? max(0.0, 1.0 - dist / r) : 1.0;
            if (u_light_type[i] == 2) {
                // Spot: Winkel zwischen Kegelachse und der Richtung zur
                // Oberflaeche. Innerhalb des inneren Winkels volle Helligkeit,
                // dazwischen weicher Uebergang, ausserhalb nichts.
                float cd = dot(normalize(u_light_dir[i]), -L);
                float ci = u_light_cos_inner[i];
                float co = u_light_cos_outer[i];
                float cone = (ci > co) ? clamp((cd - co) / (ci - co), 0.0, 1.0)
                                       : step(co, cd);
                atten *= cone;
            }
        }
        float diff = max(dot(N, L), 0.0);
        result += diff * u_light_color[i] * atten;
        if (u_shininess > 0.001) {
            vec3  H    = normalize(L + V);
            float spec = pow(max(dot(N, H), 0.0), u_shininess * 128.0);
            result += spec * u_light_color[i] * atten;
        }
    }

    vec4 base = u_color;
    if (u_use_tex != 0) base *= texture(u_tex, v_uv);
    frag_color = vec4(clamp(result, 0.0, 1.0) * base.rgb, base.a);
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

// ============================================================
// Init (lazy — called once from bb_RenderWorld on the first frame)
// ============================================================

inline void bb_shaders_init_() {
  bb_shader_unlit_    = bb_shader_compile_(BB_GLSL_UNLIT_VERT,    BB_GLSL_UNLIT_FRAG);
  bb_shader_textured_ = bb_shader_compile_(BB_GLSL_TEXTURED_VERT, BB_GLSL_TEXTURED_FRAG);
  bb_shader_lit_      = bb_shader_compile_(BB_GLSL_LIT_VERT,      BB_GLSL_LIT_FRAG);
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
