#ifndef BLITZNEXT_BB_LOADER_H
#define BLITZNEXT_BB_LOADER_H

#include "bb_mesh.h"
#include <cstdint>
#include <cstdio>
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
  BB_3DS_NODE_HDR  = 0xB010, BB_3DS_PIVOT     = 0xB013
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

struct bb_3ds_scene_ {
  std::vector<bb_3ds_mat_> mats;
  std::vector<bb_3ds_obj_> objs;
  // Drehpunkte aus dem Keyframe-Abschnitt, ueber den Objektnamen
  // zugeordnet.
  std::unordered_map<std::string, std::array<float, 3>> pivots;
};

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
        if (id == BB_3DS_MAIN || id == BB_3DS_EDIT ||
            id == BB_3DS_KEYFRAME) {
          walk(b, body, bend, sc);
        } else if (id == BB_3DS_OBJNODE) {
          // Ein Knoten traegt seinen Namen (0xB010) und den Drehpunkt
          // (0xB013). Das Original schiebt das Netz so, dass der
          // Drehpunkt im Ursprung liegt - gemessen an drei Dateien,
          // siehe Kopf.
          std::string nname;
          std::array<float, 3> piv{ 0.0f, 0.0f, 0.0f };
          size_t q = body;
          while (q + 6 <= bend) {
            uint16_t sid = b.u16(q);
            uint32_t sln = b.u32(q + 2);
            if (sln < 6 || q + sln > bend) break;
            if (sid == BB_3DS_NODE_HDR) { size_t l; nname = b.cstr(q + 6, l); }
            else if (sid == BB_3DS_PIVOT) {
              for (int k = 0; k < 3; ++k) piv[k] = b.f32(q + 6 + k * 4);
            }
            q += sln;
          }
          if (!nname.empty()) sc.pivots.emplace(nname, piv);
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
              bb_3ds_obj_ o;
              o.name = oname;
              bb_3ds_read_trimesh_(b, q + 6, q + sln, sc, o);
              if (!o.fa.empty()) sc.objs.push_back(std::move(o));
            }
            q += sln;
          }
        }
        p += ln;
      }
    }
  };
  W::walk(b, 0, b.n, sc);
  return !sc.objs.empty();
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
    const float r = texhandle ? 255.0f : m->r;
    const float g = texhandle ? 255.0f : m->g;
    const float b = texhandle ? 255.0f : m->b;
    std::snprintf(buf, sizeof buf, "%.3f,%.3f,%.3f|%.3f|%.3f|%d",
                  r, g, b, m->shininess,
                  m->alpha + (m->twosided ? 1000.0f : 0.0f), texhandle);
  }
  return buf;
}

inline int bb_load_3ds_(const bbString& file, int parent) {
  std::FILE* f = std::fopen(file.c_str(), "rb");
  if (!f) {
    std::cerr << "[runtime] LoadMesh: cannot open '" << file << "'\n";
    return 0;
  }
  std::fseek(f, 0, SEEK_END);
  long sz = std::ftell(f);
  std::fseek(f, 0, SEEK_SET);
  if (sz <= 0) { std::fclose(f); return 0; }
  std::vector<uint8_t> data(static_cast<size_t>(sz));
  size_t got = std::fread(data.data(), 1, data.size(), f);
  std::fclose(f);
  data.resize(got);

  bb_3ds_buf_ b{ data.data(), data.size() };
  bb_3ds_scene_ sc;
  if (!bb_3ds_parse_(b, sc)) {
    std::cerr << "[runtime] LoadMesh: '" << file << "' ist keine lesbare "
                 ".3ds-Datei\n";
    return 0;
  }

  // Texturen liegen neben der Modelldatei.
  std::filesystem::path dir = std::filesystem::path(file).parent_path();
  std::vector<int> mat_tex(sc.mats.size(), 0);
  for (size_t i = 0; i < sc.mats.size(); ++i) {
    if (sc.mats[i].texfile.empty()) continue;
    std::error_code ec;
    std::filesystem::path cand = dir / sc.mats[i].texfile;
    if (!std::filesystem::exists(cand, ec)) cand = sc.mats[i].texfile;
    mat_tex[i] = bb_LoadTexture(cand.string(), 1);
  }

  auto ent = std::make_unique<bb_MeshEntity_>();

  // Ein Eintrag je Brush. Der Vertexschluessel enthaelt die Glaettungsgruppe:
  // zwei Dreiecke teilen sich einen Vertex nur, wenn sie in derselben Gruppe
  // liegen - sonst entsteht eine harte Kante. Gruppe 0 heisst "keine
  // Glaettung", jedes Dreieck bekommt dann eigene Vertices.
  struct Group { size_t surf; std::unordered_map<uint64_t, unsigned> vmap; };
  std::unordered_map<std::string, Group> groups;
  std::vector<int> surf_mat;                    // Material je Flaeche

  for (size_t oi = 0; oi < sc.objs.size(); ++oi) {
    const bb_3ds_obj_& o = sc.objs[oi];
    const bool has_uv = o.tu.size() >= o.vx.size();

    // Das Netz wird so verschoben, dass der Drehpunkt des Objekts im
    // Ursprung liegt. Der Drehpunkt steht im Keyframe-Abschnitt in
    // **lokalen** Einheiten, muss also durch die Achsenmatrix - deren
    // Skalierung ist genau der Grund, warum sie ueberhaupt gelesen wird.
    // Gemessen an wcrate1.3ds (13.583), oildrum.3ds und fighter.3ds
    // (0.257); Dateien mit Drehpunkt 0 und Einheitsachsen bleiben
    // unveraendert, und genau die verschiebt das Original auch nicht.
    float tx[3] = { o.org[0], o.org[1], o.org[2] };
    auto pit = sc.pivots.find(o.name);
    if (pit != sc.pivots.end()) {
      const std::array<float, 3>& pv = pit->second;
      for (int k = 0; k < 3; ++k)
        tx[k] += o.axes[k] * pv[0] + o.axes[3 + k] * pv[1] + o.axes[6 + k] * pv[2];
    }

    for (size_t fi = 0; fi < o.fa.size(); ++fi) {
      const int mi = (fi < o.fmat.size()) ? o.fmat[fi] : -1;
      const bb_3ds_mat_* m = (mi >= 0 && mi < (int)sc.mats.size()) ? &sc.mats[mi] : nullptr;
      const int th = (mi >= 0 && mi < (int)mat_tex.size()) ? mat_tex[mi] : 0;

      const std::string key = bb_3ds_brush_key_(m, th);
      auto it = groups.find(key);
      if (it == groups.end()) {
        Group gnew;
        gnew.surf = ent->surfaces.size();
        ent->surfaces.emplace_back();
        surf_mat.push_back(mi);
        bb_Brush_& br = ent->surfaces.back().brush;
        if (m) {
          // Gemessen: die Diffusfarbe aus der Datei gilt nur, solange die
          // Flaeche **keine** Textur hat. rocket.3ds hat vier Materialien
          // ohne Textur und erscheint im Original genau in deren Farben
          // (255,191,0 / 191,191,255 / 236,42,42 / 255,255,255). Die
          // texturierte Kiste dagegen traegt die Diffusfarbe 191,191,191
          // und kommt trotzdem mit 254 heraus - die Farbe wuerde die
          // Textur sonst abdunkeln.
          if (!th) { br.r = m->r; br.g = m->g; br.b = m->b; }
          br.shininess = m->shininess;
          br.alpha = m->alpha;
          br.twosided = m->twosided;
        }
        if (th) { br.tex.tex[0] = bb_texture_ref_(th); br.tex.frame[0] = 0; }
        it = groups.emplace(key, std::move(gnew)).first;
      }
      Group& g = it->second;
      bb_MeshData_& surf = ent->surfaces[g.surf];

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
          unsigned ni = static_cast<unsigned>(surf.vertices.size() / 11);
          // Achsen: blitz(x,y,z) = 3ds(x,z,y) - gemessen, siehe Kopf.
          // Gemessen mit einer Vierquadrantentextur: das Original zeigt
          // die linke obere Ecke des Bildes an der linken oberen Ecke der
          // Flaeche. In der Datei laeuft v von unten nach oben, unsere
          // Texturen fangen oben an - also umkehren.
          float u = has_uv ? o.tu[vi] : 0.0f;
          float v = has_uv ? (1.0f - o.tv[vi]) : 0.0f;
          const float px_ = o.vx[vi] - tx[0];
          const float py_ = o.vy[vi] - tx[1];
          const float pz_ = o.vz[vi] - tx[2];
          const float vd[11] = { px_, pz_, py_,
                                 0, 0, 0,
                                 u, v,
                                 1, 1, 1 };
          surf.vertices.insert(surf.vertices.end(), vd, vd + 11);
          g.vmap.emplace(vkey, ni);
          out[k] = ni;
        }
      }
      // Der Achsentausch y/z kehrt die Haendigkeit um, also auch den
      // Umlaufsinn: was in der Datei von aussen gegen den Uhrzeigersinn
      // laeuft, laeuft nach dem Tausch mit dem Uhrzeigersinn. Ohne dieses
      // Vertauschen zeigt die Rueckseitenentfernung die **Rueckseite** des
      // Modells - eine geschlossene Kiste sieht in der Silhouette dann
      // unveraendert aus, ist aber seitenverkehrt beleuchtet und texturiert.
      surf.indices.push_back(out[0]);
      surf.indices.push_back(out[2]);
      surf.indices.push_back(out[1]);
    }
  }

  for (auto& s : ent->surfaces) s.dirty = true;
  int h = bb_entity_register_(std::move(ent), parent);
  bb_UpdateNormals(h);
  return h;
}

// ============================================================
// LoadMesh / LoadAnimMesh
// ============================================================

inline bbString bb_file_ext_lower_(const bbString& file) {
  size_t dot = file.find_last_of('.');
  if (dot == bbString::npos) return "";
  bbString e = file.substr(dot);
  for (auto& c : e) c = static_cast<char>(::tolower((unsigned char)c));
  return e;
}

inline int bb_LoadMesh(const bbString& file, int parent = 0) {
  const bbString ext = bb_file_ext_lower_(file);
  if (ext == ".3ds") return bb_load_3ds_(file, parent);
  // .x und .b3d kommen noch. Ein stilles 0 waere hier besonders
  // irrefuehrend, weil das Programm dann ohne Modell weiterlaeuft.
  std::cerr << "[runtime] LoadMesh: '" << file << "' - bisher nur .3ds "
               "umgesetzt (3D-13)\n";
  return 0;
}

// Laut Doku behaelt LoadAnimMesh Hierarchie und Animation, waehrend LoadMesh
// beides verwirft. Beides gibt es bei uns noch nicht (3D-19), also ist das
// hier dasselbe wie LoadMesh - einmal gemeldet, damit niemand eine Animation
// erwartet, die nicht kommt.
inline int bb_LoadAnimMesh(const bbString& file, int parent = 0) {
  static bool warned = false;
  if (!warned) {
    warned = true;
    std::cerr << "[runtime] LoadAnimMesh: Hierarchie und Animation sind noch "
                 "nicht umgesetzt - laedt wie LoadMesh (3D-13)\n";
  }
  return bb_LoadMesh(file, parent);
}

#endif // BLITZNEXT_BB_LOADER_H
