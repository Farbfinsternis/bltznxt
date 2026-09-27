#ifndef BLITZNEXT_BB_LOADER_H
#define BLITZNEXT_BB_LOADER_H

#include "bb_mesh.h"
#include "bb_loader_x.h"
#include "bb_animation.h"
#include "bb_loader_b3d.h"
#include "bb_loader_gltf.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <array>
#include <unordered_map>
#include <vector>

// ============================================================
//  Netze aus Dateien  -  bb_loader.h   (3D-13, Teil 2)
//
//  LoadMesh kann laut Doku .X, .3DS und .B3D. Gezaehlt an der mitgelieferten
//  Installation laden die 32 Beispielprogramme **39-mal .x und 20-mal .3ds -
//  kein einziges Mal .b3d**; .b3d kommt dort ueberhaupt nicht vor. Deshalb
//  faengt der Loader mit .3ds an: ein starrer Chunk-Walk, und 38 Dateien zum
//  Gegenpruefen.
//
//  Das .3ds-Format ist eine Baumstruktur aus Chunks. Jeder Chunk hat einen
//  Kopf aus 2 Byte Kennung und 4 Byte Laenge (die den Kopf mitzaehlt), alles
//  little-endian. Genutzt werden:
//
//    0x4D4D  Datei          0x3D3D  Editorabschnitt
//    0xAFFF  Material         0xA000 Name        0xA020 Diffusfarbe
//                             0xA040 Glanz       0xA050 Transparenz
//                             0xA200 Texturkarte  -> 0xA300 Dateiname
//    0x4000  Objekt (Name, dann Unterchunks)
//    0x4100  Dreiecksnetz     0x4110 Vertices    0x4140 UV-Koordinaten
//                             0x4120 Dreiecke    -> 0x4130 Materialgruppe
//                                                -> 0x4150 Glaettungsgruppen
//                             0x4160 lokales Koordinatensystem
//
//  Vier Dinge sind am laufenden Original nachgemessen und nicht geraten -
//  jedes davon haette man plausibel anders gemacht:
//
//  1. **Die Achsen tauschen y und z:** blitz(x,y,z) = 3ds(x,z,y). 3D Studio
//     ist rechtshaendig mit z nach oben, Blitz3D linkshaendig mit y nach
//     oben. Nachweis ueber MeshWidth/Height/Depth an zehn Dateien: die rohe
//     3DS-Ausdehnung (x,y,z) taucht bei Blitz als (x,z,y) auf, zum Beispiel
//     fighter.3ds roh 372.47/529.94/152.39 gegen gemeldete
//     372.472/152.386/529.937.
//
//  2. **Das lokale Koordinatensystem (0x4160) wird ignoriert.** Sechs der
//     zehn Dateien haben dort keine Einheitsmatrix - wcrate1.3ds traegt eine
//     Skalierung von 13.583, fighter.3ds eine von 0.257, rock.3DS eine
//     Drehung um rund 6 Grad. Die gemeldeten Ausmasse entsprechen trotzdem
//     genau den **rohen** Vertexkoordinaten. Wer die Matrix anwendet, macht
//     die Kiste um das Dreizehnfache zu gross.
//
//  3. **Flaechen entstehen je Material, und gleiche Brushes werden
//     zusammengefasst.** ufo.3ds hat drei Materialien in drei Objekten und
//     meldet **zwei** Flaechen, solange die beiden Texturdateien fehlen -
//     dann sind beide Brushes schlicht "weiss ohne Textur" und damit
//     gleich. Legt man die Texturen daneben, meldet dieselbe Datei **drei**.
//     Objektgrenzen spielen dabei keine Rolle: zusammengefasst wird ueber
//     die ganze Datei.
//
//  4. Die Vertexfarbe ist 255,255,255; Materialfarbe, Glanz und Transparenz
//     landen im Brush der Flaeche.
// ============================================================

// ============================================================
// Loadermatrix  (LoaderMatrix)
//
// Der Achsentausch ist im Original kein Sonderfall des .3ds-Lesers,
// sondern eine **Matrix je Dateiendung**, die das Programm aendern kann.
// help/commands/3d_commands/LoaderMatrix.htm nennt die Vorgaben:
//
//   LoaderMatrix "x"  ,1,0,0, 0,1,0, 0,0,1   ; unveraendert
//   LoaderMatrix "3ds",1,0,0, 0,0,1, 0,1,0   ; y/z vertauscht
//
// Die drei Zahlentripel sind die **Spalten**: wohin x, y und z gehen.
// Aus dem Vorzeichen der Determinante folgt, ob der Umlaufsinn kippt -
// eine Vertauschung hat Determinante -1, also muessen zwei Indizes
// getauscht werden. Damit stimmt der Umlaufsinn auch dann noch, wenn ein
// Programm die Matrix selbst setzt.
// ============================================================

inline bbString bb_file_ext_lower_(const bbString& file) {
  size_t dot = file.find_last_of('.');
  if (dot == bbString::npos) return "";
  bbString e = file.substr(dot);
  for (auto& c : e) c = static_cast<char>(::tolower((unsigned char)c));
  return e;
}

struct bb_LoaderMat_ { float m[9]; };

// Endung ohne Punkt und klein geschrieben.
inline std::unordered_map<std::string, bb_LoaderMat_> bb_loader_mats_ = {
  { "x"  , { { 1,0,0, 0,1,0, 0,0,1 } } },
  { "3ds", { { 1,0,0, 0,0,1, 0,1,0 } } },
  // glTF ist rechtshaendig: z gespiegelt (3D-24, siehe bb_loader_gltf.h).
  { "gltf", { { 1,0,0, 0,1,0, 0,0,-1 } } },
  { "glb" , { { 1,0,0, 0,1,0, 0,0,-1 } } },
};

inline std::string bb_loader_key_(const bbString& ext) {
  std::string e(ext);
  if (!e.empty() && e[0] == '.') e.erase(0, 1);
  for (auto& c : e) c = static_cast<char>(::tolower((unsigned char)c));
  return e;
}

inline void bb_LoaderMatrix(const bbString& file_ext,
                            float xx, float xy, float xz,
                            float yx, float yy, float yz,
                            float zx, float zy, float zz) {
  bb_LoaderMat_ m = { { xx, xy, xz, yx, yy, yz, zx, zy, zz } };
  bb_loader_mats_[bb_loader_key_(file_ext)] = m;
}

// Spalten mal Vektor.
inline void bb_loader_apply_(const bb_LoaderMat_& m, float x, float y, float z,
                             float* out) {
  out[0] = m.m[0] * x + m.m[3] * y + m.m[6] * z;
  out[1] = m.m[1] * x + m.m[4] * y + m.m[7] * z;
  out[2] = m.m[2] * x + m.m[5] * y + m.m[8] * z;
}

inline float bb_loader_det_(const bb_LoaderMat_& m) {
  return m.m[0] * (m.m[4] * m.m[8] - m.m[5] * m.m[7])
       - m.m[3] * (m.m[1] * m.m[8] - m.m[2] * m.m[7])
       + m.m[6] * (m.m[1] * m.m[5] - m.m[2] * m.m[4]);
}

// Dieselbe Texturdatei darf nur **ein** Handle bekommen. Sonst
// unterscheiden sich zwei Brushes allein durch die Handlenummer und
// werden nicht zusammengefasst - mak_robotic.x meldete so 38 Flaechen
// statt 3. Das Original haelt dafuer einen Texturzwischenspeicher
// (blitz3d/cachedtexture.cpp).
inline int bb_loader_tex_(std::unordered_map<std::string, int>& cache,
                          const std::filesystem::path& dir,
                          const std::string& name) {
  if (name.empty()) return 0;
  std::error_code ec;
  std::filesystem::path cand = dir / name;
  if (!std::filesystem::exists(cand, ec)) cand = name;
  const std::string key = cand.string();
  auto it = cache.find(key);
  if (it != cache.end()) return it->second;
  const int h = bb_LoadTexture(key, 1);
  cache.emplace(key, h);
  return h;
}

// ---- Chunkkennungen ----
enum : uint16_t {
  BB_3DS_MAIN      = 0x4D4D, BB_3DS_EDIT      = 0x3D3D,
  BB_3DS_MATERIAL  = 0xAFFF, BB_3DS_MAT_NAME  = 0xA000,
  BB_3DS_MAT_DIFF  = 0xA020, BB_3DS_MAT_SHIN  = 0xA040,
  BB_3DS_MAT_TRANS = 0xA050, BB_3DS_MAT_TEX   = 0xA200,
  BB_3DS_MAT_FILE  = 0xA300, BB_3DS_MAT_2SIDE = 0xA081,
  BB_3DS_COL_RGB1  = 0x0011, BB_3DS_COL_RGB2  = 0x0012,
  BB_3DS_PCT_INT   = 0x0030, BB_3DS_PCT_FLOAT = 0x0031,
  BB_3DS_OBJECT    = 0x4000, BB_3DS_TRIMESH   = 0x4100,
  BB_3DS_VERTICES  = 0x4110, BB_3DS_FACES     = 0x4120,
  BB_3DS_FACE_MAT  = 0x4130, BB_3DS_MAPCOORDS = 0x4140,
  BB_3DS_SMOOTH    = 0x4150, BB_3DS_LOCALAXES = 0x4160,
  BB_3DS_KEYFRAME  = 0xB000, BB_3DS_OBJNODE   = 0xB002,
  BB_3DS_NODE_HDR  = 0xB010, BB_3DS_PIVOT     = 0xB013,
  // Keyframer (3D-19)
  BB_3DS_KFHDR     = 0xB00A, BB_3DS_NODE_ID   = 0xB030,
  BB_3DS_INSTANCE  = 0xB011, BB_3DS_POS_TRACK = 0xB020,
  BB_3DS_ROT_TRACK = 0xB021, BB_3DS_SCL_TRACK = 0xB022
};

// ============================================================
// Lesen mit Grenzen - eine beschaedigte Datei darf nicht ueber den Puffer
// hinauslaufen.
// ============================================================

struct bb_3ds_buf_ {
  const uint8_t* d = nullptr;
  size_t         n = 0;

  bool ok(size_t p, size_t len) const { return p + len <= n; }
  uint16_t u16(size_t p) const {
    return ok(p, 2) ? static_cast<uint16_t>(d[p] | (d[p + 1] << 8)) : 0;
  }
  uint32_t u32(size_t p) const {
    if (!ok(p, 4)) return 0;
    return static_cast<uint32_t>(d[p]) | (static_cast<uint32_t>(d[p + 1]) << 8) |
           (static_cast<uint32_t>(d[p + 2]) << 16) |
           (static_cast<uint32_t>(d[p + 3]) << 24);
  }
  float f32(size_t p) const {
    uint32_t v = u32(p);
    float f;
    std::memcpy(&f, &v, 4);
    return f;
  }
  // Nullterminierte Zeichenkette; gibt die Laenge einschliesslich der Null
  // ueber `len` zurueck.
  std::string cstr(size_t p, size_t& len) const {
    std::string s;
    while (p + s.size() < n && d[p + s.size()] != 0) s += static_cast<char>(d[p + s.size()]);
    len = s.size() + 1;
    return s;
  }
};

// ============================================================
// Zwischenformen beim Einlesen
// ============================================================

struct bb_3ds_mat_ {
  std::string name;
  float r = 255.0f, g = 255.0f, b = 255.0f;
  float shininess = 0.0f;
  float alpha     = 1.0f;
  std::string texfile;
  bool twosided = false;
};

struct bb_3ds_obj_ {
  std::string           name;
  float                 axes[9] = { 1,0,0, 0,1,0, 0,0,1 };
  float                 org[3]  = { 0, 0, 0 };
  std::vector<float>    vx, vy, vz;      // rohe 3DS-Koordinaten
  std::vector<float>    tu, tv;
  std::vector<uint16_t> fa, fb, fc;
  std::vector<uint32_t> smooth;          // je Dreieck, 0 = keine Glaettung
  std::vector<int>      fmat;            // Materialindex je Dreieck, -1 = keins
};

// Ein Schluessel einer Spur im Keyframer, roh wie in der Datei: Lage und
// Skalierung in v[0..2], Drehung als Winkel v[0] um die Achse v[1..3]
// (relativ zum vorigen Schluessel).
struct bb_3ds_key_ {
  uint16_t type = 0;
  int      time = 0;
  float    v[4] = { 0, 0, 0, 0 };
};

// Ein Knoten des Keyframers (0xB002), gelesen fuer LoadAnimMesh.
struct bb_3ds_node_ {
  uint16_t    id = 65535, parent = 65535;
  std::string name, inst;
  float       pivot[3] = { 0, 0, 0 };
  std::vector<bb_3ds_key_> keys;
};

struct bb_3ds_scene_ {
  std::vector<bb_3ds_mat_> mats;
  std::vector<bb_3ds_obj_> objs;
  // Drehpunkte aus dem Keyframe-Abschnitt, ueber den Objektnamen
  // zugeordnet.
  std::unordered_map<std::string, std::array<float, 3>> pivots;
  // Keyframer (3D-19): Laenge aus KFHDR und die Knoten in Dateifolge.
  uint16_t anim_len = 0;
  bool     has_kf = false;   // Keyframer-Abschnitt vorhanden
  std::vector<bb_3ds_node_> nodes;
};

// Eine Spur (0xB020..0xB022): Kopf aus Flags, 8 Byte, Anzahl (2 Byte, dann
// 2 Byte uebergangen), danach je Schluessel Zeit, Flags und - je nach Flags -
// bis zu fuenf Spline-Werte, die das Original liest und verwirft.
inline void bb_3ds_read_track_(const bb_3ds_buf_& b, size_t s, size_t e,
                               uint16_t type, bb_3ds_node_& n) {
  const uint16_t cnt = b.u16(s + 10);
  size_t p = s + 14;
  const int nv = (type == BB_3DS_ROT_TRACK) ? 4 : 3;
  for (uint16_t k = 0; k < cnt; ++k) {
    if (p + 6 > e) break;
    bb_3ds_key_ key;
    key.type = type;
    key.time = static_cast<int>(b.u32(p));
    const uint16_t fl = b.u16(p + 4);
    p += 6;
    for (int bit = 0; bit < 5; ++bit) if (fl & (1 << bit)) p += 4;
    if (p + nv * 4 > e) break;
    for (int i = 0; i < nv; ++i) key.v[i] = b.f32(p + i * 4);
    p += nv * 4;
    n.keys.push_back(key);
  }
}

// ---- Materialabschnitt ----

inline void bb_3ds_read_pct_(const bb_3ds_buf_& b, size_t s, size_t e, float& out) {
  size_t p = s;
  while (p + 6 <= e) {
    uint16_t id = b.u16(p);
    uint32_t ln = b.u32(p + 2);
    if (ln < 6 || p + ln > e) break;
    if (id == BB_3DS_PCT_INT)        out = b.u16(p + 6) / 100.0f;
    else if (id == BB_3DS_PCT_FLOAT) out = b.f32(p + 6);
    p += ln;
  }
}

inline void bb_3ds_read_color_(const bb_3ds_buf_& b, size_t s, size_t e,
                               float& r, float& g, float& bl) {
  size_t p = s;
  while (p + 6 <= e) {
    uint16_t id = b.u16(p);
    uint32_t ln = b.u32(p + 2);
    if (ln < 6 || p + ln > e) break;
    if (id == BB_3DS_COL_RGB1 || id == BB_3DS_COL_RGB2) {
      // 0x0011 sind drei Bytes, 0x0012 ebenfalls (die Gammavariante).
      if (b.ok(p + 6, 3)) {
        r  = b.d[p + 6];
        g  = b.d[p + 7];
        bl = b.d[p + 8];
      }
    }
    p += ln;
  }
}

inline void bb_3ds_read_material_(const bb_3ds_buf_& b, size_t s, size_t e,
                                  bb_3ds_mat_& m) {
  size_t p = s;
  while (p + 6 <= e) {
    uint16_t id = b.u16(p);
    uint32_t ln = b.u32(p + 2);
    if (ln < 6 || p + ln > e) break;
    const size_t body = p + 6, bend = p + ln;
    switch (id) {
      case BB_3DS_MAT_NAME: { size_t l; m.name = b.cstr(body, l); break; }
      case BB_3DS_MAT_DIFF: bb_3ds_read_color_(b, body, bend, m.r, m.g, m.b); break;
      case BB_3DS_MAT_SHIN: bb_3ds_read_pct_(b, body, bend, m.shininess); break;
      case BB_3DS_MAT_2SIDE: m.twosided = true; break;
      case BB_3DS_MAT_TRANS: {
        float t = 0.0f;
        bb_3ds_read_pct_(b, body, bend, t);
        m.alpha = 1.0f - t;
        break;
      }
      case BB_3DS_MAT_TEX: {
        size_t q = body;
        while (q + 6 <= bend) {
          uint16_t sid = b.u16(q);
          uint32_t sln = b.u32(q + 2);
          if (sln < 6 || q + sln > bend) break;
          if (sid == BB_3DS_MAT_FILE) { size_t l; m.texfile = b.cstr(q + 6, l); }
          q += sln;
        }
        break;
      }
      default: break;
    }
    p += ln;
  }
}

// ---- Netzabschnitt ----

inline void bb_3ds_read_trimesh_(const bb_3ds_buf_& b, size_t s, size_t e,
                                 bb_3ds_scene_& sc, bb_3ds_obj_& o) {
  size_t p = s;
  while (p + 6 <= e) {
    uint16_t id = b.u16(p);
    uint32_t ln = b.u32(p + 2);
    if (ln < 6 || p + ln > e) break;
    const size_t body = p + 6, bend = p + ln;

    if (id == BB_3DS_VERTICES) {
      uint16_t cnt = b.u16(body);
      for (uint16_t i = 0; i < cnt; ++i) {
        size_t q = body + 2 + static_cast<size_t>(i) * 12;
        if (!b.ok(q, 12)) break;
        o.vx.push_back(b.f32(q));
        o.vy.push_back(b.f32(q + 4));
        o.vz.push_back(b.f32(q + 8));
      }
    } else if (id == BB_3DS_MAPCOORDS) {
      uint16_t cnt = b.u16(body);
      for (uint16_t i = 0; i < cnt; ++i) {
        size_t q = body + 2 + static_cast<size_t>(i) * 8;
        if (!b.ok(q, 8)) break;
        o.tu.push_back(b.f32(q));
        o.tv.push_back(b.f32(q + 4));
      }
    } else if (id == BB_3DS_FACES) {
      uint16_t cnt = b.u16(body);
      for (uint16_t i = 0; i < cnt; ++i) {
        size_t q = body + 2 + static_cast<size_t>(i) * 8;
        if (!b.ok(q, 8)) break;
        o.fa.push_back(b.u16(q));
        o.fb.push_back(b.u16(q + 2));
        o.fc.push_back(b.u16(q + 4));
      }
      o.smooth.assign(o.fa.size(), 0);
      o.fmat.assign(o.fa.size(), -1);

      // Unterchunks stehen hinter der Dreiecksliste.
      size_t q = body + 2 + static_cast<size_t>(cnt) * 8;
      while (q + 6 <= bend) {
        uint16_t sid = b.u16(q);
        uint32_t sln = b.u32(q + 2);
        if (sln < 6 || q + sln > bend) break;
        if (sid == BB_3DS_FACE_MAT) {
          size_t l;
          std::string mn = b.cstr(q + 6, l);
          int mi = -1;
          for (size_t k = 0; k < sc.mats.size(); ++k)
            if (sc.mats[k].name == mn) { mi = static_cast<int>(k); break; }
          size_t r = q + 6 + l;
          uint16_t fc = b.u16(r);
          for (uint16_t i = 0; i < fc; ++i) {
            uint16_t fi = b.u16(r + 2 + static_cast<size_t>(i) * 2);
            if (fi < o.fmat.size()) o.fmat[fi] = mi;
          }
        } else if (sid == BB_3DS_SMOOTH) {
          for (size_t i = 0; i < o.smooth.size(); ++i) {
            size_t r = q + 6 + i * 4;
            if (!b.ok(r, 4)) break;
            o.smooth[i] = b.u32(r);
          }
        }
        q += sln;
      }
    } else if (id == BB_3DS_LOCALAXES) {
      // Die Matrix selbst wird **nicht** auf die Vertices angewandt (das
      // tut das Original auch nicht, sonst waere die Kiste um das
      // Dreizehnfache zu gross). Gebraucht wird sie nur, um den
      // Drehpunkt aus dem Keyframe-Abschnitt in dieselben Einheiten zu
      // bringen wie die Vertices.
      for (int i = 0; i < 9; ++i) o.axes[i] = b.f32(body + i * 4);
      for (int i = 0; i < 3; ++i) o.org[i]  = b.f32(body + 36 + i * 4);
    }
    p += ln;
  }
}

inline bool bb_3ds_parse_(const bb_3ds_buf_& b, bb_3ds_scene_& sc) {
  if (b.n < 6 || b.u16(0) != BB_3DS_MAIN) return false;

  // Rekursiv nur ueber die Ebenen, die uns angehen.
  struct W {
    static void walk(const bb_3ds_buf_& b, size_t s, size_t e, bb_3ds_scene_& sc) {
      size_t p = s;
      while (p + 6 <= e) {
        uint16_t id = b.u16(p);
        uint32_t ln = b.u32(p + 2);
        if (ln < 6 || p + ln > e) break;
        const size_t body = p + 6, bend = p + ln;
        if (id == BB_3DS_KEYFRAME) sc.has_kf = true;
        if (id == BB_3DS_MAIN || id == BB_3DS_EDIT ||
            id == BB_3DS_KEYFRAME) {
          walk(b, body, bend, sc);
        } else if (id == BB_3DS_KFHDR) {
          // Revision, Dateiname, dann die Laenge der Animation.
          size_t l;
          b.cstr(body + 2, l);
          sc.anim_len = b.u16(body + 2 + l);
        } else if (id == BB_3DS_OBJNODE) {
          // Ein Knoten traegt seinen Namen (0xB010) und den Drehpunkt
          // (0xB013). Das Original schiebt das Netz so, dass der
          // Drehpunkt im Ursprung liegt - gemessen an drei Dateien,
          // siehe Kopf.
          bb_3ds_node_ node;
          std::array<float, 3> piv{ 0.0f, 0.0f, 0.0f };
          size_t q = body;
          while (q + 6 <= bend) {
            uint16_t sid = b.u16(q);
            uint32_t sln = b.u32(q + 2);
            if (sln < 6 || q + sln > bend) break;
            if (sid == BB_3DS_NODE_HDR) {
              size_t l;
              node.name = b.cstr(q + 6, l);
              node.parent = b.u16(q + 6 + l + 4);   // nach zwei Flagworten
            } else if (sid == BB_3DS_PIVOT) {
              for (int k = 0; k < 3; ++k) piv[k] = node.pivot[k] = b.f32(q + 6 + k * 4);
            } else if (sid == BB_3DS_NODE_ID) {
              node.id = b.u16(q + 6);
            } else if (sid == BB_3DS_INSTANCE) {
              size_t l;
              node.inst = b.cstr(q + 6, l);
            } else if (sid == BB_3DS_POS_TRACK || sid == BB_3DS_ROT_TRACK ||
                       sid == BB_3DS_SCL_TRACK) {
              bb_3ds_read_track_(b, q + 6, q + sln, sid, node);
            }
            q += sln;
          }
          if (!node.name.empty()) sc.pivots.emplace(node.name, piv);
          sc.nodes.push_back(std::move(node));
        } else if (id == BB_3DS_MATERIAL) {
          bb_3ds_mat_ m;
          bb_3ds_read_material_(b, body, bend, m);
          sc.mats.push_back(std::move(m));
        } else if (id == BB_3DS_OBJECT) {
          size_t l;
          const std::string oname = b.cstr(body, l);
          size_t q = body + l;
          while (q + 6 <= bend) {
            uint16_t sid = b.u16(q);
            uint32_t sln = b.u32(q + 2);
            if (sln < 6 || q + sln > bend) break;
            if (sid == BB_3DS_TRIMESH) {
              // Auch ohne Dreiecke: der Lader mit Hierarchie macht daraus
              // eine Entity, beim Einschmelzen traegt es nichts bei.
              bb_3ds_obj_ o;
              o.name = oname;
              bb_3ds_read_trimesh_(b, q + 6, q + sln, sc, o);
              sc.objs.push_back(std::move(o));
            }
            q += sln;
          }
        }
        p += ln;
      }
    }
  };
  W::walk(b, 0, b.n, sc);
  for (const auto& o : sc.objs) if (!o.fa.empty()) return true;
  return false;
}

// ============================================================
// Aus der eingelesenen Szene ein Netz bauen
// ============================================================

// Der Schluessel, nach dem Flaechen zusammengefasst werden. Gemessen: zwei
// Materialien mit gleicher Farbe und (weil die Datei fehlt) ohne Textur
// landen im Original in **einer** Flaeche.
inline std::string bb_3ds_brush_key_(const bb_3ds_mat_* m, int texhandle) {
  char buf[160];
  if (!m) {
    std::snprintf(buf, sizeof buf, "255,255,255|0|1|%d", texhandle);
  } else {
    // Dieselbe Regel wie beim Anlegen des Brushes: mit Textur zaehlt die
    // Diffusfarbe nicht mit, sonst wuerden Flaechen getrennt, die im Bild
    // gleich aussehen.
    const bool named = !m->texfile.empty();
    const float r = named ? 255.0f : m->r;
    const float g = named ? 255.0f : m->g;
    const float b = named ? 255.0f : m->b;
    std::snprintf(buf, sizeof buf, "%.3f,%.3f,%.3f|%.3f|%.3f|%d",
                  r, g, b, m->shininess,
                  m->alpha + (m->twosided ? 1000.0f : 0.0f), texhandle);
  }
  return buf;
}

inline bool bb_3ds_read_file_(const bbString& file, std::vector<uint8_t>& data) {
  std::FILE* f = std::fopen(file.c_str(), "rb");
  if (!f) {
    std::cerr << "[runtime] LoadMesh: cannot open '" << file << "'\n";
    return false;
  }
  std::fseek(f, 0, SEEK_END);
  long sz = std::ftell(f);
  std::fseek(f, 0, SEEK_SET);
  if (sz <= 0) { std::fclose(f); return false; }
  data.resize(static_cast<size_t>(sz));
  size_t got = std::fread(data.data(), 1, data.size(), f);
  std::fclose(f);
  data.resize(got);
  return true;
}

// Die Flaechen eines Netzes, das gerade gebaut wird: ein Eintrag je Brush.
// Der Vertexschluessel enthaelt die Glaettungsgruppe: zwei Dreiecke teilen
// sich einen Vertex nur, wenn sie in derselben Gruppe liegen - sonst entsteht
// eine harte Kante. Gruppe 0 heisst "keine Glaettung", jedes Dreieck bekommt
// dann eigene Vertices.
struct bb_3ds_build_ {
  bb_MeshEntity_* ent = nullptr;
  struct Group { size_t surf; std::unordered_map<uint64_t, unsigned> vmap; };
  std::unordered_map<std::string, Group> groups;
};

// Die Dreiecke von Objekt `oi` in das Netz `B` einbauen. `xf(x, y, z, out)`
// bringt einen rohen 3DS-Vertex an seinen Platz - beim Einschmelzen in den
// Raum des einen Netzes, mit Hierarchie in den Raum der Entity des Objekts.
template <class Xf>
inline void bb_3ds_build_obj_(bb_3ds_build_& B, const bb_3ds_scene_& sc,
                              const std::vector<int>& mat_tex, size_t oi,
                              bool flip, Xf xf) {
  const bb_3ds_obj_& o = sc.objs[oi];
  const bool has_uv = o.tu.size() >= o.vx.size();

  for (size_t fi = 0; fi < o.fa.size(); ++fi) {
    const int mi = (fi < o.fmat.size()) ? o.fmat[fi] : -1;
    const bb_3ds_mat_* m = (mi >= 0 && mi < (int)sc.mats.size()) ? &sc.mats[mi] : nullptr;
    const int th = (mi >= 0 && mi < (int)mat_tex.size()) ? mat_tex[mi] : 0;

    const std::string key = bb_3ds_brush_key_(m, th);
    auto it = B.groups.find(key);
    if (it == B.groups.end()) {
      bb_3ds_build_::Group gnew;
      gnew.surf = B.ent->surfaces().size();
      B.ent->surfaces().emplace_back();
      bb_Brush_& br = B.ent->surfaces().back().brush;
      if (m) {
        // Gemessen: die Diffusfarbe aus der Datei gilt nur, solange die
        // Flaeche **keine** Textur hat. rocket.3ds hat vier Materialien
        // ohne Textur und erscheint im Original genau in deren Farben
        // (255,191,0 / 191,191,255 / 236,42,42 / 255,255,255). Die
        // texturierte Kiste dagegen traegt die Diffusfarbe 191,191,191
        // und kommt trotzdem mit 254 heraus - die Farbe wuerde die
        // Textur sonst abdunkeln.
        // Der Quelltext setzt die Farbe auf weiss, sobald ein
        // Texturname dasteht - unabhaengig davon, ob die Datei
        // gefunden wurde. Ein Netz ohne Material faellt dadurch mit
        // den texturlosen weissen zusammen.
        if (m->texfile.empty()) { br.r = m->r; br.g = m->g; br.b = m->b; }
        br.shininess = m->shininess;
        br.alpha = m->alpha;
        br.twosided = m->twosided;
      }
      if (th) { br.tex.tex[0] = bb_texture_ref_(th); br.tex.frame[0] = 0; }
      it = B.groups.emplace(key, std::move(gnew)).first;
    }
    bb_3ds_build_::Group& g = it->second;
    bb_MeshData_& surf = B.ent->surfaces()[g.surf];

    const uint32_t sm = (fi < o.smooth.size()) ? o.smooth[fi] : 0;
    const uint16_t idx[3] = { o.fa[fi], o.fb[fi], o.fc[fi] };
    unsigned out[3];
    for (int k = 0; k < 3; ++k) {
      const uint16_t vi = idx[k];
      if (vi >= o.vx.size()) { out[k] = 0; continue; }
      // Schluessel: Objekt, Vertex und Glaettungsgruppe. Ohne Gruppe wird
      // je Dreieck ein eigener Vertex angelegt, damit die Flaeche flach
      // bleibt.
      uint64_t vkey = (static_cast<uint64_t>(oi) << 48) ^
                      (static_cast<uint64_t>(vi) << 32) ^
                      (sm ? sm : (0x80000000u | static_cast<uint32_t>(fi)));
      auto vit = g.vmap.find(vkey);
      if (vit != g.vmap.end()) {
        out[k] = vit->second;
      } else {
        unsigned ni = static_cast<unsigned>(surf.vertices.size() / BB_VF);
        // Achsen: blitz(x,y,z) = 3ds(x,z,y) - gemessen, siehe Kopf.
        // Gemessen mit einer Vierquadrantentextur: das Original zeigt
        // die linke obere Ecke des Bildes an der linken oberen Ecke der
        // Flaeche. In der Datei laeuft v von unten nach oben, unsere
        // Texturen fangen oben an - also umkehren.
        float u = has_uv ? o.tu[vi] : 0.0f;
        float v = has_uv ? (1.0f - o.tv[vi]) : 0.0f;
        float pc_[3];
        xf(o.vx[vi], o.vy[vi], o.vz[vi], pc_);
        bb_vert_push_(surf, pc_[0], pc_[1], pc_[2], 0, 0, 0, u, v);
        g.vmap.emplace(vkey, ni);
        out[k] = ni;
      }
    }
    // Kehrt die Loadermatrix die Haendigkeit um, kippt auch der
    // Umlaufsinn - das Original entscheidet das genauso am Vorzeichen
    // der Determinante. Ohne das Vertauschen zeigt die
    // Rueckseitenentfernung die **Rueckseite** des Modells; eine
    // geschlossene Kiste sieht in der Silhouette dann unveraendert aus
    // und ist nur seitenverkehrt beleuchtet und texturiert.
    surf.indices.push_back(out[0]);
    surf.indices.push_back(flip ? out[2] : out[1]);
    surf.indices.push_back(flip ? out[1] : out[2]);
  }
}

// Texturen je Material; sie liegen neben der Modelldatei.
inline std::vector<int> bb_3ds_textures_(const bbString& file, const bb_3ds_scene_& sc) {
  std::filesystem::path dir = std::filesystem::path(file).parent_path();
  std::vector<int> mat_tex(sc.mats.size(), 0);
  std::unordered_map<std::string, int> texcache;
  for (size_t i = 0; i < sc.mats.size(); ++i)
    mat_tex[i] = bb_loader_tex_(texcache, dir, sc.mats[i].texfile);
  return mat_tex;
}

inline int bb_load_3ds_(const bbString& file, int parent) {
  std::vector<uint8_t> data;
  if (!bb_3ds_read_file_(file, data)) return 0;

  bb_3ds_buf_ b{ data.data(), data.size() };
  bb_3ds_scene_ sc;
  if (!bb_3ds_parse_(b, sc)) {
    std::cerr << "[runtime] LoadMesh: '" << file << "' ist keine lesbare "
                 ".3ds-Datei\n";
    return 0;
  }

  const std::vector<int> mat_tex = bb_3ds_textures_(file, sc);

  // Die Matrix zur Endung; ohne Eintrag bleibt alles unveraendert.
  bb_LoaderMat_ lm = { { 1,0,0, 0,1,0, 0,0,1 } };
  {
    auto it = bb_loader_mats_.find(bb_loader_key_(bb_file_ext_lower_(file)));
    if (it != bb_loader_mats_.end()) lm = it->second;
  }
  const bool flip = bb_loader_det_(lm) < 0.0f;

  auto ent = std::make_unique<bb_MeshEntity_>();
  bb_3ds_build_ B;
  B.ent = ent.get();

  for (size_t oi = 0; oi < sc.objs.size(); ++oi) {
    const bb_3ds_obj_& o = sc.objs[oi];

    // Das Netz wird so verschoben, dass der Drehpunkt des Objekts im
    // Ursprung liegt. Wie genau, steht im Quelltext des Originals
    // (blitz3d/loader_3ds.cpp): die lokale Matrix wird die Weltmatrix des
    // Netzes, die Vertices werden mit deren Kehrwert in den lokalen Raum
    // geholt, dort um -pivot verschoben, und beim Einschmelzen zu einem
    // Netz wieder herausgerechnet. Ausmultipliziert bleibt davon
    //
    //     v' = L * ( v - M * pivot )
    //
    // uebrig: L ist die Loadermatrix, M der **Dreh- und Skalenanteil** der
    // lokalen Matrix. Ihr Translationsanteil faellt heraus - er hebt sich
    // zwischen Hin- und Rueckweg auf. Genau den hatte die erste, allein
    // aus Messungen abgeleitete Fassung hier faelschlich addiert; auf den
    // Testdateien machte das unter 0.2 Einheiten aus und war im Bild nicht
    // zu sehen.
    float tx[3] = { 0.0f, 0.0f, 0.0f };
    auto pit = sc.pivots.find(o.name);
    if (pit != sc.pivots.end()) {
      const std::array<float, 3>& pv = pit->second;
      for (int k = 0; k < 3; ++k)
        tx[k] = o.axes[k] * pv[0] + o.axes[3 + k] * pv[1] + o.axes[6 + k] * pv[2];
    }
    bb_3ds_build_obj_(B, sc, mat_tex, oi, flip,
                      [&](float x, float y, float z, float* out) {
                        bb_loader_apply_(lm, x - tx[0], y - tx[1], z - tx[2], out);
                      });
  }

  for (auto& s : ent->surfaces()) s.dirty = true;
  int h = bb_entity_register_(std::move(ent), parent);
  bb_UpdateNormals(h);
  return h;
}

// ============================================================
// .3ds mit Hierarchie und Animation (3D-19)
//
// Nach blitz3d/loader_3ds.cpp ohne HINT_COLLAPSE:
//
//  * Die Wurzel ist ein namenloses Mesh. Jedes Objekt mit Dreiecksnetz wird
//    ein Mesh mit seinem Namen unter der Wurzel. Seine lokale Matrix (0x4160)
//    wird seine Lage (zerlegt wie setLocalTform), die Vertices werden mit
//    ihrem **genauen** Kehrwert in den eigenen Raum geholt.
//  * Danach der Keyframer, Knoten fuer Knoten: der Elternteil kommt ueber
//    die Knotennummer (fehlt er, wird der Knoten uebergangen). "$$$DUMMY"
//    legt ein leeres Mesh mit dem Instanznamen an. Sonst wird das Objekt
//    gleichen Namens umgehaengt - nur beim ersten Knoten, der es nennt -,
//    um den Drehpunkt verschoben und behaelt dabei seine Weltlage.
//  * Die Spuren werden Schluessel, soweit ihre Zeit nicht hinter der Laenge
//    aus KFHDR liegt. Drehschluessel sind **relativ** zum vorigen: sie
//    werden aufmultipliziert.
//  * Die Wurzel bekommt einen Animator mit der Laenge aus KFHDR.
// ============================================================

// Quat::operator* des Originals: das Kreuzprodukt steht dort als q.v x v.
inline bb_Quat_ bb_quat_mul_(const bb_Quat_& a, const bb_Quat_& b) {
  return { a.w * b.w - (a.x * b.x + a.y * b.y + a.z * b.z),
           (b.y * a.z - b.z * a.y) + b.x * a.w + a.x * b.w,
           (b.z * a.x - b.x * a.z) + b.y * a.w + a.y * b.w,
           (b.x * a.y - b.y * a.x) + b.z * a.w + a.z * b.w };
}

// Die Weltmatrix aus den lokalen Lagen neu rechnen, ohne auf UpdateWorld
// zu warten (die Kette hinauf).
inline void bb_3ds_world_(bb_Entity_* e, float out[16]) {
  bb_entity_refresh_world_(e);
  std::memcpy(out, e->world, sizeof(float) * 16);
}

inline int bb_load_3ds_anim_(const bbString& file, int parent, bool animonly) {
  std::vector<uint8_t> data;
  if (!bb_3ds_read_file_(file, data)) return 0;
  bb_3ds_buf_ b{ data.data(), data.size() };
  if (b.n < 6 || b.u16(0) != BB_3DS_MAIN) {
    std::cerr << "[runtime] LoadMesh: '" << file << "' ist keine lesbare "
                 ".3ds-Datei\n";
    return 0;
  }
  bb_3ds_scene_ sc;
  bb_3ds_parse_(b, sc);

  const std::vector<int> mat_tex =
      animonly ? std::vector<int>(sc.mats.size(), 0) : bb_3ds_textures_(file, sc);

  bb_LoaderMat_ lm = { { 1,0,0, 0,1,0, 0,0,1 } };
  {
    auto it = bb_loader_mats_.find(bb_loader_key_(bb_file_ext_lower_(file)));
    if (it != bb_loader_mats_.end()) lm = it->second;
  }
  const bool flip = bb_loader_det_(lm) < 0.0f;
  float conv[16], conv_inv[16];
  mat4_identity_(conv);
  for (int c = 0; c < 3; ++c)
    for (int r = 0; r < 3; ++r) conv[c * 4 + r] = lm.m[c * 3 + r];
  if (!mat4_inverse_(conv_inv, conv)) mat4_identity_(conv_inv);

  // Die Wurzel haengt erst am Ende unter `parent`: bis dahin ist ihr Raum
  // der Weltraum, und "Weltlage behalten" beim Umhaengen ist einfach.
  const int root = bb_entity_register_(std::make_unique<bb_MeshEntity_>(), 0);

  std::unordered_map<std::string, int> name_map;
  for (size_t oi = 0; oi < sc.objs.size(); ++oi) {
    const bb_3ds_obj_& o = sc.objs[oi];
    auto ent = std::make_unique<bb_MeshEntity_>();
    ent->name = o.name;
    bb_MeshEntity_* me = ent.get();
    const int h = bb_entity_register_(std::move(ent), root);
    name_map[o.name] = h;

    float t[16] = { o.axes[0], o.axes[1], o.axes[2], 0,
                    o.axes[3], o.axes[4], o.axes[5], 0,
                    o.axes[6], o.axes[7], o.axes[8], 0,
                    o.org[0],  o.org[1],  o.org[2],  1 };
    float tc[16], a[16];
    mat4_mul_(a, conv, t);
    mat4_mul_(tc, a, conv_inv);
    bb_ent_set_local_tform_(me, tc);
    if (animonly) continue;

    float inv[16], xf[16];
    if (!mat4_inverse_(inv, tc)) mat4_identity_(inv);
    mat4_mul_(xf, inv, conv);
    bb_3ds_build_ B;
    B.ent = me;
    bb_3ds_build_obj_(B, sc, mat_tex, oi, flip,
                      [&](float x, float y, float z, float* out) {
                        mat4_xform_pt_(out, xf, x, y, z);
                      });
    for (auto& s : me->surfaces()) s.dirty = true;
    bb_UpdateNormals(h);
  }

  std::unordered_map<int, int> id_map;
  for (const bb_3ds_node_& n : sc.nodes) {
    int p = root;
    if (n.parent != 65535) {
      auto it = id_map.find(n.parent);
      if (it == id_map.end()) continue;
      p = it->second;
    }
    int h = 0;
    if (n.name == "$$$DUMMY") {
      auto ent = std::make_unique<bb_MeshEntity_>();
      ent->name = n.inst;
      h = bb_entity_register_(std::move(ent), p);
    } else {
      auto it = name_map.find(n.name);
      if (it == name_map.end()) continue;
      h = it->second;
      name_map.erase(it);
      bb_Entity_* e = bb_entity_get_(h);
      float pv[3];
      mat4_xform_pt_(pv, conv, n.pivot[0], n.pivot[1], n.pivot[2]);
      if (pv[0] != 0 || pv[1] != 0 || pv[2] != 0) {
        auto* me = static_cast<bb_MeshEntity_*>(e);
        for (auto& s : me->surfaces()) {
          for (size_t i = 0; i + 2 < s.vertices.size(); i += BB_VF) {
            s.vertices[i] -= pv[0]; s.vertices[i + 1] -= pv[1]; s.vertices[i + 2] -= pv[2];
          }
          s.dirty = true;
        }
      }
      // Umhaengen, die Weltlage bleibt (setWorldTform).
      float w[16], pw[16], pinv[16], l[16];
      bb_3ds_world_(e, w);
      bb_3ds_world_(bb_entity_get_(p), pw);
      if (!mat4_inverse_(pinv, pw)) mat4_identity_(pinv);
      mat4_mul_(l, pinv, w);
      // Derselbe Elternteil laesst auch die Reihenfolge stehen - setParent
      // kehrt dann sofort zurueck.
      if (e->parent != p) {
        if (bb_Entity_* old = bb_entity_get_(e->parent)) {
          auto& ch = old->children;
          ch.erase(std::remove(ch.begin(), ch.end(), h), ch.end());
        }
        e->parent = p;
        bb_entity_get_(p)->children.push_back(h);
      }
      bb_ent_set_local_tform_(e, l);
    }

    // Die Spuren als Schluessel.
    bb_Entity_* e = bb_entity_get_(h);
    auto keys = std::make_shared<bb_AnimKeys_>();
    bb_Quat_ quat;
    for (const bb_3ds_key_& k : n.keys) {
      const bool in_len = k.time <= sc.anim_len;
      if (k.type == BB_3DS_POS_TRACK) {
        float v[3];
        mat4_xform_pt_(v, conv, k.v[0], k.v[1], k.v[2]);
        if (in_len) keys->pos[k.time] = { v[0], v[1], v[2] };
      } else if (k.type == BB_3DS_ROT_TRACK) {
        float angle = k.v[0];
        float ax[3] = { k.v[1], k.v[2], k.v[3] };
        const float len = sqrtf(ax[0] * ax[0] + ax[1] * ax[1] + ax[2] * ax[2]);
        if (len > 1e-6f) {
          if (flip) angle = -angle;
          float c[3];
          bb_loader_apply_(lm, ax[0], ax[1], ax[2], c);
          const float cl = sqrtf(c[0] * c[0] + c[1] * c[1] + c[2] * c[2]);
          const float s = sinf(angle / 2) / cl;
          quat = bb_quat_mul_({ cosf(angle / 2), c[0] * s, c[1] * s, c[2] * s }, quat);
          const float ql = sqrtf(quat.w * quat.w + quat.x * quat.x +
                                 quat.y * quat.y + quat.z * quat.z);
          quat = { quat.w / ql, quat.x / ql, quat.y / ql, quat.z / ql };
        }
        if (in_len) keys->rot[k.time] = quat;
      } else if (k.type == BB_3DS_SCL_TRACK) {
        float v[3];
        bb_loader_apply_(lm, k.v[0], k.v[1], k.v[2], v);
        if (in_len) keys->scl[k.time] = { v[0], v[1], v[2] };
      }
    }
    e->anim = std::move(keys);
    if (n.id != 65535) id_map[n.id] = h;
  }

  // Den Animator legt erst parseKeyFramer an: ohne Keyframer-Abschnitt hat
  // das Modell keinen, und Animate bricht ab wie bei einem Wuerfel.
  if (sc.has_kf) bb_entity_get_(root)->animator = bb_animator_new_(root, sc.anim_len);

  // Jetzt erst unter den Elternteil.
  if (parent) {
    bb_Entity_* r = bb_entity_get_(root);
    r->parent = parent;
    bb_entity_get_(parent)->children.push_back(root);
  }
  return root;
}

// ============================================================
// .x aufbauen (3D-13, Teil 3)
//
// Die Bedeutung der Vorlagen stammt aus blitz3d/loader_x.cpp; das Original
// laesst d3dxof.dll parsen und laeuft nur den Objektbaum ab. Siehe
// bb_loader_x.h fuer die Regeln im einzelnen.
// ============================================================

// Vorlagennamen ohne Ruecksicht auf Gross- und Kleinschreibung
// vergleichen. Das Original geht ueber GUIDs, dem ist die Schreibweise
// egal - in interior.X steht "TextureFileName" mit grossem N, in
// anderen Dateien "TextureFilename". Buchstabengenau verglichen findet
// man dort keine einzige Textur.
inline bool bb_x_is_(const std::string& a, const char* b) {
  size_t i = 0;
  for (; i < a.size() && b[i]; ++i)
    if (::tolower((unsigned char)a[i]) != ::tolower((unsigned char)b[i])) return false;
  return i == a.size() && !b[i];
}

struct bb_XMatState_ {
  float r = 255.0f, g = 255.0f, b = 255.0f;
  float alpha = 1.0f;
  int   tex = 0;
};

// Material: vier Farbwerte (rgb + Deckkraft), dann Glanz und zwei Farben.
// Die Deckkraft gilt laut Quelltext nur, wenn sie ungleich 0 ist.
inline bb_XMatState_ bb_x_material_(const bb_XObj_& o,
                                    const std::filesystem::path& dir,
                                    std::unordered_map<std::string, int>& texcache) {
  bb_XMatState_ m;
  if (o.nums.size() >= 4) {
    m.r = static_cast<float>(o.nums[0] * 255.0);
    m.g = static_cast<float>(o.nums[1] * 255.0);
    m.b = static_cast<float>(o.nums[2] * 255.0);
    if (o.nums[3] != 0.0) m.alpha = static_cast<float>(o.nums[3]);
  }
  for (const auto& k : o.kids) {
    if (!bb_x_is_(k.type, "texturefilename") || k.strs.empty()) continue;
    m.tex = bb_loader_tex_(texcache, dir, k.strs[0]);
    // Wie bei .3ds setzt der Quelltext die Farbe auf weiss, sobald ein
    // Texturname dasteht - auch wenn die Datei fehlt.
    m.r = m.g = m.b = 255.0f;
  }
  return m;
}

inline std::string bb_x_brush_key_(const bb_XMatState_& m) {
  char buf[128];
  std::snprintf(buf, sizeof buf, "%.3f,%.3f,%.3f|%.3f|%d",
                m.r, m.g, m.b, m.alpha, m.tex);
  return buf;
}

struct bb_XBuild_ {
  bb_MeshEntity_*                                ent = nullptr;
  std::unordered_map<std::string, size_t>        surf_of;   // Brushschluessel
  std::filesystem::path                          dir;
  const std::unordered_map<std::string, const bb_XObj_*>* named = nullptr;
  std::unordered_map<std::string, int>            texcache;
  bb_LoaderMat_                                  lm{};
  bool                                           flip = false;
};

inline size_t bb_x_surface_(bb_XBuild_& B, const bb_XMatState_& m) {
  const std::string key = bb_x_brush_key_(m);
  auto it = B.surf_of.find(key);
  if (it != B.surf_of.end()) return it->second;
  const size_t idx = B.ent->surfaces().size();
  B.ent->surfaces().emplace_back();
  bb_Brush_& br = B.ent->surfaces().back().brush;
  br.r = m.r; br.g = m.g; br.b = m.b;
  br.alpha = m.alpha;
  if (m.tex) { br.tex.tex[0] = bb_texture_ref_(m.tex); br.tex.frame[0] = 0; }
  B.surf_of.emplace(key, idx);
  return idx;
}

inline void bb_x_mesh_(bb_XBuild_& B, const bb_XObj_& o, const bb_XMat_& tform) {
  if (o.nums.empty()) return;
  const size_t nv = static_cast<size_t>(o.nums[0]);
  if (!nv || o.nums.size() < 1 + nv * 3 + 1) return;

  // ---- Vertices, gleich in Weltlage und durch die Loadermatrix ----
  std::vector<float> px(nv * 3);
  for (size_t i = 0; i < nv; ++i) {
    float v[3] = { static_cast<float>(o.nums[1 + i * 3]),
                   static_cast<float>(o.nums[2 + i * 3]),
                   static_cast<float>(o.nums[3 + i * 3]) };
    float w[3];
    bb_x_point_(tform, v, w);
    bb_loader_apply_(B.lm, w[0], w[1], w[2], &px[i * 3]);
  }

  // ---- Vielecke ----
  size_t p = 1 + nv * 3;
  const size_t nf = static_cast<size_t>(o.nums[p++]);
  std::vector<std::vector<unsigned>> faces;
  faces.reserve(nf);
  for (size_t f = 0; f < nf && p < o.nums.size(); ++f) {
    const size_t cnt = static_cast<size_t>(o.nums[p++]);
    if (!cnt || p + cnt > o.nums.size()) break;
    std::vector<unsigned> idx(cnt);
    for (size_t k = 0; k < cnt; ++k) idx[k] = static_cast<unsigned>(o.nums[p + k]);
    p += cnt;
    faces.push_back(std::move(idx));
  }

  // ---- Kinder: Materialliste, UV, Normalen ----
  std::vector<bb_XMatState_> mats;
  std::vector<int>           face_mat(faces.size(), 0);
  std::vector<float>         uv;
  std::vector<float>         nrm;

  for (const auto& k : o.kids) {
    if (bb_x_is_(k.type, "meshmateriallist")) {
      if (k.nums.size() >= 2) {
        const size_t nmat = static_cast<size_t>(k.nums[0]);
        const size_t nfac = static_cast<size_t>(k.nums[1]);
        (void)nmat;
        for (size_t f = 0; f < nfac && 2 + f < k.nums.size() && f < face_mat.size(); ++f)
          face_mat[f] = static_cast<int>(k.nums[2 + f]);
      }
      for (const auto& mk : k.kids)
        if (bb_x_is_(mk.type, "material")) mats.push_back(bb_x_material_(mk, B.dir, B.texcache));
      // Verweise auf frueher benannte Materialien aufloesen
      for (const auto& rn : k.refs) {
        if (!B.named) continue;
        auto it = B.named->find(rn);
        if (it != B.named->end() && bb_x_is_(it->second->type, "material"))
          mats.push_back(bb_x_material_(*it->second, B.dir, B.texcache));
      }
    } else if (bb_x_is_(k.type, "meshtexturecoords")) {
      // Gilt laut Quelltext nur bei genau passender Anzahl - und **ohne**
      // Spiegeln der v-Achse, anders als bei .3ds.
      if (!k.nums.empty() && static_cast<size_t>(k.nums[0]) == nv &&
          k.nums.size() >= 1 + nv * 2) {
        uv.resize(nv * 2);
        for (size_t i = 0; i < nv * 2; ++i) uv[i] = static_cast<float>(k.nums[1 + i]);
      }
    } else if (bb_x_is_(k.type, "meshnormals")) {
      if (!k.nums.empty() && static_cast<size_t>(k.nums[0]) == nv &&
          k.nums.size() >= 1 + nv * 3) {
        nrm.resize(nv * 3);
        for (size_t i = 0; i < nv; ++i) {
          float n[3] = { static_cast<float>(k.nums[1 + i * 3]),
                         static_cast<float>(k.nums[2 + i * 3]),
                         static_cast<float>(k.nums[3 + i * 3]) };
          float w[3];
          bb_x_dir_(tform, n, w);
          bb_loader_apply_(B.lm, w[0], w[1], w[2], &nrm[i * 3]);
          float len = std::sqrt(nrm[i*3]*nrm[i*3] + nrm[i*3+1]*nrm[i*3+1] +
                                nrm[i*3+2]*nrm[i*3+2]);
          if (len > 1e-9f) { nrm[i*3] /= len; nrm[i*3+1] /= len; nrm[i*3+2] /= len; }
        }
      }
    }
  }
  if (mats.empty()) mats.push_back(bb_XMatState_());

  // ---- Dreiecke: Faecher ab der ersten Ecke, je Flaeche in ihre Flaeche ----
  std::unordered_map<size_t, std::unordered_map<unsigned, unsigned>> remap;
  for (size_t f = 0; f < faces.size(); ++f) {
    const std::vector<unsigned>& idx = faces[f];
    if (idx.size() < 3) continue;
    int mi = (f < face_mat.size()) ? face_mat[f] : 0;
    if (mi < 0 || mi >= static_cast<int>(mats.size())) mi = 0;
    const size_t si = bb_x_surface_(B, mats[mi]);
    bb_MeshData_& surf = B.ent->surfaces()[si];
    auto& vm = remap[si];

    auto emit = [&](unsigned vi) -> unsigned {
      auto it = vm.find(vi);
      if (it != vm.end()) return it->second;
      const unsigned ni = static_cast<unsigned>(surf.vertices.size() / BB_VF);
      const float* q = &px[vi * 3];
      bb_vert_push_(surf, q[0], q[1], q[2],
                    nrm.empty() ? 0.0f : nrm[vi * 3],
                    nrm.empty() ? 0.0f : nrm[vi * 3 + 1],
                    nrm.empty() ? 0.0f : nrm[vi * 3 + 2],
                    uv.empty()  ? 0.0f : uv[vi * 2],
                    uv.empty()  ? 0.0f : uv[vi * 2 + 1]);
      vm.emplace(vi, ni);
      return ni;
    };

    for (size_t j = 2; j < idx.size(); ++j) {
      if (idx[0] >= nv || idx[j - 1] >= nv || idx[j] >= nv) continue;
      const unsigned a = emit(idx[0]);
      const unsigned b = emit(idx[B.flip ? j : j - 1]);
      const unsigned c = emit(idx[B.flip ? j - 1 : j]);
      surf.indices.push_back(a);
      surf.indices.push_back(b);
      surf.indices.push_back(c);
    }
  }
}

// Frames sammeln ihre Matrix und geben sie an ihre Kinder weiter.
inline void bb_x_walk_(bb_XBuild_& B, const std::vector<bb_XObj_>& objs,
                       const bb_XMat_& tform, bool& any_normals) {
  for (const auto& o : objs) {
    if (bb_x_is_(o.type, "frame")) {
      bb_XMat_ t = tform;
      for (const auto& k : o.kids)
        if (bb_x_is_(k.type, "frametransformmatrix") && k.nums.size() >= 16) {
          bb_XMat_ local{};
          for (int i = 0; i < 16; ++i) local.m[i] = static_cast<float>(k.nums[i]);
          t = bb_x_mul_(local, tform);
        }
      bb_x_walk_(B, o.kids, t, any_normals);
    } else if (bb_x_is_(o.type, "mesh")) {
      for (const auto& k : o.kids)
        if (bb_x_is_(k.type, "meshnormals")) any_normals = true;
      bb_x_mesh_(B, o, tform);
      bb_x_walk_(B, o.kids, tform, any_normals);
    } else {
      bb_x_walk_(B, o.kids, tform, any_normals);
    }
  }
}

// Nur **Materialien** ins Namensregister. In interior.X heissen Frames und
// Materialien gleich (x3dc_1, x3dc_2, ...); ein gemeinsames Register
// behaelt den ersten Treffer, das war dort der Frame - und die
// Materialverweise liefen ins Leere.
inline void bb_x_collect_named_(const std::vector<bb_XObj_>& objs,
                                std::unordered_map<std::string, const bb_XObj_*>& out) {
  for (const auto& o : objs) {
    if (!o.name.empty() && bb_x_is_(o.type, "material")) out.emplace(o.name, &o);
    bb_x_collect_named_(o.kids, out);
  }
}

// Datei lesen und in den Objektbaum zerlegen; false mit Meldung, wenn das
// nicht geht.
inline bool bb_x_read_(const bbString& file, std::vector<bb_XObj_>& roots) {
  std::FILE* f = std::fopen(file.c_str(), "rb");
  if (!f) {
    std::cerr << "[runtime] LoadMesh: cannot open '" << file << "'\n";
    return false;
  }
  std::string src;
  char buf[65536];
  size_t got;
  while ((got = std::fread(buf, 1, sizeof buf, f)) > 0) src.append(buf, got);
  std::fclose(f);

  // Kopf: "xof " + Version + Kodierung + Fliesskommagroesse, 16 Byte.
  if (src.size() < 16 || src.compare(0, 4, "xof ") != 0) {
    std::cerr << "[runtime] LoadMesh: '" << file << "' ist keine .x-Datei\n";
    return false;
  }
  const std::string enc    = src.substr(8, 3);
  const bool        is_bin = (enc == "bin");
  if (!is_bin && enc != "txt") {
    // Bleiben "com" und "cmp", die MSZIP-gepackten Varianten. In der
    // Installation kommt keine davon vor - 43 Dateien sind Text, 8 binaer.
    std::cerr << "[runtime] LoadMesh: '" << file << "' ist im Format '" << enc
              << "' - umgesetzt sind 'txt' und 'bin'\n";
    return false;
  }
  // Die letzten vier Byte des Kopfes sind die Groesse einer Kommazahl in der
  // Datei, "0032" oder "0064"; im Textformat tragen sie nichts.
  int fsize = std::atoi(src.substr(12, 4).c_str());
  if (fsize != 32 && fsize != 64) fsize = 32;
  src.erase(0, 16);

  if (!bb_x_parse_(src, roots, is_bin, fsize)) {
    std::cerr << "[runtime] LoadMesh: '" << file << "' ist nicht lesbar\n";
    return false;
  }
  return true;
}

inline bb_LoaderMat_ bb_x_loader_mat_(const bbString& file) {
  auto it = bb_loader_mats_.find(bb_loader_key_(bb_file_ext_lower_(file)));
  return (it != bb_loader_mats_.end()) ? it->second
                                       : bb_LoaderMat_{ { 1,0,0, 0,1,0, 0,0,1 } };
}

inline int bb_load_x_(const bbString& file, int parent) {
  std::vector<bb_XObj_> roots;
  if (!bb_x_read_(file, roots)) return 0;

  std::unordered_map<std::string, const bb_XObj_*> named;
  bb_x_collect_named_(roots, named);

  auto ent = std::make_unique<bb_MeshEntity_>();
  bb_XBuild_ B;
  B.ent   = ent.get();
  B.dir   = std::filesystem::path(file).parent_path();
  B.named = &named;
  B.lm    = bb_x_loader_mat_(file);
  B.flip  = bb_loader_det_(B.lm) < 0.0f;

  bool any_normals = false;
  bb_x_walk_(B, roots, bb_x_ident_(), any_normals);

  if (ent->surfaces().empty()) {
    std::cerr << "[runtime] LoadMesh: '" << file << "' enthaelt kein Netz\n";
    return 0;
  }
  for (auto& s : ent->surfaces()) s.dirty = true;
  int h = bb_entity_register_(std::move(ent), parent);
  // Nur rechnen, wenn die Datei keine brauchbaren Normalen mitbrachte -
  // genauso entscheidet das Original.
  if (!any_normals) bb_UpdateNormals(h);
  return h;
}

// ============================================================
// .x mit Hierarchie und Animation (3D-19)
//
// Was LoadAnimMesh aus einer .x-Datei macht, nach blitz3d/loader_x.cpp ohne
// HINT_COLLAPSE:
//
//  * Die Wurzel ist ein namenloses Mesh. Netze, die ausserhalb jedes Frames
//    stehen, gehoeren ihr.
//  * Jeder Frame wird ein eigenes Mesh mit dem Namen des Frames, als Kind
//    seines umgebenden Frames. Seine FrameTransformMatrix wird seine
//    **lokale** Lage (zerlegt in Lage, Drehung, Skalierung); die Netze darin
//    bleiben in seinem eigenen Raum.
//  * Ein AnimationSet haengt Schluessel an die Frames, die seine Animationen
//    per Namen nennen: Typ 0 Drehung (Quaternion w,x,y,z), 1 Skalierung,
//    2 Lage. Matrixschluessel (Typ 4) liest das Original nicht. Die Laenge
//    der Sequenz ist die hoechste Bildnummer, die vorkommt.
//  * Die Wurzel bekommt immer einen Animator, auch ohne AnimationSet.
//
// Eine Loadermatrix wirkt auf die Vertices wie beim Einschmelzen, auf die
// Frames als conv * M * conv^-1 und auf die Schluessel wie im Original
// (Drehachse durch die Matrix, bei Spiegelung der Winkel umgekehrt).
// ============================================================

struct bb_XAnimLoad_ {
  const std::unordered_map<std::string, const bb_XObj_*>* named = nullptr;
  std::unordered_map<std::string, int> frames;    // frames_map: Name -> Handle
  std::unordered_map<std::string, int> texcache;
  std::unordered_map<int, std::unordered_map<std::string, size_t>> surf_of;  // je Entity
  std::filesystem::path dir;
  bb_LoaderMat_ lm{};
  float conv[16] = {}, conv_inv[16] = {};
  bool  conv_on = false, flip = false, animonly = false;
  int   anim_len = 0;
};

// Ein Mesh-Objekt in die Entity `h` einbauen, im Raum dieser Entity.
inline void bb_xa_mesh_(bb_XAnimLoad_& L, int h, const bb_XObj_& o) {
  auto* me = static_cast<bb_MeshEntity_*>(bb_entity_get_(h));
  bb_XBuild_ B;
  B.ent   = me;
  B.dir   = L.dir;
  B.named = L.named;
  B.lm    = L.lm;
  B.flip  = L.flip;
  B.texcache = std::move(L.texcache);
  // Gleiche Brushes in derselben Entity landen in derselben Flaeche, auch
  // ueber mehrere Netze hinweg (MeshModel::findSurface).
  B.surf_of = std::move(L.surf_of[h]);
  bb_x_mesh_(B, o, bb_x_ident_());
  L.texcache  = std::move(B.texcache);
  L.surf_of[h] = std::move(B.surf_of);
  for (auto& s : me->surfaces()) s.dirty = true;
  bool normals = false;
  for (const auto& k : o.kids)
    if (bb_x_is_(k.type, "meshnormals")) normals = true;
  // parseMesh: ohne Normalen wird sofort das ganze Netz der Entity
  // nachgerechnet.
  if (!normals) bb_UpdateNormals(h);
}

inline void bb_xa_animkey_(bb_XAnimLoad_& L, bb_Entity_* fe, const bb_XObj_& k) {
  if (k.nums.size() < 2) return;
  const int type = static_cast<int>(k.nums[0]);
  const size_t cnt = static_cast<size_t>(k.nums[1]);
  bb_AnimKeys_& keys = bb_anim_write_(fe->anim);
  size_t p = 2;
  for (size_t i = 0; i < cnt && p + 2 <= k.nums.size(); ++i) {
    const int time = static_cast<int>(k.nums[p]);
    const size_t n = static_cast<size_t>(k.nums[p + 1]);
    p += 2;
    if (p + n > k.nums.size()) break;
    const double* d = &k.nums[p];
    if (time > L.anim_len) L.anim_len = time;
    if (type == 0 && n == 4) {
      bb_Quat_ q{ (float)d[0], (float)d[1], (float)d[2], (float)d[3] };
      if (L.conv_on) {
        if (std::fabs(q.w) < 1 - 1e-6f) {
          const float l = sqrtf(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
          q.w /= l; q.x /= l; q.y /= l; q.z /= l;
          float half = acosf(q.w);
          if (L.flip) half = -half;
          float a[3];
          bb_loader_apply_(L.lm, q.x, q.y, q.z, a);
          const float al = sqrtf(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
          const float s = al > 0 ? sinf(half) / al : 0;
          q = { cosf(half), a[0] * s, a[1] * s, a[2] * s };
        } else {
          q = {};
        }
      }
      keys.rot[time] = q;
    } else if (type == 1 && n == 3) {
      float s[3];
      bb_loader_apply_(L.lm, (float)d[0], (float)d[1], (float)d[2], s);
      keys.scl[time] = { std::fabs(s[0]), std::fabs(s[1]), std::fabs(s[2]) };
    } else if (type == 2 && n == 3) {
      float v[3];
      bb_loader_apply_(L.lm, (float)d[0], (float)d[1], (float)d[2], v);
      keys.pos[time] = { v[0], v[1], v[2] };
    }
    p += n;
  }
}

// parseAnimSet / parseAnim: der Frame kommt ueber seinen Namen, als
// Verweis {Name} in der Animation.
inline void bb_xa_animset_(bb_XAnimLoad_& L, const bb_XObj_& set) {
  for (const auto& a : set.kids) {
    if (!bb_x_is_(a.type, "animation")) continue;
    bb_Entity_* fe = nullptr;
    for (const auto& r : a.refs) {
      auto it = L.frames.find(r);
      if (it != L.frames.end()) fe = bb_entity_get_(it->second);
    }
    if (!fe) continue;
    for (const auto& k : a.kids)
      if (bb_x_is_(k.type, "animationkey")) bb_xa_animkey_(L, fe, k);
  }
}

inline int bb_xa_frame_(bb_XAnimLoad_& L, const bb_XObj_& o, int parent) {
  auto ent = std::make_unique<bb_MeshEntity_>();
  ent->name = o.name;
  const int h = bb_entity_register_(std::move(ent), parent);
  L.frames[o.name] = h;
  for (const auto& k : o.kids) {
    if (bb_x_is_(k.type, "frametransformmatrix")) {
      if (k.nums.size() < 16) continue;
      float t[16];
      for (int i = 0; i < 16; ++i) t[i] = static_cast<float>(k.nums[i]);
      if (L.conv_on) {
        float a[16];
        mat4_mul_(a, L.conv, t);
        mat4_mul_(t, a, L.conv_inv);
      }
      bb_ent_set_local_tform_(bb_entity_get_(h), t);
    } else if (bb_x_is_(k.type, "mesh")) {
      if (!L.animonly) bb_xa_mesh_(L, h, k);
    } else if (bb_x_is_(k.type, "frame")) {
      bb_xa_frame_(L, k, h);
    }
  }
  return h;
}

inline int bb_load_x_anim_(const bbString& file, int parent, bool animonly) {
  std::vector<bb_XObj_> roots;
  if (!bb_x_read_(file, roots)) return 0;

  std::unordered_map<std::string, const bb_XObj_*> named;
  bb_x_collect_named_(roots, named);

  bb_XAnimLoad_ L;
  L.named    = &named;
  L.dir      = std::filesystem::path(file).parent_path();
  L.lm       = bb_x_loader_mat_(file);
  L.flip     = bb_loader_det_(L.lm) < 0.0f;
  L.animonly = animonly;
  mat4_identity_(L.conv);
  for (int c = 0; c < 3; ++c)
    for (int r = 0; r < 3; ++r) L.conv[c * 4 + r] = L.lm.m[c * 3 + r];
  static const float ident[9] = { 1,0,0, 0,1,0, 0,0,1 };
  L.conv_on = std::memcmp(L.lm.m, ident, sizeof ident) != 0;
  if (!mat4_inverse_(L.conv_inv, L.conv)) mat4_identity_(L.conv_inv);

  const int root = bb_entity_register_(std::make_unique<bb_MeshEntity_>(), parent);
  for (const auto& o : roots) {
    if (bb_x_is_(o.type, "mesh")) {
      if (!animonly) bb_xa_mesh_(L, root, o);
    } else if (bb_x_is_(o.type, "frame")) {
      bb_xa_frame_(L, o, root);
    } else if (bb_x_is_(o.type, "animationset")) {
      bb_xa_animset_(L, o);
    }
  }
  bb_entity_get_(root)->animator = bb_animator_new_(root, L.anim_len);
  return root;
}

// ============================================================
// LoadMesh / LoadAnimMesh
// ============================================================

// collapseMesh: der ganze Baum in ein Netz, Kinder vor den Eltern, jedes
// Netz mit seiner Weltlage in die Vertices gerechnet (die Normalen mit der
// Kofaktormatrix, ohne sie zu normieren). Flaechen mit gleichem Brush
// werden zusammengelegt. Der Brush eines Netzes selbst (EntityColor & Co.,
// bei .b3d der Netzbrush) geht dabei verloren - im Original genauso.
// Gleichheit der Brushes: bb_brush_same_ aus bb_surface.h.
inline void bb_collapse_(bb_MeshEntity_* dst, int h) {
  bb_Entity_* e = bb_entity_get_(h);
  if (!e) return;
  for (int c : std::vector<int>(e->children)) bb_collapse_(dst, c);
  if (e->kind() != bb_EntityKind_::Mesh) return;
  bb_entity_refresh_world_(e);
  float co[9];
  mat4_cofactor3_(co, e->world);
  for (const bb_MeshData_& src : static_cast<bb_MeshEntity_*>(e)->surfaces()) {
    bb_MeshData_* d = nullptr;
    for (auto& s : dst->surfaces()) if (bb_brush_same_(s.brush, src.brush)) { d = &s; break; }
    if (!d) { dst->surfaces().emplace_back(); d = &dst->surfaces().back(); d->brush = src.brush; }
    const unsigned base = static_cast<unsigned>(d->vertices.size() / BB_VF);
    for (unsigned i : src.indices) d->indices.push_back(base + i);
    const size_t at = d->vertices.size();
    d->vertices.insert(d->vertices.end(), src.vertices.begin(), src.vertices.end());
    for (size_t i = at; i + BB_VF <= d->vertices.size(); i += BB_VF) {
      float* v = &d->vertices[i];
      const float p[3] = { v[0], v[1], v[2] }, n[3] = { v[3], v[4], v[5] };
      mat4_xform_pt_(v, e->world, p[0], p[1], p[2]);
      mat3_xform_vec_(v + 3, co, n[0], n[1], n[2]);
    }
    d->dirty = true;
  }
}

inline int bb_load_b3d_collapsed_(const bbString& file, int parent) {
  const int t = bb_load_b3d_(file);
  if (!t) return 0;
  auto ent = std::make_unique<bb_MeshEntity_>();
  bb_collapse_(ent.get(), t);
  bb_free_entity_(t);
  return bb_entity_register_(std::move(ent), parent);
}

inline bool bb_is_gltf_ext_(const bbString& ext) { return ext == ".gltf" || ext == ".glb"; }

// glTF mit der Loadermatrix seiner Endung laden, als Baum (3D-24).
inline int bb_load_gltf_tree_(const bbString& file, int parent) {
  const auto it = bb_loader_mats_.find(bb_loader_key_(bb_file_ext_lower_(file)));
  const bb_LoaderMat_ lm = it != bb_loader_mats_.end() ? it->second : bb_LoaderMat_{ { 1,0,0, 0,1,0, 0,0,1 } };
  return bb_load_gltf_(file, lm.m, parent);
}

inline int bb_LoadMesh(const bbString& file, int parent = 0) {
  bb_parent_chk_(parent);   // vor dem Laden, wie bbLoadMesh (BUG-170)
  const bbString ext = bb_file_ext_lower_(file);
  if (ext == ".3ds") return bb_load_3ds_(file, parent);
  if (ext == ".x")   return bb_load_x_(file, parent);
  if (ext == ".b3d") return bb_load_b3d_collapsed_(file, parent);
  if (bb_is_gltf_ext_(ext)) {
    const int t = bb_load_gltf_tree_(file, 0);
    if (!t) return 0;
    auto ent = std::make_unique<bb_MeshEntity_>();
    bb_collapse_(ent.get(), t);
    bb_free_entity_(t);
    return bb_entity_register_(std::move(ent), parent);
  }
  // Ein stilles 0 waere hier besonders irrefuehrend, weil das Programm dann
  // ohne Modell weiterlaeuft.
  std::cerr << "[runtime] LoadMesh: '" << file << "' - gelesen werden .x, "
               ".3ds, .b3d, .gltf und .glb\n";
  return 0;
}

// Die Datei mit Hierarchie und Animation laden (loadEntity ohne
// HINT_COLLAPSE); 0, wenn das Format das noch nicht kann oder die Datei
// nicht lesbar ist.
inline int bb_load_anim_(const bbString& file, int parent, bool animonly) {
  const bbString ext = bb_file_ext_lower_(file);
  if (ext == ".x")   return bb_load_x_anim_(file, parent, animonly);
  if (ext == ".3ds") return bb_load_3ds_anim_(file, parent, animonly);
  if (ext == ".b3d") {
    // Der .b3d-Leser kennt kein "nur Animation" - er laedt alles.
    const int h = bb_load_b3d_(file);
    if (h) bb_b3d_attach_(h, parent);
    return h;
  }
  // glTF: noch ohne Animation (3D-24) - bei LoadAnimSeq kommt nichts dazu.
  if (bb_is_gltf_ext_(ext)) return animonly ? 0 : bb_load_gltf_tree_(file, parent);
  return 0;
}

// LoadAnimMesh behaelt die Hierarchie und die Animation. Danach steht das
// Modell auf Bild 0 der ersten Sequenz, ohne zu laufen - bbLoadAnimMesh ruft
// animate(1,0,0,0).
inline int bb_LoadAnimMesh(const bbString& file, int parent = 0) {
  bb_parent_chk_(parent);
  const bbString ext = bb_file_ext_lower_(file);
  if (ext != ".x" && ext != ".3ds" && ext != ".b3d" && !bb_is_gltf_ext_(ext))
    return bb_LoadMesh(file, parent);
  const int h = bb_load_anim_(file, parent, false);
  if (!h) return 0;
  bb_Entity_* e = bb_entity_get_(h);
  if (e->animator) e->animator->animate(1, 0, 0, 0);
  bb_Entity_* p = parent ? bb_entity_get_(parent) : nullptr;
  bb_update_entity_world_(e, p ? p->world : nullptr);
  return h;
}

// Die Sequenzen einer zweiten Datei anhaengen, ohne ihre Netze zu laden
// (HINT_ANIMONLY). Zugeordnet wird ueber die Namen der Entities. Ohne
// Animator an der Entity: -1, und die Datei wird gar nicht erst gelesen.
inline int bb_LoadAnimSeq(int entity, const bbString& file) {
  bb_Entity_* e = bb_ent_chk_(entity);
  if (!e->animator) return -1;
  if (const int t = bb_load_anim_(file, 0, true)) {
    if (const bb_Entity_* te = bb_entity_get_(t); te && te->animator)
      e->animator->add_seqs(*te->animator);
    bb_free_entity_(t);
  }
  return static_cast<int>(e->animator->seqs.size()) - 1;
}

#endif // BLITZNEXT_BB_LOADER_H
