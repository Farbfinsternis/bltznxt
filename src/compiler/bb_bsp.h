#ifndef BLITZNEXT_BB_BSP_H
#define BLITZNEXT_BB_BSP_H

// Quake-3-Karten: LoadBSP, BSPLighting, BSPAmbientLight (BUG-195).
//
// Nachbau von blitz3d/q3bsprep.cpp: derselbe Weg durch den BSP-Baum ab Knoten
// 0 (nur Flaechen, die ein Blatt erreicht), dieselben Filter (Typ 1/3 mit
// Dreiecksliste, Typ 2 als Bezier-Patch eine Stufe unterteilt; Inhalt "solid"
// noetig, Flags 0x84 = nicht zeichnen), dieselbe Achsdrehung (-y, z, x), die
// Lichtkarten 128x128 mit pow(k/255, 1-gamma_adj) und dieselbe Zuordnung der
// Texturen: Lichtkarte auf Ebene 0 (Koordinaten 0, Mischung Add), Textur auf
// Ebene 1 mit Koordinatensatz 1; ohne Lichtkarte die Textur auf Ebene 0 und
// die Vertexfarben als Grundfarbe. Statt eines eigenen Renderers wird daraus
// ein gewoehnliches Netz: Flaechen, die nur der Kollision dienen (nodraw,
// Blaetter ohne Cluster), liegen in einer unsichtbaren Surface mit Alpha 0.
// Nicht nachgebaut: die Sichtbarkeitspruefung (PVS - verdeckt ohnehin nur
// Flaechen hinter Waenden) und Punktlichter auf der Karte (FX_CONDLIGHT).
// Gemessen an einer erzeugten Karte (build/bug195_bsp_20261026/mkbsp.py).

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <vector>

#pragma pack(push, 1)
struct bb_q3_tex_     { char name[64]; int flags, contents; };
struct bb_q3_vertex_  { float coords[3]; float tex_coords[4]; float normal[3]; unsigned char color[4]; };
struct bb_q3_node_    { int plane; int children[2]; int mins[3]; int maxs[3]; };
struct bb_q3_face_ {
  int texture, effect, type, vertex, n_verts, meshvert, n_meshverts, lm_index;
  int lm_start[2], lm_size[2];
  float lm_origin[3], lm_vecs[2][3], normal[3];
  int patch_size[2];
};
struct bb_q3_leaf_    { int cluster, area; int mins[3], maxs[3]; int leafface, n_leaffaces, leafbrush, n_leafbrushes; };
#pragma pack(pop)

struct bb_BspSurf_ {
  int surf = 0;
  int texture = -1, lm_index = -1;
  std::vector<std::array<unsigned char, 3>> colors;   // Vertexfarben der Karte
};

struct bb_BspInfo_ {
  std::vector<bb_BspSurf_> surfs;
  std::vector<int> textures;    // je Q3-Textur ein Handle oder 0
  std::vector<int> lightmaps;
  bool  use_lmap = false;
  float amb[3] = { 0, 0, 0 };   // 0..1
};

inline std::map<int, bb_BspInfo_> bb_bsps_;

inline bb_BspInfo_* bb_bsp_chk_(int h) {
  bb_ent_chk_(h);
  auto it = bb_bsps_.find(h);
  if (it == bb_bsps_.end()) bb_RuntimeError("Entity is not a BSP Model");
  return &it->second;
}

// Grundfarbe und Texturen je Surface wie Q3BSPRep::setLighting.
inline void bb_bsp_paint_(bb_BspInfo_& info) {
  for (auto& s : info.surfs) {
    const bool lm = info.use_lmap && s.lm_index >= 0 &&
                    s.lm_index < static_cast<int>(info.lightmaps.size());
    const int tex = (s.texture >= 0 && s.texture < static_cast<int>(info.textures.size()))
                        ? info.textures[s.texture] : 0;
    const int br = bb_CreateBrush();
    bb_BrushFX(br, 1 | 2);   // keine Lichter der Szene, Grundfarbe aus den Vertices
    if (lm) {
      bb_BrushTexture(br, info.lightmaps[s.lm_index], 0, 0);
      if (tex) bb_BrushTexture(br, tex, 0, 1);
    } else if (tex) {
      bb_BrushTexture(br, tex, 0, 0);
    }
    bb_PaintSurface(s.surf, br);
    bb_FreeBrush(br);
    // Mit Lichtkarte ist die Grundfarbe das Umgebungslicht der Karte (Stufe 0
    // addiert die Lichtkarte dazu), ohne sie leuchten die Vertexfarben selbst
    // (FX_EMISSIVE) plus Umgebungslicht.
    for (size_t v = 0; v < s.colors.size(); ++v) {
      float c[3];
      for (int k = 0; k < 3; ++k) {
        c[k] = info.amb[k] * 255.0f + (lm ? 0.0f : static_cast<float>(s.colors[v][k]));
        if (c[k] > 255.0f) c[k] = 255.0f;
      }
      bb_VertexColor(s.surf, static_cast<int>(v), c[0], c[1], c[2], 1.0f);
    }
  }
}

namespace bb_bsp_detail_ {

inline void tf(const float* q, float* out) { out[0] = -q[1]; out[1] = q[2]; out[2] = q[0]; }

inline void average(const bb_q3_vertex_& a, const bb_q3_vertex_& b, bb_q3_vertex_* c) {
  for (int k = 0; k < 3; ++k) {
    c->coords[k] = (a.coords[k] + b.coords[k]) * .5f;
    c->normal[k] = (a.normal[k] + b.normal[k]) * .5f;
  }
  for (int k = 0; k < 4; ++k) {
    c->color[k] = static_cast<unsigned char>((a.color[k] + b.color[k] + 1) / 2);
    c->tex_coords[k] = (a.tex_coords[k] + b.tex_coords[k]) * .5f;
  }
}

inline void subdivide(std::vector<bb_q3_vertex_>& v, int level, int index, int step) {
  if (!level) {
    bb_q3_vertex_ t1, t2;
    average(v[index], v[index + step], &t1);
    average(v[index + step], v[index + step * 2], &t2);
    average(t1, t2, &v[index + step]);
    return;
  }
  average(v[index], v[index + step], &v[index + step / 2]);
  average(v[index + step], v[index + step * 2], &v[index + step + step / 2]);
  average(v[index + step / 2], v[index + step + step / 2], &v[index + step]);
  subdivide(v, level - 1, index, step / 2);
  subdivide(v, level - 1, index + step, step / 2);
}

// Gitter eines Patches wie patchFace (Stufe 1); false bei kaputten Groessen.
inline bool patch(const bb_q3_vertex_* base, size_t n_base, const bb_q3_face_& f,
                  std::vector<bb_q3_vertex_>& verts, int& sx, int& sy) {
  const int level = 1, step = 1 << level;
  if (f.patch_size[0] < 3 || f.patch_size[1] < 3 || (f.patch_size[0] & 1) == 0 ||
      (f.patch_size[1] & 1) == 0 || f.patch_size[0] > 65 || f.patch_size[1] > 65) return false;
  if (f.vertex < 0 || static_cast<size_t>(f.vertex) + f.patch_size[0] * f.patch_size[1] > n_base) return false;
  sx = (f.patch_size[0] - 1) * step + 1;
  sy = (f.patch_size[1] - 1) * step + 1;
  verts.assign(static_cast<size_t>(sx) * sy, bb_q3_vertex_{});
  const bb_q3_vertex_* t = base + f.vertex;
  for (int y = 0; y < sy; y += step)
    for (int x = 0; x < sx; x += step) verts[y * sx + x] = *t++;
  for (int y = 0; y < sy; y += step)
    for (int x = 0; x < sx - 1; x += step * 2) subdivide(verts, level, y * sx + x, step);
  for (int x = 0; x < sx; ++x)
    for (int y = 0; y < sy - 1; y += step * 2) subdivide(verts, level, y * sx + x, sx * step);
  return true;
}

}  // namespace bb_bsp_detail_

inline int bb_LoadBSP(const bbString& file, float gamma_adj = 0.0f, int parent = 0) {
  using namespace bb_bsp_detail_;
  std::vector<char> data;
  {
    FILE* fp = std::fopen(file.c_str(), "rb");   // ANSI wie die Lader in bb_loader.h
    if (!fp) return 0;
    std::fseek(fp, 0, SEEK_END);
    const long sz = std::ftell(fp);
    std::fseek(fp, 0, SEEK_SET);
    if (sz > 0) {
      data.resize(static_cast<size_t>(sz));
      if (std::fread(data.data(), 1, data.size(), fp) != data.size()) data.clear();
    }
    std::fclose(fp);
  }
  if (data.size() < 8 + 17 * 8 || std::memcmp(data.data(), "IBSP", 4) != 0) return 0;
  int version = 0;
  std::memcpy(&version, data.data() + 4, 4);
  if (version != 0x2e) return 0;

  struct Lump { const char* p = nullptr; size_t n = 0; };
  Lump lump[17];
  for (int k = 0; k < 17; ++k) {
    int off = 0, len = 0;
    std::memcpy(&off, data.data() + 8 + k * 8, 4);
    std::memcpy(&len, data.data() + 12 + k * 8, 4);
    if (off > 0 && len > 0 && static_cast<size_t>(off) + len <= data.size()) {
      lump[k].p = data.data() + off;
      lump[k].n = static_cast<size_t>(len);
    }
  }
  auto arr = [&](int k, size_t sz) { return lump[k].n / sz; };
  const size_t n_tex = arr(1, sizeof(bb_q3_tex_)), n_planes = arr(2, 16);
  const size_t n_nodes = arr(3, sizeof(bb_q3_node_)), n_leafs = arr(4, sizeof(bb_q3_leaf_));
  const size_t n_lf = arr(5, 4), n_verts = arr(10, sizeof(bb_q3_vertex_));
  const size_t n_mv = arr(11, 4), n_faces = arr(13, sizeof(bb_q3_face_));
  if (!n_nodes || !n_planes) return 0;
  const auto* q3tex   = reinterpret_cast<const bb_q3_tex_*>(lump[1].p);
  const auto* nodes   = reinterpret_cast<const bb_q3_node_*>(lump[3].p);
  const auto* leafs   = reinterpret_cast<const bb_q3_leaf_*>(lump[4].p);
  const auto* lfaces  = reinterpret_cast<const int*>(lump[5].p);
  const auto* q3verts = reinterpret_cast<const bb_q3_vertex_*>(lump[10].p);
  const auto* mverts  = reinterpret_cast<const int*>(lump[11].p);
  const auto* q3faces = reinterpret_cast<const bb_q3_face_*>(lump[13].p);

  // Baum ab Knoten 0 wie createNode/createLeaf: welche Flaeche erreicht wird
  // und ob sie in einem Blatt mit Cluster liegt (nur solche zeichnet das
  // Original; die Kollision nimmt alle).
  std::vector<int> face_order;
  std::map<int, bool> face_vis;
  {
    std::vector<int> stack{ 0 };
    std::set<int> seen_nodes;
    while (!stack.empty()) {
      const int n = stack.back(); stack.pop_back();
      if (n < 0 || static_cast<size_t>(n) >= n_nodes || !seen_nodes.insert(n).second) continue;
      for (int k = 1; k >= 0; --k) {   // Kind 0 zuerst bearbeiten
        const int c = nodes[n].children[k];
        if (c >= 0) { stack.push_back(c); continue; }
        const int l = -c - 1;
        if (static_cast<size_t>(l) >= n_leafs) continue;
        const bb_q3_leaf_& lf = leafs[l];
        for (int j = 0; j < lf.n_leaffaces; ++j) {
          const int idx = lf.leafface + j;
          if (idx < 0 || static_cast<size_t>(idx) >= n_lf) continue;
          const int fn = lfaces[idx];
          if (fn < 0 || static_cast<size_t>(fn) >= n_faces) continue;
          auto it = face_vis.find(fn);
          if (it == face_vis.end()) { face_vis[fn] = lf.cluster >= 0; face_order.push_back(fn); }
          else if (lf.cluster >= 0) it->second = true;
        }
      }
    }
  }

  bb_BspInfo_ info;
  const std::string dir = [&] {
    std::string f = file;
    const size_t p = f.find_last_of("/\\");
    return p == std::string::npos ? std::string() : f.substr(0, p + 1);
  }();

  // Texturen wie createTextures: .tga, .jpg, .png, .dds neben der Karte
  for (size_t k = 0; k < n_tex; ++k) {
    char nm[65] = {};
    std::memcpy(nm, q3tex[k].name, 64);
    int t = 0;
    for (const char* ext : { ".tga", ".jpg", ".png", ".dds" }) {
      const bbString path = dir + nm + ext;
      if (!bb_FileType(path)) continue;
      t = bb_LoadTexture(path, 1);
      if (t) break;
    }
    if (t) bb_TextureCoords(t, 1);
    info.textures.push_back(t);
  }

  // Lichtkarten wie createLightMaps
  {
    unsigned char adj[256];
    for (int k = 0; k < 256; ++k)
      adj[k] = static_cast<unsigned char>(std::pow(k / 255.0f, 1.0f - gamma_adj) * 255.0f);
    const size_t n_lm = lump[14].n / (128 * 128 * 3);
    const auto* rgb = reinterpret_cast<const unsigned char*>(lump[14].p);
    for (size_t k = 0; k < n_lm; ++k) {
      const int t = bb_CreateTexture(128, 128, 1 + 8 + 16 + 32);
      const int buf = bb_TextureBuffer(t);
      bb_LockBuffer(buf);
      for (int y = 0; y < 128; ++y)
        for (int x = 0; x < 128; ++x, rgb += 3)
          bb_WritePixelFast(x, y, static_cast<int>(0xff000000u | (adj[rgb[0]] << 16) |
                                                   (adj[rgb[1]] << 8) | adj[rgb[2]]), buf);
      bb_UnlockBuffer(buf);
      bb_TextureBlend(t, 3);
      info.lightmaps.push_back(t);
    }
  }

  const int mesh = bb_CreateMesh(parent);
  std::map<std::pair<int, int>, size_t> surf_of;   // (Textur, Lichtkarte) -> info.surfs
  int coll_surf = 0;
  auto add_vertex = [&](int surf, const bb_q3_vertex_& t, bb_BspSurf_* bs) {
    float p[3], n[3];
    tf(t.coords, p);
    tf(t.normal, n);
    const int v = bb_AddVertex(surf, p[0], p[1], p[2], t.tex_coords[2], t.tex_coords[3]);
    bb_VertexTexCoords(surf, v, t.tex_coords[0], t.tex_coords[1], 1.0f, 1);
    bb_VertexNormal(surf, v, n[0], n[1], n[2]);
    if (bs) bs->colors.push_back({ t.color[0], t.color[1], t.color[2] });
    return v;
  };

  for (const int fn : face_order) {
    const bb_q3_face_& f = q3faces[fn];
    if (f.type == 1 || f.type == 3) {
      if (!f.n_meshverts || (f.n_meshverts % 3)) continue;
    } else if (f.type != 2) {
      continue;
    }
    bool draw = true;
    if (f.texture >= 0 && static_cast<size_t>(f.texture) < n_tex) {
      if (!(q3tex[f.texture].contents & 1)) continue;
      if (q3tex[f.texture].flags & 0x84) draw = false;
    }
    if (!face_vis[fn]) draw = false;

    int surf;
    bb_BspSurf_* bs = nullptr;
    if (draw) {
      const auto key = std::make_pair(f.texture, f.lm_index);
      auto it = surf_of.find(key);
      if (it == surf_of.end()) {
        bb_BspSurf_ s;
        s.surf = bb_CreateSurface(mesh);
        s.texture = f.texture;
        s.lm_index = f.lm_index;
        it = surf_of.emplace(key, info.surfs.size()).first;
        info.surfs.push_back(std::move(s));
      }
      bs = &info.surfs[it->second];
      surf = bs->surf;
    } else {
      if (!coll_surf) {
        coll_surf = bb_CreateSurface(mesh);
        const int br = bb_CreateBrush();
        bb_BrushAlpha(br, 0.0f);   // nur Kollision: nie sichtbar, kein Z-Puffer
        bb_PaintSurface(coll_surf, br);
        bb_FreeBrush(br);
      }
      surf = coll_surf;
    }

    if (f.type == 2) {
      std::vector<bb_q3_vertex_> pv;
      int sx = 0, sy = 0;
      if (!patch(q3verts, n_verts, f, pv, sx, sy)) continue;
      const int first = add_vertex(surf, pv[0], bs);
      for (size_t k = 1; k < pv.size(); ++k) add_vertex(surf, pv[k], bs);
      for (int y = 0; y < sy - 1; ++y) {
        int n = first + y * sx;
        for (int x = 0; x < sx - 1; ++n, ++x) {
          bb_AddTriangle(surf, n, n + sx, n + 1);
          bb_AddTriangle(surf, n + sx + 1, n + 1, n + sx);
        }
      }
    } else {
      std::map<int, int> vert_map;   // je Flaeche, wie meshFace
      for (int j = 0; j + 2 < f.n_meshverts; j += 3) {
        int tri[3];
        bool ok = true;
        for (int q = 0; q < 3 && ok; ++q) {
          const int mi = f.meshvert + j + q;
          if (mi < 0 || static_cast<size_t>(mi) >= n_mv) { ok = false; break; }
          const int n = mverts[mi] + f.vertex;
          if (n < 0 || static_cast<size_t>(n) >= n_verts) { ok = false; break; }
          auto it = vert_map.find(n);
          if (it == vert_map.end()) it = vert_map.emplace(n, add_vertex(surf, q3verts[n], bs)).first;
          tri[q] = it->second;
        }
        if (ok) bb_AddTriangle(surf, tri[0], tri[1], tri[2]);
      }
    }
  }

  info.use_lmap = true;
  bb_bsp_paint_(info);
  bb_bsps_[mesh] = std::move(info);
  return mesh;
}

inline void bb_BSPLighting(int bsp, int use_lightmaps) {
  bb_BspInfo_* info = bb_bsp_chk_(bsp);
  const bool on = use_lightmaps != 0;
  if (on == info->use_lmap) return;
  info->use_lmap = on;
  bb_bsp_paint_(*info);
}

inline void bb_BSPAmbientLight(int bsp, float red, float green, float blue) {
  bb_BspInfo_* info = bb_bsp_chk_(bsp);
  info->amb[0] = red / 255.0f;
  info->amb[1] = green / 255.0f;
  info->amb[2] = blue / 255.0f;
  bb_bsp_paint_(*info);
}

#endif // BLITZNEXT_BB_BSP_H
