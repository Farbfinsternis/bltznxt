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
//  Noch nicht: Skinning und Animationen (kommen auf das Blitz-System aus
//  3D-19), Morph Targets, KHR_texture_transform. Draco- und meshopt-
//  komprimierte Dateien werden abgelehnt.
// ============================================================

#include "bb_mesh.h"
#include "bb_texture.h"
#include <algorithm>
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
inline int bb_gltf_texture_(bb_GltfLoad_& L, int ti, int flags, int coords, double cut, bool grey) {
  const bb_Json_& tx = L.doc["textures"][ti];
  if (!tx.has()) return 0;
  const int img = tx["source"].i();
  const bb_Json_& sm = L.doc["samplers"][tx["sampler"].i()];
  if (sm["wrapS"].i(10497) == 33071) flags |= BB_TEX_CLAMPU;
  if (sm["wrapT"].i(10497) == 33071) flags |= BB_TEX_CLAMPV;

  const std::string key = std::to_string(img) + "/" + std::to_string(flags) + "/" +
                          std::to_string(coords) + "/" + std::to_string(cut) + "/" + (grey ? "g" : "c");
  if (auto it = L.texcache.find(key); it != L.texcache.end()) return it->second;

  std::vector<uint8_t> bytes;
  std::string name;
  int w = 0, h = 0, ch = 0;
  unsigned char* data = nullptr;
  if (bb_gltf_image_(L, img, bytes, name) && !bytes.empty())
    data = stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()), &w, &h, &ch, 4);
  if (!data) {
    std::cerr << "[runtime] LoadMesh: '" << L.file << "' - Bild " << img
              << " nicht lesbar (PNG und JPEG gehen)\n";
    L.texcache.emplace(key, 0);
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
  const int handle = bb_texture_register_(std::move(t));
  L.texcache.emplace(key, handle);
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

  auto tex_coords = [&](const bb_Json_& info) {
    const int tc = info["texCoord"].i(0);
    if (tc > 1) L.warn("nur TEXCOORD_0 und TEXCOORD_1 werden gelesen");
    return tc == 1 ? 1 : 0;
  };
  const bb_Json_& base = pbr["baseColorTexture"];
  if (base.has()) {
    if (const int t = bb_gltf_texture_(L, base["index"].i(), tflags, tex_coords(base), cut, false)) {
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
    if (const int t = bb_gltf_texture_(L, occ["index"].i(), BB_TEX_COLOR, tex_coords(occ), -1, packed)) {
      br.tex.tex[1]   = bb_texture_ref_(t);
      br.tex.frame[1] = 0;
    }
  }
  return br;
}

// ---- Netze ----

inline void bb_gltf_mesh_(bb_GltfLoad_& L, int h, int mi) {
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
    for (size_t v = 0; v < nv; ++v) {
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
inline void bb_gltf_node_matrix_(const bb_Json_& n, float t[16]) {
  const bb_Json_& m = n["matrix"];
  if (m.size() == 16) {
    for (int i = 0; i < 16; ++i) t[i] = static_cast<float>(m[i].n(0));
    return;
  }
  const bb_Json_& T = n["translation"];
  const bb_Json_& R = n["rotation"];
  const bb_Json_& S = n["scale"];
  double x = R[0].n(0), y = R[1].n(0), z = R[2].n(0), w = R[3].n(1);
  const double l = std::sqrt(x * x + y * y + z * z + w * w);
  if (l > 0) { x /= l; y /= l; z /= l; w /= l; } else { w = 1; }
  const double r[3][3] = {
    { 1 - 2 * (y * y + z * z), 2 * (x * y - z * w),     2 * (x * z + y * w) },
    { 2 * (x * y + z * w),     1 - 2 * (x * x + z * z), 2 * (y * z - x * w) },
    { 2 * (x * z - y * w),     2 * (y * z + x * w),     1 - 2 * (x * x + y * y) },
  };
  const double s[3] = { S[0].n(1), S[1].n(1), S[2].n(1) };
  for (int c = 0; c < 3; ++c) {
    for (int row = 0; row < 3; ++row) t[c * 4 + row] = static_cast<float>(r[row][c] * s[c]);
    t[c * 4 + 3] = 0;
  }
  t[12] = static_cast<float>(T[0].n(0));
  t[13] = static_cast<float>(T[1].n(0));
  t[14] = static_cast<float>(T[2].n(0));
  t[15] = 1;
}

inline void bb_gltf_node_(bb_GltfLoad_& L, int ni, int parent) {
  if (ni < 0 || ni >= static_cast<int>(L.node_seen.size()) || L.node_seen[ni]) {
    L.warn("der Knotenbaum ist kaputt (Zyklus oder falscher Index)");
    return;
  }
  L.node_seen[ni] = true;
  const bb_Json_& n = L.doc["nodes"][ni];
  auto ent = std::make_unique<bb_MeshEntity_>();
  ent->name = n["name"].str;
  const int h = bb_entity_register_(std::move(ent), parent);

  float t[16];
  bb_gltf_node_matrix_(n, t);
  if (L.conv_on) {
    float a[16];
    mat4_mul_(a, L.conv, t);
    mat4_mul_(t, a, L.conv_inv);
  }
  bb_ent_set_local_tform_(bb_entity_get_(h), t);

  if (n["mesh"].has()) bb_gltf_mesh_(L, h, n["mesh"].i());
  const bb_Json_& kids = n["children"];
  for (size_t k = 0; k < kids.size(); ++k) bb_gltf_node_(L, kids[k].i(), h);
}

// Datei lesen und in die Welt bringen: eine namenlose Wurzel, darunter die
// Wurzelknoten der Szene. 0 mit Meldung, wenn die Datei nicht lesbar ist.
// `lm` ist die Loadermatrix (spaltenweise, wie bb_LoaderMat_).
inline int bb_load_gltf_(const bbString& file, const float lm[9], int parent) {
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
    if (x != "KHR_mesh_quantization" && x != "KHR_materials_unlit")
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
  return root;
}

#endif // BLITZNEXT_BB_LOADER_GLTF_H
