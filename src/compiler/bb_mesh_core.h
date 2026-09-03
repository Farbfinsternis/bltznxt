#ifndef BB_MESH_CORE_H
#define BB_MESH_CORE_H

// Geometry Buffer Infrastructure (3D-08).
//
// Provides bb_MeshData_ (CPU storage + GL handles) and three helpers:
//   bb_mesh_upload_   — create / update VAO + VBO + EBO on the GPU
//   bb_mesh_draw_     — bind shader uniforms + glDrawElements
//   bb_mesh_free_gpu_ — delete VAO/VBO/EBO (CPU data untouched)
//
// Vertex format — interleaved, 11 floats per vertex, stride = 44 bytes:
//   byte  0 | offset  0 : vec3  position   (x, y, z)
//   byte 12 | offset 12 : vec3  normal     (nx, ny, nz)
//   byte 24 | offset 24 : vec2  texcoord   (u, v)
//   byte 32 | offset 32 : vec3  color      (r, g, b)  — vertex colour for EntityFX
//
// Attribute locations (must match bb_shader.h):
//   0 = a_pos  |  1 = a_normal  |  2 = a_uv  |  3 = a_color

#include "bb_shader.h"
#include <cstdint>   // uintptr_t
#include <vector>

// ============================================================
// bb_MeshData_ — CPU-side geometry + GPU handle set
// ============================================================

struct bb_MeshData_ {
  std::vector<float>        vertices;  // interleaved, 11 floats per vertex
  std::vector<unsigned int> indices;   // 3 indices per triangle

  GLuint vao      = 0;
  GLuint vbo      = 0;
  GLuint ebo      = 0;
  bool   dirty    = true;  // true = GPU buffers need (re-)uploading
  int    triCount = 0;     // updated by bb_mesh_upload_; = indices.size()/3
};

// ============================================================
// Upload — create or update VAO / VBO / EBO on the GPU.
// Must be called while a GL context is current.
// Sets dirty=false and updates triCount.
// ============================================================

inline void bb_mesh_upload_(bb_MeshData_* m) {
  if (!m || m->vertices.empty() || m->indices.empty()) return;

  if (!m->vao) {
    glGenVertexArrays(1, &m->vao);
    glGenBuffers(1, &m->vbo);
    glGenBuffers(1, &m->ebo);
  }

  glBindVertexArray(m->vao);

  // Upload vertex data
  glBindBuffer(GL_ARRAY_BUFFER, m->vbo);
  glBufferData(GL_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(m->vertices.size() * sizeof(float)),
               m->vertices.data(), GL_STATIC_DRAW);

  // Upload index data (must be bound while VAO is bound)
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(m->indices.size() * sizeof(unsigned int)),
               m->indices.data(), GL_STATIC_DRAW);

  // Vertex attribute pointers (offsets as byte offsets into the VBO)
  constexpr GLsizei stride = 11 * sizeof(float);

  glEnableVertexAttribArray(0);  // a_pos
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
                        (const void*)(uintptr_t)0);

  glEnableVertexAttribArray(1);  // a_normal  — byte offset 12
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                        (const void*)(uintptr_t)(3 * sizeof(float)));

  glEnableVertexAttribArray(2);  // a_uv      — byte offset 24
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
                        (const void*)(uintptr_t)(6 * sizeof(float)));

  glEnableVertexAttribArray(3);  // a_color   — byte offset 32
  glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride,
                        (const void*)(uintptr_t)(8 * sizeof(float)));

  glBindVertexArray(0);

  m->triCount = static_cast<int>(m->indices.size()) / 3;
  m->dirty    = false;
}

// ============================================================
// Draw — upload if dirty, set uniforms, call glDrawElements.
//
//   mvp      : column-major proj*view*model (16 floats)  — required
//   model    : column-major model matrix    (16 floats)  — LIT shader world-space
//   color    : RGBA in [0, 1]               ( 4 floats)  — required
//   tex      : GL texture handle (0 = no texture)
//   view_pos : camera world position        ( 3 floats)  — LIT specular; nullptr = skip
// ============================================================

inline void bb_mesh_draw_(bb_MeshData_* mesh,
                           bb_Shader_*   s,
                           const float*  mvp,
                           const float*  model,
                           const float*  color,
                           GLuint        tex,
                           const float*  view_pos = nullptr) {
  if (!mesh || !s || mesh->indices.empty()) return;
  if (mesh->dirty) bb_mesh_upload_(mesh);
  if (!mesh->vao) return;

  bb_shader_bind_(s);

  bb_shader_uniform_mat4(s, "u_mvp",   mvp);
  bb_shader_uniform_v4  (s, "u_color", color[0], color[1], color[2], color[3]);

  // LIT-shader extras (silently ignored by UNLIT / TEXTURED — location = -1)
  if (model)    bb_shader_uniform_mat4(s, "u_model",    model);
  if (view_pos) bb_shader_uniform_v3  (s, "u_view_pos",
                                       view_pos[0], view_pos[1], view_pos[2]);

  // Texture
  if (tex) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    bb_shader_uniform_i(s, "u_tex",     0);
    bb_shader_uniform_i(s, "u_use_tex", 1);
  } else {
    bb_shader_uniform_i(s, "u_use_tex", 0);
  }

  glBindVertexArray(mesh->vao);
  glDrawElements(GL_TRIANGLES,
                 static_cast<GLsizei>(mesh->indices.size()),
                 GL_UNSIGNED_INT, nullptr);
  glBindVertexArray(0);
}

// ============================================================
// Free GPU — delete VAO / VBO / EBO.
// CPU-side vertices and indices are left intact (allows re-upload later).
// ============================================================

inline void bb_mesh_free_gpu_(bb_MeshData_* m) {
  if (!m) return;
  if (m->ebo) { glDeleteBuffers(1, &m->ebo); m->ebo = 0; }
  if (m->vbo) { glDeleteBuffers(1, &m->vbo); m->vbo = 0; }
  if (m->vao) { glDeleteVertexArrays(1, &m->vao); m->vao = 0; }
  m->dirty    = true;
  m->triCount = 0;
}

#endif // BB_MESH_CORE_H
