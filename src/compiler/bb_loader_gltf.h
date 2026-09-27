#ifndef BLITZNEXT_BB_LOADER_GLTF_H
#define BLITZNEXT_BB_LOADER_GLTF_H

// ============================================================
//  glTF 2.0  -  bb_loader_gltf.h   (3D-24)
//
//  Blitz3D kennt glTF nicht. Der Lader ist fuer heutige Werkzeuge da: wer
//  aus Blender exportiert, soll dasselbe Blitz3D bekommen wie mit einer .x-
//  oder .3ds-Datei. Gelesen wird, was der Blitz3D-Renderer darstellen kann;
//  der Rest (Metallic, Roughness, Normal Maps, ...) bleibt liegen.
//
//  Zwei Formen, beide mit LoadMesh und LoadAnimMesh:
//
//    .gltf  JSON-Text; Puffer und Bilder als Datei neben dem Modell oder als
//           data:-URI (Base64)
//    .glb   Kopf "glTF", Version 2, dann ein JSON- und ein BIN-Chunk
//
//  Was woraus wird (wie bei .x: jeder Knoten ist ein Mesh, auch ohne Netz):
//
//    Knoten (Name, Hierarchie, matrix oder TRS)   Mesh-Entity, EntityName
//    Primitive (Dreiecke, Streifen, Faecher)      Flaeche; gleiche Brushes
//                                                 in einer Entity teilen sich
//                                                 eine Flaeche
//    baseColorFactor                              BrushColor, BrushAlpha
//    baseColorTexture                             Texturlage 0
//    occlusionTexture                             Texturlage 1, Multiply -
//                                                 die Lightmap aus Blender
//    texCoord der Textur                          TextureCoords 0 / 1
//    alphaMode BLEND / MASK                       Textur-Flag 2 / Alphatest
//    doubleSided                                  FX 16
//    emissiveFactor ohne Textur, KHR_materials_   FX 1
//    unlit
//    COLOR_0                                      Vertexfarbe, FX 2
//    Sampler CLAMP_TO_EDGE                        Textur-Flag 16 / 32
//    KHR_texture_transform                        UV-Matrix der Textur (vor
//                                                 ScaleTexture & Co.)
//    Gelenk einer Skin (ohne Netz)                Pivot
//    Skin: JOINTS/WEIGHTS, inverseBindMatrices    Knochen des Netzes
//    animations[n]                                Sequenz n (AnimSeq)
//    Morph Targets (POSITION, NORMAL), weights    Versatz je Vertex, Gewichte
//                                                 an der Entity
//
//  Skinning: Ein Vertex folgt bis zu vier Gelenken (die schwersten aus
//  JOINTS_0/1, auf Summe 1 gebracht), gezeichnet wie bei .b3d: Weltlage des
//  Gelenks jetzt mal seine inverse Bind-Matrix. Die Knochenliste haengt am
//  Netz (bb_MeshEntity_::bones), der Animator an der Wurzel - Animate,
//  SetAnimTime & Co. gelten also dem Rueckgabewert von LoadAnimMesh, wie bei
//  .x. LoadMesh backt die Haltung beim Laden ein.
//
//  Animationen: glTF zaehlt in Sekunden, Blitz3D in Bildern, und Animate mit
//  Tempo 1 schaltet je UpdateWorld ein Bild weiter. Eine Sekunde sind hier
//  60 Bilder - bei 60 UpdateWorld je Sekunde laeuft eine Animation mit
//  Tempo 1 also in der Zeit, in der sie in Blender lief. Jede Sequenz
//  beginnt bei ihrem fruehesten Schluessel. LINEAR wird linear bzw. mit
//  Slerp gemischt wie jeder Blitz-Schluessel, STEP haelt den Wert bis ein
//  Bild vor dem naechsten, CUBICSPLINE nimmt nur die Werte (ohne Tangenten).
//  Bewegt eine Sequenz einen Kanal nicht, den eine andere bewegt, steht er
//  dort auf der Ruhelage - sonst bliebe beim Wechsel die alte Haltung
//  stehen.
//
//  Morph Targets (Shape Keys in Blender): Blitz3D kennt sie nicht, und es
//  gibt keinen Befehl dafuer - die Gewichte kommen aus der Datei (node.weights,
//  sonst mesh.weights) und aus dem Animationskanal "weights", laufen also mit
//  Animate, SetAnimTime und Uebergaengen wie jede Sequenz. Gezeichnet wird
//  erst der Morph, dann das Skinning; LoadMesh backt die Gewichte der Datei
//  ein. TANGENT-Ziele bleiben liegen.
//
//  Achsen: glTF ist rechtshaendig mit +Y oben, Blitz3D linkshaendig. Die
//  Vorgabe ist LoaderMatrix "glb"/"gltf" 1,0,0, 0,1,0, 0,0,-1 - z wird
//  gespiegelt, und wie beim .3ds-Lader dreht die negative Determinante den
//  Umlaufsinn um. Warum gerade z: Blenders glTF-Export schreibt
//  gltf = (x, z, -y), und blitz = (x, y, -z) ergibt dann blitz = (x, z, y) -
//  genau die Achsen, die Blenders .3ds (LoaderMatrix "3ds") in Blitz3D
//  bekommt. Ein Modell sieht also gleich aus, ob es als .3ds oder als .glb
//  aus Blender kommt; seine Vorderseite (in Blender -Y) zeigt nach -z, zur
//  Kamera hin.
//
//  Farben: glTF speichert Faktor- und Vertexfarben linear, Blitz3D rechnet
//  mit den Werten, die man sieht. Beide werden deshalb nach sRGB gewandelt;
//  Bilder sind schon sRGB.
//
//  Draco- und meshopt-komprimierte Dateien werden abgelehnt.
// ============================================================

#include "bb_mesh.h"
#include "bb_texture.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// ============================================================
// JSON - klein, ohne Abhaengigkeit, mit Grenzen
// ============================================================

struct bb_Json_ {
  enum Kind { Null, Bool, Num, Str, Arr, Obj };
  Kind        kind = Null;
  bool        b    = false;
  double      num  = 0;
  std::string str;
  std::vector<bb_Json_>                         arr;
  std::vector<std::pair<std::string, bb_Json_>> obj;

  static const bb_Json_& none() { static const bb_Json_ n; return n; }
  const bb_Json_& operator[](const char* key) const {
    if (kind == Obj)
      for (const auto& kv : obj) if (kv.first == key) return kv.second;
    return none();
  }
  // int, nicht size_t: sonst passt die Literal-0 auch auf const char*.
  const bb_Json_& operator[](int i) const {
    return (kind == Arr && i >= 0 && static_cast<size_t>(i) < arr.size()) ? arr[i] : none();
  }
  bool   has() const { return kind != Null; }
  size_t size() const { return kind == Arr ? arr.size() : 0; }
  double n(double def = 0) const { return kind == Num ? num : def; }
  int    i(int def = -1) const { return kind == Num ? static_cast<int>(num) : def; }
  bool   t(bool def = false) const { return kind == Bool ? b : def; }
};

struct bb_JsonIn_ {
  const char* p;
  const char* e;
  int         depth = 0;

  void ws() { while (p < e && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) ++p; }
  bool lit(const char* w) {
    const size_t n = std::strlen(w);
    if (static_cast<size_t>(e - p) < n || std::memcmp(p, w, n)) return false;
    p += n;
    return true;
  }
  static void utf8(std::string& o, unsigned c) {
    if (c < 0x80)         o += static_cast<char>(c);
    else if (c < 0x800) { o += static_cast<char>(0xC0 | (c >> 6)); o += static_cast<char>(0x80 | (c & 63)); }
    else if (c < 0x10000) {
      o += static_cast<char>(0xE0 | (c >> 12));
      o += static_cast<char>(0x80 | ((c >> 6) & 63));
      o += static_cast<char>(0x80 | (c & 63));
    } else {
      o += static_cast<char>(0xF0 | (c >> 18));
      o += static_cast<char>(0x80 | ((c >> 12) & 63));
      o += static_cast<char>(0x80 | ((c >> 6) & 63));
      o += static_cast<char>(0x80 | (c & 63));
    }
  }
  bool hex4(unsigned& c) {
    if (e - p < 4) return false;
    c = 0;
    for (int k = 0; k < 4; ++k) {
      const char h = *p++;
      c <<= 4;
      if (h >= '0' && h <= '9')      c |= h - '0';
      else if (h >= 'a' && h <= 'f') c |= h - 'a' + 10;
      else if (h >= 'A' && h <= 'F') c |= h - 'A' + 10;
      else return false;
    }
    return true;
  }
  bool string(std::string& o) {
    if (p >= e || *p != '"') return false;
    ++p;
    while (p < e && *p != '"') {
      const char c = *p++;
      if (c != '\\') { o += c; continue; }
      if (p >= e) return false;
      switch (*p++) {
        case '"':  o += '"';  break;
        case '\\': o += '\\'; break;
        case '/':  o += '/';  break;
        case 'b':  o += '\b'; break;
        case 'f':  o += '\f'; break;
        case 'n':  o += '\n'; break;
        case 'r':  o += '\r'; break;
        case 't':  o += '\t'; break;
        case 'u': {
          unsigned c1;
          if (!hex4(c1)) return false;
          if (c1 >= 0xD800 && c1 < 0xDC00 && e - p >= 6 && p[0] == '\\' && p[1] == 'u') {
            p += 2;
            unsigned c2;
            if (!hex4(c2)) return false;
            if (c2 >= 0xDC00 && c2 < 0xE000) {
              c1 = 0x10000 + ((c1 - 0xD800) << 10) + (c2 - 0xDC00);
            } else {
              utf8(o, c1);
              c1 = c2;
            }
          }
          utf8(o, c1);
          break;
        }
        default: return false;
      }
    }
    if (p >= e) return false;
    ++p;
    return true;
  }
  bool value(bb_Json_& v) {
    ws();
    if (p >= e || depth > 200) return false;
    ++depth;
    bool ok = true;
    if (*p == '{') {
      v.kind = bb_Json_::Obj;
      ++p;
      ws();
      if (p < e && *p == '}') { ++p; }
      else for (;;) {
        ws();
        std::string k;
        if (!string(k)) { ok = false; break; }
        ws();
        if (p >= e || *p != ':') { ok = false; break; }
        ++p;
        v.obj.emplace_back(std::move(k), bb_Json_());
        if (!value(v.obj.back().second)) { ok = false; break; }
        ws();
        if (p < e && *p == ',') { ++p; continue; }
        if (p < e && *p == '}') { ++p; break; }
        ok = false;
        break;
      }
    } else if (*p == '[') {
      v.kind = bb_Json_::Arr;
      ++p;
      ws();
      if (p < e && *p == ']') { ++p; }
      else for (;;) {
        v.arr.emplace_back();
        if (!value(v.arr.back())) { ok = false; break; }
        ws();
        if (p < e && *p == ',') { ++p; continue; }
        if (p < e && *p == ']') { ++p; break; }
        ok = false;
        break;
      }
    } else if (*p == '"') {
      v.kind = bb_Json_::Str;
      ok = string(v.str);
    } else if (lit("true"))  { v.kind = bb_Json_::Bool; v.b = true;  }
    else if (lit("false"))   { v.kind = bb_Json_::Bool; v.b = false; }
    else if (lit("null"))    { v.kind = bb_Json_::Null; }
    else {
      // from_chars haengt nicht vom Gebietsschema ab (kein Dezimalkomma).
      v.kind = bb_Json_::Num;
      const char* s = p;
      if (s < e && *s == '-') ++s;
      auto r = std::from_chars(s, e, v.num);
      if (r.ec != std::errc() || r.ptr == s) ok = false;
      else { if (*p == '-') v.num = -v.num; p = r.ptr; }
    }
    --depth;
    return ok;
  }
};

inline bool bb_json_parse_(const char* s, size_t n, bb_Json_& out) {
  bb_JsonIn_ in{ s, s + n };
  if (n >= 3 && static_cast<unsigned char>(s[0]) == 0xEF &&
      static_cast<unsigned char>(s[1]) == 0xBB && static_cast<unsigned char>(s[2]) == 0xBF)
    in.p += 3;                                  // UTF-8-BOM
  if (!in.value(out)) return false;
  in.ws();
  return in.p == in.e;
}

// ============================================================
// URIs: data:...;base64, und Dateien neben dem Modell
// ============================================================

inline bool bb_base64_(const char* s, size_t n, std::vector<uint8_t>& out) {
  auto val = [](char c) -> int {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+' || c == '-') return 62;
    if (c == '/' || c == '_') return 63;
    return -1;
  };
  unsigned acc = 0;
  int bits = 0;
  for (size_t i = 0; i < n; ++i) {
    const char c = s[i];
    if (c == '=') break;
    if (c == '\r' || c == '\n' || c == ' ') continue;
    const int v = val(c);
    if (v < 0) return false;
    acc = (acc << 6) | static_cast<unsigned>(v);
    bits += 6;
    if (bits >= 8) {
      bits -= 8;
      out.push_back(static_cast<uint8_t>((acc >> bits) & 0xFF));
    }
  }
  return true;
}

inline std::string bb_uri_decode_(const std::string& u) {
  std::string o;
  for (size_t i = 0; i < u.size(); ++i) {
    if (u[i] == '%' && i + 2 < u.size() && std::isxdigit(static_cast<unsigned char>(u[i + 1])) &&
        std::isxdigit(static_cast<unsigned char>(u[i + 2]))) {
      o += static_cast<char>(std::stoi(u.substr(i + 1, 2), nullptr, 16));
      i += 2;
    } else {
      o += u[i];
    }
  }
  return o;
}

inline bool bb_gltf_read_file_(const std::string& path, std::vector<uint8_t>& out) {
  std::FILE* f = std::fopen(path.c_str(), "rb");
  if (!f) return false;
  char buf[65536];
  size_t got;
  while ((got = std::fread(buf, 1, sizeof buf, f)) > 0) out.insert(out.end(), buf, buf + got);
  std::fclose(f);
  return true;
}

// ============================================================
// Der Ladevorgang
// ============================================================

struct bb_GltfLoad_ {
  bbString                          file;
  std::filesystem::path             dir;
  bb_Json_                          doc;
  std::vector<std::vector<uint8_t>> buffers;
  std::vector<bool>                 buffer_ok;
  float                             lm[9] = { 1,0,0, 0,1,0, 0,0,1 };
  bool                              flip = false, conv_on = false;
  float                             conv[16] = {}, conv_inv[16] = {};
  std::map<std::string, int>        texcache;   // Textur + Flags -> Handle
  std::vector<bool>                 node_seen;  // gegen Zyklen im Knotenbaum
  std::vector<int>                  node_h;     // Handle je Knoten, 0 = keins
  std::vector<bool>                 is_joint;   // Gelenk irgendeiner Skin
  struct Rest { bb_Vec3_ p, s; bb_Quat_ r; std::vector<float> w; };
  std::vector<Rest>                 rest;       // lokale Lage beim Laden
  std::vector<std::pair<int, int>>  skinned;    // (Netz-Handle, Skin)
  bool                              animonly = false;
  bool                              warned = false;

  void warn(const std::string& what) {
    if (warned) return;
    warned = true;
    std::cerr << "[runtime] LoadMesh: '" << file << "' - " << what << "\n";
  }
};

inline void bb_gltf_lm_apply_(const bb_GltfLoad_& L, const double v[3], float out[3]) {
  const float* m = L.lm;
  out[0] = static_cast<float>(m[0] * v[0] + m[3] * v[1] + m[6] * v[2]);
  out[1] = static_cast<float>(m[1] * v[0] + m[4] * v[1] + m[7] * v[2]);
  out[2] = static_cast<float>(m[2] * v[0] + m[5] * v[1] + m[8] * v[2]);
}

// Ein Element eines Accessors, als double; normierte Ganzzahlen nach
// glTF 3.11: vorzeichenbehaftete mit max(c / MAX, -1).
inline double bb_gltf_comp_(const uint8_t* p, int ct, bool norm) {
  switch (ct) {
    case 5120: { int8_t   v; std::memcpy(&v, p, 1); return norm ? std::max(v / 127.0, -1.0) : v; }
    case 5121: { uint8_t  v; std::memcpy(&v, p, 1); return norm ? v / 255.0 : v; }
    case 5122: { int16_t  v; std::memcpy(&v, p, 2); return norm ? std::max(v / 32767.0, -1.0) : v; }
    case 5123: { uint16_t v; std::memcpy(&v, p, 2); return norm ? v / 65535.0 : v; }
    case 5125: { uint32_t v; std::memcpy(&v, p, 4); return norm ? v / 4294967295.0 : v; }
    case 5126: { float    v; std::memcpy(&v, p, 4); return v; }
  }
  return 0;
}

inline int bb_gltf_comp_size_(int ct) {
  switch (ct) {
    case 5120: case 5121: return 1;
    case 5122: case 5123: return 2;
    case 5125: case 5126: return 4;
  }
  return 0;
}

inline int bb_gltf_type_comps_(const std::string& t) {
  if (t == "SCALAR") return 1;
  if (t == "VEC2")   return 2;
  if (t == "VEC3")   return 3;
  if (t == "VEC4")   return 4;
  if (t == "MAT2")   return 4;
  if (t == "MAT3")   return 9;
  if (t == "MAT4")   return 16;
  return 0;
}

// Die Bytes einer bufferView, geprueft; nullptr, wenn sie nicht passt.
inline const uint8_t* bb_gltf_view_(const bb_GltfLoad_& L, int bv, size_t& len, size_t& stride) {
  const bb_Json_& v = L.doc["bufferViews"][bv];
  if (!v.has()) return nullptr;
  const int b = v["buffer"].i();
  if (b < 0 || b >= static_cast<int>(L.buffers.size()) || !L.buffer_ok[b]) return nullptr;
  const double off = v["byteOffset"].n(0), n = v["byteLength"].n(-1);
  const std::vector<uint8_t>& buf = L.buffers[b];
  if (off < 0 || n < 0 || off + n > static_cast<double>(buf.size())) return nullptr;
  len    = static_cast<size_t>(n);
  stride = static_cast<size_t>(v["byteStride"].n(0));
  return buf.data() + static_cast<size_t>(off);
}

// Alle Werte eines Accessors, je Element `comps` Stueck. Ohne bufferView
// sind es Nullen, danach gelten die sparse-Ersetzungen.
inline bool bb_gltf_accessor_(const bb_GltfLoad_& L, int idx, std::vector<double>& out, int& comps) {
  const bb_Json_& a = L.doc["accessors"][idx];
  if (!a.has()) return false;
  const int    ct    = a["componentType"].i();
  const bool   norm  = a["normalized"].t();
  const int    csize = bb_gltf_comp_size_(ct);
  comps = bb_gltf_type_comps_(a["type"].str);
  const double cnt_d = a["count"].n(-1);
  if (!csize || !comps || cnt_d < 0 || cnt_d > 1e8) return false;
  const size_t count = static_cast<size_t>(cnt_d);
  out.assign(count * comps, 0.0);

  const size_t esize = static_cast<size_t>(csize) * comps;
  if (a["bufferView"].has()) {
    size_t len, stride;
    const uint8_t* d = bb_gltf_view_(L, a["bufferView"].i(), len, stride);
    if (!d) return false;
    if (!stride) stride = esize;
    const double off = a["byteOffset"].n(0);
    if (off < 0) return false;
    if (count && static_cast<size_t>(off) + (count - 1) * stride + esize > len) return false;
    d += static_cast<size_t>(off);
    for (size_t i = 0; i < count; ++i)
      for (int c = 0; c < comps; ++c)
        out[i * comps + c] = bb_gltf_comp_(d + i * stride + c * csize, ct, norm);
  }

  const bb_Json_& sp = a["sparse"];
  if (sp.has()) {
    const size_t n = static_cast<size_t>(sp["count"].n(0));
    const bb_Json_& si = sp["indices"];
    const bb_Json_& sv = sp["values"];
    const int ict = si["componentType"].i();
    const int isz = bb_gltf_comp_size_(ict);
    size_t ilen, vlen, dummy;
    const uint8_t* id = bb_gltf_view_(L, si["bufferView"].i(), ilen, dummy);
    const uint8_t* vd = bb_gltf_view_(L, sv["bufferView"].i(), vlen, dummy);
    const size_t ioff = static_cast<size_t>(si["byteOffset"].n(0));
    const size_t voff = static_cast<size_t>(sv["byteOffset"].n(0));
    if (!id || !vd || !isz || ioff + n * isz > ilen || voff + n * esize > vlen) return false;
    for (size_t k = 0; k < n; ++k) {
      const size_t at = static_cast<size_t>(bb_gltf_comp_(id + ioff + k * isz, ict, false));
      if (at >= count) return false;
      for (int c = 0; c < comps; ++c)
        out[at * comps + c] = bb_gltf_comp_(vd + voff + k * esize + c * csize, ct, norm);
    }
  }
  return true;
}

// ---- Texturen ----

// Die Bytes des Bildes einer Textur und ein Name fuer TextureName$.
inline bool bb_gltf_image_(const bb_GltfLoad_& L, int img, std::vector<uint8_t>& bytes,
                           std::string& name) {
  const bb_Json_& im = L.doc["images"][img];
  if (!im.has()) return false;
  const std::string& uri = im["uri"].str;
  if (!uri.empty()) {
    if (uri.rfind("data:", 0) == 0) {
      const size_t comma = uri.find(',');
      if (comma == std::string::npos || uri.find(";base64") > comma) return false;
      name = L.file + "#" + std::to_string(img);
      return bb_base64_(uri.data() + comma + 1, uri.size() - comma - 1, bytes);
    }
    const std::filesystem::path p = L.dir / bb_uri_decode_(uri);
    name = bb_tex_abs_path_(p.string());
    return bb_gltf_read_file_(p.string(), bytes);
  }
  size_t len, stride;
  const uint8_t* d = bb_gltf_view_(L, im["bufferView"].i(), len, stride);
  if (!d) return false;
  bytes.assign(d, d + len);
  name = bb_tex_abs_path_(L.file) + "#" + std::to_string(img);
  return true;
}

// Eine Textur als Blitz-Handle. `cut` >= 0: Alphatest (MASK) mit dieser
// Schwelle 0..1; `grey`: nur der Rotkanal, als Grau (eine Occlusion-Textur,
// die sich das Bild mit Metallic/Roughness teilt, traegt sie dort).
// Dieselbe Textur mit denselben Einstellungen ist dasselbe Handle - sonst
// wuerden gleiche Brushes nicht zusammengefasst.
// `uv`: die Matrix aus KHR_texture_transform (spaltenweise 2x3), nullptr ohne.
inline int bb_gltf_texture_(bb_GltfLoad_& L, int ti, int flags, int coords, double cut, bool grey,
                            const float* uv = nullptr) {
  const bb_Json_& tx = L.doc["textures"][ti];
  if (!tx.has()) return 0;
  const int img = tx["source"].i();
  const bb_Json_& sm = L.doc["samplers"][tx["sampler"].i()];
  if (sm["wrapS"].i(10497) == 33071) flags |= BB_TEX_CLAMPU;
  if (sm["wrapT"].i(10497) == 33071) flags |= BB_TEX_CLAMPV;

  const std::string key = std::to_string(img) + "/" + std::to_string(flags) + "/" +
                          std::to_string(coords) + "/" + std::to_string(cut) + "/" + (grey ? "g" : "c");
  std::string ukey = key;
  if (uv) for (int i = 0; i < 6; ++i) ukey += "/" + std::to_string(uv[i]);
  if (auto it = L.texcache.find(ukey); it != L.texcache.end()) return it->second;

  std::vector<uint8_t> bytes;
  std::string name;
  int w = 0, h = 0, ch = 0;
  unsigned char* data = nullptr;
  if (bb_gltf_image_(L, img, bytes, name) && !bytes.empty())
    data = stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()), &w, &h, &ch, 4);
  if (!data) {
    std::cerr << "[runtime] LoadMesh: '" << L.file << "' - Bild " << img
              << " nicht lesbar (PNG und JPEG gehen)\n";
    L.texcache.emplace(ukey, 0);
    return 0;
  }

  auto t   = std::make_shared<bb_Texture_>();
  t->w     = w;
  t->h     = h;
  t->flags = bb_tex_apply_filters_(name, flags);
  t->name  = name;
  bb_TexFrame_ f;
  f.px.assign(data, data + static_cast<size_t>(w) * h * 4);
  stbi_image_free(data);
  if (grey)
    for (size_t i = 0; i + 3 < f.px.size(); i += 4) f.px[i + 1] = f.px[i + 2] = f.px[i];
  if (cut >= 0) {
    // Alphatest: harte Kante nach der Schwelle, geblendet wird nicht.
    const double c255 = cut * 255.0;
    for (size_t i = 3; i < f.px.size(); i += 4) f.px[i] = (f.px[i] >= c255) ? 255 : 0;
    t->flags = (t->flags & ~BB_TEX_ALPHA) | BB_TEX_ALPHATEST_;
  } else if (t->flags & BB_TEX_ALPHA) {
    // BLEND: der Alphakanal der Datei gilt; ein Bild ohne ist deckend (nicht
    // wie LoadTexture die Helligkeit).
    if (ch < 4) for (size_t i = 3; i < f.px.size(); i += 4) f.px[i] = 255;
    if (t->flags & BB_TEX_MASKED) bb_tex_fix_alpha_(f.px, t->flags, 4);
  } else {
    bb_tex_fix_alpha_(f.px, t->flags, ch);
  }
  t->frames.push_back(std::move(f));
  t->coords = coords;
  if (uv) {
    t->pre_on = true;
    for (int i = 0; i < 6; ++i) t->pre[i] = uv[i];
  }
  const int handle = bb_texture_register_(std::move(t));
  L.texcache.emplace(ukey, handle);
  return handle;
}

// ---- Materialien ----

inline float bb_gltf_srgb_(double c) {
  c = c < 0 ? 0 : (c > 1 ? 1 : c);
  return static_cast<float>(c <= 0.0031308 ? c * 12.92 : 1.055 * std::pow(c, 1 / 2.4) - 0.055);
}

inline bb_Brush_ bb_gltf_brush_(bb_GltfLoad_& L, int mi, bool vcolors) {
  bb_Brush_ br;
  if (vcolors) br.fx |= 2;
  const bb_Json_& m = L.doc["materials"][mi];
  if (!m.has()) return br;
  const bb_Json_& pbr = m["pbrMetallicRoughness"];
  const bb_Json_& bcf = pbr["baseColorFactor"];
  br.r = bb_gltf_srgb_(bcf[0].n(1)) * 255.0f;
  br.g = bb_gltf_srgb_(bcf[1].n(1)) * 255.0f;
  br.b = bb_gltf_srgb_(bcf[2].n(1)) * 255.0f;

  const std::string mode = m["alphaMode"].has() ? m["alphaMode"].str : "OPAQUE";
  const double cutoff = m["alphaCutoff"].n(0.5);
  int    tflags = BB_TEX_COLOR;
  double cut = -1;
  if (mode == "BLEND") {
    br.alpha = static_cast<float>(bcf[3].n(1));
    tflags |= BB_TEX_ALPHA;
  } else if (mode == "MASK") {
    if (bcf[3].n(1) < cutoff) br.alpha = 0;
    cut = cutoff;
  }
  if (m["doubleSided"].t()) br.fx |= 16;
  const bb_Json_& ef = m["emissiveFactor"];
  const bool emits = ef[0].n(0) > 0 || ef[1].n(0) > 0 || ef[2].n(0) > 0;
  if ((emits && !m["emissiveTexture"].has()) || m["extensions"]["KHR_materials_unlit"].has())
    br.fx |= 1;

  // KHR_texture_transform ersetzt auch texCoord.
  auto tex_coords = [&](const bb_Json_& info) {
    const bb_Json_& x = info["extensions"]["KHR_texture_transform"]["texCoord"];
    const int tc = x.has() ? x.i(0) : info["texCoord"].i(0);
    if (tc > 1) L.warn("nur TEXCOORD_0 und TEXCOORD_1 werden gelesen");
    return tc == 1 ? 1 : 0;
  };
  // uv' = T(offset) * R(rotation) * S(scale) * uv, R = [cos sin; -sin cos]
  // (KHR_texture_transform) - spaltenweise 2x3 fuer bb_Texture_::pre.
  float uvm[2][6];
  auto uv_matrix = [&](const bb_Json_& info, int k) -> const float* {
    const bb_Json_& x = info["extensions"]["KHR_texture_transform"];
    if (!x.has()) return nullptr;
    const double r = x["rotation"].n(0), c = std::cos(r), s = std::sin(r);
    const double sx = x["scale"][0].n(1), sy = x["scale"][1].n(1);
    float* m = uvm[k];
    m[0] = static_cast<float>(c * sx);  m[1] = static_cast<float>(-s * sx);
    m[2] = static_cast<float>(s * sy);  m[3] = static_cast<float>(c * sy);
    m[4] = static_cast<float>(x["offset"][0].n(0));
    m[5] = static_cast<float>(x["offset"][1].n(0));
    return m;
  };
  const bb_Json_& base = pbr["baseColorTexture"];
  if (base.has()) {
    if (const int t = bb_gltf_texture_(L, base["index"].i(), tflags, tex_coords(base), cut, false,
                                       uv_matrix(base, 0))) {
      br.tex.tex[0]   = bb_texture_ref_(t);
      br.tex.frame[0] = 0;
    }
  }
  const bb_Json_& occ = m["occlusionTexture"];
  if (occ.has()) {
    const bb_Json_& mr = pbr["metallicRoughnessTexture"];
    const auto source = [&](const bb_Json_& info) {
      const int ti = info["index"].i();
      return L.doc["textures"][ti]["source"].i(-2);
    };
    const bool packed = mr.has() && source(mr) == source(occ);
    if (const int t = bb_gltf_texture_(L, occ["index"].i(), BB_TEX_COLOR, tex_coords(occ), -1, packed,
                                       uv_matrix(occ, 1))) {
      br.tex.tex[1]   = bb_texture_ref_(t);
      br.tex.frame[1] = 0;
    }
  }
  return br;
}

// ---- Netze ----

// Die Einfluesse eines Vertex: Knochen absteigend nach Gewicht, 255 = keiner.
struct bb_GltfBones_ {
  uint8_t id[4] = { 255, 255, 255, 255 };
  float   w[4]  = { 0, 0, 0, 0 };
  void add(int b, float wt) {
    int i = 0;
    for (; i < 4; ++i) if (id[i] == 255 || wt > w[i]) break;
    if (i == 4) return;
    for (int k = 3; k > i; --k) { id[k] = id[k - 1]; w[k] = w[k - 1]; }
    id[i] = static_cast<uint8_t>(b);
    w[i] = wt;
  }
};

// `skin` >= 0: das Netz haengt an dieser Skin, die Vertices bekommen Knochen
// (1 + Gelenknummer; 0 ist das Netz selbst).
inline void bb_gltf_mesh_(bb_GltfLoad_& L, int h, int mi, int skin) {
  auto* me = static_cast<bb_MeshEntity_*>(bb_entity_get_(h));
  const bb_Json_& mesh = L.doc["meshes"][mi];
  bool need_normals = false;
  const bb_Json_& prims = mesh["primitives"];
  for (size_t pi = 0; pi < prims.size(); ++pi) {
    const bb_Json_& pr = prims[pi];
    const int mode = pr["mode"].i(4);
    if (mode < 4 || mode > 6) { L.warn("Punkte und Linien werden uebergangen"); continue; }
    const bb_Json_& at = pr["attributes"];

    std::vector<double> pos, nrm, uv0, uv1, col, idx;
    int pc = 0, nc = 0, u0c = 0, u1c = 0, cc = 0, ic = 0;
    if (!bb_gltf_accessor_(L, at["POSITION"].i(), pos, pc) || pc != 3) {
      L.warn("ein Primitive ohne lesbare Positionen wird uebergangen");
      continue;
    }
    const size_t nv = pos.size() / 3;
    const bool has_n  = bb_gltf_accessor_(L, at["NORMAL"].i(), nrm, nc) && nc == 3;
    const bool has_u0 = bb_gltf_accessor_(L, at["TEXCOORD_0"].i(), uv0, u0c) && u0c == 2;
    const bool has_u1 = bb_gltf_accessor_(L, at["TEXCOORD_1"].i(), uv1, u1c) && u1c == 2;
    const bool has_c  = bb_gltf_accessor_(L, at["COLOR_0"].i(), col, cc) && (cc == 3 || cc == 4);
    if (!has_n) need_normals = true;

    // Morph Targets: Versatz von Lage und Normale je Ziel. Alle Primitive
    // eines Netzes haben gleich viele (glTF 3.7.2.2); gezaehlt wird am ersten.
    const int nt = static_cast<int>(prims[0]["targets"].size());
    std::vector<std::vector<double>> tp(nt), tn(nt);
    for (int t = 0; t < nt; ++t) {
      const bb_Json_& tg = pr["targets"][t];
      int c = 0;
      if (!bb_gltf_accessor_(L, tg["POSITION"].i(), tp[t], c) || c != 3 || tp[t].size() != nv * 3)
        tp[t].clear();
      if (!bb_gltf_accessor_(L, tg["NORMAL"].i(), tn[t], c) || c != 3 || tn[t].size() != nv * 3)
        tn[t].clear();
    }

    std::vector<double> jn[2], wt[2];
    int sets = 0;
    const int njoints = skin >= 0 ? static_cast<int>(L.doc["skins"][skin]["joints"].size()) : 0;
    if (njoints > 254) L.warn("mehr als 254 Gelenke - die uebrigen werden uebergangen");
    for (int k = 0; k < 2 && skin >= 0; ++k) {
      int jc = 0, wc = 0;
      const std::string n = std::to_string(k);
      if (!bb_gltf_accessor_(L, at[("JOINTS_" + n).c_str()].i(), jn[k], jc) || jc != 4 ||
          !bb_gltf_accessor_(L, at[("WEIGHTS_" + n).c_str()].i(), wt[k], wc) || wc != 4 ||
          jn[k].size() != nv * 4 || wt[k].size() != nv * 4)
        break;
      sets = k + 1;
    }

    // Dreiecke als Liste; Streifen und Faecher werden aufgeloest.
    std::vector<uint32_t> list;
    if (pr["indices"].has()) {
      if (!bb_gltf_accessor_(L, pr["indices"].i(), idx, ic) || ic != 1) {
        L.warn("ein Primitive mit unlesbaren Indizes wird uebergangen");
        continue;
      }
    } else {
      idx.resize(nv);
      for (size_t i = 0; i < nv; ++i) idx[i] = static_cast<double>(i);
    }
    const size_t ni = idx.size();
    auto I = [&](size_t k) { return static_cast<uint32_t>(idx[k]); };
    if (mode == 4) {
      for (size_t k = 0; k + 2 < ni; k += 3) list.insert(list.end(), { I(k), I(k + 1), I(k + 2) });
    } else if (mode == 5) {
      for (size_t k = 0; k + 2 < ni; ++k) {
        if (k & 1) list.insert(list.end(), { I(k + 1), I(k), I(k + 2) });
        else       list.insert(list.end(), { I(k), I(k + 1), I(k + 2) });
      }
    } else {
      for (size_t k = 1; k + 1 < ni; ++k) list.insert(list.end(), { I(k), I(k + 1), I(0) });
    }

    // Die Flaeche: eine vorhandene mit gleichem Brush, sonst eine neue
    // (MeshModel::findSurface, wie beim .x-Lader).
    const bb_Brush_ br = bb_gltf_brush_(L, pr["material"].i(), has_c);
    bb_MeshData_* s = nullptr;
    for (auto& o : me->surfaces()) if (bb_brush_same_(o.brush, br)) { s = &o; break; }
    if (!s) { me->surfaces().emplace_back(); s = &me->surfaces().back(); s->brush = br; }

    const unsigned base = static_cast<unsigned>(s->vertices.size() / BB_VF);
    if (skin >= 0) {
      s->bone_ids.resize(static_cast<size_t>(base) * 4, 255);
      s->bone_w.resize(static_cast<size_t>(base) * 4, 0.0f);
    }
    if (nt && s->morph.size() < static_cast<size_t>(nt)) s->morph.resize(nt);
    for (auto& d : s->morph) d.resize(static_cast<size_t>(base) * 6, 0.0f);
    for (size_t v = 0; v < nv; ++v) {
      if (skin >= 0) {
        bb_GltfBones_ bn;
        for (int k = 0; k < sets; ++k)
          for (int c = 0; c < 4; ++c) {
            const int j = static_cast<int>(jn[k][v * 4 + c]);
            const float w = static_cast<float>(wt[k][v * 4 + c]);
            if (w > 0 && j >= 0 && j < std::min(njoints, 254)) bn.add(j + 1, w);
          }
        float sum = 0;
        for (int k = 0; k < 4 && bn.id[k] != 255; ++k) sum += bn.w[k];
        if (sum > 0) for (float& w : bn.w) w /= sum;
        s->bone_ids.insert(s->bone_ids.end(), bn.id, bn.id + 4);
        s->bone_w.insert(s->bone_w.end(), bn.w, bn.w + 4);
      }
      float vd[BB_VF] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 };
      bb_gltf_lm_apply_(L, &pos[v * 3], vd);
      if (has_n) {
        bb_gltf_lm_apply_(L, &nrm[v * 3], vd + 3);
        const float l = std::sqrt(vd[3] * vd[3] + vd[4] * vd[4] + vd[5] * vd[5]);
        if (l > 0) { vd[3] /= l; vd[4] /= l; vd[5] /= l; }
      }
      if (has_u0) { vd[6] = static_cast<float>(uv0[v * 2]); vd[7] = static_cast<float>(uv0[v * 2 + 1]); }
      // Ohne zweiten Satz derselbe wie der erste - wie AddVertex.
      if (has_u1) { vd[8] = static_cast<float>(uv1[v * 2]); vd[9] = static_cast<float>(uv1[v * 2 + 1]); }
      else        { vd[8] = vd[6]; vd[9] = vd[7]; }
      if (has_c) {
        for (int c = 0; c < 3; ++c) vd[10 + c] = bb_gltf_srgb_(col[v * cc + c]);
        if (cc == 4) vd[13] = static_cast<float>(col[v * 4 + 3]);
      }
      s->vertices.insert(s->vertices.end(), vd, vd + BB_VF);
      for (size_t t = 0; t < s->morph.size(); ++t) {
        float d[6] = { 0, 0, 0, 0, 0, 0 };
        if (t < tp.size() && !tp[t].empty()) bb_gltf_lm_apply_(L, &tp[t][v * 3], d);
        if (t < tn.size() && !tn[t].empty()) bb_gltf_lm_apply_(L, &tn[t][v * 3], d + 3);
        s->morph[t].insert(s->morph[t].end(), d, d + 6);
      }
    }
    for (size_t k = 0; k + 2 < list.size(); k += 3) {
      const uint32_t a = list[k], b = list[k + 1], c = list[k + 2];
      if (a >= nv || b >= nv || c >= nv) continue;
      s->indices.push_back(base + a);
      s->indices.push_back(base + (L.flip ? c : b));
      s->indices.push_back(base + (L.flip ? b : c));
    }
    s->dirty = true;
  }
  if (need_normals) bb_UpdateNormals(h);
}

// ---- Knoten ----

// Die lokale Matrix eines Knotens (spaltenweise, wie die Entities): matrix,
// oder T * R * S.
// T * R * S aus einer glTF-Quaternion (x,y,z,w; rechtshaendig wie jede
// Matrix hier, erst die Loadermatrix macht Blitz daraus).
inline void bb_gltf_trs_matrix_(double tx, double ty, double tz, double x, double y, double z,
                                double w, double sx, double sy, double sz, float t[16]) {
  const double l = std::sqrt(x * x + y * y + z * z + w * w);
  if (l > 0) { x /= l; y /= l; z /= l; w /= l; } else { w = 1; }
  const double r[3][3] = {
    { 1 - 2 * (y * y + z * z), 2 * (x * y - z * w),     2 * (x * z + y * w) },
    { 2 * (x * y + z * w),     1 - 2 * (x * x + z * z), 2 * (y * z - x * w) },
    { 2 * (x * z - y * w),     2 * (y * z + x * w),     1 - 2 * (x * x + y * y) },
  };
  const double s[3] = { sx, sy, sz };
  for (int c = 0; c < 3; ++c) {
    for (int row = 0; row < 3; ++row) t[c * 4 + row] = static_cast<float>(r[row][c] * s[c]);
    t[c * 4 + 3] = 0;
  }
  t[12] = static_cast<float>(tx);
  t[13] = static_cast<float>(ty);
  t[14] = static_cast<float>(tz);
  t[15] = 1;
}

// Eine Matrix aus glTF in Blitz-Koordinaten: conv * M * conv^-1.
inline void bb_gltf_conv_(const bb_GltfLoad_& L, float t[16]) {
  if (!L.conv_on) return;
  float a[16];
  mat4_mul_(a, L.conv, t);
  mat4_mul_(t, a, L.conv_inv);
}

inline void bb_gltf_node_matrix_(const bb_Json_& n, float t[16]) {
  const bb_Json_& m = n["matrix"];
  if (m.size() == 16) {
    for (int i = 0; i < 16; ++i) t[i] = static_cast<float>(m[i].n(0));
    return;
  }
  const bb_Json_& T = n["translation"];
  const bb_Json_& R = n["rotation"];
  const bb_Json_& S = n["scale"];
  bb_gltf_trs_matrix_(T[0].n(0), T[1].n(0), T[2].n(0), R[0].n(0), R[1].n(0), R[2].n(0), R[3].n(1),
                      S[0].n(1), S[1].n(1), S[2].n(1), t);
}

inline void bb_gltf_node_(bb_GltfLoad_& L, int ni, int parent) {
  if (ni < 0 || ni >= static_cast<int>(L.node_seen.size()) || L.node_seen[ni]) {
    L.warn("der Knotenbaum ist kaputt (Zyklus oder falscher Index)");
    return;
  }
  L.node_seen[ni] = true;
  const bb_Json_& n = L.doc["nodes"][ni];
  // Ein Gelenk ohne Netz wird ein Pivot, wie ein Knochen aus einer .b3d.
  std::unique_ptr<bb_Entity_> ent;
  if (L.is_joint[ni] && !n["mesh"].has()) ent = std::make_unique<bb_PivotEntity_>();
  else                                     ent = std::make_unique<bb_MeshEntity_>();
  ent->name = n["name"].str;
  const int h = bb_entity_register_(std::move(ent), parent);
  L.node_h[ni] = h;

  float t[16];
  bb_gltf_node_matrix_(n, t);
  bb_gltf_conv_(L, t);
  bb_Entity_* e = bb_entity_get_(h);
  bb_ent_set_local_tform_(e, t);
  L.rest[ni] = { bb_ent_local_pos_(e), bb_ent_local_scl_(e), bb_ent_local_rot_(e) };

  if (n["mesh"].has() && !L.animonly) {
    const int skin = n["skin"].i();
    const bool skinned = skin >= 0 && skin < static_cast<int>(L.doc["skins"].size());
    bb_gltf_mesh_(L, h, n["mesh"].i(), skinned ? skin : -1);
    if (skinned) L.skinned.emplace_back(h, skin);
  }
  // Gewichte der Morph Targets: die des Knotens, sonst die des Netzes, sonst 0.
  if (n["mesh"].has()) {
    const bb_Json_& mesh = L.doc["meshes"][n["mesh"].i()];
    const int nt = static_cast<int>(mesh["primitives"][0]["targets"].size());
    if (nt) {
      const bb_Json_& w = n["weights"].size() ? n["weights"] : mesh["weights"];
      e->morph_w.assign(nt, 0.0f);
      for (int t = 0; t < nt; ++t) e->morph_w[t] = static_cast<float>(w[t].n(0));
      L.rest[ni].w = e->morph_w;
    }
  }
  const bb_Json_& kids = n["children"];
  for (size_t k = 0; k < kids.size(); ++k) bb_gltf_node_(L, kids[k].i(), h);
}

// ---- Skins ----

// Die Knochen jedes Netzes mit Skin: [0] das Netz selbst (fuer Vertices ohne
// Gewicht, es bewegt sie mit sich), dann die Gelenke in der Reihenfolge der
// Skin. Die Ruhelage ist die inverse Bind-Matrix (ohne sie die Einheit),
// wie die Gelenke in Blitz-Koordinaten gebracht.
inline void bb_gltf_skins_(bb_GltfLoad_& L) {
  for (const auto& [h, si] : L.skinned) {
    auto* me = static_cast<bb_MeshEntity_*>(bb_entity_get_(h));
    const bb_Json_& sk = L.doc["skins"][si];
    const bb_Json_& joints = sk["joints"];
    const int n = std::min(static_cast<int>(joints.size()), 254);
    std::vector<double> ibm;
    int ic = 0;
    const bool has_ibm = sk["inverseBindMatrices"].has() &&
                         bb_gltf_accessor_(L, sk["inverseBindMatrices"].i(), ibm, ic) &&
                         ic == 16 && ibm.size() >= static_cast<size_t>(n) * 16;
    if (sk["inverseBindMatrices"].has() && !has_ibm)
      L.warn("inverseBindMatrices nicht lesbar - die Gelenke gelten als Einheit");
    me->bones.assign(1, h);
    me->rep->bone_inv.clear();
    std::array<float, 16> m;
    mat4_identity_(m.data());
    me->rep->bone_inv.push_back(m);
    for (int j = 0; j < n; ++j) {
      const int ni = joints[j].i();
      const int jh = (ni >= 0 && ni < static_cast<int>(L.node_h.size())) ? L.node_h[ni] : 0;
      me->bones.push_back(jh ? jh : h);
      if (has_ibm) for (int k = 0; k < 16; ++k) m[k] = static_cast<float>(ibm[j * 16 + k]);
      else         mat4_identity_(m.data());
      bb_gltf_conv_(L, m.data());
      me->rep->bone_inv.push_back(m);
    }
    me->boned = true;
  }
}

// LoadMesh: die Haltung beim Laden in die Vertices backen, im Raum der
// Entity des Netzes - danach schmilzt bb_collapse_ es ein wie jedes andere.
// Die Weltmatrizen muessen stimmen (bb_update_entity_world_ vorher).
inline void bb_gltf_bake_skins_(int h) {
  bb_Entity_* e = bb_entity_get_(h);
  if (!e) return;
  for (int c : e->children) bb_gltf_bake_skins_(c);
  if (e->kind() != bb_EntityKind_::Mesh) return;
  auto* me = static_cast<bb_MeshEntity_*>(e);
  const bool skin = me->boned && !me->rep->bone_inv.empty();
  if (!skin && !bb_morph_active_(me)) return;
  bb_skin_build_(me);
  // Mit Knochen stehen die Vertices im Weltraum, ohne im Raum des Netzes.
  float inv[16], co[9];
  if (!skin || !mat4_inverse_(inv, me->world)) mat4_identity_(inv);
  mat4_cofactor3_(co, inv);
  for (size_t si = 0; si < me->surfaces().size() && si < me->skinned.size(); ++si) {
    bb_MeshData_& d = me->surfaces()[si];
    d.vertices = me->skinned[si].vertices;
    for (size_t i = 0; i + BB_VF <= d.vertices.size(); i += BB_VF) {
      float* v = &d.vertices[i];
      const float p[3] = { v[0], v[1], v[2] }, n[3] = { v[3], v[4], v[5] };
      mat4_xform_pt_(v, inv, p[0], p[1], p[2]);
      mat3_xform_vec_(v + 3, co, n[0], n[1], n[2]);
      const float l = std::sqrt(v[3] * v[3] + v[4] * v[4] + v[5] * v[5]);
      if (l > 0) { v[3] /= l; v[4] /= l; v[5] /= l; }
    }
    d.bone_ids.clear();
    d.bone_w.clear();
    d.morph.clear();
    d.dirty = true;
  }
  me->morph_w.clear();
  for (auto& s : me->skinned) bb_mesh_free_gpu_(&s);
  me->skinned.clear();
  me->bones.clear();
  me->rep->bone_inv.clear();
  me->boned = false;
}

// ---- Animationen ----

inline constexpr double BB_GLTF_FPS = 60.0;   // Bilder je Sekunde, siehe oben

// Jede Animation wird eine Sequenz des Animators an der Wurzel.
inline void bb_gltf_anims_(bb_GltfLoad_& L, int root) {
  const bb_Json_& anims = L.doc["animations"];
  const int na = static_cast<int>(anims.size());
  if (!na) return;
  const size_t nn = L.node_h.size();
  std::vector<uint8_t> used(nn, 0);                      // 1 Lage, 2 Skalierung, 4 Drehung, 8 Gewichte
  std::vector<std::map<int, std::shared_ptr<bb_AnimKeys_>>> keys(na);
  std::vector<int> len(na, 0);

  for (int a = 0; a < na; ++a) {
    const bb_Json_& an = anims[a];
    const bb_Json_& sm = an["samplers"];
    const bb_Json_& ch = an["channels"];
    const int ns = static_cast<int>(sm.size());
    std::vector<std::vector<double>> in(ns), out(ns);
    std::vector<int> oc(ns, 0);
    std::vector<bool> ok(ns, false);
    double t0 = 1e300;
    for (int s = 0; s < ns; ++s) {
      int ic = 0;
      ok[s] = bb_gltf_accessor_(L, sm[s]["input"].i(), in[s], ic) && ic == 1 && !in[s].empty() &&
              bb_gltf_accessor_(L, sm[s]["output"].i(), out[s], oc[s]);
      if (ok[s]) for (double t : in[s]) t0 = std::min(t0, t);
    }
    for (int c = 0; c < static_cast<int>(ch.size()); ++c) {
      const bb_Json_& tg = ch[c]["target"];
      const int ni = tg["node"].i();
      const int s = ch[c]["sampler"].i();
      if (ni < 0 || ni >= static_cast<int>(nn) || !L.node_h[ni] || s < 0 || s >= ns || !ok[s]) continue;
      const std::string& path = tg["path"].str;
      const int kind = path == "translation" ? 1 : path == "scale" ? 2 :
                       path == "rotation" ? 4 : path == "weights" ? 8 : 0;
      if (!kind) continue;
      // weights: ein SCALAR je Ziel und Schluessel, so viele Ziele wie das Netz.
      const int nt = kind == 8 ? static_cast<int>(
          L.doc["meshes"][L.doc["nodes"][ni]["mesh"].i()]["primitives"][0]["targets"].size()) : 0;
      const int comps = kind == 8 ? nt : kind == 4 ? 4 : 3;
      if (!comps || oc[s] != (kind == 8 ? 1 : comps)) continue;
      const std::string& ip = sm[s]["interpolation"].str;
      const bool cubic = ip == "CUBICSPLINE", step = ip == "STEP";
      const size_t n = in[s].size();
      const size_t stride = static_cast<size_t>(comps) * (cubic ? 3 : 1);
      if (out[s].size() < n * stride) continue;
      auto& kp = keys[a][ni];
      if (!kp) kp = std::make_shared<bb_AnimKeys_>();
      bb_AnimKeys_& k = *kp;
      auto frame = [&](double t) { return static_cast<int>(std::lround((t - t0) * BB_GLTF_FPS)); };
      for (size_t i = 0; i < n; ++i) {
        const double* v = &out[s][i * stride + (cubic ? comps : 0)];
        const int f = frame(in[s][i]);
        int until = f;                                   // STEP: bis ein Bild vor dem naechsten
        if (step && i + 1 < n) until = std::max(f, frame(in[s][i + 1]) - 1);
        for (int g : { f, until }) {
          if (kind == 1) {
            float p[3];
            bb_gltf_lm_apply_(L, v, p);
            k.pos[g] = { p[0], p[1], p[2] };
          } else if (kind == 2) {
            float p[3];
            bb_gltf_lm_apply_(L, v, p);
            k.scl[g] = { std::fabs(p[0]), std::fabs(p[1]), std::fabs(p[2]) };
          } else if (kind == 4) {
            float m[16];
            bb_gltf_trs_matrix_(0, 0, 0, v[0], v[1], v[2], v[3], 1, 1, 1, m);
            bb_gltf_conv_(L, m);
            k.rot[g] = bb_quat_from_mat_(m);
          } else {
            k.wts[g].assign(v, v + comps);
          }
          len[a] = std::max(len[a], g);
        }
      }
      used[ni] |= kind;
    }
  }

  // Kanaele, die nur andere Sequenzen bewegen, stehen auf der Ruhelage.
  for (int a = 0; a < na; ++a)
    for (size_t ni = 0; ni < nn; ++ni) {
      if (!used[ni]) continue;
      auto& kp = keys[a][static_cast<int>(ni)];
      if (!kp) kp = std::make_shared<bb_AnimKeys_>();
      if ((used[ni] & 1) && kp->pos.empty()) kp->pos[0] = L.rest[ni].p;
      if ((used[ni] & 2) && kp->scl.empty()) kp->scl[0] = L.rest[ni].s;
      if ((used[ni] & 4) && kp->rot.empty()) kp->rot[0] = L.rest[ni].r;
      if ((used[ni] & 8) && kp->wts.empty()) kp->wts[0] = L.rest[ni].w;
    }

  bb_Entity_* re = bb_entity_get_(root);
  for (int a = 0; a < na; ++a) {
    for (const auto& [ni, kp] : keys[a])
      if (bb_Entity_* e = bb_entity_get_(L.node_h[ni])) e->anim = kp;
    const int frames = std::max(len[a], 1);
    if (!a) re->animator = bb_animator_new_(root, frames);
    else    re->animator->add_seq(frames);
  }
}

// Datei lesen und in die Welt bringen: eine namenlose Wurzel, darunter die
// Wurzelknoten der Szene. 0 mit Meldung, wenn die Datei nicht lesbar ist.
// `lm` ist die Loadermatrix (spaltenweise, wie bb_LoaderMat_).
inline int bb_load_gltf_(const bbString& file, const float lm[9], int parent, bool animonly = false) {
  std::vector<uint8_t> data;
  if (!bb_gltf_read_file_(file, data)) {
    std::cerr << "[runtime] LoadMesh: cannot open '" << file << "'\n";
    return 0;
  }
  auto fail = [&](const std::string& why) {
    std::cerr << "[runtime] LoadMesh: '" << file << "' ist keine lesbare glTF-Datei (" << why << ")\n";
    return 0;
  };

  bb_GltfLoad_ L;
  L.file = file;
  L.animonly = animonly;
  L.dir  = std::filesystem::path(file).parent_path();
  std::memcpy(L.lm, lm, sizeof L.lm);

  // .glb: 12 Byte Kopf, dann Chunks aus Laenge, Typ, Daten.
  std::vector<uint8_t> bin;
  bool has_bin = false;
  if (data.size() >= 12 && !std::memcmp(data.data(), "glTF", 4)) {
    uint32_t ver, total;
    std::memcpy(&ver, &data[4], 4);
    std::memcpy(&total, &data[8], 4);
    if (ver != 2) return fail("GLB-Version " + std::to_string(ver));
    const size_t end = std::min<size_t>(total, data.size());
    size_t pos = 12;
    bool has_json = false;
    while (pos + 8 <= end) {
      uint32_t len, type;
      std::memcpy(&len, &data[pos], 4);
      std::memcpy(&type, &data[pos + 4], 4);
      pos += 8;
      if (len > end - pos) return fail("Chunk laenger als die Datei");
      if (type == 0x4E4F534A && !has_json) {
        if (!bb_json_parse_(reinterpret_cast<const char*>(&data[pos]), len, L.doc))
          return fail("JSON");
        has_json = true;
      } else if (type == 0x004E4942 && !has_bin) {
        bin.assign(data.begin() + pos, data.begin() + pos + len);
        has_bin = true;
      }
      pos += len;
    }
    if (!has_json) return fail("kein JSON-Chunk");
  } else if (!bb_json_parse_(reinterpret_cast<const char*>(data.data()), data.size(), L.doc)) {
    return fail("JSON");
  }
  data.clear();

  const std::string& ver = L.doc["asset"]["version"].str;
  if (ver.empty() || ver[0] != '2') return fail("Version '" + ver + "', gelesen wird 2.x");

  // Was die Datei zwingend verlangt, muss der Lader koennen.
  const bb_Json_& req = L.doc["extensionsRequired"];
  for (size_t i = 0; i < req.size(); ++i) {
    const std::string& x = req[i].str;
    if (x != "KHR_mesh_quantization" && x != "KHR_materials_unlit" && x != "KHR_texture_transform")
      return fail("braucht " + x);
  }

  const bb_Json_& bufs = L.doc["buffers"];
  L.buffers.resize(bufs.size());
  L.buffer_ok.assign(bufs.size(), false);
  for (size_t i = 0; i < bufs.size(); ++i) {
    const std::string& uri = bufs[i]["uri"].str;
    bool ok = false;
    if (uri.empty()) {
      if (i == 0 && has_bin) { L.buffers[i] = std::move(bin); ok = true; }
    } else if (uri.rfind("data:", 0) == 0) {
      const size_t comma = uri.find(',');
      ok = comma != std::string::npos && uri.find(";base64") < comma &&
           bb_base64_(uri.data() + comma + 1, uri.size() - comma - 1, L.buffers[i]);
    } else {
      ok = bb_gltf_read_file_((L.dir / bb_uri_decode_(uri)).string(), L.buffers[i]);
    }
    if (!ok) {
      std::cerr << "[runtime] LoadMesh: '" << file << "' - Puffer " << i << " fehlt ("
                << (uri.rfind("data:", 0) == 0 ? "data:-URI" : uri) << ")\n";
      continue;
    }
    if (L.buffers[i].size() < bufs[i]["byteLength"].n(0)) {
      std::cerr << "[runtime] LoadMesh: '" << file << "' - Puffer " << i << " ist zu kurz\n";
      continue;
    }
    L.buffer_ok[i] = true;
  }

  L.flip = (lm[0] * (lm[4] * lm[8] - lm[5] * lm[7]) - lm[3] * (lm[1] * lm[8] - lm[2] * lm[7]) +
            lm[6] * (lm[1] * lm[5] - lm[2] * lm[4])) < 0.0f;
  mat4_identity_(L.conv);
  for (int c = 0; c < 3; ++c)
    for (int r = 0; r < 3; ++r) L.conv[c * 4 + r] = lm[c * 3 + r];
  static const float ident[9] = { 1,0,0, 0,1,0, 0,0,1 };
  L.conv_on = std::memcmp(lm, ident, sizeof ident) != 0;
  if (!mat4_inverse_(L.conv_inv, L.conv)) mat4_identity_(L.conv_inv);

  const bb_Json_& nodes = L.doc["nodes"];
  L.node_seen.assign(nodes.size(), false);
  L.node_h.assign(nodes.size(), 0);
  L.rest.resize(nodes.size());
  L.is_joint.assign(nodes.size(), false);
  const bb_Json_& skins = L.doc["skins"];
  for (int s = 0; s < static_cast<int>(skins.size()); ++s)
    for (int j = 0; j < static_cast<int>(skins[s]["joints"].size()); ++j) {
      const int ni = skins[s]["joints"][j].i();
      if (ni >= 0 && ni < static_cast<int>(nodes.size())) L.is_joint[ni] = true;
    }

  // Die Szene: "scene", sonst die erste; ohne Szenen alle Knoten, die
  // niemandes Kind sind.
  std::vector<int> roots;
  const bb_Json_& scenes = L.doc["scenes"];
  if (scenes.size()) {
    const bb_Json_& sc = scenes[std::max(0, L.doc["scene"].i(0))];
    for (size_t k = 0; k < sc["nodes"].size(); ++k) roots.push_back(sc["nodes"][k].i());
  } else {
    std::vector<bool> is_child(nodes.size(), false);
    for (size_t n = 0; n < nodes.size(); ++n)
      for (size_t k = 0; k < nodes[n]["children"].size(); ++k) {
        const int c = nodes[n]["children"][k].i();
        if (c >= 0 && c < static_cast<int>(nodes.size())) is_child[c] = true;
      }
    for (size_t n = 0; n < nodes.size(); ++n) if (!is_child[n]) roots.push_back(static_cast<int>(n));
  }

  const int root = bb_entity_register_(std::make_unique<bb_MeshEntity_>(), parent);
  for (int r : roots) bb_gltf_node_(L, r, root);
  bb_gltf_skins_(L);
  bb_gltf_anims_(L, root);
  return root;
}

#endif // BLITZNEXT_BB_LOADER_GLTF_H
