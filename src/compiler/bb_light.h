#ifndef BLITZNEXT_BB_LIGHT_H
#define BLITZNEXT_BB_LIGHT_H

#include "bb_entity_core.h"
#include <cmath>

// ============================================================
//  Lichter  —  bb_light.h   (3D-12)
//
//  CreateLight ist mit 39 betroffenen Beispieldateien der haeufigste fehlende
//  Einzelbefehl. Der LIT-Shader konnte bereits bis zu acht Lichter; hier kommt
//  die Sprachseite und das Einsammeln fuer den Renderpass dazu.
//
//  Alle Angaben stammen aus der mitgelieferten Blitz3D-Dokumentation
//  (help/commands/3d_commands/*.htm), nicht aus Vermutungen:
//
//    CreateLight ( [type][,parent] )   1 = directional (Vorgabe)
//                                      2 = point
//                                      3 = spot
//    LightColor light,red#,green#,blue#      0-255, **negativ verdunkelt**
//    LightRange light,range#                 Vorgabe 1000.0
//    LightConeAngles light,inner#,outer#     Vorgabe 0,90
//    AmbientLight red#,green#,blue#          Vorgabe 127,127,127
//
//  Zwei Dinge, die man ohne die Doku falsch gemacht haette: die Vorgabe ist
//  **directional**, nicht point, und die Nummerierung beginnt bei 1 - der
//  Shader zaehlt intern ab 0, weshalb hier umgesetzt wird. Ein Richtungslicht
//  hat laut Doku "infinite position and infinite range"; seine Richtung kommt
//  aus der Rotation, weshalb die Beispiele es mit RotateEntity ausrichten.
//
//  Offen und bewusst nicht behauptet: ob Blitz3D die Kegelwinkel als vollen
//  Oeffnungswinkel oder als Halbwinkel versteht. Hier ist der volle Winkel
//  angenommen (Vorgabe 90 wird also zu +-45 Grad um die Achse) - das laesst
//  sich nur an einem laufenden Original-Renderer entscheiden.
// ============================================================

struct bb_LightEntity_ : bb_Entity_ {
  int   type  = 1;                       // Blitz3D-Nummerierung: 1/2/3
  float colR  = 255, colG = 255, colB = 255;
  float range = 1000.0f;
  float inner = 0.0f, outer = 90.0f;
  bb_EntityKind_ kind() const override { return bb_EntityKind_::Light; }
};

// Der zweite Parameter ist der Elternknoten. Beides ist optional; die
// Befehlstabelle traegt das als "type%?,parent%?".
inline int bb_CreateLight(int type = 1, int parent = 0) {
  auto l  = std::make_unique<bb_LightEntity_>();
  l->type = (type >= 1 && type <= 3) ? type : 1;
  return bb_entity_register_(std::move(l), parent);
}

inline bb_LightEntity_ *bb_light_get_(int h) {
  bb_Entity_ *e = bb_entity_get_(h);
  if (!e || e->kind() != bb_EntityKind_::Light) return nullptr;
  return static_cast<bb_LightEntity_ *>(e);
}

// 0-255 laut Doku, negative Werte verdunkeln ausdruecklich ("negative
// lighting", fuer Schatteneffekte). Deshalb wird hier nicht geklemmt - der
// Shader begrenzt erst das Endergebnis.
inline void bb_LightColor(int light, float r, float g, float b) {
  if (auto *l = bb_light_get_(light)) { l->colR = r; l->colG = g; l->colB = b; }
}

inline void bb_LightRange(int light, float range) {
  if (auto *l = bb_light_get_(light)) l->range = range;
}

inline void bb_LightConeAngles(int light, float inner_angle, float outer_angle) {
  if (auto *l = bb_light_get_(light)) {
    l->inner = inner_angle;
    l->outer = outer_angle;
  }
}

#endif // BLITZNEXT_BB_LIGHT_H
