#ifndef BLITZNEXT_BB_SURFACE_H
#define BLITZNEXT_BB_SURFACE_H

#include "bb_mesh.h"
#include <unordered_map>

// ============================================================
//  Flaechen und Vertices  -  bb_surface.h   (3D-15, Teil 2)
//
//  Eine Flaeche ist im Original ein eigenes Objekt mit Vertexliste,
//  Dreiecksliste und einem Brush. Bei uns ist sie ein `bb_MeshData_` im
//  Vektor einer `bb_MeshEntity_` - ein Handle darauf muss also **die
//  Stelle** meinen und nicht die Adresse, denn ein `CreateSurface`
//  verschiebt den Vektor. Deshalb speichert ein Handle (Entity, Index) und
//  wird bei jedem Zugriff aufgeloest.
//
//  Handles sind stabil: `GetSurface(m,1)` gibt zweimal dasselbe Handle, und
//  dasselbe, das `CreateSurface` geliefert hat. Am Original gemessen.
//
//  **Alles hier ist am laufenden Original nachgemessen**, denn die Getter
//  machen die Datenseite vollstaendig aus der Sprache heraus sichtbar - das
//  ist die erste Stelle im 3D-Teil, an der dafuer kein Bildpunktvergleich
//  noetig war. Vier Ergebnisse haette man anders erwartet:
//
//  * `AddVertex` und `AddTriangle` liefern **nullbasierte** Indizes
//    (`numVertices()-1`), obwohl `GetSurface` einsbasiert zaehlt.
//  * Das `w` von `AddVertex` und `VertexTexCoords` wird **weggeworfen**.
//    `Surface::Vertex` hat `tex_coords[2][2]`, da ist kein Platz dafuer -
//    und `bbVertexW` gibt bedingungslos `1` zurueck, egal was gesetzt wurde.
//  * `AddVertex` setzt **beide** Koordinatensaetze auf dasselbe Paar,
//    `VertexTexCoords` dagegen nur den angegebenen.
//  * `VertexColor` klemmt r, g, b auf 0-255 und rechnet Alpha mal 255 in ein
//    Byte - `VertexAlpha` nach `VertexColor ...,0.5` meldet deshalb
//    `0.498039` und nicht `0.5`.
//
//  Eine Abweichung ist Absicht: `GetSurface(mesh,0)` liest im Original hinter
//  den Vektor und liefert Muell (gemessen: -2013262848). Wir liefern 0.
// ============================================================

struct bb_SurfRef_ { int ent = 0; int idx = 0; };

inline std::unordered_map<int, bb_SurfRef_> bb_surfaces_;
inline std::unordered_map<long long, int>   bb_surface_ids_;
inline int bb_surface_next_id_ = 1;

// Dasselbe Paar (Entity, Index) bekommt immer dasselbe Handle.
inline int bb_surface_handle_(int ent, int idx) {
  const long long key = (static_cast<long long>(ent) << 32) |
                        static_cast<unsigned>(idx);
  auto it = bb_surface_ids_.find(key);
  if (it != bb_surface_ids_.end()) return it->second;
  const int h = bb_surface_next_id_++;
  bb_surfaces_[h]      = { ent, idx };
  bb_surface_ids_[key] = h;
  return h;
}

inline bb_MeshData_* bb_surface_get_(int h) {
  auto it = bb_surfaces_.find(h);
  if (it == bb_surfaces_.end()) return nullptr;
  auto* me = bb_mesh_ent_(it->second.ent);
  if (!me) return nullptr;
  const int i = it->second.idx;
  if (i < 0 || i >= static_cast<int>(me->surfaces().size())) return nullptr;
  return &me->surfaces()[i];
}

// Nach jeder Aenderung an der Geometrie muss die Flaeche neu hochgeladen
// werden; die Ausmasse rechnet MeshWidth ohnehin bei jedem Aufruf neu.
inline void bb_surface_touch_(int h) {
  if (auto* s = bb_surface_get_(h)) s->dirty = true;
}

// ============================================================
// Flaechen holen und anlegen
// ============================================================

// 1-basiert. Ausserhalb des Bereichs 0 - siehe die Anmerkung oben.
inline int bb_GetSurface(int mesh, int surface_index) {
  auto* me = bb_mesh_ent_(mesh);
  if (!me) return 0;
  if (surface_index < 1 ||
      surface_index > static_cast<int>(me->surfaces().size())) return 0;
  return bb_surface_handle_(mesh, surface_index - 1);
}

inline int bb_CreateSurface(int mesh, int brush = 0) {
  auto* me = bb_mesh_ent_(mesh);
  if (!me) return 0;
  me->surfaces().emplace_back();
  if (auto* b = bb_brush_get_(brush)) me->surfaces().back().brush = *b;
  return bb_surface_handle_(mesh, static_cast<int>(me->surfaces().size()) - 1);
}

// Gleichheit zweier Brushes. Das Original vergleicht den ganzen
// Renderzustand ueber ein operator< ; hier sind es dieselben Felder, die wir
// fuehren.
inline bool bb_brush_same_(const bb_Brush_& a, const bb_Brush_& b) {
  if (a.r != b.r || a.g != b.g || a.b != b.b) return false;
  if (a.alpha != b.alpha || a.shininess != b.shininess) return false;
  if (a.blend != b.blend || a.fx != b.fx) return false;
  if (a.twosided != b.twosided) return false;
  for (int k = 0; k < BB_TEX_SLOTS; ++k) {
    if (a.tex.tex[k] != b.tex.tex[k]) return false;
    if (a.tex.tex[k] && a.tex.frame[k] != b.tex.frame[k]) return false;
  }
  return true;
}

inline int bb_FindSurface(int mesh, int brush) {
  auto* me = bb_mesh_ent_(mesh);
  auto* b  = bb_brush_get_(brush);
  if (!me || !b) return 0;
  for (size_t i = 0; i < me->surfaces().size(); ++i)
    if (bb_brush_same_(me->surfaces()[i].brush, *b))
      return bb_surface_handle_(mesh, static_cast<int>(i));
  return 0;
}

// Wie GetEntityBrush eine Kopie, kein Verweis.
inline int bb_GetSurfaceBrush(int surface) {
  auto* s = bb_surface_get_(surface);
  if (!s) return 0;
  return bb_brush_register_(s->brush);
}

inline void bb_PaintSurface(int surface, int brush) {
  auto* s = bb_surface_get_(surface);
  auto* b = bb_brush_get_(brush);
  if (!s || !b) return;
  s->brush = *b;
}

inline void bb_ClearSurface(int surface, int clear_vertices = 1,
                            int clear_triangles = 1) {
  auto* s = bb_surface_get_(surface);
  if (!s) return;
  if (clear_vertices)  s->vertices.clear();
  if (clear_triangles) s->indices.clear();
  s->triCount = static_cast<int>(s->indices.size()) / 3;
  s->dirty    = true;
}

// ============================================================
// Zaehlen
// ============================================================

inline int bb_CountVertices(int surface) {
  auto* s = bb_surface_get_(surface);
  return s ? static_cast<int>(s->vertices.size()) / BB_VF : 0;
}

inline int bb_CountTriangles(int surface) {
  auto* s = bb_surface_get_(surface);
  return s ? static_cast<int>(s->indices.size()) / 3 : 0;
}

// ============================================================
// Bauen
// ============================================================

// Das dritte Texturmass wird verworfen - im Original genauso, siehe oben.
// Beide Koordinatensaetze bekommen dasselbe Paar, die Farbe deckendes Weiss.
inline int bb_AddVertex(int surface, float x, float y, float z,
                        float u = 0.0f, float v = 0.0f, float w = 1.0f) {
  (void)w;
  auto* s = bb_surface_get_(surface);
  if (!s) return 0;
  bb_vert_push_(*s, x, y, z, 0.0f, 0.0f, 0.0f, u, v);
  s->dirty = true;
  return static_cast<int>(s->vertices.size()) / BB_VF - 1;
}

inline int bb_AddTriangle(int surface, int v0, int v1, int v2) {
  auto* s = bb_surface_get_(surface);
  if (!s) return 0;
  s->indices.push_back(static_cast<unsigned>(v0));
  s->indices.push_back(static_cast<unsigned>(v1));
  s->indices.push_back(static_cast<unsigned>(v2));
  s->triCount = static_cast<int>(s->indices.size()) / 3;
  s->dirty    = true;
  return static_cast<int>(s->indices.size()) / 3 - 1;
}

// Zeiger auf den Anfang eines Vertex, oder nullptr.
inline float* bb_vertex_at_(int surface, int index) {
  auto* s = bb_surface_get_(surface);
  if (!s || index < 0) return nullptr;
  if (static_cast<size_t>(index + 1) * BB_VF > s->vertices.size()) return nullptr;
  return &s->vertices[static_cast<size_t>(index) * BB_VF];
}

inline void bb_VertexCoords(int surface, int index, float x, float y, float z) {
  float* p = bb_vertex_at_(surface, index);
  if (!p) return;
  p[0] = x; p[1] = y; p[2] = z;
  bb_surface_touch_(surface);
}

inline void bb_VertexNormal(int surface, int index,
                            float nx, float ny, float nz) {
  float* p = bb_vertex_at_(surface, index);
  if (!p) return;
  p[3] = nx; p[4] = ny; p[5] = nz;
  bb_surface_touch_(surface);
}

// r, g, b werden auf 0-255 geklemmt, Alpha auf 0-1 und dann in ein Byte
// gerechnet - deshalb kommt aus 0.5 spaeter 0.498039 zurueck.
inline void bb_VertexColor(int surface, int index, float red, float green,
                           float blue, float alpha = 1.0f) {
  float* p = bb_vertex_at_(surface, index);
  if (!p) return;
  auto clamp255 = [](float x) {
    return x < 0.0f ? 0.0f : (x > 255.0f ? 255.0f : x);
  };
  float a = alpha * 255.0f;
  if (a < 0.0f) a = 0.0f; else if (a > 255.0f) a = 255.0f;
  p[10] = clamp255(red)   / 255.0f;
  p[11] = clamp255(green) / 255.0f;
  p[12] = clamp255(blue)  / 255.0f;
  p[13] = static_cast<float>(static_cast<int>(a)) / 255.0f;
  bb_surface_touch_(surface);
}

// coord_set 0 oder 1; anders als AddVertex wird nur der angegebene gesetzt.
inline void bb_VertexTexCoords(int surface, int index, float u, float v,
                               float w = 1.0f, int coord_set = 0) {
  (void)w;
  float* p = bb_vertex_at_(surface, index);
  if (!p) return;
  const int off = (coord_set == 1) ? 8 : 6;
  p[off] = u; p[off + 1] = v;
  bb_surface_touch_(surface);
}

// ============================================================
// Lesen
// ============================================================

inline float bb_VertexX(int surface, int index) {
  const float* p = bb_vertex_at_(surface, index); return p ? p[0] : 0.0f;
}
inline float bb_VertexY(int surface, int index) {
  const float* p = bb_vertex_at_(surface, index); return p ? p[1] : 0.0f;
}
inline float bb_VertexZ(int surface, int index) {
  const float* p = bb_vertex_at_(surface, index); return p ? p[2] : 0.0f;
}
inline float bb_VertexNX(int surface, int index) {
  const float* p = bb_vertex_at_(surface, index); return p ? p[3] : 0.0f;
}
inline float bb_VertexNY(int surface, int index) {
  const float* p = bb_vertex_at_(surface, index); return p ? p[4] : 0.0f;
}
inline float bb_VertexNZ(int surface, int index) {
  const float* p = bb_vertex_at_(surface, index); return p ? p[5] : 0.0f;
}
inline float bb_VertexRed(int surface, int index) {
  const float* p = bb_vertex_at_(surface, index);
  return p ? p[10] * 255.0f : 0.0f;
}
inline float bb_VertexGreen(int surface, int index) {
  const float* p = bb_vertex_at_(surface, index);
  return p ? p[11] * 255.0f : 0.0f;
}
inline float bb_VertexBlue(int surface, int index) {
  const float* p = bb_vertex_at_(surface, index);
  return p ? p[12] * 255.0f : 0.0f;
}
inline float bb_VertexAlpha(int surface, int index) {
  const float* p = bb_vertex_at_(surface, index); return p ? p[13] : 0.0f;
}
inline float bb_VertexU(int surface, int index, int coord_set = 0) {
  const float* p = bb_vertex_at_(surface, index);
  return p ? p[(coord_set == 1) ? 8 : 6] : 0.0f;
}
inline float bb_VertexV(int surface, int index, int coord_set = 0) {
  const float* p = bb_vertex_at_(surface, index);
  return p ? p[(coord_set == 1) ? 9 : 7] : 0.0f;
}
// Immer 1 - das Original gibt bedingungslos 1 zurueck, weil es das dritte
// Mass gar nicht erst speichert.
inline float bb_VertexW(int surface, int index, int coord_set = 0) {
  (void)surface; (void)index; (void)coord_set;
  return 1.0f;
}

// vertex ist die Ecke 0, 1 oder 2.
inline int bb_TriangleVertex(int surface, int index, int vertex) {
  auto* s = bb_surface_get_(surface);
  if (!s || vertex < 0 || vertex > 2 || index < 0) return 0;
  const size_t k = static_cast<size_t>(index) * 3 + vertex;
  if (k >= s->indices.size()) return 0;
  return static_cast<int>(s->indices[k]);
}

#endif // BLITZNEXT_BB_SURFACE_H
