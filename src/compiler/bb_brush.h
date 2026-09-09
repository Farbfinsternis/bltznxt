#ifndef BLITZNEXT_BB_BRUSH_H
#define BLITZNEXT_BB_BRUSH_H

#include "bb_texture.h"
#include <memory>
#include <unordered_map>

// ============================================================
//  Brushes  -  bb_brush.h   (3D-15, Teil 1)
//
//  Ein Brush ist das Aussehen als **Wert**: Farbe, Deckkraft, Glanz,
//  Blendmodus, FX-Flags und bis zu acht Texturlagen. Bis hierher gab es ihn
//  schon - seit dem `.3ds`-Loader haengt er an jeder Flaeche -, aber nicht
//  als Handle, das ein Programm anfassen kann.
//
//  **Kopie, nicht Verweis.** `PaintEntity`, `PaintMesh` und `PaintSurface`
//  legen den Brush **ab**, sie merken ihn sich nicht: im Original ist
//  `Brush` eine Wertklasse und `bbPaintSurface` ruft `s->setBrush(*b)`.
//  Wer den Brush danach aendert, aendert das Bemalte nicht mehr. Ebenso
//  gibt `GetEntityBrush` eine **neue** Kopie zurueck
//  (`d_new Brush( m->getBrush() )`), kein Handle auf das Original - deshalb
//  sagt die Doku, man solle sie mit `FreeBrush` wieder loswerden.
//
//  **Wie Entity- und Flaechenbrush zusammenkommen**, steht im Original in
//  `blitz3d/brush.cpp` als eigener Konstruktor `Brush(a,b)`, und
//  `meshmodel.cpp` ruft ihn als `Brush( s->getBrush(), render_brush )` -
//  a ist also die Flaeche, b die Entity:
//
//      Farbe      a * b          (beide 0-255, das Ergebnis wieder 0-255)
//      Deckkraft  a * b
//      Glanz      a + b          <- Summe, nicht das Groessere
//      Blend      b, falls b ungleich 0, sonst a
//      FX         a ODER b       (bitweise)
//      Texturen   b ueberschreibt a je Lage, in der b eine hat
//
//  Der Glanz war bei uns bis hierher das Groessere von beiden. Das ist aus
//  dem Quelltext berichtigt und **nicht** gemessen: solange BUG-66 offen ist
//  (wir rechnen den Glanzpunkt je Bildpunkt, das Original je Vertex), sagt
//  ein Bildpunktvergleich ueber diese Formel ohnehin nichts.
// ============================================================

// Was eine Flaeche oder eine Entity an Aussehen mitbringt.
struct bb_Brush_ {
  float        r = 255.0f, g = 255.0f, b = 255.0f;
  float        alpha     = 1.0f;
  float        shininess = 0.0f;
  // 0 = nicht gesetzt (das Original leitet daraus "deckend" ab),
  // 1 = Alpha, 2 = Multiply, 3 = Add.
  int          blend     = 0;
  // 1 Fullbright, 2 Vertexfarben, 4 Flat, 8 kein Nebel,
  // 16 keine Rueckseitenentfernung, 32 Alphablending erzwingen.
  int          fx        = 0;
  // 3DS kennt zweiseitige Materialien (Chunk 0xA081); solche Flaechen
  // werden ohne Rueckseitenentfernung gezeichnet (3D-13). Das Original
  // wertet den Chunk gar nicht aus, wir schon - deshalb ein eigenes Feld
  // und nicht FX 16.
  bool         twosided  = false;
  bb_TexSlots_ tex;
};

// Flaechenbrush a mit Entitybrush b verrechnen - die Formel oben.
inline bb_Brush_ bb_brush_combine_(const bb_Brush_& a, const bb_Brush_& b) {
  bb_Brush_ o = a;
  o.r         = a.r * b.r / 255.0f;
  o.g         = a.g * b.g / 255.0f;
  o.b         = a.b * b.b / 255.0f;
  o.alpha     = a.alpha * b.alpha;
  o.shininess = a.shininess + b.shininess;
  if (b.blend) o.blend = b.blend;
  o.fx       |= b.fx;
  o.twosided  = a.twosided || b.twosided;
  for (int k = 0; k < BB_TEX_SLOTS; ++k) {
    if (b.tex.tex[k]) {
      o.tex.tex[k]   = b.tex.tex[k];
      o.tex.frame[k] = b.tex.frame[k];
    }
  }
  return o;
}

// ============================================================
// Handles
// ============================================================
//
// Wie bei den Texturen: eine Abbildung von fortlaufender Nummer auf den
// Wert. 0 ist kein gueltiges Handle.

inline std::unordered_map<int, bb_Brush_> bb_brushes_;
inline int bb_brush_next_id_ = 1;

inline bb_Brush_* bb_brush_get_(int h) {
  auto it = bb_brushes_.find(h);
  return (it != bb_brushes_.end()) ? &it->second : nullptr;
}

inline int bb_brush_register_(const bb_Brush_& b) {
  int h = bb_brush_next_id_++;
  bb_brushes_[h] = b;
  return h;
}

// ============================================================
// Befehle
// ============================================================

// Vorgabe 255,255,255 - laut Doku und laut Signatur des Originals
// ("%CreateBrush#red=255#green=255#blue=255").
inline int bb_CreateBrush(float r = 255.0f, float g = 255.0f, float b = 255.0f) {
  bb_Brush_ br;
  br.r = r; br.g = g; br.b = b;
  return bb_brush_register_(br);
}

// Laedt die Textur, skaliert sie und haengt sie an einen weissen Brush.
// Im Original genau diese Reihenfolge, und die Skalierung ist dieselbe
// Rechnung wie bei ScaleTexture (dort `t->setScale( 1/u, 1/v )`).
// Laesst sich die Textur nicht laden, kommt 0 heraus - kein leerer Brush.
inline int bb_LoadBrush(const bbString& file, int flags = 1,
                        float u_scale = 1.0f, float v_scale = 1.0f) {
  int t = bb_LoadTexture(file, flags);
  if (!t) return 0;
  if (u_scale != 1.0f || v_scale != 1.0f) bb_ScaleTexture(t, u_scale, v_scale);
  bb_Brush_ br;
  br.tex.tex[0]   = bb_texture_ref_(t);
  br.tex.frame[0] = 0;
  return bb_brush_register_(br);
}

inline void bb_FreeBrush(int brush) { bb_brushes_.erase(brush); }

inline void bb_BrushColor(int brush, float r, float g, float b) {
  if (auto* br = bb_brush_get_(brush)) { br->r = r; br->g = g; br->b = b; }
}

inline void bb_BrushAlpha(int brush, float alpha) {
  if (auto* br = bb_brush_get_(brush)) br->alpha = alpha;
}

inline void bb_BrushShininess(int brush, float shininess) {
  if (auto* br = bb_brush_get_(brush)) br->shininess = shininess;
}

// Bis zu acht Lagen. Die Doku nennt bei BrushTexture vier (0-3) und bei
// GetBrushTexture acht (0-7); das Original haelt gxScene::MAX_TEXTURES
// Lagen, gezeichnet werden die ersten vier.
inline void bb_BrushTexture(int brush, int texture, int frame = 0, int index = 0) {
  auto* br = bb_brush_get_(brush);
  if (!br || index < 0 || index >= BB_TEX_SLOTS) return;
  br->tex.tex[index]   = bb_texture_ref_(texture);
  br->tex.frame[index] = frame;
}

// Das Original legt hier eine **neue** Textur an, die sich die Bilddaten mit
// der alten teilt, und traegt sie in seine Texturliste ein. Deshalb ein
// neues Handle auf dieselben Daten: `FreeTexture` darauf nimmt dem Brush
// seine Textur nicht weg.
inline int bb_GetBrushTexture(int brush, int index = 0) {
  auto* br = bb_brush_get_(brush);
  if (!br || index < 0 || index >= BB_TEX_SLOTS) return 0;
  bb_TexRef_ t = br->tex.tex[index];
  if (!t) return 0;
  return bb_texture_register_(t);
}

// 1 = Alpha, 2 = Multiply, 3 = Add. 0 heisst "nicht gesetzt" und ueberlaesst
// die Entscheidung dem Flaechenbrush.
inline void bb_BrushBlend(int brush, int blend) {
  if (auto* br = bb_brush_get_(brush)) br->blend = blend;
}

inline void bb_BrushFX(int brush, int fx) {
  if (auto* br = bb_brush_get_(brush)) br->fx = fx;
}

#endif // BLITZNEXT_BB_BRUSH_H
