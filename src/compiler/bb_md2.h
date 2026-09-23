#ifndef BB_MD2_H
#define BB_MD2_H

// MD2-Modelle (3D-23).
//
// Nachgebaut nach blitz3d/md2rep.cpp und md2model.cpp und am Original
// gemessen (build/md2_20260919/, 2026-09-19):
//
//  - Achsen: das Original nimmt (x,y,z) = (md2.y, md2.z, md2.x) - fuer
//    Skalierung, Verschiebung, Vertices und die 162 Normalen der Tabelle.
//    Die Dreiecke laufen 0,2,1.
//  - Vertices mit gleichem Index und gleicher UV werden zusammengelegt, in
//    der Reihenfolge ihres ersten Auftretens; UV = u/skinWidth, v/skinHeight.
//  - Gezeichnet wird linear zwischen zwei Frames, Lage und Normale. Die
//    Normale bleibt unnormiert; Direct3D normiert sie (NORMALIZENORMALS),
//    unser Shader auch.
//  - Die Animationszeit laeuft in UpdateWorld um speed * elapsed weiter, nur
//    bei sichtbaren Modellen. Im Loop-Modus steht an der Stelle von Frame
//    `last` Frame `first`.
//  - Ein Uebergang haelt den gerade gezeigten Stand fest; trans_time waechst
//    je UpdateWorld um 1/transition, unabhaengig von elapsed.
//  - Eine Kopie teilt die Daten, beginnt aber ohne Animation bei Frame 0.
//  - Ein Modell, dessen Huellbox ausserhalb des Sichtbereichs liegt, wird
//    nicht gezeichnet und nicht gezaehlt (MD2Model::render).
//
// Das Aussehen ist der Brush der Entity (EntityTexture, EntityColor, ...);
// die Datei bringt keine Textur mit, das sagt auch die Doku.

#include "bb_entity_core.h"
#include "bb_mesh_core.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <vector>

// Die Normalentabelle der MD2-Datei (blitz3d/md2norms.cpp), schon mit
// vertauschten Achsen wie im Original: (y, z, x).
inline const float* bb_md2_norms_() {
  static const float raw[162][3] = {
    {-0.525731f, 0.000000f, 0.850651f},
    {-0.442863f, 0.238856f, 0.864188f},
    {-0.295242f, 0.000000f, 0.955423f},
    {-0.309017f, 0.500000f, 0.809017f},
    {-0.162460f, 0.262866f, 0.951056f},
    {0.000000f, 0.000000f, 1.000000f},
    {0.000000f, 0.850651f, 0.525731f},
    {-0.147621f, 0.716567f, 0.681718f},
    {0.147621f, 0.716567f, 0.681718f},
    {0.000000f, 0.525731f, 0.850651f},
    {0.309017f, 0.500000f, 0.809017f},
    {0.525731f, 0.000000f, 0.850651f},
    {0.295242f, 0.000000f, 0.955423f},
    {0.442863f, 0.238856f, 0.864188f},
    {0.162460f, 0.262866f, 0.951056f},
    {-0.681718f, 0.147621f, 0.716567f},
    {-0.809017f, 0.309017f, 0.500000f},
    {-0.587785f, 0.425325f, 0.688191f},
    {-0.850651f, 0.525731f, 0.000000f},
    {-0.864188f, 0.442863f, 0.238856f},
    {-0.716567f, 0.681718f, 0.147621f},
    {-0.688191f, 0.587785f, 0.425325f},
    {-0.500000f, 0.809017f, 0.309017f},
    {-0.238856f, 0.864188f, 0.442863f},
    {-0.425325f, 0.688191f, 0.587785f},
    {-0.716567f, 0.681718f, -0.147621f},
    {-0.500000f, 0.809017f, -0.309017f},
    {-0.525731f, 0.850651f, 0.000000f},
    {0.000000f, 0.850651f, -0.525731f},
    {-0.238856f, 0.864188f, -0.442863f},
    {0.000000f, 0.955423f, -0.295242f},
    {-0.262866f, 0.951056f, -0.162460f},
    {0.000000f, 1.000000f, 0.000000f},
    {0.000000f, 0.955423f, 0.295242f},
    {-0.262866f, 0.951056f, 0.162460f},
    {0.238856f, 0.864188f, 0.442863f},
    {0.262866f, 0.951056f, 0.162460f},
    {0.500000f, 0.809017f, 0.309017f},
    {0.238856f, 0.864188f, -0.442863f},
    {0.262866f, 0.951056f, -0.162460f},
    {0.500000f, 0.809017f, -0.309017f},
    {0.850651f, 0.525731f, 0.000000f},
    {0.716567f, 0.681718f, 0.147621f},
    {0.716567f, 0.681718f, -0.147621f},
    {0.525731f, 0.850651f, 0.000000f},
    {0.425325f, 0.688191f, 0.587785f},
    {0.864188f, 0.442863f, 0.238856f},
    {0.688191f, 0.587785f, 0.425325f},
    {0.809017f, 0.309017f, 0.500000f},
    {0.681718f, 0.147621f, 0.716567f},
    {0.587785f, 0.425325f, 0.688191f},
    {0.955423f, 0.295242f, 0.000000f},
    {1.000000f, 0.000000f, 0.000000f},
    {0.951056f, 0.162460f, 0.262866f},
    {0.850651f, -0.525731f, 0.000000f},
    {0.955423f, -0.295242f, 0.000000f},
    {0.864188f, -0.442863f, 0.238856f},
    {0.951056f, -0.162460f, 0.262866f},
    {0.809017f, -0.309017f, 0.500000f},
    {0.681718f, -0.147621f, 0.716567f},
    {0.850651f, 0.000000f, 0.525731f},
    {0.864188f, 0.442863f, -0.238856f},
    {0.809017f, 0.309017f, -0.500000f},
    {0.951056f, 0.162460f, -0.262866f},
    {0.525731f, 0.000000f, -0.850651f},
    {0.681718f, 0.147621f, -0.716567f},
    {0.681718f, -0.147621f, -0.716567f},
    {0.850651f, 0.000000f, -0.525731f},
    {0.809017f, -0.309017f, -0.500000f},
    {0.864188f, -0.442863f, -0.238856f},
    {0.951056f, -0.162460f, -0.262866f},
    {0.147621f, 0.716567f, -0.681718f},
    {0.309017f, 0.500000f, -0.809017f},
    {0.425325f, 0.688191f, -0.587785f},
    {0.442863f, 0.238856f, -0.864188f},
    {0.587785f, 0.425325f, -0.688191f},
    {0.688191f, 0.587785f, -0.425325f},
    {-0.147621f, 0.716567f, -0.681718f},
    {-0.309017f, 0.500000f, -0.809017f},
    {0.000000f, 0.525731f, -0.850651f},
    {-0.525731f, 0.000000f, -0.850651f},
    {-0.442863f, 0.238856f, -0.864188f},
    {-0.295242f, 0.000000f, -0.955423f},
    {-0.162460f, 0.262866f, -0.951056f},
    {0.000000f, 0.000000f, -1.000000f},
    {0.295242f, 0.000000f, -0.955423f},
    {0.162460f, 0.262866f, -0.951056f},
    {-0.442863f, -0.238856f, -0.864188f},
    {-0.309017f, -0.500000f, -0.809017f},
    {-0.162460f, -0.262866f, -0.951056f},
    {0.000000f, -0.850651f, -0.525731f},
    {-0.147621f, -0.716567f, -0.681718f},
    {0.147621f, -0.716567f, -0.681718f},
    {0.000000f, -0.525731f, -0.850651f},
    {0.309017f, -0.500000f, -0.809017f},
    {0.442863f, -0.238856f, -0.864188f},
    {0.162460f, -0.262866f, -0.951056f},
    {0.238856f, -0.864188f, -0.442863f},
    {0.500000f, -0.809017f, -0.309017f},
    {0.425325f, -0.688191f, -0.587785f},
    {0.716567f, -0.681718f, -0.147621f},
    {0.688191f, -0.587785f, -0.425325f},
    {0.587785f, -0.425325f, -0.688191f},
    {0.000000f, -0.955423f, -0.295242f},
    {0.000000f, -1.000000f, 0.000000f},
    {0.262866f, -0.951056f, -0.162460f},
    {0.000000f, -0.850651f, 0.525731f},
    {0.000000f, -0.955423f, 0.295242f},
    {0.238856f, -0.864188f, 0.442863f},
    {0.262866f, -0.951056f, 0.162460f},
    {0.500000f, -0.809017f, 0.309017f},
    {0.716567f, -0.681718f, 0.147621f},
    {0.525731f, -0.850651f, 0.000000f},
    {-0.238856f, -0.864188f, -0.442863f},
    {-0.500000f, -0.809017f, -0.309017f},
    {-0.262866f, -0.951056f, -0.162460f},
    {-0.850651f, -0.525731f, 0.000000f},
    {-0.716567f, -0.681718f, -0.147621f},
    {-0.716567f, -0.681718f, 0.147621f},
    {-0.525731f, -0.850651f, 0.000000f},
    {-0.500000f, -0.809017f, 0.309017f},
    {-0.238856f, -0.864188f, 0.442863f},
    {-0.262866f, -0.951056f, 0.162460f},
    {-0.864188f, -0.442863f, 0.238856f},
    {-0.809017f, -0.309017f, 0.500000f},
    {-0.688191f, -0.587785f, 0.425325f},
    {-0.681718f, -0.147621f, 0.716567f},
    {-0.442863f, -0.238856f, 0.864188f},
    {-0.587785f, -0.425325f, 0.688191f},
    {-0.309017f, -0.500000f, 0.809017f},
    {-0.147621f, -0.716567f, 0.681718f},
    {-0.425325f, -0.688191f, 0.587785f},
    {-0.162460f, -0.262866f, 0.951056f},
    {0.442863f, -0.238856f, 0.864188f},
    {0.162460f, -0.262866f, 0.951056f},
    {0.309017f, -0.500000f, 0.809017f},
    {0.147621f, -0.716567f, 0.681718f},
    {0.000000f, -0.525731f, 0.850651f},
    {0.425325f, -0.688191f, 0.587785f},
    {0.587785f, -0.425325f, 0.688191f},
    {0.688191f, -0.587785f, 0.425325f},
    {-0.955423f, 0.295242f, 0.000000f},
    {-0.951056f, 0.162460f, 0.262866f},
    {-1.000000f, 0.000000f, 0.000000f},
    {-0.850651f, 0.000000f, 0.525731f},
    {-0.955423f, -0.295242f, 0.000000f},
    {-0.951056f, -0.162460f, 0.262866f},
    {-0.864188f, 0.442863f, -0.238856f},
    {-0.951056f, 0.162460f, -0.262866f},
    {-0.809017f, 0.309017f, -0.500000f},
    {-0.864188f, -0.442863f, -0.238856f},
    {-0.951056f, -0.162460f, -0.262866f},
    {-0.809017f, -0.309017f, -0.500000f},
    {-0.681718f, 0.147621f, -0.716567f},
    {-0.681718f, -0.147621f, -0.716567f},
    {-0.850651f, 0.000000f, -0.525731f},
    {-0.688191f, 0.587785f, -0.425325f},
    {-0.587785f, 0.425325f, -0.688191f},
    {-0.425325f, 0.688191f, -0.587785f},
    {-0.425325f, -0.688191f, -0.587785f},
    {-0.587785f, -0.425325f, -0.688191f},
    {-0.688191f, -0.587785f, -0.425325f},
  };
  static float swz[162][3];
  static bool done = false;
  if (!done) {
    for (int k = 0; k < 162; ++k) {
      swz[k][0] = raw[k][1];
      swz[k][1] = raw[k][2];
      swz[k][2] = raw[k][0];
    }
    done = true;
  }
  return &swz[0][0];
}

// Die Daten einer Datei; Kopien einer Entity teilen sie (MD2Model::Rep).
struct bb_Md2Rep_ {
  struct Frame {
    float scale[3], trans[3];
    std::vector<unsigned char> v;   // je Vertex x,y,z,n - schon vertauscht
  };
  int n_frames = 0, n_verts = 0, n_tris = 0;
  std::vector<Frame>        frames;
  std::vector<float>        uvs;     // je Vertex u,v
  std::vector<unsigned int> indices; // je Dreieck 0,2,1
  float boxA[3] = {  INFINITY,  INFINITY,  INFINITY };
  float boxB[3] = { -INFINITY, -INFINITY, -INFINITY };

  // Lage und Normale von Vertex k in Frame f, wie t_a/n_a im Original.
  void vert(int f, int k, float* t, const float** n) const {
    const Frame& fr = frames[f];
    const unsigned char* v = &fr.v[static_cast<size_t>(k) * 4];
    t[0] = v[0] * fr.scale[0] + fr.trans[0];
    t[1] = v[1] * fr.scale[1] + fr.trans[1];
    t[2] = v[2] * fr.scale[2] + fr.trans[2];
    *n = bb_md2_norms_() + v[3] * 3;
  }
};

// MD2Rep::MD2Rep. Liefert nullptr, wenn die Datei fehlt oder kein MD2 der
// Version 8 ist - dann gibt LoadMD2 im Original 0 zurueck.
inline std::shared_ptr<bb_Md2Rep_> bb_md2_load_(const char* path) {
  std::FILE* f = std::fopen(path, "rb");
  if (!f) return nullptr;
  struct Hdr {
    int magic, version, skinWidth, skinHeight, frameSize, numSkins,
        numVertices, numTexCoords, numTriangles, numGlCommands, numFrames,
        offsetSkins, offsetTexCoords, offsetTriangles, offsetFrames,
        offsetGlCommands, offsetEnd;
  } h;
  auto rd = [f](void* p, size_t n, long at) {
    if (at >= 0 && std::fseek(f, at, SEEK_SET) != 0) return false;
    return std::fread(p, 1, n, f) == n;
  };
  if (!rd(&h, sizeof(h), 0) ||
      std::memcmp(&h.magic, "IDP2", 4) != 0 || h.version != 8 ||
      h.numFrames <= 0 || h.numTriangles < 0 || h.numVertices <= 0 ||
      h.numTexCoords < 0) {
    std::fclose(f);
    return nullptr;
  }

  auto rep = std::make_shared<bb_Md2Rep_>();
  rep->n_frames = h.numFrames;
  rep->n_tris   = h.numTriangles;

  std::vector<short> st(static_cast<size_t>(h.numTexCoords) * 2);
  std::vector<unsigned short> tr(static_cast<size_t>(h.numTriangles) * 6);
  if ((!st.empty() && !rd(st.data(), st.size() * 2, h.offsetTexCoords)) ||
      (!tr.empty() && !rd(tr.data(), tr.size() * 2, h.offsetTriangles))) {
    std::fclose(f);
    return nullptr;
  }

  // Vertex = (Index, UV-Index); gleiche Paare teilen sich einen Vertex. Der
  // Vergleich des Originals (memcmp ueber beide Shorts) ordnet nur die Map,
  // die Nummern entstehen in der Reihenfolge des ersten Auftretens.
  std::map<unsigned, int> seen;
  std::vector<unsigned short> vidx;
  for (int k = 0; k < h.numTriangles; ++k) {
    unsigned tv[3];
    for (int j = 0; j < 3; ++j) {
      const unsigned short i  = tr[static_cast<size_t>(k) * 6 + j];
      const unsigned short uv = tr[static_cast<size_t>(k) * 6 + 3 + j];
      const unsigned key = (static_cast<unsigned>(uv) << 16) | i;
      auto it = seen.find(key);
      if (it == seen.end()) {
        const int n = static_cast<int>(vidx.size());
        seen.emplace(key, n);
        vidx.push_back(i);
        const float u = (uv < h.numTexCoords) ? st[uv * 2]     : 0;
        const float v = (uv < h.numTexCoords) ? st[uv * 2 + 1] : 0;
        rep->uvs.push_back(u / static_cast<float>(h.skinWidth));
        rep->uvs.push_back(v / static_cast<float>(h.skinHeight));
        tv[j] = static_cast<unsigned>(n);
      } else {
        tv[j] = static_cast<unsigned>(it->second);
      }
    }
    rep->indices.push_back(tv[0]);
    rep->indices.push_back(tv[2]);
    rep->indices.push_back(tv[1]);
  }
  rep->n_verts = static_cast<int>(vidx.size());

  rep->frames.resize(h.numFrames);
  std::vector<unsigned char> raw(static_cast<size_t>(h.numVertices) * 4);
  if (std::fseek(f, h.offsetFrames, SEEK_SET) != 0) { std::fclose(f); return nullptr; }
  for (int k = 0; k < h.numFrames; ++k) {
    bb_Md2Rep_::Frame& fr = rep->frames[k];
    float s[3], t[3];
    char name[16];
    if (!rd(s, 12, -1) || !rd(t, 12, -1) || !rd(name, 16, -1) ||
        !rd(raw.data(), raw.size(), -1)) {
      std::fclose(f);
      return nullptr;
    }
    fr.scale[0] = s[1]; fr.scale[1] = s[2]; fr.scale[2] = s[0];
    fr.trans[0] = t[1]; fr.trans[1] = t[2]; fr.trans[2] = t[0];
    fr.v.resize(static_cast<size_t>(rep->n_verts) * 4);
    for (int j = 0; j < rep->n_verts; ++j) {
      const unsigned char* mv = &raw[static_cast<size_t>(vidx[j] % h.numVertices) * 4];
      unsigned char* v = &fr.v[static_cast<size_t>(j) * 4];
      v[0] = mv[1];
      v[1] = mv[2];
      v[2] = mv[0];
      v[3] = (mv[3] < 162) ? mv[3] : 0;
      for (int c = 0; c < 3; ++c) {
        const float p = v[c] * fr.scale[c] + fr.trans[c];
        if (p < rep->boxA[c]) rep->boxA[c] = p;
        if (p > rep->boxB[c]) rep->boxB[c] = p;
      }
    }
  }
  std::fclose(f);
  return rep;
}

// Animationsmodi wie Animator::ANIM_MODE_*; 0x8000 = Uebergang laeuft.
inline constexpr int BB_MD2_LOOP     = 1;
inline constexpr int BB_MD2_PINGPONG = 2;
inline constexpr int BB_MD2_TRANS    = 0x8000;

struct bb_Md2Entity_ : bb_Entity_ {
  std::shared_ptr<const bb_Md2Rep_> rep;
  int   anim_mode = 0;
  float anim_time = 0, anim_speed = 0;
  int   anim_first = 0, anim_last = 0, anim_len = 0;
  float render_t = 0;
  int   render_a = 0, render_b = 0;
  float trans_time = 0, trans_speed = 0;
  std::vector<float> trans_verts;   // je Vertex Lage und Normale
  bb_MeshData_ mesh;                // je Zeichnen neu gefuellt

  bb_EntityKind_ kind() const override { return bb_EntityKind_::Md2; }

  // MD2Model::MD2Model(const MD2Model&): dieselben Daten, keine Animation.
  std::unique_ptr<bb_Entity_> clone() const override {
    auto c = std::make_unique<bb_Md2Entity_>();
    bb_entity_copy_fields_(*c, *this);
    c->rep = rep;
    return c;
  }

  ~bb_Md2Entity_() override { bb_mesh_free_gpu_(&mesh); }
};

static inline bb_Md2Entity_* bb_md2_ent_(int h) {
  bb_Entity_* e = bb_entity_get_(h);
  if (!e || e->kind() != bb_EntityKind_::Md2) return nullptr;
  return static_cast<bb_Md2Entity_*>(e);
}

// debugMD2 (BUG-170): erst debugModel wie bei den Sprites.
static inline bb_Md2Entity_* bb_md2_chk_(int h) {
  if (bb_model_chk_(h)->kind() != bb_EntityKind_::Md2)
    bb_RuntimeError("Entity is not an MD2 Model");
  return bb_md2_ent_(h);
}

// MD2Rep::render(Vert*, frame_a, frame_b, t): den gezeigten Stand festhalten.
inline void bb_md2_capture_(bb_Md2Entity_* m, int a, int b, float t) {
  const bb_Md2Rep_& r = *m->rep;
  m->trans_verts.resize(static_cast<size_t>(r.n_verts) * 6);
  for (int k = 0; k < r.n_verts; ++k) {
    float ta[3], tb[3];
    const float *na, *nb;
    r.vert(a, k, ta, &na);
    r.vert(b, k, tb, &nb);
    float* v = &m->trans_verts[static_cast<size_t>(k) * 6];
    for (int c = 0; c < 3; ++c) {
      v[c]     = (tb[c] - ta[c]) * t + ta[c];
      v[3 + c] = (nb[c] - na[c]) * t + na[c];
    }
  }
}

// MD2Rep::render(Vert*, frame, time): einen laufenden Uebergang weiter zum
// Frame hin ziehen - so wird ein neuer Uebergang waehrend eines alten gebaut.
inline void bb_md2_capture_toward_(bb_Md2Entity_* m, int frame, float t) {
  const bb_Md2Rep_& r = *m->rep;
  if (m->trans_verts.size() != static_cast<size_t>(r.n_verts) * 6)
    m->trans_verts.assign(static_cast<size_t>(r.n_verts) * 6, 0.0f);
  for (int k = 0; k < r.n_verts; ++k) {
    float tb[3];
    const float* nb;
    r.vert(frame, k, tb, &nb);
    float* v = &m->trans_verts[static_cast<size_t>(k) * 6];
    for (int c = 0; c < 3; ++c) {
      v[c]     += (tb[c] - v[c]) * t;
      v[3 + c] += (nb[c] - v[3 + c]) * t;
    }
  }
}

// ---- LoadMD2 ----

inline int bb_LoadMD2(const bbString& file, int parent = 0) {
  bb_parent_chk_(parent);   // vor dem Laden (BUG-170)
  auto rep = bb_md2_load_(file.c_str());
  if (!rep) return 0;
  auto m = std::make_unique<bb_Md2Entity_>();
  m->rep = rep;
  return bb_entity_register_(std::move(m), parent);
}

// ---- AnimateMD2 (MD2Model::startMD2Anim) ----

inline void bb_AnimateMD2(int md2, int mode = 1, float speed = 1.0f,
                          int first = 0, int last = 9999, float trans = 0.0f) {
  bb_Md2Entity_* m = bb_md2_chk_(md2);
  if (!m) return;
  const int nf = m->rep->n_frames;

  if (last < first) std::swap(first, last);
  if (first < 0) first = 0;
  else if (first >= nf) first = nf - 1;
  if (last < 0) last = 0;
  else if (last >= nf) last = nf - 1;

  if (trans > 0) {
    if (m->anim_mode & BB_MD2_TRANS)
      bb_md2_capture_toward_(m, static_cast<int>(m->anim_time), m->trans_time);
    else
      bb_md2_capture_(m, m->render_a, m->render_b, m->render_t);
    m->trans_speed = 1.0f / trans;
    m->trans_time  = 0;
    mode |= BB_MD2_TRANS;
  }

  m->anim_first = first;
  m->anim_last  = last;
  m->anim_len   = last - first;
  m->anim_speed = speed;
  m->anim_time  = static_cast<float>(
      ((mode & 0x7fff) == BB_MD2_LOOP || m->anim_speed >= 0) ? m->anim_first
                                                               : m->anim_last);
  m->anim_mode = mode;

  if (!m->anim_speed || !m->anim_len) {
    m->render_a = m->render_b = static_cast<int>(m->anim_time);
    m->render_t = 0;
    m->anim_mode &= BB_MD2_TRANS;
  }
}

inline float bb_MD2AnimTime(int md2) {
  bb_Md2Entity_* m = bb_md2_chk_(md2);
  return m ? m->anim_time : 0.0f;
}

inline int bb_MD2AnimLength(int md2) {
  bb_Md2Entity_* m = bb_md2_chk_(md2);
  return m ? m->rep->n_frames : 0;
}

inline int bb_MD2Animating(int md2) {
  bb_Md2Entity_* m = bb_md2_chk_(md2);
  return (m && m->anim_mode) ? 1 : 0;
}

// ---- UpdateWorld (MD2Model::animate) ----

inline void bb_md2_animate_(bb_Md2Entity_* m, float e) {
  if (!m->anim_mode) return;
  if (m->anim_mode & BB_MD2_TRANS) {
    m->trans_time += m->trans_speed;
    if (m->trans_time < 1) return;
    m->anim_mode &= ~BB_MD2_TRANS;
    if (!m->anim_mode) return;
  }
  m->anim_time = m->anim_time + m->anim_speed * e;
  if (m->anim_time < m->anim_first) {
    switch (m->anim_mode) {
      case BB_MD2_LOOP:
        m->anim_time += m->anim_len;
        break;
      case BB_MD2_PINGPONG:
        m->anim_time = m->anim_first + (m->anim_first - m->anim_time);
        m->anim_speed = -m->anim_speed;
        break;
      default:
        m->anim_time = static_cast<float>(m->anim_first);
        m->anim_mode = 0;
        break;
    }
  } else if (m->anim_time >= m->anim_last) {
    switch (m->anim_mode) {
      case BB_MD2_LOOP:
        m->anim_time -= m->anim_len;
        break;
      case BB_MD2_PINGPONG:
        m->anim_time = m->anim_last - (m->anim_time - m->anim_last);
        m->anim_speed = -m->anim_speed;
        break;
      default:
        m->anim_time = static_cast<float>(m->anim_last);
        m->anim_mode = 0;
        break;
    }
  }
  m->render_a = static_cast<int>(std::floor(m->anim_time));
  m->render_b = m->render_a + 1;
  if (m->anim_mode == BB_MD2_LOOP && m->render_b == m->anim_last)
    m->render_b = m->anim_first;
  m->render_t = m->anim_time - m->render_a;
  // Endet eine Animation auf dem letzten Frame der Datei, liest das Original
  // hier Frame n_frames und stuerzt ab (gemessen: Speicherzugriffsfehler).
  // Wir bleiben auf dem letzten Frame; mit render_t = 0 ist das dasselbe
  // Bild, das dort zu sehen sein muesste.
  if (m->render_b >= m->rep->n_frames) m->render_b = m->rep->n_frames - 1;
}

// Alle sichtbaren MD2 um `e` weiterschalten - World::update ruft animate
// nur fuer eingeschaltete (nicht versteckte) Objekte.
inline void bb_md2_animate_all_(float e) {
  for (auto& [h, ent] : bb_entities_) {
    if (ent->kind() != bb_EntityKind_::Md2) continue;
    if (!bb_entity_shown_(ent.get())) continue;
    bb_md2_animate_(static_cast<bb_Md2Entity_*>(ent.get()), e);
  }
}

// ---- Zeichnen (MD2Model::render / MD2Rep::render) ----

// Die Vertices fuer den aktuellen Stand in das Netz schreiben.
inline void bb_md2_build_(bb_Md2Entity_* m) {
  const bb_Md2Rep_& r = *m->rep;
  bb_MeshData_& mesh = m->mesh;
  if (mesh.indices.size() != r.indices.size()) mesh.indices = r.indices;
  mesh.vertices.resize(static_cast<size_t>(r.n_verts) * BB_VF);
  const bool trans = (m->anim_mode & BB_MD2_TRANS) != 0;
  int fb = trans ? static_cast<int>(m->anim_time) : m->render_b;
  if (fb < 0) fb = 0;
  if (fb >= r.n_frames) fb = r.n_frames - 1;
  const float t    = trans ? m->trans_time : m->render_t;
  for (int k = 0; k < r.n_verts; ++k) {
    float ta[3], tb[3];
    const float *na, *nb;
    r.vert(fb, k, tb, &nb);
    float* o = &mesh.vertices[static_cast<size_t>(k) * BB_VF];
    if (trans && m->trans_verts.size() == static_cast<size_t>(r.n_verts) * 6) {
      const float* v = &m->trans_verts[static_cast<size_t>(k) * 6];
      for (int c = 0; c < 3; ++c) {
        o[c]     = (tb[c] - v[c]) * t + v[c];
        o[3 + c] = (nb[c] - v[3 + c]) * t + v[3 + c];
      }
    } else {
      r.vert(m->render_a, k, ta, &na);
      for (int c = 0; c < 3; ++c) {
        o[c]     = (tb[c] - ta[c]) * t + ta[c];
        o[3 + c] = (nb[c] - na[c]) * t + na[c];
      }
    }
    const float u = r.uvs[static_cast<size_t>(k) * 2];
    const float v = r.uvs[static_cast<size_t>(k) * 2 + 1];
    o[6] = u; o[7] = v; o[8] = u; o[9] = v;
    o[10] = o[11] = o[12] = o[13] = 1.0f;
  }
  mesh.dirty = true;
}

#endif // BB_MD2_H
