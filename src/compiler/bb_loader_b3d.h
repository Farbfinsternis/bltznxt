#ifndef BLITZNEXT_BB_LOADER_B3D_H
#define BLITZNEXT_BB_LOADER_B3D_H

// ============================================================
//  Blitz3D-Modelle  -  bb_loader_b3d.h   (3D-19)
//
//  .b3d ist das eigene Format von Blitz3D. In der Installation liegt keine
//  einzige Datei davon, aber alte Programme, die mit Exportern der Zeit
//  gebaut wurden, laden es - und es ist das einzige Format mit Knochen und
//  Gewichten. Nachgebaut nach blitz3d/loader_b3d.cpp und meshloader.cpp.
//
//  Die Datei ist ein Baum aus Chunks: vier Zeichen Kennung, vier Byte Laenge
//  (ohne den Kopf), alles little-endian.
//
//    BB3D  Version (1)            TEXS  Texturen      BRUS  Brushes
//    NODE  Name, Lage, Skalierung, Drehung (w,x,y,z), dann Unterchunks:
//          MESH  Brush, VRTS (Vertices), TRIS (Dreiecke je Brush)
//          BONE  Vertex/Gewicht-Paare
//          KEYS  Schluessel: Bild, dann je nach Flags Lage, Skalierung,
//                Drehung
//          ANIM  Flags, Bilder, Bildrate
//          NODE  Kinder
//
//  Was die Knoten werden:
//
//  * Ein Knoten mit MESH wird ein Mesh, einer mit BONE ein Pivot, alle
//    anderen ein leeres Mesh (auch wenn sie nur Kinder haben).
//  * Knochen sammeln sich in einer **einzigen** Liste, bis ein Netz sie
//    abholt: am Ende des Knotens, der das Netz traegt, wird es selbst Knochen
//    0, die gesammelten Knochen folgen, und das Netz bekommt einen Animator
//    ueber genau diese Liste. Die Gewichte eines BONE gelten dem Netz, das
//    beim Lesen gerade offen ist (dem innersten).
//  * Ein Knoten mit ANIM, der kein solches Netz ist, bekommt einen Animator
//    ueber sich und seine Nachfahren.
//  * Je Vertex bleiben die vier schwersten Knochen, ihre Gewichte werden auf
//    die Summe 1 gebracht. Ein Vertex ohne Knochen folgt Knochen 0, dem Netz.
//  * Die Ruhelage jedes Knochens ist seine Weltlage in dem Moment, in dem das
//    Netz fertig ist - das Netz haengt dann noch an keinem Elternteil. Beim
//    Zeichnen wird jeder Vertex mit (Knochen jetzt) * (Ruhelage)^-1 bewegt,
//    und das Ergebnis steht im Weltraum.
//
//  Eigenheiten, die mit Absicht mitkommen: ohne Normalen (VRTS-Flag 1)
//  werden sie berechnet; ein Texturkoordinatensatz, der nicht in der Datei
//  steht, ist 0 (nicht eine Kopie des ersten); die Vertexfarbe wird auf 0..1
//  geklemmt und auf 8 Bit abgeschnitten; die Datei nennt Texturen, die
//  zuerst im Ordner des Modells (nur der Dateiname) und dann wie angegeben
//  gesucht werden.
// ============================================================

#include "bb_mesh.h"
#include "bb_animation.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

// ---- Chunks lesen, mit Grenzen ----

struct bb_B3DIn_ {
  const uint8_t*      d = nullptr;
  size_t              n = 0, pos = 0;
  std::vector<size_t> ends;
  bool                bad = false;

  size_t limit() const { return ends.empty() ? n : ends.back(); }
  bool   take(void* out, size_t len) {
    if (pos + len > limit()) { bad = true; pos = limit(); std::memset(out, 0, len); return false; }
    std::memcpy(out, d + pos, len);
    pos += len;
    return true;
  }
  int32_t  i32() { int32_t v; take(&v, 4); return v; }
  float    f32() { float v;   take(&v, 4); return v; }
  std::string str() {
    std::string s;
    while (pos < limit() && d[pos]) s += static_cast<char>(d[pos++]);
    if (pos < limit()) ++pos;
    return s;
  }
  // Kennung des naechsten Chunks, "" wenn keiner mehr passt.
  std::string chunk() {
    if (pos + 8 > limit()) return "";
    char tag[4];
    std::memcpy(tag, d + pos, 4);
    int32_t len;
    std::memcpy(&len, d + pos + 4, 4);
    pos += 8;
    size_t end = (len < 0) ? limit() : pos + static_cast<size_t>(len);
    if (end > limit()) end = limit();
    ends.push_back(end);
    return std::string(tag, 4);
  }
  void   leave() { pos = ends.back(); ends.pop_back(); }
  size_t left() const { return limit() > pos ? limit() - pos : 0; }
};

// ---- Das Netz, das gerade gelesen wird (MeshLoader) ----

struct bb_B3DVert_ {
  float    pos[3] = { 0, 0, 0 }, nrm[3] = { 0, 0, 0 };
  float    rgba[4] = { 1, 1, 1, 1 };
  float    uv[2][2] = { { 0, 0 }, { 0, 0 } };
  uint8_t  bone[4] = { 255, 255, 255, 255 };
  float    w[4] = { 0, 0, 0, 0 };
};

struct bb_B3DMesh_ {
  std::vector<bb_B3DVert_>            verts;
  std::vector<int>                    tri_brush;   // je Dreieck: Brushindex (-1 = keiner)
  std::vector<std::array<int, 3>>     tris;
};

// MeshLoader::addBone: die vier schwersten behalten, absteigend sortiert.
inline void bb_b3d_add_bone_(bb_B3DVert_& v, float w, int b) {
  int i = 0;
  for (; i < 4; ++i)
    if (v.bone[i] == 255 || w > v.w[i]) break;
  if (i == 4) return;
  for (int k = 3; k > i; --k) { v.bone[k] = v.bone[k - 1]; v.w[k] = v.w[k - 1]; }
  v.bone[i] = static_cast<uint8_t>(b);
  v.w[i] = w;
}

struct bb_B3DLoad_ {
  bb_B3DIn_                  in;
  std::filesystem::path      dir;
  std::vector<int>           textures;   // Handles, 0 = fehlt
  std::vector<bb_Brush_>     brushes;
  std::vector<int>           bones;      // gesammelte Knochen (Handles)
  std::vector<bb_B3DMesh_*>  mesh_stack; // offene Netze, das letzte ist das innerste
};

inline int bb_b3d_texture_(const bb_B3DLoad_& L, const std::string& name, int flags) {
  if (name.empty()) return 0;
  std::string f = name;
  if (f.rfind(".\\", 0) == 0 || f.rfind("./", 0) == 0) f = f.substr(2);
  std::error_code ec;
  const std::filesystem::path only = std::filesystem::path(f).filename();
  std::filesystem::path cand = L.dir / only;
  if (!std::filesystem::exists(cand, ec)) cand = f;
  return bb_LoadTexture(cand.string(), flags);
}

inline void bb_b3d_textures_(bb_B3DLoad_& L) {
  while (L.in.left() && !L.in.bad) {
    const std::string name = L.in.str();
    const int flags = L.in.i32();
    const int blend = L.in.i32();
    float pos[2] = { L.in.f32(), L.in.f32() };
    float scl[2] = { L.in.f32(), L.in.f32() };
    const float rot = L.in.f32();
    const int t = bb_b3d_texture_(L, name, flags & 0xffff);
    if (t) {
      bb_TextureBlend(t, blend);
      if (flags & 0x10000) bb_TextureCoords(t, 1);
      if (pos[0] != 0 || pos[1] != 0) bb_PositionTexture(t, pos[0], pos[1]);
      if (scl[0] != 1 || scl[1] != 1) bb_ScaleTexture(t, scl[0], scl[1]);
      if (rot != 0) bb_RotateTexture(t, rot);
    }
    L.textures.push_back(t);
  }
}

inline void bb_b3d_brushes_(bb_B3DLoad_& L) {
  const int n_texs = L.in.i32();
  int tex_id[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
  while (L.in.left() && !L.in.bad) {
    L.in.str();                                 // Name, bleibt ungenutzt
    float col[4];
    for (float& c : col) c = L.in.f32();
    bb_Brush_ br;
    br.r = col[0] * 255.0f; br.g = col[1] * 255.0f; br.b = col[2] * 255.0f;
    br.alpha     = col[3];
    br.shininess = L.in.f32();
    br.blend     = L.in.i32();
    br.fx        = L.in.i32();
    // Wie das Original: die Kennungen bleiben ueber die Brushes hinweg
    // stehen, nur die ersten n_texs werden je Brush neu gelesen.
    for (int k = 0; k < n_texs; ++k) {
      const int v = L.in.i32();
      if (k < 8) tex_id[k] = v;
    }
    for (int k = 0; k < 8; ++k) {
      if (tex_id[k] < 0 || tex_id[k] >= static_cast<int>(L.textures.size())) continue;
      if (const int t = L.textures[tex_id[k]]) {
        br.tex.tex[k]   = bb_texture_ref_(t);
        br.tex.frame[k] = 0;
      }
    }
    L.brushes.push_back(br);
  }
}

inline int bb_b3d_vertices_(bb_B3DLoad_& L, bb_B3DMesh_& m) {
  const int flags   = L.in.i32();
  const int tc_sets = L.in.i32();
  const int tc_size = L.in.i32();
  float tc[4] = { 0, 0, 0, 0 };
  bb_B3DVert_ t;                                // bleibt wie im Original stehen
  while (L.in.left() && !L.in.bad) {
    for (float& c : t.pos) c = L.in.f32();
    if (flags & 1) for (float& c : t.nrm) c = L.in.f32();
    if (flags & 2) {
      for (float& c : t.rgba) {
        float v = L.in.f32();
        v = v < 0 ? 0 : (v > 1 ? 1 : v);
        c = static_cast<int>(v * 255) / 255.0f;
      }
    }
    for (int k = 0; k < tc_sets; ++k) {
      for (int i = 0; i < tc_size; ++i) {
        const float v = L.in.f32();
        if (i < 4) tc[i] = v;
      }
      if (k < 2) { t.uv[k][0] = tc[0]; t.uv[k][1] = tc[1]; }
    }
    if (L.in.bad) break;
    m.verts.push_back(t);
  }
  return flags;
}

inline void bb_b3d_triangles_(bb_B3DLoad_& L, bb_B3DMesh_& m) {
  const int brush_id = L.in.i32();
  while (L.in.left() >= 12 && !L.in.bad) {
    std::array<int, 3> v = { L.in.i32(), L.in.i32(), L.in.i32() };
    m.tris.push_back(v);
    m.tri_brush.push_back(brush_id);
  }
}

inline void bb_b3d_keys_(bb_B3DLoad_& L, bb_AnimKeys_& keys) {
  const int flags = L.in.i32();
  while (L.in.left() && !L.in.bad) {
    const int frame = L.in.i32();
    if (flags & 1) {
      float p[3] = { L.in.f32(), L.in.f32(), L.in.f32() };
      keys.pos[frame] = { p[0], p[1], p[2] };
    }
    if (flags & 2) {
      float s[3] = { L.in.f32(), L.in.f32(), L.in.f32() };
      keys.scl[frame] = { s[0], s[1], s[2] };
    }
    if (flags & 4) {
      float r[4] = { L.in.f32(), L.in.f32(), L.in.f32(), L.in.f32() };
      keys.rot[frame] = { r[0], r[1], r[2], r[3] };
    }
    if (L.in.bad) break;
  }
}

// Die Reihenfolge der Flaechen. MeshLoader sammelt die Dreiecke in einer
// map<Brush,...>, und Brush::operator< ist ein memcmp ueber den
// Renderzustand: Farbe (0..1), Glanz, Deckkraft, der aufgeloeste Blendmodus,
// FX, dann je Texturlage ein Zeiger. Die Flaechen kommen also nicht in
// Dateifolge, sondern nach den Bytes dieser Werte - gemessen: Rot (1,0,0)
// steht hinter Gruen (0,1,0). Gleiche Schluessel sind dieselbe Flaeche.
struct bb_B3DBrushKey_ {
  struct { float color[3], shininess, alpha; int blend, fx; } rs;
  const void* tex[BB_TEX_SLOTS];
};

inline bb_B3DBrushKey_ bb_b3d_brush_key_(const bb_Brush_& b) {
  bb_B3DBrushKey_ k;
  std::memset(&k, 0, sizeof k);
  k.rs.color[0] = b.r / 255.0f; k.rs.color[1] = b.g / 255.0f; k.rs.color[2] = b.b / 255.0f;
  k.rs.shininess = b.shininess;
  k.rs.alpha     = b.alpha;
  k.rs.fx        = b.fx;
  int ntex = 0;
  bool transp = false;
  for (int i = 0; i < BB_TEX_SLOTS; ++i) {
    k.tex[i] = b.tex.tex[i].get();
    if (b.tex.tex[i]) {
      ntex = i + 1;
      if (b.tex.tex[i]->flags & (2 | 4)) transp = true;
    }
  }
  // Brush::getBlend: Maske in Lage 0 setzt FX_ALPHATEST.
  if (b.tex.tex[0] && (b.tex.tex[0]->flags & 4)) k.rs.fx |= 0x2000;
  if (b.blend && b.blend != 1)                     k.rs.blend = b.blend;
  else if ((b.blend == 1 || ntex == 1) && transp)  k.rs.blend = 1;
  else if ((b.fx & 32) || b.alpha < 1)             k.rs.blend = 1;
  else                                             k.rs.blend = 0;
  return k;
}

inline int bb_b3d_brush_cmp_(const bb_Brush_& a, const bb_Brush_& b) {
  const bb_B3DBrushKey_ ka = bb_b3d_brush_key_(a), kb = bb_b3d_brush_key_(b);
  const int c = std::memcmp(&ka.rs, &kb.rs, sizeof ka.rs);
  if (c) return c;
  for (int i = 0; i < BB_TEX_SLOTS; ++i)
    if (ka.tex[i] != kb.tex[i]) return std::less<const void*>()(ka.tex[i], kb.tex[i]) ? -1 : 1;
  return 0;
}

// MeshLoader::endMesh: Gewichte normieren, Dreiecke nach Brush in
// Flaechen, Vertices je Flaeche nur einmal.
inline void bb_b3d_end_mesh_(bb_B3DLoad_& L, bb_MeshEntity_* me, const bb_B3DMesh_& m) {
  std::vector<bb_B3DVert_> verts = m.verts;
  for (auto& v : verts) {
    if (v.bone[0] == 255) continue;
    float sum = 0;
    int j = 0;
    for (; j < 4 && v.bone[j] != 255; ++j) sum += v.w[j];
    if (sum != 0) for (int i = 0; i < j; ++i) v.w[i] /= sum;
  }
  auto brush_of = [&](int bid) {
    return (bid >= 0 && bid < static_cast<int>(L.brushes.size())) ? L.brushes[bid] : bb_Brush_();
  };
  // Eine Flaeche je verschiedenem Brush, sortiert wie die map im Original.
  std::vector<bb_Brush_> order;
  for (size_t t = 0; t < m.tris.size(); ++t) {
    const bb_Brush_ b = brush_of(m.tri_brush[t]);
    bool found = false;
    for (const auto& o : order) if (!bb_b3d_brush_cmp_(o, b)) { found = true; break; }
    if (!found) order.push_back(b);
  }
  std::stable_sort(order.begin(), order.end(), [](const bb_Brush_& a, const bb_Brush_& b) {
    return bb_b3d_brush_cmp_(a, b) < 0; });
  const int nv = static_cast<int>(verts.size());
  for (const bb_Brush_& sb : order) {
    me->surfaces().emplace_back();
    bb_MeshData_& s = me->surfaces().back();
    s.brush = sb;
    std::unordered_map<int, unsigned> vmap;
    for (size_t t = 0; t < m.tris.size(); ++t) {
      if (bb_b3d_brush_cmp_(brush_of(m.tri_brush[t]), sb)) continue;
      const auto& tri = m.tris[t];
      if (tri[0] < 0 || tri[0] >= nv || tri[1] < 0 || tri[1] >= nv ||
          tri[2] < 0 || tri[2] >= nv) continue;
      for (int k = 0; k < 3; ++k) {
        const int vi = tri[k];
        auto it = vmap.find(vi);
        unsigned id;
        if (it != vmap.end()) {
          id = it->second;
        } else {
          id = static_cast<unsigned>(s.vertices.size() / BB_VF);
          const bb_B3DVert_& v = verts[vi];
          const float vd[BB_VF] = { v.pos[0], v.pos[1], v.pos[2],
                                    v.nrm[0], v.nrm[1], v.nrm[2],
                                    v.uv[0][0], v.uv[0][1], v.uv[1][0], v.uv[1][1],
                                    v.rgba[0], v.rgba[1], v.rgba[2], v.rgba[3] };
          s.vertices.insert(s.vertices.end(), vd, vd + BB_VF);
          s.bone_ids.insert(s.bone_ids.end(), v.bone, v.bone + 4);
          s.bone_w.insert(s.bone_w.end(), v.w, v.w + 4);
          vmap.emplace(vi, id);
        }
        s.indices.push_back(id);
      }
    }
    s.dirty = true;
  }
  // Ohne ein einziges Gewicht braucht die Flaeche keine Knochendaten.
  for (auto& s : me->surfaces()) {
    bool any = false;
    for (size_t i = 0; i < s.bone_ids.size(); i += 4) if (s.bone_ids[i] != 255) { any = true; break; }
    if (!any) { s.bone_ids.clear(); s.bone_w.clear(); }
  }
}

// Ein Kind an seinen Elternteil haengen (setParent am Ende von readObject).
inline void bb_b3d_attach_(int h, int parent) {
  if (!parent) return;
  bb_Entity_* e = bb_entity_get_(h);
  e->parent = parent;
  bb_entity_get_(parent)->children.push_back(h);
}

// Ein neues Entity, vorerst ohne Elternteil.
template <class T>
inline int bb_b3d_new_() { return bb_entity_register_(std::make_unique<T>(), 0); }

inline int bb_b3d_object_(bb_B3DLoad_& L, int parent) {
  int obj = 0;
  const std::string name = L.in.str();
  float pos[3], scl[3], rot[4];
  for (float& v : pos) v = L.in.f32();
  for (float& v : scl) v = L.in.f32();
  for (float& v : rot) v = L.in.f32();

  auto keys = std::make_shared<bb_AnimKeys_>();
  int anim_len = 0;
  int mesh = 0, mesh_flags = 0, mesh_brush = -1;
  std::unique_ptr<bb_B3DMesh_> mdata;

  for (std::string tag; !(tag = L.in.chunk()).empty(); L.in.leave()) {
    if (tag == "MESH") {
      mdata = std::make_unique<bb_B3DMesh_>();
      L.mesh_stack.push_back(mdata.get());
      obj = mesh = bb_b3d_new_<bb_MeshEntity_>();
      mesh_brush = L.in.i32();
      for (std::string sub; !(sub = L.in.chunk()).empty(); L.in.leave()) {
        if (sub == "VRTS")      mesh_flags = bb_b3d_vertices_(L, *mdata);
        else if (sub == "TRIS") bb_b3d_triangles_(L, *mdata);
      }
    } else if (tag == "BONE") {
      obj = bb_b3d_new_<bb_PivotEntity_>();
      L.bones.push_back(obj);
      bb_B3DMesh_* m = L.mesh_stack.empty() ? nullptr : L.mesh_stack.back();
      while (L.in.left() >= 8 && !L.in.bad) {
        const int vert = L.in.i32();
        const float weight = L.in.f32();
        if (m && vert >= 0 && vert < static_cast<int>(m->verts.size()))
          bb_b3d_add_bone_(m->verts[vert], weight, static_cast<int>(L.bones.size()));
      }
    } else if (tag == "KEYS") {
      bb_b3d_keys_(L, *keys);
    } else if (tag == "ANIM") {
      L.in.i32();
      anim_len = L.in.i32();
      L.in.f32();
    } else if (tag == "NODE") {
      if (!obj) obj = bb_b3d_new_<bb_MeshEntity_>();
      bb_b3d_object_(L, obj);
    }
  }

  if (!obj) obj = bb_b3d_new_<bb_MeshEntity_>();
  bb_Entity_* e = bb_entity_get_(obj);
  e->name = name;
  bb_ent_set_local_pos_(e, { pos[0], pos[1], pos[2] });
  bb_ent_set_local_scl_(e, { scl[0], scl[1], scl[2] });
  bb_ent_set_local_rot_(e, { rot[0], rot[1], rot[2], rot[3] });
  e->anim = keys;

  if (mesh) {
    L.mesh_stack.pop_back();
    auto* me = static_cast<bb_MeshEntity_*>(bb_entity_get_(mesh));
    bb_b3d_end_mesh_(L, me, *mdata);
    if (!(mesh_flags & 1)) bb_UpdateNormals(mesh);
    // Model::setBrush: der Brush des Netzes gilt fuer die Entity.
    if (mesh_brush >= 0 && mesh_brush < static_cast<int>(L.brushes.size()))
      me->brush = L.brushes[mesh_brush];
  }

  if (mesh && !L.bones.empty()) {
    std::vector<int> list = L.bones;
    list.insert(list.begin(), mesh);
    auto* me = static_cast<bb_MeshEntity_*>(bb_entity_get_(mesh));
    me->animator = bb_animator_new_(list, anim_len);
    // createBones: die Ruhelage ist die Weltlage jetzt - das Netz haengt
    // noch an keinem Elternteil.
    me->rep->bone_inv.clear();
    for (int b : list) {
      bb_Entity_* be = bb_entity_get_(b);
      bb_entity_refresh_world_(be);
      std::array<float, 16> inv;
      if (!mat4_inverse_(inv.data(), be->world)) mat4_identity_(inv.data());
      me->rep->bone_inv.push_back(inv);
    }
    me->boned = true;
    L.bones.clear();
  } else if (anim_len) {
    e->animator = bb_animator_new_(obj, anim_len);
  }

  bb_b3d_attach_(obj, parent);
  // Traegt derselbe Knoten nach dem MESH noch ein BONE, ist das Netz im
  // Original verloren (es haengt nirgends). Hier wuerde es sonst als
  // Wurzel in der Welt stehen.
  if (mesh && mesh != obj) bb_free_entity_(mesh);
  return obj;
}

inline int bb_load_b3d_(const bbString& file) {
  std::FILE* f = std::fopen(file.c_str(), "rb");
  if (!f) {
    std::cerr << "[runtime] LoadMesh: cannot open '" << file << "'\n";
    return 0;
  }
  std::vector<uint8_t> data;
  char buf[65536];
  size_t got;
  while ((got = std::fread(buf, 1, sizeof buf, f)) > 0) data.insert(data.end(), buf, buf + got);
  std::fclose(f);

  bb_B3DLoad_ L;
  L.in.d = data.data();
  L.in.n = data.size();
  L.dir  = std::filesystem::path(file).parent_path();

  if (L.in.chunk() != "BB3D" || L.in.i32() > 1) {
    std::cerr << "[runtime] LoadMesh: '" << file << "' ist keine lesbare .b3d-Datei\n";
    return 0;
  }
  int obj = 0;
  for (std::string tag; !(tag = L.in.chunk()).empty(); L.in.leave()) {
    if (tag == "TEXS")      bb_b3d_textures_(L);
    else if (tag == "BRUS") bb_b3d_brushes_(L);
    else if (tag == "NODE") {
      // Mehrere Wurzelknoten: das Original behaelt den letzten, die
      // frueheren kommen nie in die Welt.
      if (obj) bb_free_entity_(obj);
      obj = bb_b3d_object_(L, 0);
    }
  }
  // Das Original gibt nur ein Netz zurueck; ist die Wurzel ein Knochen
  // (Pivot), faellt die Datei durch.
  if (obj && bb_entity_get_(obj)->kind() != bb_EntityKind_::Mesh) {
    bb_free_entity_(obj);
    return 0;
  }
  return obj;
}

#endif // BLITZNEXT_BB_LOADER_B3D_H
