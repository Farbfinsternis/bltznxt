#ifndef BB_GL_CTX_H
#define BB_GL_CTX_H

// Minimal OpenGL 3.3 Core interface for BLTZNXT 3D.
//
// Does NOT include system GL headers (gl.h / glext.h) to avoid type and
// symbol conflicts on Windows.  All GL types, constants, and function
// pointers are defined here and loaded via SDL_GL_GetProcAddress at
// runtime inside bb_Graphics3D().
//
// Usage pattern (identical to stb_image):
//   bb_runtime.h includes this file once.  Every other 3D header that
//   needs GL symbols just includes this header — the inline definitions
//   are shared across the single translation unit.

#include <SDL3/SDL.h>
#include <cstddef>   // ptrdiff_t
#include <iostream>
#include "bb_graphics2d.h"   // bb_gfx_width_, bb_window_, bb_renderer_, …

// ============================================================
// GL Types
// ============================================================

typedef unsigned int    GLenum;
typedef unsigned char   GLboolean;
typedef unsigned int    GLbitfield;
typedef int             GLint;
typedef unsigned int    GLuint;
typedef int             GLsizei;
typedef float           GLfloat;
typedef double          GLdouble;
typedef void            GLvoid;
typedef char            GLchar;
typedef unsigned char   GLubyte;
typedef unsigned short  GLushort;
typedef ptrdiff_t       GLintptr;
typedef ptrdiff_t       GLsizeiptr;

#ifdef _WIN32
#  ifndef APIENTRY
#    define APIENTRY __stdcall
#  endif
#else
#  define APIENTRY
#endif

// ============================================================
// GL Constants
// ============================================================

// Booleans
#define GL_FALSE                          0
#define GL_TRUE                           1

// Data types
#define GL_BYTE                           0x1400
#define GL_UNSIGNED_BYTE                  0x1401
#define GL_SHORT                          0x1402
#define GL_UNSIGNED_SHORT                 0x1403
#define GL_INT                            0x1404
#define GL_UNSIGNED_INT                   0x1405
#define GL_FLOAT                          0x1406

// Primitives
#define GL_POINTS                         0x0000
#define GL_LINES                          0x0001
#define GL_LINE_STRIP                     0x0003
#define GL_TRIANGLES                      0x0004
#define GL_TRIANGLE_STRIP                 0x0005
#define GL_TRIANGLE_FAN                   0x0006

// Blend factors
#define GL_ZERO                           0
#define GL_ONE                            1
#define GL_SRC_COLOR                      0x0300
#define GL_ONE_MINUS_SRC_COLOR            0x0301
#define GL_SRC_ALPHA                      0x0302
#define GL_ONE_MINUS_SRC_ALPHA            0x0303
#define GL_DST_ALPHA                      0x0304
#define GL_ONE_MINUS_DST_ALPHA            0x0305
#define GL_DST_COLOR                      0x0306
#define GL_ONE_MINUS_DST_COLOR            0x0307

// Blend equations
#define GL_FUNC_ADD                       0x8006
#define GL_FUNC_SUBTRACT                  0x800A
#define GL_FUNC_REVERSE_SUBTRACT          0x800B
#define GL_MIN                            0x8007
#define GL_MAX                            0x8008

// Depth functions
#define GL_NEVER                          0x0200
#define GL_LESS                           0x0201
#define GL_EQUAL                          0x0202
#define GL_LEQUAL                         0x0203
#define GL_GREATER                        0x0204
#define GL_NOTEQUAL                       0x0205
#define GL_GEQUAL                         0x0206
#define GL_ALWAYS                         0x0207

// Culling / winding
#define GL_FRONT                          0x0404
#define GL_BACK                           0x0405
#define GL_FRONT_AND_BACK                 0x0408
#define GL_CW                             0x0900
#define GL_CCW                            0x0901

// Polygon mode
#define GL_POINT                          0x1B00
#define GL_LINE                           0x1B01
#define GL_FILL                           0x1B02

// Enable caps
#define GL_CULL_FACE                      0x0B44
#define GL_DEPTH_TEST                     0x0B71
#define GL_BLEND                          0x0BE2
#define GL_SCISSOR_TEST                   0x0C11
#define GL_STENCIL_TEST                   0x0B90

// Clear bits
#define GL_DEPTH_BUFFER_BIT               0x00000100
#define GL_STENCIL_BUFFER_BIT             0x00000400
#define GL_COLOR_BUFFER_BIT               0x00004000

// Textures
#define GL_TEXTURE_2D                     0x0DE1
#define GL_TEXTURE_WRAP_S                 0x2802
#define GL_TEXTURE_WRAP_T                 0x2803
#define GL_TEXTURE_MAG_FILTER             0x2800
#define GL_TEXTURE_MIN_FILTER             0x2801
#define GL_NEAREST                        0x2600
#define GL_LINEAR                         0x2601
#define GL_NEAREST_MIPMAP_NEAREST         0x2700
#define GL_LINEAR_MIPMAP_NEAREST          0x2701
#define GL_NEAREST_MIPMAP_LINEAR          0x2702
#define GL_LINEAR_MIPMAP_LINEAR           0x2703
#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_REPEAT                         0x2901
#define GL_MIRRORED_REPEAT                0x8370
#define GL_RGB                            0x1907
#define GL_RGBA                           0x1908
#define GL_RGB8                           0x8051
#define GL_RGBA8                          0x8058
#define GL_DEPTH_COMPONENT                0x1902
#define GL_DEPTH_COMPONENT16              0x81A5
#define GL_DEPTH_COMPONENT24              0x81A6
#define GL_UNPACK_ALIGNMENT               0x0CF5
#define GL_PACK_ALIGNMENT                 0x0D05
#define GL_TEXTURE0                       0x84C0
#define GL_TEXTURE1                       0x84C1
#define GL_TEXTURE2                       0x84C2
#define GL_TEXTURE3                       0x84C3
#define GL_TEXTURE4                       0x84C4
#define GL_TEXTURE5                       0x84C5
#define GL_TEXTURE6                       0x84C6
#define GL_TEXTURE7                       0x84C7

// Buffers
#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_UNIFORM_BUFFER                 0x8A11
#define GL_STATIC_DRAW                    0x88E4
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_STREAM_DRAW                    0x88E0

// Shaders
#define GL_VERTEX_SHADER                  0x8B31
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_VALIDATE_STATUS                0x8B83
#define GL_INFO_LOG_LENGTH                0x8B84

// Framebuffers
#define GL_FRAMEBUFFER                    0x8D40
#define GL_RENDERBUFFER                   0x8D41
#define GL_COLOR_ATTACHMENT0              0x8CE0
#define GL_DEPTH_ATTACHMENT               0x8D00
#define GL_STENCIL_ATTACHMENT             0x8D20
#define GL_DEPTH_STENCIL_ATTACHMENT       0x821A
#define GL_FRAMEBUFFER_COMPLETE           0x8CD5
#define GL_READ_FRAMEBUFFER               0x8CA8
#define GL_DRAW_FRAMEBUFFER               0x8CA9

// Misc queries
#define GL_NO_ERROR                       0
#define GL_VENDOR                         0x1F00
#define GL_RENDERER                       0x1F01
#define GL_VERSION                        0x1F02
#define GL_SHADING_LANGUAGE_VERSION       0x8B8C

// ============================================================
// Function pointer declarations
//
// BB_GL_DECL(return_type, function_name, ...param_types)
// generates:
//   typedef return_type (APIENTRY * PFN_function_name_)(params);
//   inline PFN_function_name_ function_name = nullptr;
//
// The resulting inline variable is callable exactly like a real GL function.
// ============================================================

#define BB_GL_DECL(ret, name, ...) \
  typedef ret (APIENTRY * PFN_##name##_)(__VA_ARGS__); \
  inline PFN_##name##_ name = nullptr;

// ---- State ----
BB_GL_DECL(void,   glEnable,              GLenum cap)
BB_GL_DECL(void,   glDisable,             GLenum cap)
BB_GL_DECL(void,   glBlendFunc,           GLenum sfactor, GLenum dfactor)
BB_GL_DECL(void,   glBlendFuncSeparate,   GLenum srcRGB, GLenum dstRGB, GLenum srcA, GLenum dstA)
BB_GL_DECL(void,   glBlendEquation,       GLenum mode)
BB_GL_DECL(void,   glDepthFunc,           GLenum func)
BB_GL_DECL(void,   glDepthMask,           GLboolean flag)
BB_GL_DECL(void,   glColorMask,           GLboolean r, GLboolean g, GLboolean b, GLboolean a)
BB_GL_DECL(void,   glCullFace,            GLenum mode)
BB_GL_DECL(void,   glFrontFace,           GLenum mode)
BB_GL_DECL(void,   glPolygonMode,         GLenum face, GLenum mode)
BB_GL_DECL(void,   glLineWidth,           GLfloat width)
BB_GL_DECL(void,   glScissor,             GLint x, GLint y, GLsizei width, GLsizei height)

// ---- Clear / Viewport ----
BB_GL_DECL(void,   glClear,               GLbitfield mask)
BB_GL_DECL(void,   glClearColor,          GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
BB_GL_DECL(void,   glClearDepth,          GLdouble depth)
BB_GL_DECL(void,   glViewport,            GLint x, GLint y, GLsizei width, GLsizei height)
BB_GL_DECL(void,   glFlush,               void)
BB_GL_DECL(void,   glFinish,              void)

// ---- Query ----
BB_GL_DECL(GLenum,         glGetError,    void)
BB_GL_DECL(void,           glGetIntegerv, GLenum pname, GLint* data)
BB_GL_DECL(const GLubyte*, glGetString,   GLenum name)

// ---- Draw ----
BB_GL_DECL(void, glDrawArrays,   GLenum mode, GLint first, GLsizei count)
BB_GL_DECL(void, glDrawElements, GLenum mode, GLsizei count, GLenum type, const void* indices)
BB_GL_DECL(void, glReadPixels,   GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels)

// ---- Textures ----
BB_GL_DECL(void, glGenTextures,    GLsizei n, GLuint* textures)
BB_GL_DECL(void, glDeleteTextures, GLsizei n, const GLuint* textures)
BB_GL_DECL(void, glBindTexture,    GLenum target, GLuint texture)
BB_GL_DECL(void, glActiveTexture,  GLenum texture)
BB_GL_DECL(void, glTexImage2D,     GLenum target, GLint level, GLint internalformat,
                                   GLsizei width, GLsizei height, GLint border,
                                   GLenum format, GLenum type, const void* pixels)
BB_GL_DECL(void, glTexSubImage2D,  GLenum target, GLint level, GLint xoffset, GLint yoffset,
                                   GLsizei width, GLsizei height,
                                   GLenum format, GLenum type, const void* pixels)
BB_GL_DECL(void, glTexParameteri,  GLenum target, GLenum pname, GLint param)
BB_GL_DECL(void, glTexParameterf,  GLenum target, GLenum pname, GLfloat param)
BB_GL_DECL(void, glPixelStorei,    GLenum pname, GLint param)
BB_GL_DECL(void, glGenerateMipmap, GLenum target)

// ---- Buffers (VBO) ----
BB_GL_DECL(void, glGenBuffers,    GLsizei n, GLuint* buffers)
BB_GL_DECL(void, glDeleteBuffers, GLsizei n, const GLuint* buffers)
BB_GL_DECL(void, glBindBuffer,    GLenum target, GLuint buffer)
BB_GL_DECL(void, glBufferData,    GLenum target, GLsizeiptr size, const void* data, GLenum usage)
BB_GL_DECL(void, glBufferSubData, GLenum target, GLintptr offset, GLsizeiptr size, const void* data)

// ---- Vertex Arrays (VAO) ----
BB_GL_DECL(void, glGenVertexArrays,    GLsizei n, GLuint* arrays)
BB_GL_DECL(void, glDeleteVertexArrays, GLsizei n, const GLuint* arrays)
BB_GL_DECL(void, glBindVertexArray,    GLuint array)

// ---- Vertex Attributes ----
BB_GL_DECL(void, glVertexAttribPointer,
           GLuint index, GLint size, GLenum type, GLboolean normalized,
           GLsizei stride, const void* pointer)
BB_GL_DECL(void, glVertexAttribIPointer,
           GLuint index, GLint size, GLenum type,
           GLsizei stride, const void* pointer)
BB_GL_DECL(void, glEnableVertexAttribArray,  GLuint index)
BB_GL_DECL(void, glDisableVertexAttribArray, GLuint index)

// ---- Shaders ----
BB_GL_DECL(GLuint, glCreateShader,     GLenum type)
BB_GL_DECL(void,   glDeleteShader,     GLuint shader)
BB_GL_DECL(void,   glShaderSource,     GLuint shader, GLsizei count,
                                       const GLchar* const* string, const GLint* length)
BB_GL_DECL(void,   glCompileShader,    GLuint shader)
BB_GL_DECL(void,   glGetShaderiv,      GLuint shader, GLenum pname, GLint* params)
BB_GL_DECL(void,   glGetShaderInfoLog, GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog)

// ---- Programs ----
BB_GL_DECL(GLuint, glCreateProgram,      void)
BB_GL_DECL(void,   glDeleteProgram,      GLuint program)
BB_GL_DECL(void,   glAttachShader,       GLuint program, GLuint shader)
BB_GL_DECL(void,   glDetachShader,       GLuint program, GLuint shader)
BB_GL_DECL(void,   glLinkProgram,        GLuint program)
BB_GL_DECL(void,   glUseProgram,         GLuint program)
BB_GL_DECL(void,   glGetProgramiv,       GLuint program, GLenum pname, GLint* params)
BB_GL_DECL(void,   glGetProgramInfoLog,  GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog)
BB_GL_DECL(void,   glValidateProgram,    GLuint program)

// ---- Uniforms ----
BB_GL_DECL(GLint, glGetUniformLocation,   GLuint program, const GLchar* name)
BB_GL_DECL(void,  glUniform1i,            GLint location, GLint v0)
BB_GL_DECL(void,  glUniform1f,            GLint location, GLfloat v0)
BB_GL_DECL(void,  glUniform2f,            GLint location, GLfloat v0, GLfloat v1)
BB_GL_DECL(void,  glUniform3f,            GLint location, GLfloat v0, GLfloat v1, GLfloat v2)
BB_GL_DECL(void,  glUniform4f,            GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
BB_GL_DECL(void,  glUniform1iv,           GLint location, GLsizei count, const GLint* value)
BB_GL_DECL(void,  glUniform1fv,           GLint location, GLsizei count, const GLfloat* value)
BB_GL_DECL(void,  glUniform2fv,           GLint location, GLsizei count, const GLfloat* value)
BB_GL_DECL(void,  glUniform3fv,           GLint location, GLsizei count, const GLfloat* value)
BB_GL_DECL(void,  glUniform4fv,           GLint location, GLsizei count, const GLfloat* value)
BB_GL_DECL(void,  glUniformMatrix3fv,     GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
BB_GL_DECL(void,  glUniformMatrix4fv,     GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
BB_GL_DECL(GLuint,glGetUniformBlockIndex, GLuint program, const GLchar* uniformBlockName)
BB_GL_DECL(void,  glUniformBlockBinding,  GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding)

// ---- Framebuffers ----
BB_GL_DECL(void,   glGenFramebuffers,         GLsizei n, GLuint* framebuffers)
BB_GL_DECL(void,   glDeleteFramebuffers,      GLsizei n, const GLuint* framebuffers)
BB_GL_DECL(void,   glBindFramebuffer,         GLenum target, GLuint framebuffer)
BB_GL_DECL(void,   glFramebufferTexture2D,    GLenum target, GLenum attachment,
                                              GLenum textarget, GLuint texture, GLint level)
BB_GL_DECL(GLenum, glCheckFramebufferStatus,  GLenum target)
BB_GL_DECL(void,   glGenRenderbuffers,        GLsizei n, GLuint* renderbuffers)
BB_GL_DECL(void,   glDeleteRenderbuffers,     GLsizei n, const GLuint* renderbuffers)
BB_GL_DECL(void,   glBindRenderbuffer,        GLenum target, GLuint renderbuffer)
BB_GL_DECL(void,   glRenderbufferStorage,     GLenum target, GLenum internalformat,
                                              GLsizei width, GLsizei height)
BB_GL_DECL(void,   glFramebufferRenderbuffer, GLenum target, GLenum attachment,
                                              GLenum renderbuffertarget, GLuint renderbuffer)

// ============================================================
// Loader
// ============================================================

#define BB_GL_LOAD(name) \
  name = reinterpret_cast<PFN_##name##_>( \
    reinterpret_cast<void(*)()>(SDL_GL_GetProcAddress(#name))); \
  if (!name) { std::cerr << "[GL] Missing: " #name "\n"; ok = false; }

inline bool bb_gl_load_() {
  bool ok = true;

  // State
  BB_GL_LOAD(glEnable)
  BB_GL_LOAD(glDisable)
  BB_GL_LOAD(glBlendFunc)
  BB_GL_LOAD(glBlendFuncSeparate)
  BB_GL_LOAD(glBlendEquation)
  BB_GL_LOAD(glDepthFunc)
  BB_GL_LOAD(glDepthMask)
  BB_GL_LOAD(glColorMask)
  BB_GL_LOAD(glCullFace)
  BB_GL_LOAD(glFrontFace)
  BB_GL_LOAD(glPolygonMode)
  BB_GL_LOAD(glLineWidth)
  BB_GL_LOAD(glScissor)

  // Clear / Viewport
  BB_GL_LOAD(glClear)
  BB_GL_LOAD(glClearColor)
  BB_GL_LOAD(glClearDepth)
  BB_GL_LOAD(glViewport)
  BB_GL_LOAD(glFlush)
  BB_GL_LOAD(glFinish)

  // Query
  BB_GL_LOAD(glGetError)
  BB_GL_LOAD(glGetIntegerv)
  BB_GL_LOAD(glGetString)

  // Draw
  BB_GL_LOAD(glDrawArrays)
  BB_GL_LOAD(glDrawElements)
  BB_GL_LOAD(glReadPixels)

  // Textures
  BB_GL_LOAD(glGenTextures)
  BB_GL_LOAD(glDeleteTextures)
  BB_GL_LOAD(glBindTexture)
  BB_GL_LOAD(glActiveTexture)
  BB_GL_LOAD(glTexImage2D)
  BB_GL_LOAD(glTexSubImage2D)
  BB_GL_LOAD(glTexParameteri)
  BB_GL_LOAD(glTexParameterf)
  BB_GL_LOAD(glPixelStorei)
  BB_GL_LOAD(glGenerateMipmap)

  // Buffers
  BB_GL_LOAD(glGenBuffers)
  BB_GL_LOAD(glDeleteBuffers)
  BB_GL_LOAD(glBindBuffer)
  BB_GL_LOAD(glBufferData)
  BB_GL_LOAD(glBufferSubData)

  // Vertex Arrays
  BB_GL_LOAD(glGenVertexArrays)
  BB_GL_LOAD(glDeleteVertexArrays)
  BB_GL_LOAD(glBindVertexArray)

  // Vertex Attribs
  BB_GL_LOAD(glVertexAttribPointer)
  BB_GL_LOAD(glVertexAttribIPointer)
  BB_GL_LOAD(glEnableVertexAttribArray)
  BB_GL_LOAD(glDisableVertexAttribArray)

  // Shaders
  BB_GL_LOAD(glCreateShader)
  BB_GL_LOAD(glDeleteShader)
  BB_GL_LOAD(glShaderSource)
  BB_GL_LOAD(glCompileShader)
  BB_GL_LOAD(glGetShaderiv)
  BB_GL_LOAD(glGetShaderInfoLog)

  // Programs
  BB_GL_LOAD(glCreateProgram)
  BB_GL_LOAD(glDeleteProgram)
  BB_GL_LOAD(glAttachShader)
  BB_GL_LOAD(glDetachShader)
  BB_GL_LOAD(glLinkProgram)
  BB_GL_LOAD(glUseProgram)
  BB_GL_LOAD(glGetProgramiv)
  BB_GL_LOAD(glGetProgramInfoLog)
  BB_GL_LOAD(glValidateProgram)

  // Uniforms
  BB_GL_LOAD(glGetUniformLocation)
  BB_GL_LOAD(glUniform1i)
  BB_GL_LOAD(glUniform1f)
  BB_GL_LOAD(glUniform2f)
  BB_GL_LOAD(glUniform3f)
  BB_GL_LOAD(glUniform4f)
  BB_GL_LOAD(glUniform1iv)
  BB_GL_LOAD(glUniform1fv)
  BB_GL_LOAD(glUniform2fv)
  BB_GL_LOAD(glUniform3fv)
  BB_GL_LOAD(glUniform4fv)
  BB_GL_LOAD(glUniformMatrix3fv)
  BB_GL_LOAD(glUniformMatrix4fv)
  BB_GL_LOAD(glGetUniformBlockIndex)
  BB_GL_LOAD(glUniformBlockBinding)

  // Framebuffers
  BB_GL_LOAD(glGenFramebuffers)
  BB_GL_LOAD(glDeleteFramebuffers)
  BB_GL_LOAD(glBindFramebuffer)
  BB_GL_LOAD(glFramebufferTexture2D)
  BB_GL_LOAD(glCheckFramebufferStatus)
  BB_GL_LOAD(glGenRenderbuffers)
  BB_GL_LOAD(glDeleteRenderbuffers)
  BB_GL_LOAD(glBindRenderbuffer)
  BB_GL_LOAD(glRenderbufferStorage)
  BB_GL_LOAD(glFramebufferRenderbuffer)

  return ok;
}

#undef BB_GL_LOAD

// ============================================================
// Quit
// ============================================================

// Graphics3D stellt die Vorgabe der Texturfilter wieder her (3D-11). Die
// Filterliste liegt in bb_texture.h, das erst spaeter eingebunden wird -
// deshalb wie bei den Quit-Hooks ueber einen Funktionszeiger.
inline void (*bb_texture_gfxreset_hook_)() = nullptr;

inline void bb_gl_quit_() {
  if (bb_gl_ctx_) {
    SDL_GL_DestroyContext(bb_gl_ctx_);
    bb_gl_ctx_ = nullptr;
  }
  bb_gl_active_ = false;
}

// ============================================================
// bb_Graphics3D
// ============================================================

inline void bb_Graphics3D(int w, int h, int depth = 32, int mode = 0) {
  bb_gfx_width_  = w;
  bb_gfx_height_ = h;
  bb_gfx_depth_  = depth;
  bb_gfx_rate_   = 0;
  bb_close_scene_();             // alte Welt freigeben, solange der Kontext lebt
  bb_gfx_reset_draw_state_(1);   // BackBuffer (BUG-131)

  bb_sdl_ensure_();
  if (!bb_sdl_initialized_) return;

  // Tear down any existing window / context / renderer.
  if (bb_renderer_) { SDL_DestroyRenderer(bb_renderer_); bb_renderer_ = nullptr; }
  if (bb_gl_ctx_)   { SDL_GL_DestroyContext(bb_gl_ctx_); bb_gl_ctx_   = nullptr; }
  if (bb_window_)   { SDL_DestroyWindow(bb_window_);     bb_window_   = nullptr; }
  bb_gl_active_ = false;

  // Request OpenGL 3.3 Core Profile attributes *before* window creation.
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,  SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,   24);

  const bool fullscreen = (mode == 1 || mode == 6);
  SDL_WindowFlags flags = SDL_WINDOW_OPENGL;
  if (fullscreen) flags |= SDL_WINDOW_FULLSCREEN;

  const char* title = bb_app_title_.empty() ? "BLTZNXT" : bb_app_title_.c_str();
  int win_w = fullscreen ? 0 : w;
  int win_h = fullscreen ? 0 : h;

  bb_window_ = SDL_CreateWindow(title, win_w, win_h, flags);
  if (!bb_window_) {
    std::cerr << "[runtime] Graphics3D: SDL_CreateWindow failed: " << SDL_GetError() << "\n";
    return;
  }

  // Create OpenGL context.
  bb_gl_ctx_ = SDL_GL_CreateContext(bb_window_);
  if (!bb_gl_ctx_) {
    std::cerr << "[runtime] Graphics3D: SDL_GL_CreateContext failed: " << SDL_GetError() << "\n";
    SDL_DestroyWindow(bb_window_); bb_window_ = nullptr;
    return;
  }
  SDL_GL_MakeCurrent(bb_window_, bb_gl_ctx_);

  // Load all GL 3.3 Core function pointers.
  bb_gl_load_();

  // SDL_Renderer fuer die 2D-Befehle (Plot, Text, DrawImage, …) auf demselben
  // Fenster - ausdruecklich das OpenGL-Backend (BUG-63). Ohne Angabe waehlt
  // SDL unter Windows direct3d11, und das zeichnet in einen eigenen Puffer:
  // gemessen (2026-09-15) war im 3D-Modus keine einzige 2D-Zeichnung auf dem
  // Bildschirm, und LockBuffer/ReadPixel/GetColor lasen aus diesem Puffer, sahen
  // also das 3D-Bild nicht. Der GL-Renderer hat einen eigenen Kontext, zeichnet
  // aber in denselben Backbuffer des Fensters wie RenderWorld; beide kommen in
  // der Reihenfolge der Befehle ins Bild, wie im Original.
  //
  // Waehrend des Anlegens stehen die Attribute auf GL 2.1: der GL-Renderer von
  // SDL verlangt diese Version und legt das Fenster sonst neu an (gemessen:
  // anderes HWND). Danach wieder 3.3 Core, und unser Kontext wird aktuell.
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,  0);
  bb_renderer_ = SDL_CreateRenderer(bb_window_, "opengl");
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,  SDL_GL_CONTEXT_PROFILE_CORE);
  if (!bb_renderer_) {
    std::cerr << "[runtime] Graphics3D: OpenGL 2D renderer unavailable ("
              << SDL_GetError() << ") - 2D drawing will not appear over 3D\n";
    bb_renderer_ = SDL_CreateRenderer(bb_window_, nullptr);
  }
  if (!bb_renderer_) {
    std::cerr << "[runtime] Graphics3D: SDL_CreateRenderer failed"
                 " — 2D commands unavailable: " << SDL_GetError() << "\n";
    // Not fatal: 3D rendering still works without the 2D renderer.
  }
  SDL_GL_MakeCurrent(bb_window_, bb_gl_ctx_);

  // Im Vollbild ist das Fenster so gross wie der Bildschirm; das Bild des
  // Programms wird darauf skaliert, nicht in die Ecke gelegt (BUG-159). Fuer
  // die 2D-Befehle macht das der Renderer, fuer RenderWorld der Viewport.
  if (bb_renderer_ && fullscreen)
    SDL_SetRenderLogicalPresentation(bb_renderer_, w, h,
                                     SDL_LOGICAL_PRESENTATION_LETTERBOX);
  bb_present_update_(w, h);

  bb_gl_active_    = true;
  bb_scene_open_   = true;       // BUG-120, siehe bb_close_scene_
  bb_gl_quit_hook_ = bb_gl_quit_;
  if (bb_texture_gfxreset_hook_) bb_texture_gfxreset_hook_();

  // Print context info to help diagnose driver issues.
  if (glGetString) {
    std::cerr << "[GL] Vendor:   " << (const char*)glGetString(GL_VENDOR)   << "\n"
              << "[GL] Renderer: " << (const char*)glGetString(GL_RENDERER) << "\n"
              << "[GL] Version:  " << (const char*)glGetString(GL_VERSION)  << "\n";
  }
}

#endif // BB_GL_CTX_H
